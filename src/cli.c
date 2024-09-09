#include "cli.h"
#include "enc.h" 
#include "pm.h" 

//enum verb_type { 

// private function declarations
int _ls_find(pm_inst *, char *);
int _recover_or_forget(pm_inst *, char *, char *, int);
int _delete(pm_inst *, char *);
int _delete_val(pm_inst *, char *);
char * * _find_by_key( pm_inst *, char *, char *, int);
int _fbk_compare( const void *, const void *);
char * * _all_in_table(pm_inst *, char *);
int _apply_kwargs( int, char **, char * * kw_vals, pm_inst * );  
int _apply_flags ( int, char **, pm_inst * );  

char * keywords[KW_NUM] = { "vault", "ctext", "pword", "name" }; 
/*
#define NUM_FLAGS 3 
char * flags[NUM_FLAGS] = { "unique-key", "skip-validate", "no-confirm" };  
int flag_bit[NUM_FLAGS] = { UKEY, SKIPVAL, NOCONFIRM }; 
*/
// void cli_main( int argc, char * * argv, pm_inst * pm_monolith) {
	// 	if ( argc < 1 ) {
	// 		printf("pm: Not enough arguments. [pm help] for help, or read the docs.\n");
	// 		//help(void, void);
	// 		return;
	// 	}
	// 	#ifndef NUM_VERBS
	// 	#define NUM_VERBS 10 
	// 	#endif 
	// 	char * verbs[NUM_VERBS] = { "set", "get", "mkv", "forg", "rec", "del",
	// 		"check", "ls", "help", "conf" };
	// 	// See idocs for explanation of verb_type
	// 
	// 	//						   set, get, mkv, forg, rec, del, chk, ls, help, conf
	// 	char verb_type[NUM_VERBS] = { 0 , 0  , 1  , 0   , 0  , 0  , 0  , 2 , 0, 0};
	// 	void (*functions[])(pm_inst *, char *) = { set, get, mkvault, forget, recover,
	// 		delete, check, ls, help, conf };
	// 
	// 	int i = 0;
	// 	while ( i < NUM_VERBS ) {
	// 		if ( !strcmp(argv[0], verbs[i]) )
	// 			break;
	// 		 ++i;
	// 	}
	// 	if ( i == NUM_VERBS ) {
	// 		printf("verb %s unknown\n", argv[0] );
	// 		help(NULL, NULL);
	// 		return;
	// 	}
	// 
	// 	char vault_skipval = 0;
	// 	char * vault = NULL;
	// 	char * name = NULL;
	// 	if ( verb_type[i] == 0 ) {
	// 		if ( argc < 2 ) {
	// 			printf("%s: Name required. [pm help].\n", verbs[i] );
	// 			return;
	// 		}
	// 		name = ( argc > 2 ) ? argv[2] : argv[1];
	// 		vault = ( argc > 2 ) ? argv[1] : NULL;
	// 	} else if ( verb_type[i] == 1 ) {
	// 		if ( argc < 2 ) {
	// 			printf("%s: Name required. [pm help].\n", verbs[i] );
	// 			return;
	// 		}
	// 		name = argv[1];
	// 	} else if ( verb_type[i] == 2 ) {
	// 		if ( argc > 1 ) {
	// 			if ( pm_monolith->pm_opts & OVAULT ) {
	// 				vault = argv[1];
	// 				name = ( argc > 2 ) ? argv[2] : NULL;
	// 			} else {
	// 				vault = ( argc > 2 ) ? argv[1] : NULL;
	// 				name = ( argc > 2 ) ? argv[2] : argv[1];
	// 			}
	// 		} else if ( pm_monolith->pm_opts & OVAULT ) {
	// 			vault = "_index";
	// 			vault_skipval = 1;
	// 		}
	// 	}
	// 	// } else if ( verb_type[i] == 3 ) {
	// 	//	 if ( argc < 2 ) {
	// 	//		 printf("%s: Vault name required. [pm help].\n", verbs[i] );
	// 	//		 return;
	// 	//	 }
	// 	//	 vault = argv[1];
	// 	//	 name = ( argc > 2 ) ? argv[2] : NULL;
	// 	// }
	// 	// --- testing ---
	// 	// char * option_names[] = { "Echo", "Skip pswd hash val", "Use ukey vault",
	// 	//	 "vault" , "Confirm cipertexts" , "Warn on no validation" , "Use only default vaults"};
	// 	// unsigned char opts = pm_monolith->pm_opts;
	// 	// for ( int i = 0; i < 7; i++) {
	// 	//	 printf("%i : %s\n", ( opts & 1 ), option_names[i]);
	// 	//	 opts = opts >> 1;
	// 	// }
	// 	// ---
	// 	if (vault && !vault_skipval) {
	// 		if ( !pmsql_safe_in(vault) || strlen(vault) > 15 ) {
	// 			printf("Illegal vault name: %s\n", vault);
	// 			return;
	// 		}
	// 		if ( pm_monolith->pm_opts & DEFVAULT ) {
	// 			printf("Vault specified, but pm configured to only use defaults."
	// 				" [ pm chattr def_vaults 0 ] to enable user created vaults.\n");
	// 			return;
	// 		}
	// 		if ( !_entry_in_table(pm_monolith, "_index", argv[1]) && !verb_type[i] ) {
	// 			printf("No vault named %s. [pm mkvault] to make a vault.\n", argv[1]);
	// 			return;
	// 		}
	// 	} else if ( !vault_skipval ) {
	// 		vault = ( pm_monolith->pm_opts & UKEY ) ? "_main_ukey" : "_main_cmk" ;
	// 	}
	// 	strcpy(pm_monolith->table_name, vault);
	// 
	// 	if (name) {
	// 		if ( !pmsql_safe_in(name) || strlen(name) > 31 ) {
	// 			printf("Illegal name: %s\n", name);
	// 			return;
	// 		}
	// 	}
	// 	// printf("%s : %s {%s}\n", verbs[i], name, pm_monolith->table_name );
	// 	(*functions[i])(pm_monolith, name);
	// 	return;
	// }

