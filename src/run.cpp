#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

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
    mapping_fd = -1;
}

Run::~Run(void) {
    assert(mapping == nullptr);
    remove(tmp_file.c_str());
}

bool Run::key_in_range(KEY_t key) const {
    return key >= min_key && key <= max_key;
}

bool Run::range_overlaps_with(KEY_t start, KEY_t end) const {
    return start <= max_key && min_key <= end;
};

entry_t * Run::map_read(void) {
    assert(mapping == nullptr);

    mapping_fd = open(tmp_file.c_str(), O_RDONLY);
    assert(mapping_fd != -1);

    mapping = (entry_t *)mmap(0, file_size(), PROT_READ, MAP_SHARED, mapping_fd, 0);
    assert(mapping != MAP_FAILED);

    return mapping;
}

entry_t * Run::map_write(void) {
    assert(mapping == nullptr);
    int result;

    mapping_fd = open(tmp_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    assert(mapping_fd != -1);

    // Set the file to the appropriate length
    result = lseek(mapping_fd, file_size() - 1, SEEK_SET);
    assert(result != -1);
    result = write(mapping_fd, "", 1);
    assert(result != -1);

    mapping = (entry_t *)mmap(0, file_size(), PROT_WRITE, MAP_SHARED, mapping_fd, 0);
    assert(mapping != MAP_FAILED);

    return mapping;
}

void Run::unmap(long mapping_size) {
    assert(mapping != nullptr);

    munmap(mapping, file_size());
    close(mapping_fd);

    mapping = nullptr;
    mapping_fd = -1;
}

void Run::unmap_read(void) {
    unmap(size);
}

void Run::unmap_write(void) {
    unmap(max_size);
}

long Run::lower_bound(KEY_t key) {
    assert(mapping != nullptr);

    long low, high, mid;

    low = 0;
    high = size;

    while (low < high) {
        mid = low + (high - low) / 2;
        if (key <= mapping[mid].key) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return low;
}

long Run::upper_bound(KEY_t key) {
    assert(mapping != nullptr);

    long low, high, mid;

    low = 0;
    high = size;

    while (low < high) {
        mid = low + (high - low) / 2;
        if (key >= mapping[mid].key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return low;
}

VAL_t * Run::get(KEY_t key) {
    VAL_t *val;
    long search_index;
    entry_t search_entry;

    val = nullptr;

    if (!key_in_range(key) || !bloom_filter.is_set(key)) {
        return val;
    }

    map_read();

    search_index = lower_bound(key);

    if (search_index < size && mapping[search_index].key == key) {
        val = new VAL_t;
        *val = mapping[search_index].val;
    }

    unmap_read();

    return val;
}

vector<entry_t> * Run::range(KEY_t start, KEY_t end) {
    vector<entry_t> *subrange;
    long subrange_start, subrange_end;

    subrange = new vector<entry_t>;

    if (!range_overlaps_with(start, end)) {
        return subrange;
    }

    map_read();

    subrange_start = lower_bound(start);
    subrange_end = upper_bound(end);

    if (subrange_start < subrange_end) {
        subrange->reserve(subrange_end - subrange_start);
        subrange->assign(&mapping[subrange_start], &mapping[subrange_end]);
    }

    unmap_read();

    return subrange;
}

void Run::put(entry_t entry) {
    assert(size < max_size);

    bloom_filter.set(entry.key);
    min_key = min(entry.key, min_key);
    max_key = max(entry.key, max_key);

    mapping[size] = entry;
    size++;
}
