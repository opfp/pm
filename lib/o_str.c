#include "o_str.h"
// TB depreciated - should have just used snprintf
// char * concat( char * base, int num_ins, ... ){ //currently only supports string
//     if ( base == NULL )
//         return NULL;
//     if ( num_ins == 0 )
//         return base;
//     int base_len = strlen(base);
//     if ( base_len == 0 )
//         return NULL;
//
//     char * base_copy = malloc(base_len + 1); // copy base to own memory
//     memcpy(base_copy, base, base_len);      // so we don't have to deal with read
//     base_copy[base_len] = 0;               // only string literals.
//
//     char * * insertions = malloc( num_ins * sizeof(char *));
//     int * insertions_len = malloc( num_ins * sizeof(int *)); //yes i know they are all the same. for readability
//
//     va_list args;
//     va_start(args, num_ins);
//
//     char * rover = base_copy;
//     int full_len = base_len;
//     int i = 0;
//
//     while ( (rover < base_copy + base_len ) && ( i < num_ins ) ) {
//         char * ins = strchr(rover, '%');
//         char * this = va_arg(args, char * );
//
//         if ( ins == NULL || this == NULL ) {
//             printf("o_str.concat: argument / insertion char mismatch\n");
//             return NULL;
//         }
//         int ins_len = strlen(this);
//         if ( ins_len == 0 ) {
//             printf("o_str.concat: invalid argument passed\n");
//             return NULL;
//         }
//         insertions[i] = this;
//         insertions_len[i] = ins_len;
//         *ins = 1; // Flag for do insertion here
//
//         full_len += ins_len;
//         i++;
//         rover = ins + 1;
//     }
//
//     char * ret = malloc(full_len - i);
//     char * inserter = ret;
//     rover = base_copy;
//     i = 0;
//     while( rover < base_copy + base_len )   {
//         if ( *rover == 1 ) {
//             strncpy(inserter, insertions[i], insertions_len[i]);
//             inserter += insertions_len[i];
//             i++;
//         } else {
//             *inserter = *rover;
//             inserter++;
//         }
//         rover++;
//     }
//     *inserter = 0;
//     free(base_copy);
//     free(insertions);
//     free(insertions_len);
//
//     return ret;
// }
unsigned o_search(char * res_in, char * key_in ) {// CASE INSENSITIVE
    int res_len = strlen(res_in);
    int key_len = strlen(key_in);
    if ( res_len > 4 * sizeof(int) || key_len > 4 * sizeof(int) ) {
        //fprintf(stderr, "o_search does not support strings over 32 characters long.\n");
        printf("o_search does not support strings over 32 characters long.\n");
        return 0;
    } else if ( res_len == 0 || key_len == 0 ) {
        printf("o_search: param with length 0\n" );
        return 0;
    }
    char * res = malloc(res_len + 1);
    char * key = malloc(key_len + 1);
    res[res_len] = 0;
    key[key_len] = 0;
    //printf("%s ? %s\n", res_in, key_in );
    int k = 0;

    while ( k < res_len ) {
        res[k] = tolower(res_in[k]);
        ++k;
    }
    k = 0;
    while ( k < key_len ) {
        key[k] = tolower(key_in[k]);
        ++k;
    }
    //printf("%s ? %s\n", res, key );
    // base case
    if ( !strcmp(res, key) ) {
        //printf("Returning 1..1\n");
        return ~0;
    }

    unsigned match = 0;
    unsigned sub = 0;
    int sub_sz = 2; // size of smallest substring to match
    if ( res_len > 12 )
        sub_sz++; // 3
    if ( res_len > 24 )
        sub_sz++; // 4
    if ( sub_sz > key_len )
        sub_sz = key_len;
    // Now lets try and match all the substrings of sub_sz
    char * sub_bf = malloc(sub_sz + 1);
    for( int i = 0; i < ( res_len - sub_sz); i++ ) {
        memcpy(sub_bf, res + i, sub_sz);
        sub_bf[sub_sz] = 0;
        int this_sz = sub_sz;
        char * sub_ikey, * sub_ires;
        char c1, c2;
        if (( sub_ikey = strstr(key, sub_bf) )) {
            int key_offset = sub_ikey - key;
            // res offset is i
            sub_ikey += sub_sz;
            sub_ires = res + i + sub_sz;

            //printf("%s ? %s share: %s", res, key, sub_bf );
            while ( ((c1 = *sub_ikey++)) && ((c2 = *sub_ires++)) ) {
                if ( c1 != c2 )
                    break;
                this_sz++;
                //putchar(c1);
            }
            //printf(" \n");

            // add the corresponding bits to match
            match |= ( (unsigned int) (~0 << (32 - this_sz)) ) >> i ;
            sub += 1 << key_offset;
            //printf("%s ? %s match : %x\nsub: %x\n", res, key, match, sub );

            // int32_t exp = i - key_offset;
            // if ( i < 0 )
            //     exp = ~exp + 1; // take abs
            // sub += ( 1 << exp );
            //sub += ( (1 << i) * (res_len - this_sz) ) + sub_ikey_i ;
            i += this_sz;
        }
    }
    // if ( match )
    //     printf("%s ? %s : %x\n", res, key, match - sub );
    free(res);
    free(key);
    free(sub_bf);
    return match - sub;
}

int strcmp_for_qsort(const void *a, const void *b) {
    const char * c = *(const char **) a; // why? who knows
    const char * d = *(const char **) b;
    // printf("%s ? %s\n",c,d );
    return strcmp(c, d);
}