int cli_new( int argc, char * * argv, pm_inst * pm_monolith) {
  if ( argc < 1 ) {
	  printf("pm: verb required. [pm help] for help, or read the docs.\n");
	  //help(void, void);
	  return SYNTAX;
  }
	int err_no; 
	/* pm [verb] [name] [kwargs . . . ] */ 
	/* first apply kwargs */ 
	char * * kwargs_vals = calloc( KW_NUM, sizeof(char *) ); 	
	if ( _apply_kwargs( argc, argv, kwargs_vals, pm_monolith) ) 
		return SYNTAX; 
	/* then apply flags */ 
	if ( _apply_flags( argc, argv, pm_monolith) ) 
		return SYNTAX; 

	/* all that is left should be the verb and perhaps a name */ 
	char * verb = kwargs_vals[KW_VAULT];  
	char * name = kwargs_vals[KW_NAME]; 
	for ( int i = 0; i < argc; ++i ) { 
		if ( argv[i] == NULL ) 
			continue; 
		if ( verb == NULL ) {  
			verb = argv[i]; 
		} else if ( name == NULL ) { 
			name = argv[i]; 
		} else { 
			printf("Ignored extraneous argument: %s\n", argv[i]); 
		}
	}

	if ( verb == NULL ) {	
  	printf("pm: verb required. [pm help] for help, or read the docs.\n");
		//help(void, void);
		return SYNTAX;
	} 

  //#ifndef NUM_VERBS
  #define NUM_VERBS 10 
  //#endif 
  char * verbs[NUM_VERBS] = { "set", "get", "mkv", "forg", "rec", "del",
  	"check", /*last 3 don't require name*/ "ls", "help", "conf" };

	int (*functions[])(pm_inst *, char *, char **) = { set, get, mkvault, forget, recover,
		delete, check, ls, help, conf };

  int verb_idx = 0;
  while ( verb_idx < NUM_VERBS ) {
	  if ( !strcmp(argv[0], verbs[verb_idx]) )
		  break;
	  ++verb_idx;
  }
  if ( verb_idx == NUM_VERBS ) {
	  printf("Unknown verb %s\n", verb );
	  help(NULL, NULL, NULL);
	  return SYNTAX;
  }

	if ( verb_idx < 7 && name == NULL ) { 
		printf("Name required for %s operation\n", verbs[verb_idx] );   
		return SYNTAX; 
	}

  if (name && ( !pmsql_safe_in(name) || strlen(name) > 31 ) ) {
  	printf("Illegal name: %s\n", name);
		return ILLEGAL_IN;
  }

	char * vault = kwargs_vals[KW_VAULT]; 

	if (vault /*&& !vault_skipval*/) {
		if ( !pmsql_safe_in(vault) || strlen(vault) > 15 ) {
			printf("Illegal vault name: %s\n", vault);
			return ILLEGAL_IN;
		}
		if ( pm_monolith->pm_opts & DEFVAULT ) {
			printf("Vault specified, but pm configured to only use defaults."
				" [ pm chattr def_vaults 0 ] to enable user created vaults.\n");
			return SYNTAX;
		}
		//if ( !_entry_in_table(pm_monolith, "_index", argv[1]) && !verb_type[i] ) {
		//	printf("No vault named %s. [pm mkvault] to make a vault.\n", argv[1]);
		//	return;
		//}
	} else /*if ( !vault_skipval )*/ {
		vault = ( pm_monolith->pm_opts & UKEY ) ? "_main_ukey" : "_main_cmk" ;
	}
	strncpy( pm_monolith->table_name, vault, 16);  

	if ( kwargs_vals[KW_CIPHER] ) { 
		if ( strlen( kwargs_vals[KW_CIPHER] ) >= DATASIZE ) { 
			printf("Ciphertext is too long (limit %u characters)\n", DATASIZE-1 ); 
			return ILLEGAL_IN; 
		}
		strncpy( pm_monolith->plaintext, kwargs_vals[KW_CIPHER], DATASIZE ); 
	}

	//if ( kw


	return (*functions[verb_idx])(pm_monolith, name, kwargs_vals);
}

