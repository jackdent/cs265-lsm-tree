#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <vector>

#include "bloom.h"
#include "types.h"

using namespace std;

#define DEFAULT_TREE_DEPTH 3
#define DEFAULT_TREE_FANOUT 10
#define DEFAULT_BUFFER_NUM_PAGES 1000

#define TMP_FILE_PATTERN "/tmp/lsm-XXXXXX"
#define DUMP_PATTERN "%d:%d:L%d"
#define LEVEL_NUM_ENTRIES_PATTERN "LVL%d: %lu"

struct entry {
    KEY_t key;
    VAL_t val;
    bool operator<(const entry& other) const {return key < other.key;}
    bool operator>(const entry& other) const {return key > other.key;}
};

typedef struct entry entry_t;

class EntryContainer {
public:
    unsigned long num_entries, max_entries;
    EntryContainer(unsigned long max_entries) : max_entries(max_entries) {num_entries = 0;}
};

class Level : public EntryContainer {
    BloomFilter bloom_filter;
public:
    string tmp_file;
    Level(unsigned long);
    ~Level(void);
    bool get(KEY_t, VAL_t *);
    bool put(KEY_t, VAL_t val);
    bool put(entry_t *, unsigned long);
    void dump(int);
};

class Buffer : public EntryContainer {
    unsigned long head() {return max_entries - num_entries;}
public:
    entry_t *entries;
    Buffer(unsigned long);
    ~Buffer();
    bool get(KEY_t, VAL_t *);
    bool put(KEY_t, VAL_t val);
    void dump(void);
    void sort(void);
    void empty(void) {num_entries = 0;}
    Level * to_level(void);
};

struct merge_entry {
    entry_t entry;
    int stream;
    bool operator<(const merge_entry& other) const {return entry < other.entry;}
    bool operator>(const merge_entry& other) const {return entry > other.entry;}
};

typedef struct merge_entry merge_entry_t;

class MergeContext {
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
    vector<ifstream> streams;
public:
    MergeContext(vector<Level *> &);
    entry_t next(void);
    bool done(void);
};

class LSMTree {
    Buffer buffer;
    vector<Level *> levels;
    void merge_down(int);
    void flush_buffer(void) {merge_down(0);};
public:
    LSMTree(unsigned long, int, int);
    ~LSMTree();
    int depth() {return levels.size() + 1;}
    tuple<Level *, MergeContext *> all(void);
    void put(KEY_t, VAL_t);
    void get(KEY_t);
    void range(KEY_t, KEY_t);
    void del(KEY_t);
    void load(string);
    void stats(void);
};
