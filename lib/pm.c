#include "pm.h"

pm_inst * __init__() {
    if ( hydro_init() != 0 ) {
        printf("Init: error initilizing hydrogen\n");
        exit(0);
    }

    pm_inst * PM_INST = malloc( sizeof(pm_inst));
    memset(PM_INST, 0, sizeof(pm_inst));
    PM_INST->enc_flags = 3;
    /*  ENC flags will be sourced from config file
        & 1 : Keep master_key for password validation ( via hash and keysearch )
        & 2 : Warn if retreiveing or setting an entry with validation off
    */

    /*
        Connect to database ~ eventually should come from config file, and
        should be configurable from the command line
    */
    char db_path[] = "/Users/owen/cs/dev/pm/rsc/pswds.db";
    sqlite3 * db;
    if ( sqlite3_open(db_path, &db) != 0 ) {
        printf("Init: error connecting to database\n");
        exit(0);
    }

    char table_name[] = "test";
    strncpy(PM_INST->table_name, table_name, 15);

    PM_INST->db = db;

    //For now, lets avoid creating the guardian process until we're out of early testing
    //int pid = fork();

    // if ( pid == -1 )  { //pid returns 0 to child, and the child's pid to parent
    //     printf("Init: Error initilizing guardian");
    //     exit(-1);
    // } else if (pid == 0 ) {
    //     init_guardian(PM_INST);
    // } else {
    //     PM_INST->guardian_pid = pid;
    // }

    // IN THE FUTURE THESE WILL COME FROM pm.conf
    //char help_path = "/Users/owen/cs/dev/pm/docs.txt";
    return PM_INST;
}

int val_pad(char * in) {
    char pad = 0;
    for ( int i = 0; i < DATASIZE; i++ ) {
        unsigned char c = in[i];
        if ( ( c > 0 && c < 32)  || c == 127 || c == 59) {
            return -1;
        }
        if ( c == 0 ){
            in[i] = 59; // padding
        }
    }
    return 0;
}

// sqlite3_stmt * pm_sqlite3_make_stmt(char * basequery, ...) {
//     // First, concat the statement
//     basequery_len = strlen(basequery);
//     int num_strings = 0;
//     char * rover = basequery;
//     while( rover < basequery + basequery_len ) {
//
//     }
//
// }

// void init_guardian( pm_inst * PM_INST ){
//     //signal( SIGABRT, guardian_clear_pm( PM_INST));
//     //signal( SIGUSR1, guardian_sleep10 ); // This is
//     while (1) {
//         // Do guardian timecheck
//         if ( PM_INST->key_write_time == 0 ) {
//             sleep(5);
//             continue;
//         }
//         time_t ctime;
//         time(&ctime);
//         ctime -= PM_INST->key_write_time;
//         if ( ctime >= COOLDWN ) {
//             guardian_clear_pm(PM_INST);
//         }
//         sleep(5);
//     }
// }
//
// void guardian_clear_pm( pm_inst * PM_INST ) {
//     memcpy(PM_INST->master_key, 0, hydro_pwhash_MASTERKEYBYTES);
//     memcpy(PM_INST->derived_key, 0, KEYSIZE);
//     memcpy(PM_INST->plaintext, 0, DATASIZE);
//     memcpy(PM_INST->ciphertext, 0, DATASIZE + hydro_secretbox_HEADERBYTES);
//     PM_INST->key_write_time = 0;
// }
//
// void guardian_sleep10() {
//     printf("Guarding sleeping to allow enc\n");
//     sleep(10);
// }
