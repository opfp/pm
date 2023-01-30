#ifndef IMAN 
#define IMAN 

#include "common.h"
#include <fcntl.h> 

#define LCONN_CLWRITE 1 
#define LCONN_HSWRITE 2 
#define LCONN_NOIO 3 

typedef struct { 
    int status; 
    pthread_mutex_t mux;
    pthread_cond_t io_flag; 
    int inp[2]; 
    int outp[2];  
} lconn_connection; 

typedef union { 
    int msg_type; 
    int status; 
    char options; 
    char vault_name[VAULTNAMESIZE]; 
    char search_str[NAMESIZE]; 
    char data[DATASIZE]; 
    char cipher[CIPHERSIZE]; 
} lconn_msg; 

// recieves db filepath and timeout, returns lconn pid 
int lconn_init(lconn_connection *, char *, time_t); 
lconn_msg * lconn_protected_buffer_init( ); // requires call to lconn_init lest return error 
// ( conn, response buffer, request ) 
void lconn_post_req( lconn_connection *, lconn_msg *, lconn_msg *); // does this even have a use case ???? 
void lconn_post_req_async( lconn_connection *, lconn_msg *, lconn_msg *);

#endif