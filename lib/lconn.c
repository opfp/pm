#include "lconn.h" 

typedef struct { 
    sqlite3 * db; 
    time_t clenup_tm; 
    pthread_mutex_t mux; 
    pthread_cond_t read; 
    lconn_connection * client; 
} lconn_inst; 

void sleep_await_message(); 

static lconn_inst lconn_obj; 
static char isinit = 0; 

int lconn_init( lconn_connection * retcon, char * db_path, time_t cooldown ) { 
    // lconn_connection * retcon = malloc(sizeof(lconn_connection)); 
    if ( isinit ) { 
        retcon->status = -1; 
        return -1;
    } 
    if ( sqlite3_open(db_path, &(lconn_obj.db) ) ) {
        return -1;
    } 

    int retcds = 0; 
    retcds |= pthread_cond_init(&(retcon->io_flag), NULL); 
    retcds |= pthrad_mutex_init(&(retcon->mux) ); 
    // retcds |= fdcntl( retcon->inp[0], F_SETFL, O_NONBLOCK ); // do we need to do this for every one? 
    // retcds |= fdcntl( retcon->inp[1], F_SETFL, O_NONBLOCK ); 
    // retcds |= fdcntl( retcon->outp[0], F_SETFL, O_NONBLOCK ); 
    // retcds |= fdcntl( retcon->outp[1], F_SETFL, O_NONBLOCK ); 
    retcds |= pipe(retcon->inp); 
    retcds |= pipe(retcon->outp); 

    if ( retcds ) { 
        retcon->status = -1; 
        return -1;    
    } 

    time(&(lconn_obj.clenup_tm)); 
    lconn_obj.clenup_tm += ( cooldown < 600 ) ? cooldown : 600;  

    int lconn_pid = fork(); 
    if ( lconn_pid < 0 ) { 
        return -3; 
    } else if ( lconn_pid > 0 ) { 
        // lconn_obj.lconn_pid = lconn_pid;
        lconn_obj.mux = retcon->mux; 
        pthread_mutex_lock(&(retcon->mux)); 
        retcon->status = LCONN_NOIO;  
        return retcon; 
    } else { 
        sleep_await_message(); 
    } 
   
} 

void sleep_await_message() { 
    int ecode; 
    while ( ecode == pthread_cond_timedwait(&(lconn_obj.io_flag), &(lconn_obj.mux), lconn_obj.clenup_tm) ) { 
        if (  ecode == ETIMEDOUT ) { 
            cleanup(); 
        } 
        if ( ecode == EOK ) { 
            read_message();  
        } 
        // handle error 
        lconn_obj.client->status = ecode; 
        cleanup(); 
    } 
} 

void read_message() { 
    // assert( sizeof(lconn_
} 

