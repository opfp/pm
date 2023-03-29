#include "dbgk.h" 

typedef struct { 
    sqlite3 * db; 
    struct timespec cleanup_tm; 
    pthread_mutex_t mux; 
    pthread_cond_t io_flag; 
    struct dbgk_connection * client; 
    int pipe[2]; 
} dbgk_inst; 

void sleep_await_message(); 
void read_message(); 
void cleanup(); 

static dbgk_inst dbgk_obj = {0}; 
static char isinit = 0; 

int dbgk_init( dbgk_connection * client, char * db_path, unsigned cooldown ) { 
    // dbgk_connection * retcon = malloc(sizeof(dbgk_connection)); 
    if ( isinit++ ) { 
        // retcon->status = -1; 
        return -1;
    } 
    dbgk_obj.client = client; 
    if ( sqlite3_open(db_path, &(dbgk_obj.db) ) ) {
        return -1;
    } 

    int retcds = 0; 
    retcds |= pthread_cond_init(&(dbgk_obj.io_flag), NULL); 
    retcds |= pthread_mutex_init(&(dbgk_obj.mux), 0 ); 

    // retcds |= fdcntl( dbgk_obj.pipe[0], F_SETFL, O_NONBLOCK ); // do we need to do this for every one? 
    // retcds |= fdcntl( dbgk_obj.pipe[1], F_SETFL, O_NONBLOCK ); 

    retcds |= pipe(dbgk_obj.pipe); 

    if ( retcds ) { 
        client->status = -1; 
        return -1;    
    } 
    
    client->io_flag = &dbgk_obj.io_flag; 
    client->mux = &dbgk_obj.mux; 

    client->pipe[0] = dbgk_obj.pipe[0]; 
    client->pipe[1] = dbgk_obj.pipe[1]; 

    client->status = DBGK_NOIO; 

    clock_gettime(CLOCK_REALTIME, &(dbgk_obj.cleanup_tm));
    dbgk_obj.cleanup_tm.tv_sec += ( cooldown < 600 ) ? cooldown : 600;  

    int dbgk_pid = fork(); 
    if ( dbgk_pid < 0 ) { 
        return -3; 
    } else if ( dbgk_pid > 0 ) { 
        // dbgk_obj.dbgk_pid = dbgk_pid;
        // dbgk_obj.mux = retcon->mux; 
        
        // client->status = dbgk_NOIO;  
        return 0; 
    } else { 
        sleep_await_message(); 
    } 
   
} 

void sleep_await_message() { 
    int ecode; 
    pthread_mutex_lock(&(dbgk_obj.mux)); 
    // typecast required because "incomplete definition"
    while ( ( (dbgk_connection *) dbgk_obj.client )->status == DBGK_NOIO ) { 
        ecode = pthread_cond_timedwait(&(dbgk_obj.io_flag), &(dbgk_obj.mux), dbgk_obj.cleanup_tm.tv_sec); 

        if (  ecode == ETIMEDOUT ) { 
            cleanup(); 
        } else if (ecode ) { 
            fprintf(stderr, "Error %i returned by pthread_cond_wait\n", ecode); 
            cleanup(); 
        } 
        if ( ( (dbgk_connection * )dbgk_obj.client)->status == DBGK_CLWRITE ) { 
            read_message();  
        } 
        // handle error 
        // dbgk_obj.client->status = ecode; 
        // cleanup(); 
    } 

} 

void read_message() { 
    // assert( sizeof(dbgk_
    close(dbgk_obj.pipe[1]); 
    int msg_type = 0; 
    int read_ret = read(dbgk_obj.pipe[0], &msg_type, 1); 
    if ( read_ret == -1 ) { 
        fprintf(stderr, "dbgk awoken on empty pipe\n"); 
        return; 
    } 
    // else if ( read_ret == 0 ) { 

    // } 
    switch (msg_type) { 
        case 0: 
            printf("Keepawake recieved\n"); 
            if ( read_ret ) { 
                goto _pipe_ovf;  
            }  
            break; 
        case 4: 
            printf("Unauthreq recieved\n"); 
            break;  
        // default: 
    } 
    // close(dbgk_obj.pipe[0]); 
    return; 
    _pipe_ovf:; 
    fprintf(stderr, "dbgk: pipe overflow\n"); 
} 

void cleanup() { 

} 

