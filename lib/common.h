#ifndef COMMON
#define COMMON

#define CONTEXT "pm:psmgr"
#define OPSLIMIT 10000 // DO we need these 2?
#define MEMLIMIT 0
#define THREADS  1
#define DATASIZE 64
#define I_KEYSIZE 32
#define NAMELEN 32

#define SALTSIZE 9
#define M_KEYSIZE 23
//hydro_pwhash_MASTERKEYBYTES = 32 ( immutable for our intents / purposes )
#define CIPHERSIZE (DATASIZE + hydro_secretbox_HEADERBYTES)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "hydrogen.h" // Need?
#include "sqlite3.h"
#include "o_str.h"

typedef struct {
    sqlite3 * db;
    char table_name[16];
    uint8_t master_key[hydro_pwhash_MASTERKEYBYTES]; // 32
    uint8_t derived_key[I_KEYSIZE];
    uint8_t plaintext[DATASIZE];
    uint8_t ciphertext[CIPHERSIZE];// (64 + 32 = 96)
    int guardian_pid;
    int cooldown;
    char crypt_opts; // see idocs for explanation
    char pm_opts;
} pm_inst;

#include "cli.h" 
#include "pm.h"
#include "enc.h"

#endif
