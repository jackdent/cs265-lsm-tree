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
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <float.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "data_types.h"
#include "logo.h"

#define PRODUCT "\
-------------------------------------------------------------------------\n\
                    W O R K L O A D   E V A L U A T O R                  \n\
-------------------------------------------------------------------------\n\
"

// ====================================================================== //
//                         MAIN WORKLOAD EVALUATOR
// ====================================================================== //
/**
 * This function evaluates the workload
 */
void evaluate_workload(char *filename) {
    unsigned int i;

    // TEST: EITHER USE MMAPPED FILES OR MEMORY
    unsigned long memsize = (unsigned long) ((unsigned long) KEY_MAX - (unsigned long) KEY_MIN);
    FILE *fp = fopen("active", "w");
    printf("SIZE: %lu\n", memsize);
    fseek(fp, memsize, SEEK_SET);
    fputc('\0', fp);
    fclose(fp);

    int val_fd, active_fd;
    char *f_values;
    char *f_active;

    val_fd = open("values", O_RDWR);
    active_fd = open("active", O_RDWR);


    size_t pagesize = getpagesize();
    f_values = mmap((caddr_t)0, pagesize, PROT_READ, 
                    MAP_SHARED, val_fd, pagesize);
    f_active = mmap((caddr_t)0, pagesize, PROT_READ, 
                    MAP_SHARED, val_fd, pagesize);


    // // All values are stored here
    // VAL_t *values = malloc(sizeof(VAL_t) * KEY_MAX);
    // char  *active = malloc(sizeof(char) * KEY_MAX);

    printf("ERASING...\n");
    // Initialize to inactive
    for(i=1; i<KEY_MAX; i++) {
        f_active[i] = 0;
    }

    // Variables to scan in file
    KEY_t k;
    KEY_t k2;
    VAL_t v;    
    char path[4096];
    
    FILE *f = fopen(filename, "r");

    while(!feof(f)) {
        char operation;
        fscanf(f, "%c", &operation);
        switch(operation) {
            case 'p':
                // Put value in array
                fscanf(f, PUT_PATTERN_SCAN, &k, &v);
                // values[k] = v;
                f_active[k] = 1;
                //f_active[k] = 1;
                break;
            case 'g':
                // Get value from array
                fscanf(f, GET_PATTERN_SCAN, &k);
                if(f_active[k]) {
                    // printf("%d\n", values[k]);
                }
                else {
                    printf("\n");
                }
                break;
            case 'l':
                // Load external file into array
                fscanf(f, "%s\n", path);
                FILE *pf = fopen(path, "rb");
                while(!feof(f)) {
                    fread(&k, sizeof(KEY_t), 1, pf);
                    fread(&v, sizeof(KEY_t), 1, pf);
                    // values[k] = v;
                    f_active[k] = 1;
                }
                break;
            case 'r':
                // Issue range query
                fscanf(f, RANGE_PATTERN_SCAN, &k, &k2);
                // TODO: <= ??? or <
                for(i=k; i<=k2; i++) {
                    if(f_active[i]) {
                        // printf("%d ", values[i]);
                    }
                }
                printf("\n");
                break;
            case 'd':
                // Delete value from array
                fscanf(f, DELETE_PATTERN_SCAN, &k);
                f_active[k] = 0;
                break;
            default:
                break;
        }
    }

    // free(values);
    // free(active);
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
