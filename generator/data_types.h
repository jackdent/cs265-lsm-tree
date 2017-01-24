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

// KEY
typedef int32_t KEY_t;
typedef int32_t VAL_t;

#define KEY_MAX 2147483647
#define KEY_MIN -2147483647

#define GEN_RANDOM_KEY_GAUSS(r) gsl_ran_gaussian(r, 2147483647/3);
#define GEN_RANDOM_VAL_GAUSS(r) gsl_ran_gaussian(r, 2147483647/3);

#define GEN_RANDOM_KEY_UNIFORM(r) gsl_rng_get(r);
#define GEN_RANDOM_VAL_UNIFORM(r) gsl_rng_get(r);

// PRINT PATTERNS
#define PUT_PATTERN "p %d %d\n"
#define GET_PATTERN "g %d\n"
#define RANGE_PATTERN "r %d %d\n"
#define DELETE_PATTERN "d %d\n"

// SCAN PATTERNS
#define PUT_PATTERN_SCAN "%d %d"
#define GET_PATTERN_SCAN "%d"
#define RANGE_PATTERN_SCAN "%d %d"
#define DELETE_PATTERN_SCAN "%d"

#endif