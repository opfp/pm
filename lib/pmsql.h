#ifndef PMSQL
#define PMSQL

#include "common.h"

#define PMSQL_BLOB 10 // nesecary?
#define PMSQL_INT 11
#define PMSQL_TEXT 12

typedef struct {
    sqlite3_destructor_type dest;
    sqlite3 * db;
    sqlite3_stmt * stmt;
    char * pmsql_error;
} pmsql_stmt;

int pmsql_compile( pmsql_stmt *, char *, int, void * *, int *, int * );
int pmsql_read(pmsql_stmt *, int, void * *, int *, int * );

#endif
