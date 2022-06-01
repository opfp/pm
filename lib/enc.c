#include "enc.h"

/* @ssert enc_plaintext
    PM_INST . enc_flags
    have been set
*/
int enc_plaintext( pm_inst * PM_INST ) {
    // if ( PM_INST->key_write_time != 0 ) {  //check if we are in the key_cooldown period
    //     //kill(PM_INST->guardian_pid, 10); // Send sleep signal so guardian doesnt interfere
    //     hydro_secretbox_encrypt(PM_INST->ciphertext, PM_INST->plaintext,
    //         strlen(PM_INST->plaintext), 0, CONTEXT, PM_INST->derived_key);
    //     return;
    // }
    // Else derive keys and then enc
    char prompt[] = "Enter passphrase to encrypt your entry:\n";
    char * pswd = getpass(prompt);
    int pswd_len = strlen(pswd);

    if ( pswd_len >= DATASIZE ){ //this will literally NEVER happen. see man getpass()
        printf("Enc: pm is configured to accept passphrases up to %i characters\
            long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
        return -5;
    } else if ( pswd_len == 0 ) {
        printf("Enc: no passphrase entered\n");
        return -5;
    }

    if ( ! _entry_in_table(PM_INST, "_index" , PM_INST->table_name) ) { // if no entry
        return -3; // No vault with such a name
    }

    char * query = "SELECT UKEY, SALT, MASTER_KEY FROM _index WHERE ID = ?";
    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, strlen(query),
        &statement, NULL);
    if ( ecode != SQLITE_OK )
        return -1;

    ecode = sqlite3_bind_text(statement, 1, PM_INST->table_name,
        strlen(PM_INST->table_name), SQLITE_STATIC/*?*/);
    if ( ecode != SQLITE_OK )
        return -1;

    ecode = sqlite3_step(statement);
    if ( ecode != SQLITE_ROW )
        return -1;

    int ukey = sqlite3_column_int(statement, 0);
    char first_commkey = ukey & 2;
    ukey &= 1;

    if ( ukey != (PM_INST->pm_opts & 4 ) >> 2 )
        return -4; // commkey / ukey mismatch

    // if comkey, check entered pswd for equality
    // but if first commkey, we can't
    if ( !ukey && !first_commkey ) {
        int lens[] = { SALTSIZE, M_KEYSIZE };
        char check_salt[SALTSIZE+1] = {0};
        char check_mkey[M_KEYSIZE+1] = {0};
        char * res_buffs[] = { check_salt, check_mkey };

        for ( int i = 0; i < 2; i++ ) {
            if (sqlite3_column_bytes(statement, i+1) != lens[i] ) {
                printf("len mismatch row %i : %i != %i\n", i,
                    sqlite3_column_bytes(statement, i+1), lens[i] );
                return -1;
            }
            memcpy(res_buffs[i], sqlite3_column_text(statement,i+1), lens[i] );
            //res_buffs[lens[i]] = 0;
        }

        if ( strcmp(check_mkey, crypt(pswd, check_salt) /*+SALTSIZE, M_KEYSIZE*/) ) // if not equal
            return -2;
    }

    // Derive master key with crypt()
    char salt[] = "_$345678$";

    srandom(mix( clock(), time(NULL), getpid() ));
    unsigned long raw_salt = random();
    raw_salt = raw_salt << 32;
    srandom(mix( getpid(),time(NULL), clock() ));
    raw_salt += random();
    memcpy(salt + 2, &raw_salt, 6);

    for(int i = 2; i < 8; i++ ) {
        unsigned char t = (unsigned char) ((unsigned char) salt[i] % 64); // Hmm I wonder what type this is supposed to be
        if ( t < 12 ) { // [0,11]
            t += 46;
        } else if ( t < 38 ) { // [12,37]
            t += 53; // 65 = A , 53 = 65 - 12
        } else { // [38,63]
            t += 58; // 97 = a , 60 = 97 - 39
        }
        salt[i] = t;
    }
    // even in commkey vaults, a seperate salt is maintained for each entry for enhanced security
    char * m_key = crypt(pswd, salt);
    strncpy( PM_INST->master_key, m_key, SALTSIZE + M_KEYSIZE);

    if ( first_commkey ) {
        char * query2 = "UPDATE _index SET (UKEY, SALT, MASTER_KEY) = (0,?,?) WHERE ID = ?";

        ecode = sqlite3_prepare_v2(PM_INST->db, query2, strlen(query2),
            &statement, NULL);
        if ( ecode != SQLITE_OK )
            return -1;

        int ecodes[3];
        ecodes[0] = sqlite3_bind_text(statement, 1, salt, SALTSIZE, SQLITE_STATIC);
        ecodes[1] = sqlite3_bind_text(statement, 2, m_key, M_KEYSIZE, SQLITE_STATIC);
        ecodes[2] = sqlite3_bind_text(statement, 3, PM_INST->table_name,
            strlen( PM_INST->table_name), SQLITE_STATIC);

        if ( ecodes[0] != SQLITE_OK || ecodes[1] != SQLITE_OK || ecodes[2] != SQLITE_OK )
            return -1;

        ecode = sqlite3_step(statement);
        if ( ecode != SQLITE_DONE )
            return -1;
    }

    sqlite3_finalize(statement);

    /*
        EVENTUALLY master_key and derived_key will not be stored in an instance of pm,
        but rather by the guardian process, which will auto-destroy them after a
        configurable cooldown time not greater than 1 hour. PM_INST will instead
        hold a pointer to both keys ( which are in guradian heap memory ), and will
        keep track of weather guardain is running ( has stored keys ), or not.
    */

    // derive derived key
    hydro_pwhash_deterministic(PM_INST->derived_key, I_KEYSIZE, pswd, pswd_len,
        CONTEXT, PM_INST->master_key, OPSLIMIT, MEMLIMIT, THREADS);
    // Destroy password, and our copy of m_key
    int j = 0;
    while(pswd[j++] != 0) {
        pswd[j] = 0;
    }
    j = 0;
    while( m_key[j++] != 0 ) {
        m_key[j] = 0;
    }
    // Enc plaintext and return

    hydro_secretbox_encrypt(PM_INST->ciphertext, PM_INST->plaintext,
        strlen(PM_INST->plaintext), 0, CONTEXT, PM_INST->derived_key);

    return 0; // to tell cli weather to add entry to index
}

