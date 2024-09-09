#ifndef PMSQL
#define PMSQL

#include "common.h"

#define PMSQL_BLOB 10 // nesecary?
#define PMSQL_INT 11
#define PMSQL_INT_WB 12
#define PMSQL_TEXT 13 

typedef union { 
	uint8_t * blob; 
	int integer; 
	int * int_wb; 
	char * text; 
} pmsql_data_t; 

typedef struct {
    sqlite3_destructor_type dest;
    sqlite3 * db;
    sqlite3_stmt * stmt;
    char * pmsql_error;
} pmsql_stmt;

int pmsql_compile( pmsql_stmt *, char *, int, pmsql_data_t *, int *, int * );
int pmsql_read(pmsql_stmt *, int, pmsql_data_t *, int *, int * );
int pmsql_safe_in(char *); 

#endif
