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
 * Enclosure
 */

Enclosure::Enclosure() : min_key(KEY_MIN), max_key(KEY_MAX) {
    char *tmp_fn;
    tmp_fn = strdup(TMP_FILE_PATTERN);
    tmp_file = mktemp(tmp_fn);
}

Enclosure::~Enclosure(void) {
    remove(tmp_file.c_str());
}

bool Enclosure::get(KEY_t key, VAL_t *val) const {
    ifstream stream;
    entry_t entry_buffer[EntryContainer::max_entries];
    entry_t entry;
    unsigned long low, mid, high;

    if (key >= min_key && key <= max_key && bloom_filter.is_set(key)) {
        stream.open(tmp_file, ifstream::binary);
        stream.read((char *)&entry_buffer, num_entries * sizeof(entry_t));

        low = 0;
        high = num_entries - 1;

        // Perform a binary search to find the key
        while (low <= high) {
            mid = low + (high - low) / 2;
            entry = entry_buffer[mid];

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

bool Enclosure::put(entry_t *entries, unsigned long len) {
    int i;
    ofstream stream;

    if (num_entries + len > max_entries) {
        return false;
    } else {
        num_entries += len;

        for (i = 0; i < len; i++) {
            min_key = min(entries[i].key, min_key);
            max_key = max(entries[i].key, max_key);
            bloom_filter.set(entries[i].key);
        }

        stream.open(tmp_file, ofstream::app | ofstream::binary);
        stream.write((char *)entries, num_entries * sizeof(entry_t));

        return true;
    }
}

/*
 * Merge context
 */

void MergeContext::add(Enclosure const &enclosure) {
    ifstream *stream;
    merge_entry_t merge_entry;

    assert(enclosure.num_entries > 0);

    stream = new ifstream(enclosure.tmp_file, ifstream::binary);
    merge_entry.stream = stream;
    assert(*stream >> merge_entry.entry);

    queue.push(merge_entry);
}

void MergeContext::add(deque<Enclosure> const &enclosures) {
    for (auto const &enclosure : enclosures) {
        add(enclosure);
    }
}

MergeContext::~MergeContext(void) {
    merge_entry_t merge_entry;

    while (!queue.empty()) {
        merge_entry = queue.top();
        delete merge_entry.stream;
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
        if (*current.stream >> entry) {
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

Buffer::Buffer() {
    entries = new entry_t[max_entries];
}

Buffer::~Buffer() {
    delete[] entries;
}

bool Buffer::get(KEY_t key, VAL_t *val) const {
    int i;

    for (i = 0; i < num_entries; i++) {
        if (at(i)->key == key) {
            *val = at(i)->val;
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
        entry = head();
        entry->key = key;
        entry->val = val;
        return true;
    }
}

void Buffer::dump(void) const {
    int i;
    entry_t entry;

    for (i = 0; i < num_entries; i++) {
        entry = *at(i);
        printf(DUMP_PATTERN, entry.key, entry.val);
        if (i < num_entries - 1) cout << " ";
    }

    cout << endl;
}

/*
 * Level
 */

bool Level::get(KEY_t key, VAL_t *val) const {
    for (auto const &enclosure : enclosures) {
        if (enclosure.get(key, val)) {
            return true;
        }
    }

    return false;
}

void Level::dump(void) const {
    MergeContext merge_ctx;
    entry_t entry;

    merge_ctx = MergeContext();
    merge_ctx.add(enclosures);

    while (!merge_ctx.done()) {
        entry = merge_ctx.next();
        printf(DUMP_PATTERN, entry.key, entry.val);
        if (!merge_ctx.done()) cout << " ";
    }

    cout << endl;
}

void Level::empty(void) {
    enclosures.clear();
}

/*
 * LSM Tree
 */

LSMTree::LSMTree(int depth, int fanout) {
    int i, max_enclosures;

    max_enclosures = 1;

    for (i = 1; i < depth; i++) {
        max_enclosures *= fanout;
        levels.emplace_back(max_enclosures);
    }
}

void LSMTree::merge_down(int level) {
    Level *upper, *lower;
    MergeContext merge_ctx;
    entry_t entry_buffer[EntryContainer::max_entries];
    entry_t entry;
    unsigned long i, num_entries;

    assert(level >= 0);

    if (level > levels.size() - 1) {
        die("No more space in buffer.");
    }

    upper = &levels[level];
    lower = &levels[level + 1];

    /*
     * If the lower level does not have space for the upper level,
     * recursively merge the lower level downwards to create some
     */

    if (upper->enclosures.size() > lower->max_enclosures - lower->enclosures.size()) {
        merge_down(level + 1);
        assert(upper->enclosures.size() <= lower->max_enclosures - lower->enclosures.size());
    }

    /*
     * The lower level now has space for the upper level
     */

    merge_ctx = MergeContext();
    merge_ctx.add(upper->enclosures);

    // TODO: only merge n down
    while (!merge_ctx.done()) {
        num_entries = 0;

        for (i = 0; i < EntryContainer::max_entries; i++) {
            if (merge_ctx.done()) {
                break;
            } else {
                entry = merge_ctx.next();

                // We can remove deleted keys from the final level
                if (!(lower == &levels.back() && entry.val == VAL_TOMBSTONE)) {
                    entry_buffer[num_entries] = entry;
                    num_entries += 1;
                }
            }

        }

        // Add a new enclosure to the lower level
        lower->enclosures.emplace_front();
        assert(lower->enclosures.front().put(entry_buffer, num_entries));
    }

    // Clear the upper level, which frees the old and now redundant
    // enclosure files
    upper->empty();
}

tuple<Enclosure *, MergeContext *> LSMTree::all(void) const {
    entry_t entry_buffer[buffer.num_entries];
    Enclosure *buffer_enclosure;
    MergeContext *merge_ctx;

    buffer_enclosure = new Enclosure();
    memcpy(entry_buffer, buffer.entries, buffer.num_entries * sizeof(entry_t));
    stable_sort(entry_buffer, &entry_buffer[buffer.num_entries]);
    assert(buffer_enclosure->put(entry_buffer, buffer.num_entries));

    merge_ctx = new MergeContext();
    merge_ctx->add(*buffer_enclosure);

    for (auto const &level : levels) {
        merge_ctx->add(level.enclosures);
    }

    return make_tuple(buffer_enclosure, merge_ctx);
}

void LSMTree::put(KEY_t key, VAL_t val) {
    if (!buffer.put(key, val)) {
        assert(buffer.num_entries == buffer.max_entries);

        // Flush level 0 if necessary
        if (levels[0].max_enclosures - levels[0].enclosures.size() == 0) {
            merge_down(0);
        }

        // Sort the buffer and flush it to level 0
        stable_sort(buffer.head(), buffer.tail());
        levels[0].enclosures.emplace_front();
        assert(levels[0].enclosures.front().put(buffer.head(), buffer.num_entries));

        // Empty the buffer and insert the key/value pair
        buffer.empty();
        assert(buffer.put(key, val));
    }
}

void LSMTree::get(KEY_t key) const {
    VAL_t val;

    if (buffer.get(key, &val)) {
        if (val != VAL_TOMBSTONE) cout << val;
        cout << endl;
        return;
    }

    for (auto const &level : levels) {
        if (level.get(key, &val)) {
            if (val != VAL_TOMBSTONE) cout << val;
            cout << endl;
            return;
        }
    }

    cout << endl;
}

void LSMTree::range(KEY_t start, KEY_t end) const {
    Enclosure *buffer_enclosure;
    MergeContext *merge_ctx;
    entry_t entry;

    // Filter to each subrange
    tie(buffer_enclosure, merge_ctx) = all();

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

    delete buffer_enclosure;
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

void LSMTree::stats(void) const {
    Enclosure *buffer_enclosure;
    MergeContext *merge_ctx;
    unsigned long total_pairs;

    /*
     * Tree stats
     */

    tie(buffer_enclosure, merge_ctx) = all();
    total_pairs = 0;

    while (!merge_ctx->done()) {
        merge_ctx->next();
        total_pairs++;
    }

    cout << "Total pairs: " << total_pairs << endl;

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

    buffer.dump();

    for (auto const &level : levels) {
        level.dump();
    }

    /*
     * Cleanup
     */

    delete buffer_enclosure;
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

unsigned long EntryContainer::max_entries;

int main(int argc, char *argv[]) {
    int opt, buffer_num_pages, depth, fanout;

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

    EntryContainer::max_entries = buffer_num_pages * getpagesize() / sizeof(entry_t);
    LSMTree tree = LSMTree(depth, fanout);
    command_loop(tree);

    return 0;
}
