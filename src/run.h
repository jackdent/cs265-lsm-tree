#include <cstdio>
#include <vector>

#include "types.h"
#include "bloom_filter.h"

#define TMP_FILE_PATTERN "/tmp/lsm-XXXXXX"

using namespace std;

enum MappingType {
    Read, Write
};

class Run {
    BloomFilter bloom_filter;
    KEY_t min_key, max_key;
    vector<entry_t> *mapping;
    FILE *mapping_fp;
    bool key_in_range(KEY_t) const;
    bool range_overlaps_with(KEY_t, KEY_t) const;
public:
    long size, max_size;
    string tmp_file;
    Run(long);
    ~Run(void);
    vector<entry_t> * map(MappingType);
    void unmap(void);
    VAL_t * get(KEY_t);
    vector<entry_t> * range(KEY_t, KEY_t);
    void put(entry_t);
};
