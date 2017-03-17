#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "unistd.h"
#include "lsm.h"

using namespace std;

void die(string error_msg) {
    cerr << error_msg << endl;
    cerr << "Exiting..." << endl;
    exit(EXIT_FAILURE);
}

/*
 * Entry
 */

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
 * Merge context
 */

MergeContext::MergeContext(vector<Level *>& levels) {
    merge_entry_t merge_entry;
    entry_t entry;
    int i;

    for (i = 0; i < levels.size(); i++) {
        streams.push_back(ifstream(levels[i]->tmp_file, ifstream::binary));

        if (levels[i]->num_entries > 0) {
            streams[i] >> merge_entry.entry;
            merge_entry.stream = i;
            queue.push(merge_entry);
        }
    }
}

entry_t MergeContext::next(void) {
    merge_entry_t first, current, replacement;
    entry_t entry;

    first = queue.top();
    current = first;

    // Only release the most recent value for a given key
    while (current.entry.key == first.entry.key && !queue.empty()) {
        queue.pop();

        // The queue will be empty after all the streams
        // have been exhausted
        if (streams[current.stream] >> entry) {
            replacement.entry = entry;
            replacement.stream = current.stream;
            queue.push(replacement);
        }

        current = queue.top();
    }

    return first.entry;
}

bool MergeContext::done(void) {
    return queue.empty();
}

/*
 * Buffer
 */

Buffer::Buffer(unsigned long max_entries) : EntryContainer(max_entries) {
    entries = new entry_t[max_entries];
}

Buffer::~Buffer() {
    delete[] entries;
}

bool Buffer::get(KEY_t key, VAL_t *val) {
    int i;

    for (i = 0; i < num_entries; i++) {
        if (entries[head() + i].key == key) {
            *val = entries[head() + i].val;
            return true;
        }
    }

    return false;
}

bool Buffer::put(KEY_t key, VAL_t val) {
    entry_t *entry;

    if (num_entries == max_entries) {
        return false;
    } else {
        num_entries += 1;
        entry = &entries[head()];
        entry->key = key;
        entry->val = val;
        return true;
    }
}

void Buffer::dump(void) {
    int i;
    entry_t entry;

    for (i = 0; i < num_entries; i++) {
        entry = entries[head() + i];
        printf(DUMP_PATTERN, entry.key, entry.val, 0);
        if (i < num_entries - 1) cout << " ";
    }

    cout << endl;
}

Level * Buffer::to_level(void) {
    Level *buffer_level;

    // Sort the buffer to retain the disk level invariant.
    stable_sort(&entries[head()], &entries[max_entries]);

    // Write the sorted buffer to disk.
    buffer_level = new Level(max_entries);
    assert(buffer_level->put(&entries[head()], num_entries));

    return buffer_level;
}

/*
 * Level
 */

Level::Level(unsigned long max_entries) : EntryContainer(max_entries) {
    char *tmp_fn;
    tmp_fn = strdup(TMP_FILE_PATTERN);
    tmp_file = mktemp(tmp_fn);
}

Level::~Level(void) {
    remove(tmp_file.c_str());
}