/* @ssert: dec_ciphertext
    PM_INST -> master_key , dec_flags
    have been set
*/
int dec_ciphertext( pm_inst * PM_INST ) {
    //char plaintext[DATASIZE];
    // if ( PM_INST->guardain_pid != 0 ) {  //check if we are in the key_cooldown period
    //     //kill(PM_INST->guardian_pid, 10); // Send sleep signal so guardian doesnt interfere
    //     hydro_secretbox_decrypt(plaintext, PM_INST->ciphertext,
    //         DATASIZE + hydro_secretbox_HEADERBYTES, 0, CONTEXT, PM_INST->derived_key);
    //     return;
    // } // else we have to get the key ourselves
    // char key[KEYSIZE];

    int pswd_len;

    if ( ! PM_INST->plaintext ) {
        char prompt[] = "Dec: Enter the passphrase you used to encrypt your entry:\n";
        char * pswd = getpass(prompt);
        pswd_len = strlen(pswd);

        if ( pswd_len >= DATASIZE ){
            printf("Dec: pm is configured to accept passphrases up to %i characters \
                long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
            return 1;
        } else if ( pswd_len == 0 ) {
            printf("Dec: no passphrases entered\n");
            return 1;
        }
        strncpy(PM_INST->plaintext, pswd, pswd_len); //null term??
        memset(pswd, 0, pswd_len);
    } else {
        pswd_len = strlen(PM_INST->plaintext);
    }
    // derive salt from ciphertext
    char validate = ( ! (PM_INST->pm_opts & 2) );
    uint8_t salt[SALTSIZE];
    memcpy( salt, PM_INST->master_key, SALTSIZE);

    char * user_derived_key = crypt(PM_INST->plaintext, salt);

    if ( ( validate ) && ( memcmp(PM_INST->master_key, user_derived_key,
        M_KEYSIZE + SALTSIZE) != 0 ) ) {
            return -1; // invalid passphrase
    }
    // Now derived the dervived key to decrypt the entry

    int ecode = hydro_pwhash_deterministic(PM_INST->derived_key, I_KEYSIZE,
        PM_INST->plaintext, pswd_len, CONTEXT, user_derived_key, OPSLIMIT, MEMLIMIT, THREADS);

    if ( ecode != 0 ){
        printf("Dec: Error deriving derived key : %i\n", ecode);
        return -2;
    }

    ecode = hydro_secretbox_decrypt(PM_INST->plaintext, PM_INST->ciphertext,
        DATASIZE + hydro_secretbox_HEADERBYTES, 0, CONTEXT, PM_INST->derived_key);
    if ( ecode != 0 ) {
        printf("Dec: Error decrypting ciphertext: %i\n", ecode);
        return -2;
    }

    // De pad plaintext
    char * emsg = strchr(PM_INST->plaintext, ';');
    if ( emsg != NULL ) {
        *emsg = 0;
    }

    // Destroy our copy of user_derived_key, passwd
    memset( user_derived_key, 0, strlen(user_derived_key));
    //memset( PM_INST->plaintext, 0, pswd_len );

    return 0;
}

// Robert Jenkins' 96 bit Mix Function
unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

// printf("\nDerived Key: ");
// for( int i = 0; i < I_KEYSIZE; i++ ) {
//     printf("[%u]", PM_INST->derived_key[i]);
// }
// printf("\nCiphertext: ");
// for( int i = 0; i < CIPHERSIZE; i++ ) {
//     printf("[%u]", PM_INST->ciphertext[i]);
// }
// printf("\n");
