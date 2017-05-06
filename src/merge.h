#include <cassert>
#include <queue>

#include "types.h"

using namespace std;

struct merge_entry {
    int precedence;
    entry_t *entries;
    long num_entries;
    int current_index = 0;
    entry_t head(void) const {return entries[current_index];}
    bool done(void) const {return current_index == num_entries;}
    bool operator>(const merge_entry& other) const {
        // Order first by keys, then by precedence
        if (head() == other.head()) {
            assert(precedence != other.precedence);
            return precedence > other.precedence;
        } else {
            return head() > other.head();
        }
    }
};

typedef struct merge_entry merge_entry_t;

class MergeContext {
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
public:
    void add(entry_t *, long);
    entry_t next(void);
    bool done(void);
};
