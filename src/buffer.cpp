#include <iostream>

#include "buffer.h"

using namespace std;

bool Buffer::get(KEY_t key, VAL_t *val) const {
    for (const auto& entry : entries) {
        if (entry.key == key) {
            *val = entry.val;
            return true;
        }
    }

    return false;
}

bool Buffer::put(KEY_t key, VAL_t val) {
    entry_t entry;

    if (entries.size() == max_entries) {
        return false;
    } else {
        entry.key = key;
        entry.val = val;
        entries.push_front(entry);
        return true;
    }
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
