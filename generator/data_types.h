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

#define GEN_RANDOM_KEY(r) gsl_rng_get(r);
#define GEN_RANDOM_VAL(r) gsl_rng_get(r);

// VALUE
typedef int32_t VAL_t;

#ifdef UNSIGNED_KEYS
    // KEY
    typedef uint32_t KEY_t;
    
    // PRINT PATTERNS
    #define PUT_PATTERN "p %u %d\n"
    #define GET_PATTERN "g %u\n"
    #define RANGE_PATTERN "r %u %u\n"
    #define DELETE_PATTERN "d %u\n"

    // SCAN PATTERNS
    #define PUT_PATTERN_SCAN "%u %d"
    #define GET_PATTERN_SCAN "%u"
    #define RANGE_PATTERN_SCAN "%u %u"
    #define DELETE_PATTERN_SCAN "%u"
#else
    // KEY
    typedef int32_t KEY_t;
    
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

#endif