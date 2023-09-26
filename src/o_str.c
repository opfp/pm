#include "o_str.h"

unsigned o_search(char * res_in, char * key_in ) {// CASE INSENSITIVE
    int res_len = strlen(res_in);
    int key_len = strlen(key_in);
    if ( res_len > 31 || key_len > 31 ) {
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
