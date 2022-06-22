#ifndef O_STR
#define O_STR

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

// Only supports string. Up to you to free memory after use!
//char * concat(char *, int, ...); // depreciated
uint32_t o_search(char *, char *);
// uint64_t o_search_l(char *, char *);
#endif
