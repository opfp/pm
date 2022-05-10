#include "o_str.h"
// @ssert args 2-n are all char *
char * concat( char * base, int num_ins, ... ){ //currently only supports string
    if ( base == NULL )
        return NULL;
    if ( num_ins == 0 )
        return base;
    int base_len = strlen(base);
    if ( base_len == 0 )
        return NULL;

    char * base_copy = malloc(base_len + 1); // copy base to own memory
    memcpy(base_copy, base, base_len);      // so we don't have to deal with read
    base_copy[base_len] = 0;               // only string literals.

    char * * insertions = malloc( num_ins * sizeof(char *));
    int * insertions_len = malloc( num_ins * sizeof(int *)); //yes i know they are all the same. for readability

    va_list args;
    va_start(args, num_ins);

    char * rover = base_copy;
    int full_len = base_len;
    int i = 0;

    while ( (rover < base_copy + base_len ) && ( i < num_ins ) ) {
        char * ins = strchr(rover, '%');
        char * this = va_arg(args, char * );

        if ( ins == NULL || this == NULL ) {
            printf("o_str.concat: argument / insertion char mismatch\n");
            return NULL;
        }
        int ins_len = strlen(this);
        if ( ins_len == 0 ) {
            printf("o_str.concat: invalid argument passed\n");
            return NULL;
        }
        insertions[i] = this;
        insertions_len[i] = ins_len;
        *ins = 1; // Flag for do insertion here

        full_len += ins_len;
        i++;
        rover = ins + 1;
    }

    char * ret = malloc(full_len - i);
    char * inserter = ret;
    rover = base_copy;
    i = 0;
    while( rover < base_copy + base_len )   {
        if ( *rover == 1 ) {
            strncpy(inserter, insertions[i], insertions_len[i]);
            inserter += insertions_len[i];
            i++;
        } else {
            *inserter = *rover;
            inserter++;
        }
        rover++;
    }
    *inserter = 0;
    free(base_copy);
    free(insertions);
    free(insertions_len);

    return ret;
}
