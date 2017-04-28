#include <set>
#include <vector>

#include "types.h"

using namespace std;

class Buffer {
public:
    int max_entries;
    set<entry_t> entries;
    Buffer(int max_entries) : max_entries(max_entries) {};
    VAL_t * get(KEY_t) const;
    vector<entry_t> * get_all(void) const;
    vector<entry_t> * range(KEY_t, KEY_t) const;
    bool put(KEY_t, VAL_t val);
    void empty(void);
    // void dump(void) const;
};
