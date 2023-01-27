#ifndef COMMON
#define COMMON

// libhydrogen
#define CONTEXT "pm:psmgr"
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
// pm opts
#define PIPETOSTDOUT 1
#define SKIPVAL 2
#define UKEY 4
#define OVAULT 8
#define CONFIRM 16
#define WARNNOVAL 32
#define DEFVAULT 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "libhydrogen/hydrogen.h" // Need?
#include "sqlite3.h"
#include "o_str.h"
#include "pmsql.h"

typedef struct {
    sqlite3 * db;
    char table_name[16];
    uint8_t master_key[hydro_pwhash_MASTERKEYBYTES]; // 32
    uint8_t derived_key[I_KEYSIZE];
    uint8_t plaintext[DATASIZE];
    uint8_t ciphertext[CIPHERSIZE];// (64 + 32 = 96)
    int guardian_pid;
    int cooldown;
    char pm_opts; // see idocs for explanation
    char * conf_path; 
} pm_inst;

#include "cli.h"
#include "pm.h"
#include "enc.h"

#endif
