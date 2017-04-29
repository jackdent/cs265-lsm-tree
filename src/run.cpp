#include <cassert>
#include <sys/mman.h>

#include "run.h"

using namespace std;

Run::Run(long max_size) : max_size(max_size) {
    char *tmp_fn;

    size = 0;
    min_key = KEY_MIN;
    max_key = KEY_MAX;

    tmp_fn = strdup(TMP_FILE_PATTERN);
    tmp_file = mktemp(tmp_fn);

    mapping = nullptr;
    mapping_fp = nullptr;
}

Run::~Run(void) {
    unmap();
    remove(tmp_file.c_str());
}

bool Run::key_in_range(KEY_t key) const {
    return key >= min_key && key <= max_key;
}

bool Run::range_overlaps_with(KEY_t start, KEY_t end) const {
    return start <= max_key && min_key <= end;
};

vector<entry_t> * Run::map(MappingType type) {
    if (mapping != nullptr) return mapping;

    mapping = new vector<entry_t>;

    if (type == MappingType::Read) {
        mapping_fp = fopen(tmp_file.c_str(), "rb");
        mapping->reserve(size);
        mmap(mapping->data(), size * sizeof(entry_t), PROT_READ, MAP_PRIVATE, fileno(mapping_fp), 0);
    } else {
        mapping_fp = fopen(tmp_file.c_str(), "wb");
        mapping->reserve(max_size);
        mmap(mapping->data(), max_size * sizeof(entry_t), PROT_WRITE, MAP_PRIVATE, fileno(mapping_fp), 0);
    }

    return mapping;
}

void Run::unmap(void) {
    if (mapping == nullptr) return;

    munmap(mapping->data(), mapping->capacity() * sizeof(entry_t));
    delete mapping;
    fclose(mapping_fp);

    mapping = nullptr;
    mapping_fp = nullptr;
}

VAL_t * Run::get(KEY_t key) {
    VAL_t *val;
    entry_t search_entry;
    vector<entry_t>::iterator search_iterator;

    val = nullptr;

    if (!key_in_range(key) || !bloom_filter.is_set(key)) {
        return val;
    }

    map(MappingType::Read);

    search_entry.key = key;
    search_iterator = lower_bound(mapping->begin(), mapping->end(), search_entry);

    if (search_iterator->key == key) {
        val = new VAL_t;
        *val = search_iterator->val;
    }

    unmap();

    return val;
}

vector<entry_t> * Run::range(KEY_t start, KEY_t end) {
    vector<entry_t> *subrange;
    entry_t search_entry;
    vector<entry_t>::iterator subrange_start, subrange_end;

    subrange = new vector<entry_t>;

    if (!range_overlaps_with(start, end)) {
        return subrange;
    }

    map(MappingType::Read);

    search_entry.key = start;
    subrange_start = lower_bound(mapping->begin(), mapping->end(), search_entry);

    search_entry.key = end;
    subrange_end = upper_bound(mapping->begin(), mapping->end(), search_entry);

    assert(subrange_start <= subrange_end);

    subrange->reserve(distance(subrange_start, subrange_end));
    subrange->assign(subrange_start, subrange_end);

    unmap();

    return subrange;
}

void Run::put(entry_t entry) {
    assert(mapping != nullptr);
    assert(size <= max_size);

    bloom_filter.set(entry.key);
    min_key = min(entry.key, min_key);
    max_key = max(entry.key, max_key);

    mapping->push_back(entry);
    size += 1;
}
