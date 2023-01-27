#include "dbman.h" 

static dbman_inst dbman_obj; 
static char isinit = 0; 

int dbman_init( char * db_path, int cooldown  ) { 
    if ( isinit ) { 
        return 2; 
    } 
    if ( sqlite3_open(db_path, &(dbman_inst.db) ) ) {
        return 1;
    } 
    int dbman_pid = fork(); 
    if ( dbman_pid) { 
        dbman_inst.dbman_pid = dbman_pid; 
    } else if ( !dbman_pid ) { 
        
    } else { 
        return 3; 
    } 

} 

