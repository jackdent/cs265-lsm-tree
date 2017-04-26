#include <queue>

#include "types.h"

using namespace std;

struct merge_entry {
    int index = 0;
    vector<entry_t> *entries;
    entry_t head(void) const {return entries->at(index);}
    bool done(void) const {return index == entries->size();}
    bool operator<(const merge_entry& other) const {return head() < other.head();}
    bool operator>(const merge_entry& other) const {return head() > other.head();}
};

typedef struct merge_entry merge_entry_t;

class MergeContext {
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
public:
    void add(vector<entry_t> *);
    entry_t next(void);
    bool done(void);
};
