#include <vector>

#include "types.h"
#include "bloom_filter.h"

#define TMP_FILE_PATTERN "/tmp/lsm-XXXXXX"

using namespace std;

class Run {
    BloomFilter bloom_filter;
    KEY_t min_key, max_key;
    entry_t *mapping;
    int mapping_fd;
    bool key_in_range(KEY_t) const;
    bool range_overlaps_with(KEY_t, KEY_t) const;
    long file_size() {return max_size * sizeof(entry_t);}
public:
    long size, max_size;
    string tmp_file;
    Run(long);
    ~Run(void);
    entry_t * map_read(void);
    entry_t * map_write(void);
    void unmap(long);
    void unmap_read(void);
    void unmap_write(void);
    long lower_bound(KEY_t);
    long upper_bound(KEY_t);
    VAL_t * get(KEY_t);
    vector<entry_t> * range(KEY_t, KEY_t);
    void put(entry_t);
};
