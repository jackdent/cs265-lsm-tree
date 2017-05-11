#include <cassert>
#include <iostream>

#include "merge.h"

void MergeContext::add(entry_t *entries, long num_entries) {
    merge_entry_t merge_entry;

    if (num_entries > 0) {
        merge_entry.entries = entries;
        merge_entry.num_entries = num_entries;
        merge_entry.precedence = queue.size();
        queue.push(merge_entry);
    }
}

entry_t MergeContext::next(void) {
    merge_entry_t current, next;
    entry_t entry;

    current = queue.top();
    next = current;

    // Only release the most recent value for a given key
    while (next.head().key == current.head().key && !queue.empty()) {
        queue.pop();

        next.current_index++;
        if (!next.done()) queue.push(next);

        next = queue.top();
    }

    return current.head();
}

bool MergeContext::done(void) {
    return queue.empty();
}
