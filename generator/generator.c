/**
 * ================================================
 * = Harvard University | CS265 | Systems Project =
 * ================================================
 * ========== LSM TREE WORKLOAD GENERATOR =========
 * ================================================
 * Contact:
 * ========
 * - Kostas Zoumpatianos <kostas@seas.harvard.edu>
 * - Michael Kester <kester@eecs.harvard.edu>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <float.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "data_types.h"
#include "logo.h"

// This is used for storing gets existing in the 
// workload so far, and using them for skewed 
// query workload generation
#define MAX_OLD_GETS_POOL_SIZE 10000000 // ~= 80MB

// This is the product name
#define PRODUCT "\
-------------------------------------------------------------------------\n\
                    W O R K L O A D   G E N E R A T O R                  \n\
-------------------------------------------------------------------------\n\
"

// ====================================================================== //
//                     COMMAND LINE ARGUMENTS PARSING
// ====================================================================== //
/**
 * The settings parsed from command line arguments
 */
struct settings {
    int puts;
    int gets;
    float gets_skewness;
    float gets_misses_ratio;
    int ranges;
    char uniform_ranges; /* true (uniform) or false (gaussian) */
    int deletes;
    int external_puts;
    int seed;
    int min_lookup_batch_size;
    int max_lookup_batch_size;
    int min_update_batch_size;
    int max_update_batch_size;
};

/**
 * Default values for settings 
 */
void initialize_settings(struct settings *s) {
    s->puts                  = 0;
    s->gets                  = 0;
    s->gets_skewness         = 0;
    s->gets_misses_ratio     = 0.5;
    s->ranges                = 0;
    s->deletes               = 0;
    s->external_puts         = 0;
    s->seed                  = 13141;
    s->uniform_ranges        = 1;
    s->min_lookup_batch_size = 2; 
    s->max_lookup_batch_size = 2;
    s->min_update_batch_size = 2;
    s->max_update_batch_size = 2;
}

/**
 * Prints the usage of the binary. 
 * i.e., all valid arguments.
 */
void usage(char * binary) {
    //fprintf(stderr, DASLAB_LOGO);
    fprintf(stderr, LOGO);
    fprintf(stderr, PRODUCT);
    fprintf(stderr, "Usage: %s\n\
        --puts [number of put operations]\n\
        --gets [number of get operations]\n\
        --gets-skewness [skewness (0-1) of get operations]\n\
        --gets-misses-ratio [empty result queries ratio]\n\
        --ranges [number of range operations]\n\
        --uniform-ranges *use uniform ranges (default)*\n\
        --gaussian-ranges *use gaussian ranges (default)*\n\
        --deletes [number of delete operations]\n\
        --min-lookup-batch-size [minimum size of continuous lookups]\n\
        --max-lookup-batch-size [maximum size of continuous lookups]\n\
        --min-update-batch-size [minimum size of continuous updates]\n\
        --max-update-batch-size [maximum size of continuous updates]\n\
        --external-puts *store puts in external binary files*\n\
        --seed [random number generator seed]\n\
        --help\n\
\n\n", binary);
}

/**
 * Parses the arguments to a settings struct
 */
