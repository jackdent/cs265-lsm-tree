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
    : buffer(buffer_max_entries), thread_pool(num_threads), merge_ratio(merge_ratio)
{
    int i, max_enclosures;

    enclosure_size = buffer_max_entries;
    max_enclosures = 1;

    for (i = 1; i < depth; i++) {
        max_enclosures *= fanout;
        levels.emplace_back(max_enclosures);
    }
}

void LSMTree::merge_down(vector<Level>::iterator current) {
    vector<Level>::iterator next;
    MergeContext merge_ctx;
    vector<entry_t> entry_buffer;
    entry_t entry;
    int merge_size, i, j;

    assert(current >= levels.begin());

    if (current >= levels.end() - 1) {
        die("No more space in buffer.");
    }

    next = current + 1;

    /*
     * If the next level does not have space for the current level,
     * recursively merge the next level downwards to create some
     */

    if (current->enclosures.size() > next->remaining()) {
        merge_down(current + 1);
        assert(current->enclosures.size() <= next->remaining());
    }

    /*
     * The next level now has space for the current level, so we
     * push "merge_size" enclosures downwards
     */

    merge_size = merge_ratio * current->max_enclosures;

    if (merge_size > current->enclosures.size()) {
        merge_size = current->enclosures.size();
    }

    for (i = 0; i < merge_size; i++) {
        merge_ctx.add(current->enclosures[i].map());
    }

    while (!merge_ctx.done()) {
        entry_buffer.reserve(enclosure_size);

        for (j = 0; j < enclosure_size; j++) {
            if (merge_ctx.done()) {
                break;
            } else {
                entry = merge_ctx.next();

                // We can remove deleted keys from the final level
                if (!(next == levels.end() - 1 && entry.val == VAL_TOMBSTONE)) {
                    entry_buffer.push_back(entry);
                }
            }

        }

        // Add a new enclosure to the next level
        next->enclosures.emplace_front(enclosure_size);
        next->enclosures.front().put(entry_buffer);

        entry_buffer.empty();
    }

    /*
     * Unmap and delete the old (now redundant) entry files
     */

    for (i = 0; i < merge_size; i++) {
        current->enclosures[i].unmap();
    }

    current->enclosures.erase(current->enclosures.begin(), current->enclosures.begin() + merge_size);
}

void LSMTree::put(KEY_t key, VAL_t val) {
    if (!buffer.put(key, val)) {
        // Flush level 0 if necessary
        if (levels.front().remaining() == 0) {
            merge_down(levels.begin());
        }

        // Flush the buffer to level 0
        vector<entry_t> *buffer_entries = buffer.get_all();
        levels.front().enclosures.emplace_front(enclosure_size);
        levels.front().enclosures.front().put(*buffer_entries);
        delete buffer_entries;

        // Empty the buffer and insert the key/value pair
        buffer.empty();
        assert(buffer.put(key, val));
    }
}

Enclosure * LSMTree::get_enclosure(int index) {
    for (const auto& level : levels) {
        if (index < level.enclosures.size()) {
            // TODO: why do I need to cast from const?
            return (Enclosure *) &level.enclosures[index];
        } else {
            index -= level.enclosures.size();
        }
    }

    return nullptr;
};

void LSMTree::get(KEY_t key) {
    VAL_t *buffer_val;
    VAL_t latest_val;
    int latest_enclosure;
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
     * Search enclosures
     */

    counter = 0;
    latest_enclosure = -1;

    worker_task search = [&] {
        int current_enclosure;
        Enclosure *enclosure;
        VAL_t *current_val;

        current_enclosure = counter++;
        enclosure = get_enclosure(current_enclosure);

        if (enclosure == nullptr || (current_enclosure > latest_enclosure
                                     && latest_enclosure > 0)) {
            // If we have run out of enclosures, or if we have
            // discovered a key in a more recent enclosure,
            // stop searching.
            return;
        }

        current_val = enclosure->get(key);

        if (current_val != nullptr) {
            // Search the current enclosure. If we find the key in
            // the enclosure, update val if the enclosure is more
            // recent than the last. Then stop the search on this
            // thread, since the next enclosure is guaranteed to
            // be less recent.
            lock.lock();

            if (latest_enclosure < 0 || current_enclosure < latest_enclosure) {
                latest_enclosure = current_enclosure;
                latest_val = *current_val;
            }

            lock.unlock();

            delete current_val;
        } else {
            // Keep searching until we find an entry.
            search();
        }
    };

    thread_pool.launch(search);
    thread_pool.wait_all();

    if (latest_enclosure >= 0 && latest_val != VAL_TOMBSTONE) cout << latest_val;
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
     * Search enclosures
     */

    counter = 0;

    worker_task search = [&] {
        int current_enclosure;
        Enclosure *enclosure;

        current_enclosure = counter++;
        enclosure = get_enclosure(current_enclosure);

        if (enclosure == nullptr) {
            // No more enclosures to search
            return;
        }

        lock.lock();
        ranges.insert({current_enclosure + 1, enclosure->range(start, end)});
        lock.unlock();

        search();
    };

    thread_pool.launch(search);
    thread_pool.wait_all();

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

    // tie(buffer_enclosure, merge_ctx) = all();
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

    // delete buffer_enclosure;
    // delete merge_ctx;
}