int set(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	int name_len = strlen(name);
	char exists = _entry_in_table(pm_monolith, pm_monolith->table_name, name);
	if ( exists == 2 ) {
		printf("Set: an entry with name %s already exists in %s.\n",name,pm_monolith->table_name);
		return OVERWRITE;
	} else if ( exists == 1 ) { //Entry exists but is in trash, will be overwritten.
		if ( _delete(pm_monolith, name) ) { 
			printf("Set: backend error in _delete: %s\n", sqlite3_errmsg(pm_monolith->db) );
			return -1; 
		} 
	}

	char * ctxt = kwarg_vals[KW_CIPHER]; 
	bool conf_ctxt = false; 

	if ( ctxt == NULL ) {  
		conf_ctxt = true; 
		char * rprompt = "Enter the ciphertext for the entry %s:\n";
		char * prompt = malloc(strlen(rprompt) + strlen(name) + 1);
		sprintf(prompt, rprompt, name);
		char * ctxt = getpass(prompt);
		free(prompt);
	}

	if ( ctxt == NULL ){
		printf("Set: no ciphertext entered\n");
		goto set_cleanup;
	}
	int ctxt_len = strlen(ctxt);
	if ( ctxt_len >= DATASIZE) {
		printf("Set: pm is can only accept ciphers up to %i characters long."
		" Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
		goto set_cleanup;
	}
	// Copy context to a buffer of datasize - so we can put it into pm, and so
	// getpass can be called again (it modifies the same buffer)
	char context[DATASIZE];
	memset( context, 0, DATASIZE);
	memcpy(context, ctxt, ctxt_len);

	char * check;
	// only confirm if ciphertext passed interactively and NOCONF not set 
	if ( conf_ctxt && !(pm_monolith->pm_opts & NOCONFIRM) ) { 
		check = getpass("Confirm your entry:\n");
		// if ( check == NULL )
		//	 check = context; // Not a bug - user can bypass by entering nothing
		if ( strcmp(context, check) != 0 ) {
			printf("Your entries did not match, try again.\n");
			goto set_cleanup;
		} // destroy check
		memset(check, 0, strlen(check) );
		check = NULL;
	}
	if ( val_pad(context) != 0 ) { // wish we could move this above the confirm code
		printf("Set: your cipher contained disallowed characters. Only printable"
		" characters, except ;, are allowed.\n");
		goto set_cleanup;
	}
	//Add plaintext to pm_inst obj
	memcpy(pm_monolith->plaintext, context, DATASIZE);

	memset(ctxt, 0, ctxt_len);
	ctxt = NULL;
	memset(context, 0, DATASIZE);

	int enc_code = enc_plaintext(pm_monolith, kwarg_vals[KW_PASS] );

	if ( enc_code == -2 ) {
		printf("Set: Wrong password for vault %s. Use -u to make an arbitrary key entry\n",
			pm_monolith->table_name);
		 /* We're free to just return because we've already destroyed our sensitive data 
				(and so has enc) */ 
		return PW_WRONG; 
	} else if ( enc_code == -4) {
		printf("Set: Commkey / ukey mismatch. Toggle -u to fix\n");
		return WRONG_KEYTYPE;
	} else if ( enc_code == -3) {
		printf("Set: PMSQL error in enc.\n");
		return -1;
	} else if ( enc_code == -1) {
		printf("Set: SQL error: %s\n", sqlite3_errmsg(pm_monolith->db) );
		return -1;
	} else if ( enc_code < 0 ) {
		printf("Set: Unknown error in enc.\n");
		return -1;
	}

	// Build sqlite3 statement to insert the cipher, validate key etc
	uint8_t salt[SALTSIZE];
	memcpy(salt, pm_monolith->master_key, SALTSIZE);
	uint8_t master_key[M_KEYSIZE];
	memset( master_key, 0, M_KEYSIZE);

	char validate = 0;
	if ( ! (pm_monolith->pm_opts & SKIPVAL) ) { // if not skip validation
		validate = 1;
		memcpy( master_key, pm_monolith->master_key + 9, M_KEYSIZE);
	}
	// bruh this turned out to be the same length
	char * rquery;
	void * data;
	int * data_sz;
	int * data_tp;

	char ukey = (pm_monolith->pm_opts & UKEY); // >> 2;
	if ( ukey ) {
		rquery = "INSERT INTO %s (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?,?)";
		data = (void *[6]) { name, salt, master_key, pm_monolith->ciphertext, 1, validate };
		data_sz = (int [6]) { 0, 0, M_KEYSIZE, CIPHERSIZE, 0, 0 };
		data_tp = (int [6]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_BLOB,
			PMSQL_INT, PMSQL_INT };
	} else {
		rquery = "INSERT INTO %s (ID,SALT,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?)";
		data = (void *[5]) { name, salt, pm_monolith->ciphertext, 1, validate };
		data_sz = (int [5]) { 0, 0, CIPHERSIZE, 0, 0 };
		data_tp = (int [5]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_INT, PMSQL_INT};
	}

	size_t query_len = strlen(rquery) + strlen(pm_monolith->table_name);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, pm_monolith->table_name);

	sqlite3_stmt * stmt;
	pmsql_stmt pmsql = { SQLITE_STATIC, pm_monolith->db, stmt, NULL };
	int ecode = pmsql_compile( &pmsql, query, 5+ukey, data, data_sz, data_tp );
	if ( ecode < 0 ) {
		printf("Set: pmsql error during binding (SQLITE : %i ) : %s\n", (-1 * ecode ) ,
			pmsql.pmsql_error );
		return -1;
	} else if ( ecode > 0 ) {
		printf("Set: sql error %i during binding: %s\n", ecode, sqlite3_errmsg(pm_monolith->db) );
		return -1;
	}

	int eval = sqlite3_step(pmsql.stmt); //Execute the statement
	if ( eval != SQLITE_DONE ) {
		printf("Set: SQL query evaluation error %i: %s\n", eval, sqlite3_errmsg(pm_monolith->db));
		return -1;
	}
	sqlite3_finalize(pmsql.stmt);

	set_cleanup:
		return 0; // below were causing seggy fault
		// if (ctxt)
		//	 memset(ctxt, 0, strlen(ctxt));
		// if (check)
		//	 memset(check, 0, strlen(check));
		// // context is always !NULL
		// memset(context, 0, DATASIZE);
}

