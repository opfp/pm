#include "dbgk.h"

int main() { 
    dbgk_connection conn; 
    int ecode; 
    if (( ecode = dbgk_init(&conn, "", 0) )) { 
        printf("dbgk_init returned %i\n", ecode); 
    }  

    dbgk_msg_header msh; 
    msh.type = 0; 

    pthread_mutex_unlock(conn.mux); 
    close(conn.pipe[0]); 

    write(conn.pipe[1], sizeof(dbgk_msg_header), &msh);  
} 