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
        return 1;
    } else if ( pswd_len == 0 ) {
        printf("Enc: no passphrase entered\n");
        return 1;
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

    char * m_key = crypt(pswd, salt);
    strncpy( PM_INST->master_key, m_key, SALTSIZE + M_KEYSIZE);

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

    return 0;
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
    char prompt[] = "Enter the passphrase you used to encrypt your entry:\n";
    // char key[KEYSIZE];

    char * pswd = getpass(prompt);
    int pswd_len = strlen(pswd);

    if ( pswd_len >= DATASIZE ){
        printf("Dec: pm is configured to accept passphrases up to %i characters \
            long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
        return 1;
    } else if ( pswd_len == 0 ) {
        printf("Dec: no passphrases entered\n");
        return 1;
    }
    // derive salt from ciphertext
    char validate = ( PM_INST->crypt_opts & 1 );
    uint8_t salt[SALTSIZE];
    memcpy( salt, PM_INST->master_key, SALTSIZE);

    char * user_derived_key = crypt(pswd, salt);

    if ( ( validate ) && ( strcmp(PM_INST->master_key, user_derived_key) != 0 ) ) {
            return -1; // invalid passphrase
    }
    // Now derived the dervived key to decrypt the entry

    int ecode = hydro_pwhash_deterministic(PM_INST->derived_key, I_KEYSIZE, pswd, pswd_len,
        CONTEXT, user_derived_key, OPSLIMIT, MEMLIMIT, THREADS);

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
    memset( pswd, 0, pswd_len );

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
