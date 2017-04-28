#ifndef TYPES_H
#define TYPES_H

typedef int32_t KEY_t;
typedef int32_t VAL_t;

#define KEY_MAX 2147483647
#define KEY_MIN -2147483648

#define VAL_MAX 2147483647
#define VAL_MIN -2147483647
#define VAL_TOMBSTONE -2147483648

struct entry {
    KEY_t key;
    VAL_t val;
    bool operator==(const entry& other) const {return key == other.key;}
    bool operator<(const entry& other) const {return key < other.key;}
    bool operator>(const entry& other) const {return key > other.key;}
};

typedef struct entry entry_t;

#endif