bool Level::get(KEY_t key, VAL_t *val) {
    ifstream stream;
    unsigned long low, mid, high;
    entry_t entry;

    if (bloom_filter.is_set(key)) {
        low = 0;
        high = num_entries - 1;
        stream.open(tmp_file, ifstream::binary);

        // Perform a binary search to find the key
        while (low <= high) {
            mid = low + (high - low) / 2;
            stream.seekg(mid * sizeof(entry_t));
            stream >> entry;

            if (entry.key == key) {
                *val = entry.val;
                return true;
            } else if (entry.key < key) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
    }

    return false;
}

bool Level::put(KEY_t key, VAL_t val) {
    ofstream stream;
    entry_t entry;

    if (num_entries == max_entries) {
        return false;
    } else {
        num_entries += 1;

        entry.key = key;
        entry.val = val;

        stream.open(tmp_file, ofstream::app | ofstream::binary);
        stream << entry;

        bloom_filter.set(key);

        return true;
    }
}

bool Level::put(entry_t *entries, unsigned long len) {
    int i;
    ofstream stream;

    if (num_entries + len > max_entries) {
        return false;
    } else {
        num_entries += len;

        stream.open(tmp_file, ofstream::app | ofstream::binary);
        stream.write((char *)entries, num_entries * sizeof(entry_t));

        for (i = 0; i < len; i++) {
            bloom_filter.set(entries[i].key);
        }

        return true;
    }
}

void Level::dump(int id) {
    ifstream stream;
    entry_t entry;

    stream.open(tmp_file, ifstream::binary);

    while (stream >> entry) {
        printf(DUMP_PATTERN, entry.key, entry.val, id);
        if (!stream.eof()) cout << " ";
    }

    cout << endl;
}

/*
 * LSM Tree
 */

LSMTree::LSMTree(unsigned long buffer_entries, int depth, int fanout) : buffer(buffer_entries) {
    int i, exp;
    unsigned long level_max_entries;
    Level *level;

    for (i = 1; i < depth; i++) {
        level_max_entries = buffer_entries;

        for (exp = i; exp > 0; exp--) {
            level_max_entries *= fanout;
        }

        level = new Level(level_max_entries);
        levels.push_back(level);
    }
}

LSMTree::~LSMTree() {
    int i;

    for (i = 0; i < levels.size(); i++) {
        delete levels[i];
    }
}

void LSMTree::merge_down(int i) {
    Level *prev, *new_prev, *new_current;
    unsigned long prev_max_entries;
    entry_t entry;

    if (i == 0) {
        prev = buffer.to_level();
    } else if (i < levels.size()) {
        prev = levels[i - 1];
        prev_max_entries = prev->max_entries;
    } else {
        die("No more space in buffer.");
    }

    assert(prev->max_entries <= levels[i]->max_entries);

    /*
     * If the current level does not have space for the prev level,
     * recursively merge the current level downwards to create some
     */

    if (prev->num_entries > levels[i]->max_entries - levels[i]->num_entries) {
        merge_down(i + 1);
        assert(levels[i]->num_entries == 0);
    }

    /*
     * The current level has space for the prev level, so we merge
     * the two into a new level
     */

    vector<Level *> merge_src = {prev, levels[i]};
    new_current = new Level(levels[i]->max_entries);
    MergeContext merge_ctx = MergeContext(merge_src);

    while (!merge_ctx.done()) {
        entry = merge_ctx.next();

        // We can remove deleted keys from the final level
        if (!(i == levels.size() - 1 && entry.val == VAL_TOMBSTONE)) {
            assert(new_current->put(entry.key, entry.val));
        }
    }

    /*
     * Update the levels, freeing the old (now redundant) files
     */

    delete prev;

    if (i == 0) {
        // The key/value pairs that were previously in the buffer
        // have now made it to disk
        buffer.empty();
    } else {
        levels[i - 1] = new Level(prev_max_entries);
    }

    delete levels[i];
    levels[i] = new_current;
}

tuple<Level *, MergeContext *> LSMTree::all(void) {
    Level *buffer_level;
    vector<Level *> tmp_levels;
    MergeContext *ctx;

    buffer_level = buffer.to_level();
    tmp_levels.push_back(buffer_level);
    tmp_levels.insert(tmp_levels.end(), levels.begin(), levels.end());

    ctx = new MergeContext(tmp_levels);

    return make_tuple(buffer_level, ctx);
}

void LSMTree::put(KEY_t key, VAL_t val) {
    if (!buffer.put(key, val)) {
        assert(buffer.num_entries == buffer.max_entries);
        flush_buffer();
        assert(buffer.put(key, val));
    }
}

void LSMTree::get(KEY_t key) {
    VAL_t val;
    int i;

    if (buffer.get(key, &val)) {
        if (val != VAL_TOMBSTONE) cout << val;
        cout << endl;
        return;
    }

    for (i = 0; i < levels.size(); i++) {
        if (levels[i]->get(key, &val)) {
            if (val != VAL_TOMBSTONE) cout << val;
            cout << endl;
            return;
        }
    }

    cout << endl;
}

void LSMTree::range(KEY_t start, KEY_t end) {
    Level *buffer_level;
    MergeContext *merge_ctx;
    entry_t entry;

    // Filter to each subrange
    tie(buffer_level, merge_ctx) = all();

    while (!merge_ctx->done()) {
        entry = merge_ctx->next();

        if (entry.key >= start) {
            if (entry.key < end) {
                cout << entry.key << ":" << entry.val;
                if (!merge_ctx->done()) cout << " ";
            } else {
                break;
            }
        }
    }

    cout << endl;

    delete buffer_level;
    delete merge_ctx;
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
    Level *buffer_level;
    MergeContext *merge_ctx;
    unsigned long total_pairs;
    int i;

    /*
     * Tree stats
     */

    tie(buffer_level, merge_ctx) = all();
    total_pairs = 0;

    while (!merge_ctx->done()) {
        merge_ctx->next();
        total_pairs++;
    }

    cout << "Total pairs: " << total_pairs << endl;

    /*
     * Level stats
     */

    printf(LEVEL_NUM_ENTRIES_PATTERN, 0, buffer.num_entries);

    for (i = 0; i < levels.size(); i++) {
        cout << ", ";
        printf(LEVEL_NUM_ENTRIES_PATTERN, i + 1, levels[i]->num_entries);
    }

    cout << endl;

    /*
     * Level dumps
     */

    buffer.dump();

    for (i = 0; i < levels.size(); i++) {
        levels[i]->dump(i + 1);
    }

    /*
     * Cleanup
     */

    delete buffer_level;
    delete merge_ctx;
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
    int opt, buffer_num_pages, depth, fanout;
    unsigned long buffer_max_entries;

    buffer_num_pages = DEFAULT_BUFFER_NUM_PAGES;
    depth = DEFAULT_TREE_DEPTH;
    fanout = DEFAULT_TREE_FANOUT;

    while ((opt = getopt(argc, argv, "b:d:f:")) != -1) {
        switch (opt) {
        case 'b' : buffer_num_pages = atoi(optarg); break;
        case 'd' : depth = atoi(optarg); break;
        case 'f' : fanout = atoi(optarg); break;
        default  : die("Usage: " + string(argv[0]) + " [-b buffer pages] [-d depth] [-f fanout] <[workload]");
        }
    }

    buffer_max_entries = buffer_num_pages * getpagesize() / sizeof(entry_t);
    LSMTree tree = LSMTree(buffer_max_entries, depth, fanout);
    command_loop(tree);

    return 0;
}
