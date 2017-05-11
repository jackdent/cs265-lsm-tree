#include <cassert>
#include <fstream>
#include <iostream>
#include <map>

#include "lsm_tree.h"
#include "merge.h"
#include "sys.h"

using namespace std;

ostream& operator<<(ostream& stream, const entry_t& entry) {
    stream.write((char *)&entry.key, sizeof(KEY_t));
    stream.write((char *)&entry.val, sizeof(VAL_t));
    return stream;
}

istream& operator>>(istream& stream, entry_t& entry) {
    stream.read((char *)&entry.key, sizeof(KEY_t));
    stream.read((char *)&entry.val, sizeof(VAL_t));
    return stream;
}

/*
 * LSM Tree
 */

LSMTree::LSMTree(int buffer_max_entries, int depth, int fanout,
                 int num_threads, float bf_bits_per_entry) :
                 bf_bits_per_entry(bf_bits_per_entry),
                 buffer(buffer_max_entries),
                 worker_pool(num_threads)
{
    long max_run_size;

    max_run_size = buffer_max_entries;

    while ((depth--) > 0) {
        levels.emplace_back(fanout, max_run_size);
        max_run_size *= fanout;
    }
}

void LSMTree::merge_down(vector<Level>::iterator current) {
    vector<Level>::iterator next;
    MergeContext merge_ctx;
    entry_t entry;

    assert(current >= levels.begin());

    if (current->remaining() > 0) {
        return;
    } else if (current >= levels.end() - 1) {
        die("No more space in tree.");
    } else {
        next = current + 1;
    }

    /*
     * If the next level does not have space for the current level,
     * recursively merge the next level downwards to create some
     */

    if (next->remaining() == 0) {
        merge_down(next);
        assert(next->remaining() > 0);
    }

    /*
     * Merge all runs in the current level into the first
     * run in the next level
     */

    for (auto& run : current->runs) {
        merge_ctx.add(run.map_read(), run.size);
    }

    next->runs.emplace_front(next->max_run_size, bf_bits_per_entry);
    next->runs.front().map_write();

    while (!merge_ctx.done()) {
        entry = merge_ctx.next();

        // Remove deleted keys from the final level
        if (!(next == levels.end() - 1 && entry.val == VAL_TOMBSTONE)) {
            next->runs.front().put(entry);
        }
    }

    next->runs.front().unmap();

    for (auto& run : current->runs) {
        run.unmap();
    }

    /*
     * Clear the current level to delete the old (now
     * redundant) entry files.
     */

    current->runs.clear();
}

void LSMTree::put(KEY_t key, VAL_t val) {
    /*
     * Try inserting the key into the buffer
     */

    if (buffer.put(key, val)) {
        return;
    }

    /*
     * If the buffer is full, flush level 0 if necessary
     * to create space
     */

    merge_down(levels.begin());

    /*
     * Flush the buffer to level 0
     */

    levels.front().runs.emplace_front(levels.front().max_run_size, bf_bits_per_entry);
    levels.front().runs.front().map_write();

    for (const auto& entry : buffer.entries) {
        levels.front().runs.front().put(entry);
    }

    levels.front().runs.front().unmap();

    /*
     * Empty the buffer and insert the key/value pair
     */

    buffer.empty();
    assert(buffer.put(key, val));
}

Run * LSMTree::get_run(int index) {
    for (const auto& level : levels) {
        if (index < level.runs.size()) {
            return (Run *) &level.runs[index];
        } else {
            index -= level.runs.size();
        }
    }

    return nullptr;
};

void LSMTree::get(KEY_t key) {
    VAL_t *buffer_val;
    VAL_t latest_val;
    int latest_run;
    SpinLock lock;
    atomic<int> counter;

    /*
     * Search buffer
     */

    buffer_val = buffer.get(key);

    if (buffer_val != nullptr) {
        if (*buffer_val != VAL_TOMBSTONE) cout << *buffer_val;
        cout << endl;
        delete buffer_val;
        return;
    }

    /*
     * Search runs
     */

    counter = 0;
    latest_run = -1;

    worker_task search = [&] {
        int current_run;
        Run *run;
        VAL_t *current_val;

        current_run = counter++;

        if (latest_run >= 0 || (run = get_run(current_run)) == nullptr) {
            // Stop search if we discovered a key in another run, or
            // if there are no more runs to search
            return;
        } else if ((current_val = run->get(key)) == nullptr) {
            // Couldn't find the key in the current run, so we need
            // to keep searching.
            search();
        } else {
            // Update val if the run is more recent than the
            // last, then stop searching since there's no need
            // to search later runs.
            lock.lock();

            if (latest_run < 0 || current_run < latest_run) {
                latest_run = current_run;
                latest_val = *current_val;
            }

            lock.unlock();
            delete current_val;
        }
    };

    worker_pool.launch(search);
    worker_pool.wait_all();

    if (latest_run >= 0 && latest_val != VAL_TOMBSTONE) cout << latest_val;
    cout << endl;
}

void LSMTree::range(KEY_t start, KEY_t end) {
    vector<entry_t> *buffer_range;
    map<int, vector<entry_t> *> ranges;
    SpinLock lock;
    atomic<int> counter;
    MergeContext merge_ctx;
    entry_t entry;

    if (end <= start) {
        cout << endl;
        return;
    } else {
        // Convert to inclusive bound
        end -= 1;
    }

    /*
     * Search buffer
     */

    ranges.insert({0, buffer.range(start, end)});

    /*
     * Search runs
     */

    counter = 0;

    worker_task search = [&] {
        int current_run;
        Run *run;

        current_run = counter++;

        if ((run = get_run(current_run)) != nullptr) {
            lock.lock();
            ranges.insert({current_run + 1, run->range(start, end)});
            lock.unlock();

            // Potentially more runs to search.
            search();
        }
    };

    worker_pool.launch(search);
    worker_pool.wait_all();

    /*
     * Merge ranges and print keys
     */

    for (const auto& kv : ranges) {
        merge_ctx.add(kv.second->data(), kv.second->size());
    }

    while (!merge_ctx.done()) {
        entry = merge_ctx.next();
        if (entry.val != VAL_TOMBSTONE) {
            cout << entry.key << ":" << entry.val;
            if (!merge_ctx.done()) cout << " ";
        }
    }

    cout << endl;

    /*
     * Cleanup subrange vectors
     */

    for (auto& range : ranges) {
        delete range.second;
    }
}

void LSMTree::del(KEY_t key) {
    put(key, VAL_TOMBSTONE);
}

void LSMTree::load(string file_path) {
    ifstream stream;
    entry_t entry;

    stream.open(file_path, ifstream::binary);

    if (stream.is_open()) {
        while (stream >> entry) {
            put(entry.key, entry.val);
        }
    } else {
        die("Could not locate file '" + file_path + "'.");
    }
}
