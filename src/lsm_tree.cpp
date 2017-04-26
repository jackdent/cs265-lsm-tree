#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

#include "buffer.h"
#include "level.h"
#include "lsm_tree.h"
#include "merge.h"
#include "thread_pool.h"
#include "unistd.h"

using namespace std;

void die(string error_msg) {
    cerr << error_msg << endl;
    cerr << "Exiting..." << endl;
    exit(EXIT_FAILURE);
}

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

LSMTree::LSMTree(int buffer_max_entries, int depth, int fanout, int num_threads) : buffer(buffer_max_entries) {
    int i, max_enclosures;

    enclosure_size = buffer_max_entries;
    num_threads = num_threads;
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
    int i;

    assert(current >= levels.begin());

    if (current >= levels.end() - 1) {
        die("No more space in buffer.");
    }

    next = current + 1;

    /*
     * If the next level does not have space for the current level,
     * recursively merge the next level downwards to create some
     */

    if (current->enclosures.size() > next->max_enclosures - next->enclosures.size()) {
        merge_down(current + 1);
        assert(current->enclosures.size() <= next->max_enclosures - next->enclosures.size());
    }

    /*
     * The next level now has space for the current level
     */

    for (auto& enclosure : current->enclosures) {
        enclosure.map();
    }

    // TODO: only merge n down; pull from back
    while (!merge_ctx.done()) {
        entry_buffer.reserve(enclosure_size);

        for (i = 0; i < enclosure_size; i++) {
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
        next->enclosures.front().put(entry_buffer.data(), entry_buffer.size());

        entry_buffer.empty();
    }

    // Clear the current level to unmap and free the old
    // (now redundant) entry files
    current->empty();
}

void LSMTree::put(KEY_t key, VAL_t val) {
    if (!buffer.put(key, val)) {
        // Flush level 0 if necessary
        if (levels.front().max_enclosures - levels.front().enclosures.size() == 0) {
            merge_down(levels.begin());
        }

        // Sort the buffer and flush it to level 0
        stable_sort(buffer.entries.begin(), buffer.entries.end());
        levels.front().enclosures.emplace_front(enclosure_size);
        levels.front().enclosures.front().put(&buffer.entries[0], buffer.entries.size());

        // Empty the buffer and insert the key/value pair
        buffer.entries.empty();
        assert(buffer.put(key, val));
    }
}

Enclosure * LSMTree::get_enclosure(int index) const {
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

void LSMTree::get(KEY_t key) const {
    VAL_t val;
    int latest_enclosure;
    SpinLock lock;
    atomic<int> counter;
    ThreadPool thread_pool(num_threads);

    if (buffer.get(key, &val)) {
        if (val != VAL_TOMBSTONE) cout << val;
        cout << endl;
        return;
    }

    counter = 0;
    latest_enclosure = -1;

    function<void (void)> search = [&] {
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
                val = *current_val;
                latest_enclosure = current_enclosure;
            }

            lock.unlock();

            delete current_val;
        } else {
            // Keep searching until we find an entry.
            search();
        }
    };

    thread_pool.launch(search);
    thread_pool.wait();

    if (latest_enclosure >= 0 && val != VAL_TOMBSTONE) {
        cout << val;
    }

    cout << endl;
}

void LSMTree::range(KEY_t start, KEY_t end) const {
    vector<entry_t> *buffer_range;
    map<int, vector<entry_t> *> ranges;
    SpinLock lock;
    atomic<int> counter;
    ThreadPool thread_pool(num_threads);
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

    buffer_range = new vector<entry_t>;
    buffer_range->reserve(buffer.entries.size());

    for (const auto& entry : buffer.entries) {
        if (entry.key >= start && entry.key <= end) {
            buffer_range->push_back(entry);
        }
    }

    stable_sort(buffer_range->begin(), buffer_range->end());
    ranges.insert({0, buffer_range});

    /*
     * Search enclosures
     */

    counter = 0;

    function<void (void)> search = [&] {
        int current_enclosure;
        Enclosure *enclosure;
        vector<entry_t> *range;

        current_enclosure = counter++;
        enclosure = get_enclosure(current_enclosure);

        if (enclosure == nullptr) {
            // No more enclosures to search
            return;
        }

        range = enclosure->range(start, end);

        if (range != nullptr) {
            lock.lock();
            ranges.insert({current_enclosure + 1, range});
            lock.unlock();
        }

        search();
    };

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

// tuple<Enclosure *, MergeContext *> LSMTree::all(void) const {
//     entry_t entry_buffer[buffer.num_entries];
//     Enclosure *buffer_enclosure;
//     MergeContext *merge_ctx;

//     buffer_enclosure = new Enclosure();
//     memcpy(entry_buffer, buffer.entries, buffer.num_entries * sizeof(entry_t));
//     stable_sort(entry_buffer, &entry_buffer[buffer.num_entries]);
//     assert(buffer_enclosure->put(entry_buffer, buffer.num_entries));

//     merge_ctx = new MergeContext();
//     merge_ctx->add(*buffer_enclosure);

//     for (const auto& level : levels) {
//         merge_ctx->add(level.enclosures);
//     }

//     return make_tuple(buffer_enclosure, merge_ctx);
// }

void LSMTree::stats(void) const {
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

void command_loop(LSMTree& tree) {
    char command;
    KEY_t key_a, key_b;
    VAL_t val;
    string file_path;

    while (cin >> command) {
        switch (command) {
        case 'p':
            cin >> key_a >> val;

            if (val < VAL_MIN || val > VAL_MAX) {
                die("Could not insert value " + to_string(val) + ": out of range.");
            } else {
                tree.put(key_a, val);
            }

            break;
        case 'g':
            cin >> key_a;
            tree.get(key_a);
            break;
        case 'r':
            cin >> key_a >> key_b;
            tree.range(key_a, key_b);
            break;
        case 'd':
            cin >> key_a;
            tree.del(key_a);
            break;
        case 'l':
            cin.ignore();
            getline(cin, file_path);
            // Trim quotes
            tree.load(file_path.substr(1, file_path.size() - 2));
            break;
        case 's':
            tree.stats();
            break;
        default:
            die("Invalid command.");
        }
    }
}

int main(int argc, char *argv[]) {
    int opt, buffer_num_pages, buffer_max_entries, depth, fanout, num_threads;

    buffer_num_pages = DEFAULT_BUFFER_NUM_PAGES;
    depth = DEFAULT_TREE_DEPTH;
    fanout = DEFAULT_TREE_FANOUT;
    num_threads = DEFAULT_THREAD_COUNT;

    while ((opt = getopt(argc, argv, "b:d:f:t:")) != -1) {
        switch (opt) {
        case 'b' : buffer_num_pages = atoi(optarg); break;
        case 'd' : depth = atoi(optarg); break;
        case 'f' : fanout = atoi(optarg); break;
        case 't' : num_threads = atoi(optarg); break;
        default  : die("Usage: " + string(argv[0]) + " [-b buffer pages] [-d depth] [-f fanout] <[workload]");
        }
    }

    buffer_max_entries = buffer_num_pages * getpagesize() / sizeof(entry_t);
    LSMTree tree = LSMTree(buffer_max_entries, depth, fanout, num_threads);
    command_loop(tree);

    return 0;
}