int get(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	// Check for exists / in trash
	int name_len = strlen(name);
	int exists = _entry_in_table(pm_monolith, pm_monolith->table_name, name);
	if ( exists == 0 ) {
		printf("Get: No entry %s exists in %s. Perhaps look elsewhere? [pm help]"
		" for help on searching.\n", name, pm_monolith->table_name);
		return DNE;
	} else if ( exists == 1 ) {
		printf("Get: %s is in the trash. [pm rec %s] to recover so the entry may be retreived\n", 
				name, name);
		return DNE;
	}
	// These will be used for our calls to the pmsql functions
	char * rquery;
	void * * data;
	int * data_tp;
	int * data_sz;
	sqlite3_stmt * stmt;
	pmsql_stmt * pmsql;
	int ecode = 0;

	// query to index to get meta / val data for this entry
	rquery = "SELECT UKEY, SALT, MASTER_KEY FROM _index WHERE ID = ?";
	data = ( void * [1] ) {pm_monolith->table_name};
	data_tp = ( int [1] ) { PMSQL_TEXT };
	pmsql = &(pmsql_stmt) { SQLITE_STATIC, pm_monolith->db, stmt, NULL };

	if (( ecode = pmsql_compile( pmsql, rquery, 1, data, NULL, data_tp ) )) {
		goto get_sql_fail;
	}
	ecode = sqlite3_step(pmsql->stmt);
	if ( ecode != SQLITE_ROW ) {
		goto get_sql_fail;
	} // get results from query
	int ukey;
	char i_salt[SALTSIZE];
	char i_mkey[M_KEYSIZE];

	data = ( void * [3] ) { &ukey, &i_salt, &i_mkey };
	data_sz = ( int[3] ) { 0, (-1 * SALTSIZE), ( -1 * M_KEYSIZE ) };
	data_tp = ( int[3] ) { PMSQL_INT, PMSQL_BLOB, PMSQL_BLOB };

	if (( ecode = pmsql_read( pmsql, 3, data, data_sz, data_tp )) ) {
		goto get_sql_fail;
	}

	ukey &= 1;
	char entry_val;

	if ( ukey )
		rquery = "SELECT SALT, MASTER_KEY, CIPHER, VALIDATE FROM %s WHERE ID = ?";
	else
		rquery = "SELECT SALT, CIPHER, VALIDATE FROM %s WHERE ID = ?";

	data_tp = ( int[1] ) { PMSQL_TEXT };
	data = ( char * [1] ) { name };
	size_t query_len = strlen(rquery) + strlen(pm_monolith->table_name);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, pm_monolith->table_name);

	if (( ecode = pmsql_compile( pmsql, query, 1, data, NULL, data_tp ) ))
		goto get_sql_fail;

	ecode = sqlite3_step(pmsql->stmt);
	if ( ecode != SQLITE_ROW ) {
		goto get_sql_fail;
	}

	if ( ukey ) {
		data_tp = ( int[4] ) { PMSQL_BLOB, PMSQL_BLOB, PMSQL_BLOB, PMSQL_INT };
		data = ( void * [4] ) { pm_monolith->master_key, pm_monolith->master_key+SALTSIZE,
			pm_monolith->ciphertext, &entry_val };
		data_sz = ( int[4] ) {SALTSIZE, M_KEYSIZE, CIPHERSIZE, 0};
	} else {
		data = ( void * [3] ) { pm_monolith->master_key, pm_monolith->ciphertext, &entry_val };
		data_sz = ( int[3] ) { SALTSIZE, CIPHERSIZE, 0};
		data_tp = ( int[3] ) { PMSQL_BLOB, PMSQL_BLOB, PMSQL_INT };
	}
	if (( ecode = pmsql_read(pmsql, 3+ukey, data, data_sz, data_tp) )) {
		goto get_sql_fail;
	}

	char * pswd = kwarg_vals[KW_PASS];  

	if ( pswd == NULL ) { 
		char prompt[] = "Get: Enter the passphrase you used to encrypt your entry:\n";
		/*char * */ pswd = getpass(prompt);
		//kwarg_vals[KW_PASS] = pswd;    
	}

	int pswd_len = strlen(pswd);

	if ( pswd_len >= DATASIZE ){
		printf("Get: pm is configured to accept passphrases up to %i characters"\
			"long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
		goto get_cleanup;
	} else if ( pswd_len == 0 ) {
		printf("Get: no passphrases entered\n");
		goto get_cleanup;
	}

	//memcpy( pm_monolith->plaintext, pswd, DATASIZE);
	strncpy( pm_monolith->plaintext, pswd, DATASIZE);
	memset(pswd, 0, pswd_len);

	if ( entry_val && !ukey ) {
		// if comkey, check against masterkey from _index query, then turn off val
		// so dec doesnt try to val with wrong salt
		if ( strcmp(i_mkey, crypt(pm_monolith->plaintext, i_salt) ) ) {
			printf("Get: Wrong password for vault %s. Use -u to make an arbitrary key entry\n",
				pm_monolith->table_name);
			return PW_WRONG;
		}
		// turn off val
		pm_monolith->pm_opts |= SKIPVAL;
	} else {
		pm_monolith->pm_opts |= SKIPVAL; //Add flag at 2^1 bit (don't validate result)
		if ( pm_monolith->pm_opts & WARNNOVAL ) { // if warn
			printf("Warning: decryption validation disabled. PM will be unable to verify\
				the output for %s\nTo stop seeing this warning: [ pm setattr warn 0 ]\n", name );
		}
	}

	int res = dec_ciphertext(pm_monolith /*,kwarg_vals[KW_PASS]*/ );
	if ( res == -1 ) {
		printf("Invalid passphrase\n");
		return PW_WRONG;
	} else if ( res != 0 ) {
		return -1; //error in dec
	}

	printf("Retreived: %s:%s : %s\n", pm_monolith->table_name, name, pm_monolith->plaintext);
	get_cleanup:
		memset(pm_monolith, 0, sizeof(pm_inst));
		return 0;
	get_sql_fail:
		if ( ecode < 0 )
			printf("Get: pmsql error during binding : %s\n", pmsql->pmsql_error );
		else if ( ecode > 0 )
			printf("Get: sql error during binding: %s\n", sqlite3_errmsg(pm_monolith->db) );
		return -1;
}

