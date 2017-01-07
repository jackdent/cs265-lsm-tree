/**
 * ================================================
 * = Harvard University | CS265 | Systems Project =
 * ================================================
 * ========== LSM TREE WORKLOAD EVALUATOR =========
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

#define PRODUCT "\
-------------------------------------------------------------------------\n\
                    W O R K L O A D   E V A L U A T O R                  \n\
-------------------------------------------------------------------------\n\
"


void evaluate_workload(char *filename) {
    unsigned int i;

    // All values are stored here
    VAL_t *values = malloc(sizeof(VAL_t) * KEY_MAX);
    char  *active = malloc(sizeof(char) * KEY_MAX);

    // Initialize to inactive
    for(i=1; i<KEY_MAX; i++) {
        active[i] = 0;
    }

    // Variables to scan in file
    KEY_t k;
    KEY_t k2;
    VAL_t v;    
    
    FILE *f = fopen(filename, "r");

    while(!feof(f)) {
        char operation;
        fscanf(f, "%c", &operation);
        switch(operation) {
            case 'P':
                // Put value in array
                fscanf(f, "%u %d", &k, &v);
                values[k] = v;
                active[k] = 1;
                break;
            case 'G':
                // Get value from array
                fscanf(f, "%u", &k);
                if(active[k]) {
                    printf("%d\n", values[k]);
                }
                else {
                    printf("\n");
                }
                break;
            case 'L':
                // Load external file into array
                fscanf(f, "%u", &k);
                // TODO: ...
                break;
            case 'R':
                // Issue range query
                fscanf(f, "%u %u", &k, &k2);
                // TODO: <= ??? or <
                for(i=k; i<=k2; i++) {
                    if(active[i]) {
                        printf("%d ", values[i]);
                    }
                }
                printf("\n");
                break;
            case 'D':
                // Delete value from array
                fscanf(f, "%u", &k);
                active[k] = 0;
                break;
            default:
                break;
        }
    }

    free(values);
    free(active);
}

/**
 * The main entry point to the software.
 */
int main(int argc, char **argv) {
    /////////////////////////////////////
    //          LOAD SETTINGS          //
    /////////////////////////////////////
    char *filename = NULL;
    if(argc == 1) {
        printf(LOGO);
        printf(PRODUCT);
        printf("Usage: %s [WORKLOAD FILE]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else {
        filename = argv[1];
    }

    /////////////////////////////////////
    //        EVALUATE WORKLOAD        //
    /////////////////////////////////////
    evaluate_workload(filename);

    // Done!
    exit(EXIT_SUCCESS);
}
