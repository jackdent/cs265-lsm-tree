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
#define DUMP_PATTERN "%d:%d"
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
    unsigned long num_entries;
    static unsigned long max_entries;
    EntryContainer(void) : num_entries(0) {}
};

class Enclosure : public EntryContainer {
    BloomFilter bloom_filter;
    KEY_t max_key, min_key;
public:
    string tmp_file;
    Enclosure(void);
    ~Enclosure(void);
    bool get(KEY_t, VAL_t *) const;
    bool put(entry_t *, unsigned long);
};

typedef struct merge_entry merge_entry_t;

class MergeContext {
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
public:
    ~MergeContext(void);
    void add(Enclosure const &enclosure);
    void add(deque<Enclosure> const &);
    entry_t next(void);
    bool done(void);
};

struct merge_entry {
    entry_t entry;
    ifstream *stream;
    bool operator<(const merge_entry& other) const {return entry < other.entry;}
    bool operator>(const merge_entry& other) const {return entry > other.entry;}
};

class Buffer : public EntryContainer {
public:
    entry_t *entries;
    Buffer();
    ~Buffer();
    bool get(KEY_t, VAL_t *) const;
    bool put(KEY_t, VAL_t val);
    void dump(void) const;
    void empty(void) {num_entries = 0;}
    entry_t * at(unsigned long i) const {return &entries[max_entries - num_entries + i];}
    entry_t * head(void) const {return at(0);}
    entry_t * tail(void) const {return at(num_entries);}
};

class Level {
public:
    int max_enclosures;
    deque<Enclosure> enclosures;
    Level(unsigned long max_enclosures) : max_enclosures(max_enclosures) {}
    bool get(KEY_t, VAL_t *) const;
    void dump(void) const;
    void empty(void);
};

class LSMTree {
    Buffer buffer;
    vector<Level> levels;
    void merge_down(int);
public:
    LSMTree(int, int);
    tuple<Enclosure *, MergeContext *> all(void) const;
    void put(KEY_t, VAL_t);
    void get(KEY_t) const;
    void range(KEY_t, KEY_t) const;
    void del(KEY_t);
    void load(string);
    void stats(void) const;
};