int forget(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	char * table = (pm_monolith->pm_opts & OVAULT) ? "_index" : pm_monolith->table_name ;
	return _recover_or_forget(pm_monolith, table, name, 0);
}

int recover(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	char * table = (pm_monolith->pm_opts & OVAULT) ? "_index" : pm_monolith->table_name ;
	//printf("%i : %s\n", (pm_monolith->pm_opts & 8), table  );
	return _recover_or_forget(pm_monolith, table, name, 1);
}

int delete(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	int name_len = strlen(name);
	char vault = (pm_monolith->pm_opts & OVAULT);// >> 3;
	char * table = vault ? "_index" : pm_monolith->table_name ;

	if ( ! _entry_in_table(pm_monolith, table, name) ) {
		printf("Del: no entry to delete\n");
		return DNE; 
	}

	if ( ! ( pm_monolith->pm_opts & NOCONFIRM ) ) { 
		printf("This will permanently delete %s. This action cannot be undone."
			" Use forget [pm help forg] to mark as forgotten, which can be undone"\
			" (until an entry of the same name is set).\nProceed with permanent"\
			" deletion? (y/n)\n", name);
		if ( fgetc(stdin) != 'y' ) {
			printf("Del: aborting\n");
			return 0;
		}
	}

	if ( vault )
		return _delete_val(pm_monolith, name);
	else
		return _delete(pm_monolith, name);
}

