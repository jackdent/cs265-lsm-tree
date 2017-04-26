#include "level.h"
#include "merge.h"

// void Level::dump(void) const {
//     MergeContext merge_ctx;
//     entry_t entry;

//     merge_ctx.add(enclosures);

//     while (!merge_ctx.done()) {
//         entry = merge_ctx.next();
//         printf(DUMP_PATTERN, entry.key, entry.val);
//         if (!merge_ctx.done()) cout << " ";
//     }

//     cout << endl;
// }

void Level::empty(void) {
    enclosures.clear();
}
