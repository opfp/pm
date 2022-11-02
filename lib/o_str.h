#ifndef O_STR
#define O_STR

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

// Only supports string. Up to you to free memory after use!
//char * concat(char *, int, ...); // depreciated
unsigned o_search(char *, char *);
int strcmp_for_qsort(const void *, const void *);

// uint64_t o_search_l(char *, char *);
#endif