int mkvault(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	// shouldn't happen, should be caught in cli_main.
	if ( !name || strlen(name) == 0 ) { // short circut ?
		printf("Mkvault: name required. [ pm mkvault NAME ]\n");
	}
	if ( !pmsql_safe_in(name) || strlen(name) > 15 ) {
		printf("Invalid vault name. Must contain only alphabetical characters and _."
			"Cannot start with _, and must be 15 characters or less\n");
			return ILLEGAL_IN;
	} // these need to be up here so error handling doesnt try to finalize nonexistant
	sqlite3_stmt * stmt;
	pmsql_stmt pmsql = { SQLITE_STATIC, pm_monolith->db, stmt, 0 };

	int exists = _entry_in_table(pm_monolith, "_index", name);
	if ( exists == 2 ) {
		printf("%s already exists. [ pm rmvault %s to remove ]\n", name, pm_monolith->table_name);
		return OVERWRITE;
	} else if ( exists == 1 ) {
		if ( _delete_val(pm_monolith, name) )
			goto mkv_sql_fail;
	} else if ( exists != 0  ) { // error in _e_i_v
		printf("mkvault: error in e_i_v\n" );
		return -1;
	}
	char ukey = (pm_monolith->pm_opts & UKEY);// >>2;
	char * bquery;
	if (ukey) {
		bquery = "CREATE TABLE %s (ID CHAR(32) PRIMARY KEY, SALT CHAR(9) NOT NULL, "\
			"MASTER_KEY BINARY(23), CIPHER BINARY(96) NOT NULL, VIS TINYINT, "\
			"VALIDATE TINYTINT)";
	} else {
		bquery = "CREATE TABLE %s (ID CHAR(32) PRIMARY KEY, SALT CHAR(9)"\
			"NOT NULL, CIPHER BINARY(96) NOT NULL, VIS TINYINT, VALIDATE TINYTINT)";
		ukey = 2; // first_commkey
	}

	size_t buffsz = strlen(bquery) + 32;
	char * query = malloc( buffsz );
	snprintf(query, buffsz, bquery, name);

	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, strlen(query), &stmt, NULL);
	free(query);
	if ( ecode != SQLITE_OK )
		goto mkv_sql_fail;

	ecode = sqlite3_step(stmt);
	if ( ecode != SQLITE_DONE )
		goto mkv_sql_fail;

	bquery = "INSERT INTO _index (ID, UKEY, VIS) VALUES (?,?,1)";
	void * * data = ( void * [2] ) { name, ukey };
	int * data_tp = ( int [2] ) { PMSQL_TEXT, PMSQL_INT };
	//----
	if (( ecode = pmsql_compile(&pmsql, bquery, 2, data, NULL, data_tp) ))
		goto mkv_sql_fail;
	//---
	if ( sqlite3_step(pmsql.stmt) != SQLITE_DONE )
		goto mkv_sql_fail;

	sqlite3_finalize(pmsql.stmt);
	return 0;

	mkv_sql_fail:
		printf("Mkvault: SQL Error: %s\n", sqlite3_errmsg(pm_monolith->db) );
		sqlite3_finalize(pmsql.stmt);
		return -1; 
}

int ls(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	return _ls_find(pm_monolith, name);// >> 3 ) ;
}

int check(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	printf("Check does nothing right now\n");
	return 0;
	// char * bquery = "SELECT UKEY from _index WHERE ID = ?";
}

// only takes these args for ease of calling. ignores them (nesecary?) 
int help(pm_inst * pm_monolith, char * name, char * * kwarg_vals) {
	printf("In production, this is a help page.\n");
	return 0; 
}

int conf(pm_inst * pm_monolith, char * name, char * * kwarg_vals){
	printf("%s\n", pm_monolith->conf_path);
	return 0; 
}

//void chpass( pm_inst * pm_monolith, char * name ) { 
//
//}

int _ls_find(pm_inst * pm_monolith, char * name) {
	// printf("ls_find(%s, %i)\n", name, vault );
	char vault = pm_monolith->pm_opts & OVAULT;
	char * table = pm_monolith->table_name;
	if ( vault && ! _entry_in_table(pm_monolith, "_index", pm_monolith->table_name) ) {
		if ( strcmp(table,"_index")) {
			name = pm_monolith->table_name;
			table = "_index";
		}
		//
	}
	// printf("name: %s. table: %s\n", name, table );

	if ( name )
		goto _ls_swithin;

	goto _ls_printall;

	_ls_swithin:
	if ( _entry_in_table( pm_monolith, table, name ) ) {
		printf("%s:%s found.\n", table, name );
	}

	char * * sims = _find_by_key(pm_monolith, table, name, 8);

	if (!sims ) {
		printf("%s:%s not found. No similer entries were found either.\n",
			table, name);
		return DNE;
	}
	printf("%s:%s not found. See these similar entries:\n", table, name);
	int i = 1;
	while(sims[i]) {
		printf("%i : %s\n",i, sims[i] );
		++i;
	}
	free(sims);
	return 0;

	_ls_printall:;
	char * * all = _all_in_table(pm_monolith, table );
	if (!all ) {
		return 0;
	}
	char * * ct = all+1;
	while (*ct++) {};
	int rows = ct - ( all + 2 );
	// printf("Counted %i rows\n", rows );
	// printf("%s\n",*((all+1) + rows) );
	//--
	qsort(all+1, rows, sizeof(char *), strcmp_for_qsort );
	//--
	i = 1;
	while ( all[i] ) {
		if ( all[0][i] ) {
			if ( vault && name )
				printf("\t");
			printf("%s\n", all[i]);
		}
		++i;
	}
	return 0; 
}

int _entry_in_table(pm_inst * pm_monolith, char * tb_name, char * ent_name) {
	char * rquery = "SELECT VIS FROM %s WHERE ID = ?";
	size_t query_len = strlen(rquery) + strlen(tb_name);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, tb_name);
	sqlite3_stmt * statement;

	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &statement, NULL);
	free(query);
	if ( ecode != SQLITE_OK )
		goto eit_sql_fail;

	ecode = sqlite3_bind_text(statement, 1, ent_name, strlen(ent_name), SQLITE_STATIC);
	if ( ecode != SQLITE_OK )
		goto eit_sql_fail;

	ecode = sqlite3_step(statement);
	if ( ecode == SQLITE_DONE )
		return 0;

	else if ( ecode != SQLITE_ROW )
		goto eit_sql_fail;

	ecode = sqlite3_column_int(statement, 0);
	sqlite3_finalize(statement);

	return ecode + 1;

	eit_sql_fail:
		printf("Entry in Vault: SQL Error: %s\n", sqlite3_errmsg(pm_monolith->db) );
		sqlite3_finalize(statement);
		return -1;
}

