#include <deque>

#include "types.h"

class Buffer {
public:
    int max_entries;
    std::deque<entry_t> entries;
    Buffer(int max_entries) : max_entries(max_entries) {};
    bool get(KEY_t, VAL_t *) const;
    bool put(KEY_t, VAL_t val);
    // void dump(void) const;
};

