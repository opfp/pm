#include "enc.h"
#include "cli.h" 

/* TODO : structure enc return values with enum */ 
int enc_plaintext( pm_inst * pm_monolith, char * pswd ) {
	int ecode = 0; // these need to always be defined so cleanup never refs undeclared vars
	sqlite3_stmt * stmt;
	pmsql_stmt pmsql = { SQLITE_STATIC, pm_monolith->db, stmt, NULL };

	if ( pswd == NULL ) { 
		char prompt[] = "Enter passphrase to encrypt your entry:\n";
		/*char * */ pswd = getpass(prompt);
	}
	char * m_key;
	int pswd_len = strlen(pswd);

	if ( pswd_len >= DATASIZE ){ // this should always be caught in cli  
		printf("Enc: pm is configured to accept passphrases up to %i characters\
			long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
		ecode = -5;
		goto enc_cleanup;
	} else if ( pswd_len == 0 ) {
		printf("Enc: no passphrase entered\n");
		ecode = -5;
		goto enc_cleanup;
	}

	if ( ! _entry_in_table(pm_monolith, "_index" , pm_monolith->table_name) ) { // if no entry
		ecode = -3; // No vault with such a name
		goto enc_cleanup;
	}

	pmsql_data_t * data = ( pmsql_data_t [1] ) { {.text=pm_monolith->table_name} };
	int * data_tp = ( int [1] ) { PMSQL_TEXT };
	char * query = "SELECT UKEY, SALT, MASTER_KEY FROM _index WHERE ID = ?";

	if (( ecode = pmsql_compile(&pmsql, query, 1, data, NULL, data_tp) ))
		goto enc_sql_fail;

	ecode = sqlite3_step(pmsql.stmt);
	if ( ecode != SQLITE_ROW )
		goto enc_sql_fail;

	int ukey;
	char i_salt[SALTSIZE];
	char i_mkey[M_KEYSIZE];
	data = ( pmsql_data_t [3] ) { {.int_wb=&ukey}, {.blob=i_salt}, {.blob=i_mkey} };
	int * data_sz = ( int [3] ) { 0, ( -1 * SALTSIZE ), ( -1 * M_KEYSIZE ) };
	data_tp = ( int [3] ) { PMSQL_INT_WB, PMSQL_BLOB, PMSQL_BLOB };

	if (( ecode = pmsql_read(&pmsql, 3, data, data_sz, data_tp) ))
		goto enc_sql_fail;

	char first_commkey = ukey & 2;
	ukey &= 1;

	if ( ukey != (pm_monolith->pm_opts & 4 ) >> 2 ) {
		ecode = -4; // commkey / ukey mismatch
		goto enc_cleanup;
	}
	// if comkey, check entered pswd for equality
	if ( !ukey && !first_commkey && strcmp(i_mkey, crypt(pswd, i_salt)) ) {
		ecode = -2; // wrong pswd for commkey vault
		goto enc_cleanup;
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
	m_key = crypt(pswd, salt);
	strncpy( pm_monolith->master_key, m_key, SALTSIZE + M_KEYSIZE);

	if ( first_commkey ) {
		query = "UPDATE _index SET (UKEY, SALT, MASTER_KEY) = (0,?,?) WHERE ID = ?";
		data = ( pmsql_data_t [3] ) { {.blob=salt}, {.blob=m_key}, {.text=pm_monolith->table_name} };
		data_tp = ( int [3] ) { PMSQL_BLOB, PMSQL_BLOB, PMSQL_TEXT };
		data_sz = ( int[3] ) { SALTSIZE, M_KEYSIZE, 0};

		if (( ecode = pmsql_compile(&pmsql, query, 3, data, data_sz, data_tp) ))
			goto enc_sql_fail;

		ecode = sqlite3_step(pmsql.stmt);
		if ( ecode != SQLITE_DONE )
			goto enc_sql_fail;
	}
	// derive derived key
	hydro_pwhash_deterministic(pm_monolith->derived_key, I_KEYSIZE, pswd, pswd_len,
		CONTEXT, pm_monolith->master_key, OPSLIMIT, MEMLIMIT, THREADS);
	// Enc plaintext and return
	hydro_secretbox_encrypt(pm_monolith->ciphertext, pm_monolith->plaintext,
		strlen(pm_monolith->plaintext), 0, CONTEXT, pm_monolith->derived_key);

	ecode = 0; // to tell cli weather to add entry to index
	goto enc_cleanup;

	enc_sql_fail:
		if ( ecode > 0 )
			ecode = -1;
		else {
			printf("PMSQL: %s\n", pmsql.pmsql_error );
			ecode = -3;
		}
	enc_cleanup:; //Destroy sensitive data
		sqlite3_finalize(pmsql.stmt);
		if ( pswd )
			memset(pswd, 0, strlen(pswd) );
		if ( m_key )
			memset(m_key, 0, strlen(m_key) );
		memset(pm_monolith->plaintext, 0, DATASIZE);
		return ecode;
}

/* dec expects the password in plaintext! */ 
int dec_ciphertext( pm_inst * pm_monolith /*, char * pswd*/ ) {
	//char plaintext[DATASIZE];
	// if ( pm_monolith->guardain_pid != 0 ) {  //check if we are in the key_cooldown period
	//	 //kill(pm_monolith->guardian_pid, 10); // Send sleep signal so guardian doesnt interfere
	//	 hydro_secretbox_decrypt(plaintext, pm_monolith->ciphertext,
	//		 DATASIZE + hydro_secretbox_HEADERBYTES, 0, CONTEXT, pm_monolith->derived_key);
	//	 return;
	// } // else we have to get the key ourselves
	// char key[KEYSIZE];

	int pswd_len = strnlen(pm_monolith->plaintext, DATASIZE);
	if (pswd_len == 0 ) { 
		printf("dec: password not passed\n"); 
		return -1; 
	} 

	//if ( pswd == NULL ) {
	//	char prompt[] = "Dec: Enter the passphrase you used to encrypt your entry:\n";
	//	/*char * */ pswd = getpass(prompt);
	//	pswd_len = strlen(pswd);
	//}

	//if ( pswd_len >= DATASIZE ){
	//	printf("Dec: pm is configured to accept passphrases up to %i characters \
	//		long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
	//	return 1;
	//} else if ( pswd_len == 0 ) {
	//	printf("Dec: no passphrases entered\n");
	//	return 1;
	//}
	//strncpy(pm_monolith->plaintext, pswd, pswd_len); //null term??
	//memset(pswd, 0, pswd_len);

	//} 
		//else {
	//	pswd_len = strlen(pm_monolith->plaintext);
	//}
	// derive salt from ciphertext



	char validate = ( ! (pm_monolith->pm_opts & 2) );
	uint8_t salt[SALTSIZE];
	memcpy( salt, pm_monolith->master_key, SALTSIZE);

	char * user_derived_key = crypt(pm_monolith->plaintext, salt);

	if ( ( validate ) && ( memcmp(pm_monolith->master_key, user_derived_key,
		M_KEYSIZE + SALTSIZE) != 0 ) ) {
			return -1; // invalid passphrase
	}
	// Now derived the dervived key to decrypt the entry

	int ecode = hydro_pwhash_deterministic(pm_monolith->derived_key, I_KEYSIZE,
		pm_monolith->plaintext, pswd_len, CONTEXT, user_derived_key, OPSLIMIT, MEMLIMIT, THREADS);

	if ( ecode != 0 ){
		printf("Dec: Error deriving derived key : %i\n", ecode);
		return -2;
	}

	ecode = hydro_secretbox_decrypt(pm_monolith->plaintext, pm_monolith->ciphertext,
		DATASIZE + hydro_secretbox_HEADERBYTES, 0, CONTEXT, pm_monolith->derived_key);
	if ( ecode != 0 ) {
		printf("Dec: Error decrypting ciphertext: %i\n", ecode);
		return -2;
	}

	// De pad plaintext
	char * emsg = strchr(pm_monolith->plaintext, ';');
	if ( emsg != NULL ) {
		*emsg = 0;
	}

	// Destroy our copy of user_derived_key, passwd
	memset( user_derived_key, 0, strlen(user_derived_key));
	//memset( pm_monolith->plaintext, 0, pswd_len );
	return 0;
}

// int check( pm_inst * pm_monolith, char * name ) {
//
// }

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
