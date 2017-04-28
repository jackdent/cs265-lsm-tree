#include <iostream>

#include "buffer.h"

using namespace std;

VAL_t * Buffer::get(KEY_t key) const {
    entry_t search_entry;
    set<entry_t>::iterator entry;
    VAL_t *val;

    search_entry.key = key;
    entry = entries.find(search_entry);

    if (entry == entries.end()) {
        return nullptr;
    } else {
        val = new VAL_t;
        *val = entry->val;
        return val;
    }
}

vector<entry_t> * Buffer::get_all(void) const {
    // TODO: don't want to copy set to buffer
    return new vector<entry_t>(entries.begin(), entries.end());
}

vector<entry_t> * Buffer::range(KEY_t start, KEY_t end) const {
    entry_t search_entry;
    set<entry_t>::iterator subrange_start, subrange_end;

    search_entry.key = start;
    subrange_start = entries.lower_bound(search_entry);

    search_entry.key = end;
    subrange_end = entries.upper_bound(search_entry);

    return new vector<entry_t>(subrange_start, subrange_end);
}

bool Buffer::put(KEY_t key, VAL_t val) {
    entry_t entry;

    if (entries.size() == max_entries) {
        return false;
    } else {
        entry.key = key;
        entry.val = val;
        entries.insert(entry);
        return true;
    }
}

void Buffer::empty(void) {
    entries.clear();
}

// void Buffer::dump(void) const {
//     int i;
//     entry_t entry;

//     for (i = 0; i < num_entries; i++) {
//         entry = *at(i);
//         printf(DUMP_PATTERN, entry.key, entry.val);
//         if (i < num_entries - 1) cout << " ";
//     }

//     cout << endl;
// }