int _recover_or_forget(pm_inst * pm_monolith, char * table, char * name, int op ){
	char * opstrings[] = { "Forget", "Recover" };
	if (name == NULL){
		printf("%s: a name is required.\n", opstrings[op]);
		return -1;
	}
	int name_len = strlen(name);
	int exists = _entry_in_table(pm_monolith, table, name);
	if ( exists == -1 )
		return -1;
	else if ( exists == 0 ) {
		printf("No entry %s:%s to %s.\n", table, name, opstrings[op]);
		return -1;
	} else if ( op + 1 == exists ) {
		printf("%s:%s %s operaton already complete\n", table, name, opstrings[op]);
	}

	char * rquery = "UPDATE %s SET VIS = ? WHERE ID = ?";
	size_t query_len = strlen(rquery) + strlen(table);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, table);

	sqlite3_stmt * statement;
	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &statement, NULL);
	free(query);
	if ( ecode != SQLITE_OK )
		goto roc_sql_fail;

	ecode = sqlite3_bind_int(statement, 1, op);
	if ( ecode != SQLITE_OK )
		goto roc_sql_fail;

	ecode = sqlite3_bind_text(statement, 2, name, name_len, SQLITE_STATIC);
	if ( ecode != SQLITE_OK )
		goto roc_sql_fail;

	ecode = sqlite3_step(statement);
	if ( ecode != SQLITE_DONE )
		goto roc_sql_fail;

	sqlite3_finalize(statement);
	return 0;
	roc_sql_fail:
		printf("%s: SQL Error: %s\n", opstrings[op], sqlite3_errmsg(pm_monolith->db) );
		sqlite3_finalize(statement);
		return -1;
}

int _delete(pm_inst * pm_monolith, char * name) {
	char * rquery = "DELETE FROM %s WHERE ID = ?";
	size_t query_len = strlen(rquery) + strlen(pm_monolith->table_name);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, pm_monolith->table_name);

	sqlite3_stmt * statement;
	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &statement, NULL);
	free(query);
	if ( ecode != SQLITE_OK ) {
		printf("Del: Error compiling SQL query: %s\n", sqlite3_errmsg(pm_monolith->db) );
		return -1;
	}

	ecode = sqlite3_bind_text(statement, 1, name, strlen(name), SQLITE_STATIC);
	if ( ecode != SQLITE_OK ) {
		printf("Del: Error binding SQL query: %s\n", sqlite3_errmsg(pm_monolith->db) );
		return -1;
	}

	ecode = sqlite3_step(statement);
	sqlite3_finalize(statement);
	if ( ecode == SQLITE_DONE ) {
		return 0;
	}

	printf("Del: a backend error prevented the deletion of %s : %s\n", name,
		sqlite3_errmsg(pm_monolith->db) );
	return -1;
}

int _delete_val( pm_inst * pm_monolith, char * name ) {
	char * rquery = "DROP TABLE %s";
	size_t query_len = strlen(rquery) + strlen(name);
	char * query = malloc(query_len+1);
	snprintf(query, query_len, rquery, name);

	sqlite3_stmt * stmt;
	pmsql_stmt pmsql = { SQLITE_STATIC, pm_monolith->db, stmt, 0 };

	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &stmt, NULL);
	free(query);
	if ( ecode )
		goto del_val_sql_fail;

	ecode = sqlite3_step(stmt);
	if ( ecode != SQLITE_DONE )
		goto del_val_sql_fail;

	query = "DELETE FROM _index WHERE ID = ?";

	void * * data = (void * [1] ) { name };
	int * data_tp = (int [1] ){ PMSQL_TEXT };
	if (( ecode = pmsql_compile( &pmsql, query, 1, data, NULL, data_tp ) ))
		goto del_val_sql_fail;

	if (sqlite3_step(pmsql.stmt) != SQLITE_DONE)
		goto del_val_sql_fail;

	sqlite3_finalize(pmsql.stmt);
	return 0;

	del_val_sql_fail:
		printf("Backend error prevented vault deletion: SQL Error: %s\n", sqlite3_errmsg(pm_monolith->db) );
		sqlite3_finalize(pmsql.stmt);
		return -1;
}

char * FBK_KEY; // No way to get around glob unfortuantely
char * * _find_by_key( pm_inst * pm_monolith, char * tb, char * key, int numres ) {
	char * * all = _all_in_table( pm_monolith, tb );

	if ( !all )
		return NULL;

	int c = 1;
	while ( all[c] ) { // pick out vis=0 and nonsimilar ents
		// printf("%s\n",all[c] );
		if ( !all[0][c] || !o_search(all[c], key) ) {
			all[c] = all[c+1];
			int j = c+1;
			if ( ! all[j] )
				continue;
			while(( all[j] = all[j+1] )) { j++; };
		} else
			c++;
	}

	c-=1;
	if ( !c )
		return NULL;
	FBK_KEY = key;
	qsort( &all[1], c, sizeof(char *), _fbk_compare );
	return all;
}

