#include <queue>

#include "types.h"

using namespace std;

struct merge_entry {
    int index = 0;
    entry_t *entries;
    long num_entries;
    entry_t head(void) const {return entries[index];}
    bool done(void) const {return index == num_entries;}
    bool operator<(const merge_entry& other) const {return head() < other.head();}
    bool operator>(const merge_entry& other) const {return head() > other.head();}
};

typedef struct merge_entry merge_entry_t;

class MergeContext {
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
public:
    void add(entry_t *, long);
    entry_t next(void);
    bool done(void);
};
