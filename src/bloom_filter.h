#include <boost/dynamic_bitset.hpp>
#include <bitset>

#include "types.h"

class BloomFilter {
   boost::dynamic_bitset<> table;
   uint64_t hash_1(KEY_t) const;
   uint64_t hash_2(KEY_t) const;
   uint64_t hash_3(KEY_t) const;
public:
    BloomFilter(long length) : table(length) {}
    void set(KEY_t);
    bool is_set(KEY_t) const;
};
