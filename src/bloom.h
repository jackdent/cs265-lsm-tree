#include <bitset>

#include "types.h"

using namespace std;

#define BLOOM_FILTER_SIZE 4096

class BloomFilter {
   bitset<BLOOM_FILTER_SIZE> table;
   uint64_t hash_1(KEY_t);
   uint64_t hash_2(KEY_t);
   uint64_t hash_3(KEY_t);
public:
    void set(KEY_t);
    bool is_set(KEY_t);
};
