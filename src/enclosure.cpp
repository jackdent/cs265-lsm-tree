#include <cassert>
#include <sys/mman.h>

#include "enclosure.h"

using namespace std;

Enclosure::Enclosure(int max_entries) : min_key(KEY_MIN), max_key(KEY_MAX) {
    char *tmp_fn;

    num_entries = 0;
    max_entries = max_entries;

    tmp_fn = strdup(TMP_FILE_PATTERN);
    tmp_file = mktemp(tmp_fn);

    mapping = nullptr;
    mapping_fp = nullptr;
}

Enclosure::~Enclosure(void) {
    unmap();
    remove(tmp_file.c_str());
}

bool Enclosure::key_in_range(KEY_t key) const {
    return key >= min_key && key <= max_key;
}

bool Enclosure::range_overlaps_with(KEY_t start, KEY_t end) const {
    return start <= max_key && min_key <= end;
};

void Enclosure::map(void) {
    if (mapping != nullptr) return;

    mapping = new vector<entry_t>;
    mapping_fp = fopen(tmp_file.c_str(), "rb");

    mapping->reserve(num_entries);
    mmap(mapping->data(), file_size(), PROT_READ, MAP_PRIVATE, fileno(mapping_fp), 0);
}

void Enclosure::unmap(void) {
    if (mapping == nullptr) return;

    munmap(mapping->data(), file_size());
    delete mapping;
    fclose(mapping_fp);

    mapping = nullptr;
    mapping_fp = nullptr;
}

VAL_t * Enclosure::get(KEY_t key) {
    VAL_t *val;
    entry_t search_entry;
    vector<entry_t>::iterator search_iterator;

    val = nullptr;

    if (key_in_range(key) && bloom_filter.is_set(key)) {
        map();

        search_entry.key = key;
        search_iterator = lower_bound(mapping->begin(), mapping->end(), search_entry);

        if (search_iterator->key == key) {
            val = new VAL_t;
            *val = search_iterator->val;
        }

        unmap();
    }

    return val;
}

vector<entry_t> * Enclosure::range(KEY_t start, KEY_t end) {
    vector<entry_t> *subrange;
    entry_t search_entry;
    vector<entry_t>::iterator subrange_start, subrange_end;

    subrange = new vector<entry_t>;

    if (range_overlaps_with(start, end)) {
        map();

        search_entry.key = start;
        subrange_start = lower_bound(mapping->begin(), mapping->end(), search_entry);

        search_entry.key = end;
        subrange_end = upper_bound(mapping->begin(), mapping->end(), search_entry);

        assert(subrange_start <= subrange_end);

        subrange->reserve(distance(subrange_start, subrange_end));
        subrange->assign(subrange_start, subrange_end);

        unmap();
    }

    return subrange;
}

void Enclosure::put(entry_t *entries, int len) {
    int i;
    FILE *fp;

    assert(len > 0);
    assert(num_entries + len <= max_entries);

    num_entries += len;

    for (i = 0; i < num_entries; i++) {
        bloom_filter.set(entries[i].key);
    }

    // Inclusive bounds
    min_key = min(entries[0].key, min_key);
    max_key = max(entries[len - 1].key, max_key);

    fp = fopen(tmp_file.c_str(), "ab");
    fwrite(entries, sizeof(entry_t), len, fp);
    fclose(fp);
}
