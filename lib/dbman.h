#ifndef IMAN 
#define IMAN 

#include "common.h"

typedef struct { 
    sqlite3 * db; 
    int cooldown; 
    int dbman_pid; 
} dbman_inst; 

typedef struct { 
    char opcode; 
    char options; 
    char vault_name[VAULTNAMESIZE]; 
    char search_str[NAMESIZE]; 
    char retcode; 
} dbman_req; 

typedef struct { 
    int req_id; 
    char data_req; 
    char data[DATASIZE]; 
    char cipher[CIPHERSIZE]; 
} dbman_datareq; 

typedef struct { 
    char retcode; 
    char * retdata; 
    dbman_datareq * data_request; 
} dbman_ret; 

int dbman_init(char *, int); 
dbman_ret * dbman_open_request(dbman_req); 
dbman_ret * dbamn_fufull_datareq(dbman_datareq); 

#endif