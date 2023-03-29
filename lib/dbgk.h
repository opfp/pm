#ifndef IMAN 
#define IMAN 

#include "common.h"
#include <fcntl.h> 
#include <errno.h> 

#define DBGK_CLWRITE 1 
#define DBGK_HSWRITE 2 
#define DBGK_NOIO 3 

#define DBGK_MSGHEADER_KEEPAWAKE 0 //(type) 
#define DBGK_MSGHEADER_ERRMSG 2// (type, keys {as a string l<=255} ) 
#define DBGK_MSGHEADER_SHORTRESP 3 // (type, keys {as a string l<=255} ) 
#define DBGK_MSGHEADER_UNAUTHREQ 4 //(*) 
#define DBGH_MSGHEADER_LONGRESP 5 // (*) 

typedef struct { 
    int status; 
    void * pbuff;
    size_t pbuff_sz;  
    // char * msg; 
    pthread_mutex_t * mux;
    pthread_cond_t * io_flag; 
    int pipe[2];  
} dbgk_connection; 

typedef struct { 
    int type; 
    char keys[256]; 
    int addresses[16]; 
} dbgk_msg_header; 

// recieves db filepath and timeout, returns dbgk pid 

int dbgk_init(dbgk_connection *, char *, unsigned); 
int dbgk_msg_add_data(dbgk_msg_header *, int, void *, int, void *); 
// void * dbgk_protected_buffer_init( ); // requires call to dbgk_init lest return error 


// ( conn, response buffer, request ) 
// void dbgk_post_req( dbgk_connection *, dbgk_msg *, dbgk_msg *); // does this even have a use case ???? 
// void dbgk_post_req_async( dbgk_connection *, dbgk_msg *, dbgk_msg *);

#endif