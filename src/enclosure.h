#include <cstdio>
#include <vector>

#include "types.h"
#include "bloom_filter.h"

#define TMP_FILE_PATTERN "/tmp/lsm-XXXXXX"

using namespace std;

class Enclosure {
    BloomFilter bloom_filter;
    KEY_t min_key, max_key;
    vector<entry_t> *mapping;
    FILE *mapping_fp;
    bool key_in_range(KEY_t) const;
    bool range_overlaps_with(KEY_t, KEY_t) const;
public:
    int num_entries, max_entries;
    string tmp_file;
    Enclosure(int);
    ~Enclosure(void);
    int file_size(void) {return num_entries * sizeof(entry_t);}
    void map(void);
    void unmap(void);
    VAL_t * get(KEY_t);
    vector<entry_t> * range(KEY_t, KEY_t);
    void put(entry_t *, int);
};