void parse_settings(int argc, char **argv, struct settings *s) {
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"puts",             required_argument,  0, 'p'},
            {"gets",             required_argument,  0, 'g'},
            {"gets-skewness",    required_argument,  0, 'G'},
            {"gets-misses-ratio",required_argument,  0, 't'},
            {"ranges",           required_argument,  0, 'r'},
            {"uniform-ranges",   no_argument,        0, 'u'},
            {"gaussian-ranges",  no_argument,        0, 'n'},
            {"deletes",          required_argument,  0, 'd'},
            {"min-lookup-batch-size", required_argument, 0, 'l'},
            {"max-lookup-batch-size", required_argument, 0, 'L'},
            {"min-update-batch-size", required_argument, 0, 'i'},
            {"max-update-batch-size", required_argument, 0, 'I'},
            {"seed",             required_argument,  0, 's'},
            {"external-puts",    no_argument,        0, 'e'},
            {"help",             no_argument,        0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long (argc, argv, "p:b:g:G:r:t:d:i:eh",
                         long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1)
             break;
        switch (c)
        {
            case 't':
                s->gets_misses_ratio = atof(optarg);
                break;
            case 'p':
                s->puts = atoi(optarg);
                break;
            case 'g':
                s->gets = atoi(optarg);
                break;
            case 'r':
                s->ranges = atoi(optarg);
                break;
            case 'd':
                s->deletes = atoi(optarg);
                break;
            case 'G':
                s->gets_skewness = atof(optarg);
                break;
            case 'e':
                s->external_puts = 1;
                break;
            case 's':
                s->seed = atoi(optarg);
                break;
            case 'u':
                s->uniform_ranges = 1;
                break;
            case 'n':
                s->uniform_ranges = 0;
                break;
            case 'i':
                s->min_update_batch_size = atoi(optarg);
                break;
            case 'I':
                s->max_update_batch_size = atoi(optarg);
                break;
            case 'l':
                s->min_lookup_batch_size = atoi(optarg);
                break;
            case 'L':
                s->max_lookup_batch_size = atoi(optarg);
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case '?':
                if (optopt == 'e' || optopt == 'd' || optopt == 'q')
                    fprintf (stderr, "Option -%c requires an argument.\n",
                            optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }

    // Check: 0 puts not allowed
    if(s->puts == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Make sure ranges are sane
    if(s->min_update_batch_size > s->max_update_batch_size) {
        s->max_update_batch_size = s->min_update_batch_size;
    }
    
    if(s->min_lookup_batch_size > s->max_lookup_batch_size) {
        s->max_lookup_batch_size = s->min_lookup_batch_size;
    }
}

/**
 * Prints the settings on stderr
 */
void print_settings(struct settings *s) {
    /////////////////////////////////////
    //           PRINT INFO            //
    /////////////////////////////////////
    fprintf(stderr, "+---------- CS 265 ----------+\n");
    fprintf(stderr, "|        WORKLOAD INFO       |\n");
    fprintf(stderr, "+----------------------------+\n");

    // Seed
    fprintf(stderr, "| initial-seed: %d\n", s->seed);
    // Puts
    fprintf(stderr, "| puts-total: %d\n", s->puts);
    // Gets
    fprintf(stderr, "| gets-total: %d\n", s->gets);
    fprintf(stderr, "| get-skewness: %.4lf\n", s->gets_skewness);
    // Ranges
    fprintf(stderr, "| ranges: %d\n", s->ranges);
    fprintf(stderr, "| range distribution: %s\n", 
                       s->uniform_ranges ? "uniform" : "gaussian");
    // Deletes
    fprintf(stderr, "| deletes: %d\n", s->deletes);
    // Gets misses ratio
    fprintf(stderr, "| gets-misses-ratio: %.4lf\n", s->gets_misses_ratio);
    fprintf(stderr, "+----------------------------+\n");
}

// ====================================================================== //
//                         MAIN WORKLOAD GENERATOR
// ====================================================================== //

void generate_workload(struct settings *s) {
    int i, j;
    
    /////////////////////////////////////
    ///    INITIALIZE NUMBER POOLS    ///
    /////////////////////////////////////
    // Initialize random generator
    srand(s->seed);

    // Puts pool
    int puts_pool_size = s->puts;
    KEY_t *puts_pool = malloc(sizeof(KEY_t) * puts_pool_size);
    for(i=0; i<puts_pool_size; i++) {
        puts_pool[i] = GEN_RANDOM_KEY();
    }
    // Memory of previous gets (for skewed workloads)
    int old_gets_pool_max_size = MAX_OLD_GETS_POOL_SIZE * sizeof(KEY_t);
    int old_gets_pool_count = 0;
    KEY_t *old_gets_pool = malloc(sizeof(KEY_t) * old_gets_pool_max_size);
    for(i=0; i<old_gets_pool_max_size; i++) {
        old_gets_pool[i] = 0;
    }

    /////////////////////////////////////
    //             PROCESS             //
    /////////////////////////////////////
    int remaining_deletes = s->deletes;
    int remaining_puts = s->puts;
    int remaining_gets = s->gets;
    int remaining_ranges = s->ranges;

    while(remaining_deletes || remaining_puts || remaining_gets || remaining_ranges) {
        int updates = s->min_lookup_batch_size + (rand() % s->max_update_batch_size);
        int lookups = s->min_lookup_batch_size + (rand() % s->max_update_batch_size);

        int gets_batch_size = lookups / 2;
        int ranges_batch_size = lookups / 2;
        int puts_batch_size = updates / 2;
        int deletes_batch_size = updates / 2;

        if(remaining_puts < puts_batch_size)
            puts_batch_size = remaining_puts;
        if(remaining_deletes < deletes_batch_size)
            deletes_batch_size = remaining_deletes;  
        if(remaining_gets < gets_batch_size)
            gets_batch_size = remaining_gets;        
        if(remaining_ranges < ranges_batch_size)
            ranges_batch_size = remaining_ranges;

        fprintf(stderr, ">>> *NEW BATCH* (P:%d, G:%d, D:%d, R:%d)\n", 
                puts_batch_size, gets_batch_size,
                deletes_batch_size, ranges_batch_size);
                        
        ////////////////// PUTS //////////////////
        // If puts are external, create file.
        FILE *pf;
        if(s->external_puts) {
            char pfname[256];
            sprintf(pfname, "%d.dat", i); 
            pf = fopen(pfname, "wb");
            printf("l %s\n", pfname);
        }
        // Generate puts
        for(j=0; j<puts_batch_size; j++) {
            // ---- UPDATES ----
            // TODO: ASK QUESTION USE COMPLETE PUTS POOL?
            //       IF COMPLETE THEN MORE UPDATES
            //         THE SMALLER THE POOL THE MORE THE UPDATES VS INSERTS
            // Pick key and value
            KEY_t k = puts_pool[s->puts - (remaining_puts) + j];
            VAL_t v = GEN_RANDOM_VAL();
            
            // Write
            if(s->external_puts) {
                fwrite(&k, sizeof(KEY_t), 1, pf);
                fwrite(&v, sizeof(KEY_t), 1, pf);
            }
            else {
                printf(PUT_PATTERN, k, v);
            } 
        }
        if(s->external_puts) {
            fclose(pf);
        }

        ////////////////// GETS //////////////////
        for(j=0; j<gets_batch_size; j++) {
            KEY_t k;
            // With a probability use a new key as a query
            if((rand()%10) > (s->gets_skewness*10) || 
                old_gets_pool_count == 0) {
                // With a probability use one of the 
                // previously inserted data as a query
                if((rand()%10) > (s->gets_misses_ratio*10)) {

                    // TODO: FIX THIS
                    k = puts_pool[rand() % ((i+1)*puts_batch_size)];

                }
                // Or issue a completely random query 
                // most probably causing a miss
                else {
                    k = GEN_RANDOM_KEY();
                }
                // Store this number in the pool for 
                // choosing it for future skewed queries
                if(old_gets_pool_count >= old_gets_pool_max_size) {
                    old_gets_pool[rand() % old_gets_pool_count] = k;
                }
                else {
                    old_gets_pool[old_gets_pool_count++] = k;
                }
            }
            // Or use one of the previously queried keys
            else {
                k = old_gets_pool[rand() % old_gets_pool_count];
            }

            // Write in output
            printf(GET_PATTERN, k);
            
        }

        //////////////// RANGES //////////////////
        for(j=0; j<ranges_batch_size; j++) {
            if(i+j > s->ranges) {
                break;
            }
            else {
                KEY_t a = GEN_RANDOM_KEY();
                KEY_t b = GEN_RANDOM_KEY();

                if(a < b) {
                    printf(RANGE_PATTERN, a, b);
                }
                else {
                    printf(RANGE_PATTERN, b, a);
                }
            }
        }

        ////////////////// DELETES //////////////////
        for(j=0; j<deletes_batch_size; j++) {
            if(i+j > s->deletes) {
                break;
            }
            else {

                // TODO: FIX THIS
                KEY_t d = puts_pool[rand() % ((i+1)*puts_batch_size)];
                printf(DELETE_PATTERN, d);
            }
        }

        // Operations left
        remaining_puts -= puts_batch_size;
        remaining_gets -= gets_batch_size;
        remaining_ranges -= ranges_batch_size;
        remaining_deletes -= deletes_batch_size;
    }

    // Free memory
    free(old_gets_pool);
    free(puts_pool);
}

/**
 * The main entry point to the software.
 */
int main(int argc, char **argv) {
    /////////////////////////////////////
    //          LOAD SETTINGS          //
    /////////////////////////////////////
    struct settings s;
    initialize_settings(&s);
    parse_settings(argc, argv, &s);
    print_settings(&s);
    
    /////////////////////////////////////
    //        GENERATE WORKLOAD        //
    /////////////////////////////////////
    generate_workload(&s);

    // Done!
    exit(EXIT_SUCCESS);
}