int _fbk_compare( const void * a, const void * b) {
	char * * sa = (char * *) a;
	char * * sb = (char * *) b;
	//printf("%s ? %s\n", *sa, *sb );
	uint32_t sa_sc = o_search(*sa, FBK_KEY);
	uint32_t sb_sc = o_search(*sb, FBK_KEY);

	if ( sa_sc > sb_sc ) // try return sa_sc - sb_sc
		return 1;
	else if ( sb_sc > sa_sc )
		return -1;
	else
		return 0;
}

char * * _all_in_table( pm_inst * pm_monolith, char * tb_name ) {
	// printf("all_in_table(%s)\n",tb_name );
	char * bquery = "SELECT COUNT(*) FROM %s";
	size_t query_len = strlen(bquery) + strlen(tb_name);
	char * query = malloc(query_len + 1);
	snprintf(query, query_len, bquery, tb_name);

	sqlite3_stmt * stmt;
	int ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &stmt, NULL);
	free(query);

	if ( ecode != SQLITE_OK )
		goto ait_sql_fail;
	ecode = sqlite3_step(stmt);

	if ( ecode != SQLITE_ROW )
		goto ait_sql_fail;

	int rows = sqlite3_column_int(stmt, 0);
	if ( !rows)
		return NULL;
	// if (( ecode = sqlite3_finalize(stmt) ));
	//	 goto ait_sql_fail;
	sqlite3_finalize(stmt);

	char * * ids = malloc(( rows+2 ) * sizeof(char *) );
	char * vis = malloc(rows);
	ids[0] = vis;

	bquery = (char *) "SELECT ID, VIS from %s";
	query_len = strlen(bquery) + strlen(tb_name);
	query = malloc(query_len + 1);
	snprintf(query, query_len, bquery, tb_name);

	if (( ecode = sqlite3_prepare_v2(pm_monolith->db, query, query_len, &stmt, NULL) ))
		goto ait_sql_fail;

	ecode = sqlite3_step(stmt);
	int i = 1;
	while ( ecode == SQLITE_ROW && i <= rows ) {
		char * this = malloc( sqlite3_column_bytes(stmt, 0) + 1 );
		strcpy(this, sqlite3_column_text(stmt, 0) );
		ids[i] = this;
		vis[i] = sqlite3_column_int(stmt, 1) ;
		i++;
		ecode = sqlite3_step(stmt);
	}
	// printf("Found %i rows\n", i-1 );
	ids[i] = NULL;
	if ( ecode != SQLITE_DONE )
		goto ait_sql_fail;
	return ids;

	ait_sql_fail:
		printf("all_in_table SQL error: %i : %s\n", ecode, sqlite3_errmsg(pm_monolith->db)  );
		return NULL;
}

int _apply_kwargs( int argc, char * * argv, char * * kw_vals, pm_inst * pm_monolith) {  
	int kw_len, kw_toapply, kw_val_len; 
	for ( int i = 0; i < argc-1; ++i ) { 
		if ( argv[i] == NULL ) 
			continue; 

		if ( ( kw_len = strlen( argv[i] ) ) < 2 ) 
			continue; 

		if ( argv[i][0] != '-' ) 
			continue; 

		for ( int j = 0; j < KW_NUM; ++j ) {  
			if ( strcmp( keywords[j], argv[i]+1) == 0 ) {   
				kw_val_len = strlen(argv[i+1]); 
				kw_vals[j] = malloc( kw_val_len+1 );
				strlcpy(kw_vals[j], argv[i+1], kw_val_len+1 );  
				argv[i] = NULL; 
				argv[i+1] = NULL; 
				goto keyword_matched; 
			}
		}
		printf("Unknown keyword: %s\n", argv[i] ); 
		return -1; 

		keyword_matched:; 
		i++; 
	}
	return 0; 
}

int _apply_flags ( int argc, char * * argv, pm_inst * pm_monolith) { 
	int arg_len, flag_to_apply;  
	char ** pm_flags = get_pm_flags(); 
	int * flag_bit = get_flag_bits(); 

	for ( int i = 0; i < argc; i++ ) { 	
		if ( argv[i] == NULL ) 
			continue; 

		if ( (arg_len = strlen(argv[i] ) < 2 ) ) 
			continue; 

		if ( argv[i][0] != '-' || argv[i][1] != '-' )  
			continue; 

		if ( arg_len == 3 ) { 
			for ( int j = 0; j < NUM_FLAGS; ++j ) { 	
				if ( argv[i][2] == pm_flags[j][0] ) { 
					flag_to_apply = flag_bit[j]; 
					goto apply_flag; 
				}
			}
			goto unknown_flag; 
		}

		for ( int j = 0; j < NUM_FLAGS; ++j ) { 	
			if ( strcmp( argv[i]+2, pm_flags[j]) == 0 ) { 
				flag_to_apply = flag_bit[j]; 
				goto apply_flag; 
			}
		}

		unknown_flag:; 
		printf("Unknwon flag: %s\n", argv[i] ); 
		return SYNTAX; 

		apply_flag:; 
		pm_monolith->pm_opts |= flag_to_apply; 
		argv[i] = NULL; 
	}

	return 0; 
}
