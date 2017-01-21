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
    int puts_batches;
    int gets;
    float gets_skewness;
    float gets_misses_ratio;
    int ranges;
    int deletes;
    int external_puts;
    int seed;
};

/**
 * Default values for settings 
 */
void initialize_settings(struct settings *s) {
    s->puts                 = 0;
    s->puts_batches         = 1;
    s->gets                 = 0;
    s->gets_skewness        = 0;
    s->gets_misses_ratio    = 0.5;
    s->ranges               = 0;
    s->deletes              = 0;
    s->external_puts        = 0;
    s->seed                 = 13141;
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
        --puts-batches [number of put batches]\n\
        --gets [number of get operations]\n\
        --gets-skewness [skewness (0-1) of get operations]\n\
        --gets-misses-ratio [empty result queries ratio]\n\
        --ranges [number of range operations]\n\
        --deletes [number of delete operations]\n\
        --external-puts * (flag) store puts in external binary files *\n\
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
            {"puts-batches",     required_argument,  0, 'b'},
            {"gets",             required_argument,  0, 'g'},
            {"gets-skewness",    required_argument,  0, 'G'},
            {"gets-misses-ratio",required_argument,  0, 't'},
            {"ranges",           required_argument,  0, 'r'},
            {"deletes",          required_argument,  0, 'd'},
            {"seed",             required_argument,  0, 'i'},
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
            case 'b':
                s->puts_batches = atoi(optarg);
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
            case 'i':
                s->seed = atoi(optarg);
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
    fprintf(stderr, "| puts-batches: %d\n", s->puts_batches);
    // Gets
    fprintf(stderr, "| gets-total: %d\n", s->gets);
    fprintf(stderr, "| get-skewness: %.4lf\n", s->gets_skewness);
    // Ranges
    fprintf(stderr, "| ranges: %d\n", s->ranges);
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
    // For each batch
    int puts_batch_size = s->puts / s->puts_batches; 
    int gets_batch_size = s->gets / s->puts_batches;
    int ranges_batch_size = s->ranges / s->puts_batches;
    int deletes_batch_size = s->deletes / s->puts_batches;

    for(i=0; i<s->puts_batches; i++) {
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
            if(i+j > s->puts) {
                break;
            }
            else {
                // ---- UPDATES ----
                // TODO: ASK QUESTION USE COMPLETE PUTS POOL?
                //       IF COMPLETE THEN MORE UPDATES
                //         THE SMALLER THE POOL THE MORE THE UPDATES VS INSERTS
                // Pick key and value
                KEY_t k = puts_pool[i+j];
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
        }
        if(s->external_puts) {
            fclose(pf);
        }

        ////////////////// GETS //////////////////
        for(j=0; j<gets_batch_size; j++) {
            if(i+j > s->gets) {
                break;
            }
            else {
                KEY_t k;
                // With a probability use a new key as a query
                if((rand()%10) > (s->gets_skewness*10) || 
                    old_gets_pool_count == 0) {
                    // With a probability use one of the 
                    // previously inserted data as a query
                    if((rand()%10) > (s->gets_misses_ratio*10)) {
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
                // ---- DELETES ----
                // TODO: Maybe restrict to current batch by also removing?
                KEY_t d = puts_pool[rand() % ((i+1)*puts_batch_size)];
                printf(DELETE_PATTERN, d);
            }
        }

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
