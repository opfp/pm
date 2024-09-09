#ifndef COMMON
#define COMMON

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h> 

// libhydrogen
#define CONTEXT "pm:psmgr" // should this be static? 
#define OPSLIMIT 10000 // DO we need these 2?
#define MEMLIMIT 0
#define THREADS  1
// pmsql
#define DATASIZE 64
#define I_KEYSIZE 32
#define NAMELEN 32
#define SALTSIZE 9
#define M_KEYSIZE 23
//hydro_pwhash_MASTERKEYBYTES = 32 ( immutable for our intents / purposes )
#define CIPHERSIZE (DATASIZE + hydro_secretbox_HEADERBYTES)

//  // pm opts
//  #define NUM_FLAGS 3 
//  enum pm_opts { PIPETOSTDOUT=1, SKIPVAL=2, UKEY=4, OVAULT=8, NOCONFIRM=16,
//   	WARNNOVAL=32, DEFVAULT=64 }; 
//  
//  // flags modify the opts from the cli or config 

typedef uint8_t pm_options_t; 

// application return values 
/* These are "good" errors that are defined behavior, occuring when the user does 
	 something wrong or invalid */  
enum ISSUE_RETS { PW_WRONG=1, WRONG_KEYTYPE, SYNTAX, ILLEGAL_IN, OVERWRITE, DNE }; 

#include "hydrogen.h" // Need?
#include "sqlite3.h"
#include "o_str.h"
#include "pmsql.h"

typedef struct {
    sqlite3 * db;
    char table_name[16];
    uint8_t master_key[hydro_pwhash_MASTERKEYBYTES]; // 32
    uint8_t derived_key[I_KEYSIZE];
		//uint8_t passtext[DATASIZE]; 
    uint8_t plaintext[DATASIZE];
    uint8_t ciphertext[CIPHERSIZE];// (64 + 32 = 96)
    //int guardian_pid;
    int cooldown;
    //char pm_opts; // see idocs for explanation
		pm_options_t pm_opts; 
    char * conf_path; 
} pm_inst;

//#include "cli.h"
//#include "pm.h"
//#include "enc.h"

#endif
