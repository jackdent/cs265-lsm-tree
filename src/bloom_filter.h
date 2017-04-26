#include <bitset>

#include "types.h"

#define BLOOM_FILTER_SIZE 4096

class BloomFilter {
   std::bitset<BLOOM_FILTER_SIZE> table;
   uint64_t hash_1(KEY_t) const;
   uint64_t hash_2(KEY_t) const;
   uint64_t hash_3(KEY_t) const;
public:
    void set(KEY_t);
    bool is_set(KEY_t) const;
};
