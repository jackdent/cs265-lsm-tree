/**
 * ================================================
 * = Harvard University | CS265 | Systems Project =
 * ================================================
 * ==========     LSM TREE DATA TYPES     =========
 * ================================================
 * Contact:
 * ========
 * - Kostas Zoumpatianos <kostas@seas.harvard.edu>
 * - Michael Kester <kester@eecs.harvard.edu>
 */

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

// VALUE
typedef int32_t VAL_t;
#define GEN_RANDOM_VAL() rand() - rand();

// KEY
typedef uint32_t KEY_t;
#define KEY_MAX 4294967295
#define KEY_MIN 0
#define GEN_RANDOM_KEY() rand() + rand();

// PRINT PATTERNS
#define PUT_PATTERN "P %u %d\n"
#define GET_PATTERN "G %u\n"
#define RANGE_PATTERN "R %u %u\n"
#define DELETE_PATTERN "D %u\n"

#endif