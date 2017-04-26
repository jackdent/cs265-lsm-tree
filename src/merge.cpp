#include <cassert>
#include <iostream>

#include "merge.h"

void MergeContext::add(vector<entry_t> *entries) {
    merge_entry_t merge_entry;

    assert(entries->size() > 0);
    merge_entry.entries = entries;

    queue.push(merge_entry);
}

entry_t MergeContext::next(void) {
    merge_entry_t current, next;
    entry_t entry;

    current = queue.top();
    next = current;

    // Only release the most recent value for a given key
    while (next.head().key == current.head().key && !queue.empty()) {
        queue.pop();

        next.index += 1;
        if (!next.done()) queue.push(next);

        next = queue.top();
    }

    return current.head();
}

bool MergeContext::done(void) {
    return queue.empty();
}
