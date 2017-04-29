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

LSMTree::LSMTree(int buffer_max_entries, int depth, int fanout, int num_threads, float merge_ratio)
    : buffer(buffer_max_entries), worker_pool(num_threads), merge_ratio(merge_ratio)
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
    int merge_size, i;

    assert(current >= levels.begin());

    if (current >= levels.end() - 1) {
        die("No more space in buffer.");
    }

    next = current + 1;

    /*
     * If the next level does not have space for the current level,
     * recursively merge the next level downwards to create some
     */

    if (next->remaining() == 0) {
        merge_down(next);
        assert(next->remaining() > 0);
    }

    merge_size = merge_ratio * current->max_runs;

    if (merge_size > current->runs.size()) {
        merge_size = current->runs.size();
    }

    /*
     * Merge the last "merge_size" runs in the current level
     * into the first run in the next level
     */

    for (i = current->runs.size() - merge_size; i < merge_size; i++) {
        merge_ctx.add(current->runs[i].map(MappingType::Read));
    }

    next->runs.emplace_front(next->max_run_size);
    next->runs.front().map(MappingType::Write);

    while (!merge_ctx.done()) {
        entry = merge_ctx.next();

        // We can remove deleted keys from the final level
        if (!(next == levels.end() - 1 && entry.val == VAL_TOMBSTONE)) {
            next->runs.front().put(entry);
        }
    }

    next->runs.front().unmap();

    /*
     * Unmap and delete the old (now redundant) entry files
     */

    for (i = current->runs.size() - merge_size; i < merge_size; i++) {
        current->runs[i].unmap();
    }

    current->runs.erase(current->runs.begin(), current->runs.begin() + merge_size);
}

void LSMTree::put(KEY_t key, VAL_t val) {
    if (buffer.put(key, val)) {
        return;
    }

    /*
     * Flush level 0 if necessary
     */

    if (levels.front().remaining() == 0) {
        merge_down(levels.begin());
    }

    /*
     * Flush the buffer to level 0
     */

    assert(buffer.entries.size() == levels.front().max_run_size);

    levels.front().runs.emplace_front(levels.front().max_run_size);
    levels.front().runs.front().map(MappingType::Write);

    for (const auto &entry : buffer.entries) {
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
            // TODO: why do I need to cast from const?
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
        int current_run = counter++;
        Run *run = get_run(current_run);

        if (run == nullptr || (current_run > latest_run && latest_run > 0)) {
            // If we have run out of runs, or if we have
            // discovered a key in a more recent run,
            // stop searching.
            return;
        }

        VAL_t *current_val = run->get(key);

        if (current_val != nullptr) {
            // Search the current run. If we find the key in
            // the run, update val if the run is more
            // recent than the last. Then stop the search on this
            // thread, since the next run is guaranteed to
            // be less recent.
            lock.lock();

            if (latest_run < 0 || current_run < latest_run) {
                latest_run = current_run;
                latest_val = *current_val;
            }

            lock.unlock();

            delete current_val;
        } else {
            // Keep searching until we find an entry.
            search();
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
        int current_run = counter++;
        Run *run = get_run(current_run);

        if (run != nullptr) {
            lock.lock();
            ranges.insert({current_run + 1, run->range(start, end)});
            lock.unlock();

            // Potentially more runs to search
            search();
        }
    };

    worker_pool.launch(search);
    worker_pool.wait_all();

    /*
     * Merge ranges and print keys
     */

    for (const auto& range : ranges) {
        merge_ctx.add(range.second);
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

void LSMTree::stats(void) {
    // unsigned long total_pairs;

    /*
     * Tree stats
     */

    // tie(buffer_run, merge_ctx) = all();
    // total_pairs = 0;

    // while (!merge_ctx->done()) {
    //     merge_ctx->next();
    //     total_pairs++;
    // }

    // cout << "Total pairs: " << total_pairs << endl;

    /*
     * TODO: Level stats
     */

    // printf(LEVEL_NUM_ENTRIES_PATTERN, 0, buffer.num_entries);

    // for (i = 0; i < levels.size(); i++) {
    //     cout << ", ";
    //     printf(LEVEL_NUM_ENTRIES_PATTERN, i + 1, levels[i]->num_entries);
    // }

    // cout << endl;

    /*
     * Level dumps
     */

    // buffer.dump();

    // for (const auto& level : levels) {
    //     level.dump();
    // }

    // /*
    //  * Cleanup
    //  */

    // delete buffer_run;
    // delete merge_ctx;
}
