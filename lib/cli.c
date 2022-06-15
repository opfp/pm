# include "cli.h"

int cli_main( int argc, char * * argv, pm_inst * PM_INST) {
    if ( argc < 1 ) {
        printf("pm: Not enough arguments. [pm help] for help, or read the docs.\n");
        //help(void, void);
        return -1;
    }
    char def_commkey[] = "_main_cmk";
    char def_arbkey[]  = "_main_ukey";

    if ( PM_INST->pm_opts & 32 ) { // if defaults only
        if ( PM_INST->pm_opts & 4 ) // if -u
            memcpy(PM_INST->table_name, def_arbkey, 10);
        else
            memcpy (PM_INST->table_name, def_commkey, 9 );
        if ( argc == 3 ) {
            printf("pm is configured to only use default vaults, but vault specified."\
                "[ pm chattr def_vaults 0 ] to enable user created vaults.\n");
        }
    } else if ( argc == 3 ) {
        if ( strlen(argv[1]) > 15 || !pmsql_safe_in(argv[1]) ) {
            printf("Disallowed vault name. Must be less than 16 characters, and"\
                " contain only letters, numbers and _. Cannot begin with _.\n");
            return -1;
        }
        if ( !_entry_in_table(PM_INST, "_index", argv[1]) ) {
            printf("No vault named %s. [pm mkvault] to make a vault.\n", argv[1]);
            return -1;
        }
        strncpy(PM_INST->table_name, argv[1], 15);
    } else if ( PM_INST->pm_opts & 4 ) {
        memcpy(PM_INST->table_name, def_arbkey, 10);
    } else {
        memcpy(PM_INST->table_name, def_commkey, 9 );
    }

    //Code for testing
    // char * option_names[] = { "Echo", "Skip pswd hash val", "Use ukey vault",
    //     "Confirm cipertexts" , "Warn on no validation" , "Use only default vaults"};
    // unsigned char opts = PM_INST->pm_opts;
    // for ( int i = 0; i < 6; i++) {
    //     printf("%i : %s\n", ( opts & 1 ), option_names[i]);
    //     opts = opts >> 1;
    // }
    //---

    char * verbs[] = { "set", "get", "forg", "rec", "del", "loc", "mkvault" ,
        "rmvault", "help" };
    char need_name[] = { 1, 1, 1, 1, 1, 1, 1, 1, 0 };
    int verbs_len = 9;
    void (*functions[])(pm_inst *, char *) = { set, get, forget, recover, delete,
        locate, mkvault, rmvault, help };

    char * verb = argv[0];
    int i = 0;
    for(/*i*/; i < verbs_len; i++ ) {
        if ( strcmp(verb, verbs[i]) == 0 ) {
            if ( need_name[i] ) {
                if ( argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0 ) {
                    printf("%s: name required. [pm help]\n", verbs[i]);
                    return -1;
                } else if ( strlen(argv[1]) > 31 ) {
                    printf("pm: name must be less than 32 characters long.\n");
                    return -1;
                }
            }
            (*functions[i])(PM_INST, argv[1+(argc==3)]);
            return 0;
        }
    }
    if ( i == verbs_len )
        help(NULL, NULL); // verb unmatched
    return 0; // currently unused ret for cli_main
}

void set(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    char exists = _entry_in_table(PM_INST, PM_INST->table_name, name);
    if ( exists == 2 ) {
        printf("Set: an entry with name %s already exists in %s.\n", name, PM_INST->table_name );
        return;
    } else if ( exists == 1 ) { //Entry exists but is in trash, will be overwritten.
        if ( _delete(PM_INST, name, name_len) )
            printf("Set: backend error in _delete: %s\n", sqlite3_errmsg(PM_INST->db) );
    }

    char * prompt = "Enter the ciphertext for the entry %:\n";
    prompt = concat(prompt, 1, name);
    char * ctxt = getpass(prompt);
    free(prompt);

    if ( ctxt == NULL ){
        printf("Set: no ciphertext entered\n");
        goto set_cleanup;
    }
    int ctxt_len =  strlen(ctxt);
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
    if ( PM_INST->pm_opts & 8 ) { // if confirm
        check = getpass("Confirm your entry:\n");
        // if ( check == NULL )
        //     check = context; // Not a bug - user can bypass by entering nothing
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
    memcpy(PM_INST->plaintext, context, DATASIZE);

    memset(ctxt, 0, ctxt_len);
    ctxt = NULL;
    memset(context, 0, DATASIZE);

    int enc_code = enc_plaintext(PM_INST);

    if ( enc_code == -2 ) {
        printf("Set: Wrong password for vault %s. Use -u to make an arbitrary key entry\n",
            PM_INST->table_name);
        return; // We're free to just return because we've already destroyed our sensitive data (and so had enc)
    } else if ( enc_code == -4) {
        printf("Set: Commkey / ukey mismatch. Toggle -u to fix\n");
        return;
    } else if ( enc_code == -3) {
        printf("Set: PMSQL error in enc.\n");
        return;
    } else if ( enc_code == -1) {
        printf("Set: SQL error: %s\n", sqlite3_errmsg(PM_INST->db) );
        return;
    } else if ( enc_code < 0 ) {
        printf("Set: Unknown error in enc.\n");
        return;
    }

    // Build sqlite3 statement to insert the cipher, validate key etc
    uint8_t salt[SALTSIZE];
    memcpy(salt, PM_INST->master_key, SALTSIZE);
    uint8_t master_key[M_KEYSIZE];
    memset( master_key, 0, M_KEYSIZE);

    char validate = 0;
    if ( ! (PM_INST->pm_opts & 2) ) { // if not skip validation
        validate = 1;
        memcpy( master_key, PM_INST->master_key + 9, M_KEYSIZE);
    }
    // bruh this turned out to be the same length
    char * query;
    void * data;
    int * data_sz;
    int * data_tp;

    char ukey = (PM_INST->pm_opts & 4) >> 2;
    if ( ukey ) {
        query = "INSERT INTO % (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?,?)";
        data = (void *[6]) { name, salt, master_key, PM_INST->ciphertext, 1, validate };
        data_sz = (int [6]) { 0, 0, M_KEYSIZE, CIPHERSIZE, 0, 0 };
        data_tp = (int [6]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_BLOB,
            PMSQL_INT, PMSQL_INT };
    } else {
        query = "INSERT INTO % (ID,SALT,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?)";
        data = (void *[5]) { name, salt, PM_INST->ciphertext, 1, validate };
        data_sz = (int [5]) { 0, 0, CIPHERSIZE, 0, 0 };
        data_tp = (int [5]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_INT, PMSQL_INT};
    }

    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * stmt;
    pmsql_stmt pmsql = { SQLITE_STATIC, PM_INST->db, stmt, NULL };
    int ecode = pmsql_compile( &pmsql, query, 5+ukey, data, data_sz, data_tp );
    if ( ecode < 0 ) {
        printf("Set: pmsql error during binding (SQLITE : %i ) : %s\n", (-1 * ecode ) ,
            pmsql.pmsql_error );
        return;
    } else if ( ecode > 0 ) {
        printf("Set: sql error %i during binding: %s\n", ecode, sqlite3_errmsg(PM_INST->db) );
        return;
    }

    int eval = sqlite3_step(pmsql.stmt); //Execute the statement
    if ( eval != SQLITE_DONE ) {
        printf("Set: SQL query evaluation error %i: %s\n", eval, sqlite3_errmsg(PM_INST->db));
        return;
    }
    sqlite3_finalize(pmsql.stmt);

    set_cleanup:
        return; // below were causing seggy fault
        // if (ctxt)
        //     memset(ctxt, 0, strlen(ctxt));
        // if (check)
        //     memset(check, 0, strlen(check));
        // // context is always !NULL
        // memset(context, 0, DATASIZE);
}

void get(pm_inst * PM_INST, char * name) {
    // Check for exists / in trash
    int name_len = strlen(name);
    int exists = _entry_in_table(PM_INST, PM_INST->table_name, name);
    if ( exists == 0 ) {
        printf("Get: No entry %s exists in %s. Perhaps look elsewhere? [pm ls-vaults] to list vaults\n",
            name, PM_INST->table_name);
        return;
    } else if ( exists == 1 ) {
        printf("Get: %s is in the trash. [pm rec %s] to recover so the entry may be retreived\n", name, name);
        return;
    }
    // These will be used for our calls to the pmsql functions
    char * query;
    void * * data;
    int * data_tp;
    int * data_sz;
    sqlite3_stmt * stmt;
    pmsql_stmt * pmsql;
    int ecode = 0;

    // query to index to get meta / val data for this entry
    query = "SELECT UKEY, SALT, MASTER_KEY FROM _index WHERE ID = ?";
    data = ( void * [1] ) {PM_INST->table_name};
    data_tp = ( int [1] ) { PMSQL_TEXT };
    pmsql = &(pmsql_stmt) { SQLITE_STATIC, PM_INST->db, stmt, NULL };

    if (( ecode = pmsql_compile( pmsql, query, 1, data, NULL, data_tp ) )) {
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
        query = "SELECT SALT, MASTER_KEY, CIPHER, VALIDATE FROM % WHERE ID = ?";
    else
        query = "SELECT SALT, CIPHER, VALIDATE FROM % WHERE ID = ?";

    data_tp = ( int[1] ) { PMSQL_TEXT };
    data = ( char * [1] ) { name };
    // removing concat dependency later
    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);
    //---
    if (( ecode = pmsql_compile( pmsql, query, 1, data, NULL, data_tp ) )) {
        goto get_sql_fail;
    }
    //---
    ecode = sqlite3_step(pmsql->stmt);
    if ( ecode != SQLITE_ROW ) {
        goto get_sql_fail;
    }

    if ( ukey ) {
        data_tp = ( int[4] ) { PMSQL_BLOB, PMSQL_BLOB, PMSQL_BLOB, PMSQL_INT };
        data = ( void * [4] ) { PM_INST->master_key, PM_INST->master_key+SALTSIZE,
            PM_INST->ciphertext, &entry_val };
        data_sz = ( int[4] ) {SALTSIZE, M_KEYSIZE, CIPHERSIZE, 0};
    } else {
        data = ( void * [3] ) { PM_INST->master_key, PM_INST->ciphertext, &entry_val };
        data_sz = ( int[3] ) { SALTSIZE, CIPHERSIZE, 0};
        data_tp = ( int[3] ) { PMSQL_BLOB, PMSQL_BLOB, PMSQL_INT };
    }
    if (( ecode = pmsql_read(pmsql, 3+ukey, data, data_sz, data_tp) )) {
        goto get_sql_fail;
    }

    char prompt[] = "Get: Enter the passphrase you used to encrypt your entry:\n";
    char * pswd = getpass(prompt);
    int pswd_len = strlen(pswd);

    if ( pswd_len >= DATASIZE ){
        printf("Get: pm is configured to accept passphrases up to %i characters"\
            "long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
        goto get_cleanup;
    } else if ( pswd_len == 0 ) {
        printf("Get: no passphrases entered\n");
        goto get_cleanup;
    }

    memcpy( PM_INST->plaintext, pswd, DATASIZE);
    memset(pswd, 0, pswd_len);

    if ( entry_val && !ukey ) {
        // if comkey, check against masterkey from _index query, then turn off val
        // so dec doesnt try to val with wrong salt
        if ( strcmp(i_mkey, crypt(PM_INST->plaintext, i_salt) ) ) {
            printf("Get: Wrong password for vault %s. Use -u to make an arbitrary key entry\n",
                PM_INST->table_name);
            return;
        }
        // turn off val
        PM_INST->pm_opts |= 2;
    } else {
        PM_INST->pm_opts |= 2; //Add flag at 2^1 bit (don't validate result)
        if ( PM_INST->pm_opts & 16 ) { // if warn
            printf("Warning: decryption validation disabled. PM will be unable to verify\
                the output for %s\nTo stop seeing this warning: [ pm setattr warn 0 ]\n", name );
        }
    }

    int res = dec_ciphertext(PM_INST);
    if ( res == -1 ) {
        printf("Invalid passphrase\n");
        return;
    } else if ( res != 0 ) {
        return; //error in dec
    }

    printf("Retreived: %s:%s : %s\n", PM_INST->table_name, name, PM_INST->plaintext);
    get_cleanup:
        memset(PM_INST, 0, sizeof(pm_inst));
        return;
    get_sql_fail:
        if ( ecode < 0 )
            printf("Get: pmsql error during binding : %s\n", pmsql->pmsql_error );
        else if ( ecode > 0 )
            printf("Get: sql error during binding: %s\n", sqlite3_errmsg(PM_INST->db) );
        return;
}

void forget(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 0);
}

void recover(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 1);
}

void delete(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    if ( ! _entry_in_table(PM_INST, PM_INST->table_name, name) ) {
        printf("Del: no entry to delete\n");
    }
    printf("This will permanently delete %s. This action cannot be undone."
        " Use forget [pm forg %s] to mark as forgotten, which can be undone"\
        " (until an entry of the same name is set in vault).\nProceed with permanent"\
        " deletion? (y/n)\n", name, name);
    if ( fgetc(stdin) != 'y' ) {
        printf("Del: aborting\n");
        return;
    }
    _delete(PM_INST, name, name_len);
}

void locate(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    int found = _entry_in_table( PM_INST, PM_INST->table_name, name );
    if ( found == 2 ) {
        printf("%s:%s found.\n", PM_INST->table_name, name);
        return;
    } else if ( found == 1 ) {
        printf("%s is in the trash. [ pm rec %s ] to recover.\n", name, name);
        return;
    } else if ( found ) {
        printf("Locate: error %i in _e_i_v: %s\n", found, sqlite3_errmsg(PM_INST->db) );
    }
    /* else if ( found == 0 ) {
        printf("%s not found in %s. Perhaps look elsewhere? [ pm ls-vaults ] to list valults.\n",
            name, PM_INST->table_name);
        return;
    } */
    char * * sims = _find_by_key(PM_INST, PM_INST->table_name, name, 8);
    printf("%s:%s not found. See these similar entries:\n", PM_INST->table_name, name);
    for( int i = 0; i < 8; i++ ) {
        if (!sims[i])
            break;
        printf("%i : %s\n",i+1, sims[i] );
    }
    free(sims);
}

void mkvault(pm_inst * PM_INST, char * name) {
    if ( !name || strlen(name) == 0 ) { // short circut ?
        printf("Mkvault: name required. [ pm mkvault NAME ]\n");
    }
    if ( !pmsql_safe_in(name) || strlen(name) > 15 ) {
        printf("Invalid vault name. Must contain only alphabetical characters and _."
            "Cannot start with _, and must be 15 characters or less\n");
    } // these need to be up here so error handling doesnt try to finalize nonexistant
    sqlite3_stmt * stmt;
    pmsql_stmt pmsql = { SQLITE_STATIC, PM_INST->db, stmt, 0 };

    int exists = _entry_in_table(PM_INST, "_index", name);
    if ( exists == 2 ) {
        printf("%s already exists. [ pm rmvault %s to remove ]\n", name, PM_INST->table_name);
        return;
    } else if ( exists == 1 ) {
        if ( _delete_val(PM_INST, name) )
            goto mkv_sql_fail;
    } else if ( exists != 0  ) { // error in _e_i_v
        return;
    }
    char ukey = (PM_INST->pm_opts & 4) >>2;
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

    int ecode = sqlite3_prepare_v2(PM_INST->db, query, strlen(query), &stmt, NULL);
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
    return;

    mkv_sql_fail:
        printf("Mkvault: SQL Error: %s\n", sqlite3_errmsg(PM_INST->db) );
        sqlite3_finalize(pmsql.stmt);
}

void rmvault(pm_inst * PM_INST, char * name) {
    int vis;
    if (!( vis = _entry_in_table(PM_INST, "_index", name) )) {
        printf("Rmvault: No vault %s exists.\n", name);
        return;
    } else if ( vis == 1 ) {
        printf("Rmvault: Vault %s already deleted.\n", name);
        return;
    } // this is kind of hackey but I am so done with this software
    strncpy(PM_INST->table_name, "_index", 7);
    if (( _recover_or_forget(PM_INST, name, 0 ) )) {
        printf("A backend error prevented vault deletion. SQL : %s",
            sqlite3_errmsg(PM_INST->db) );
    }
}

// only takes these args for ease of calling. ignores them
void help(pm_inst * PM_INST, char * name) {
    printf("In production, this is a help page.\n");
}

int _entry_in_table(pm_inst * PM_INST, char * tb_name, char * ent_name) {
    char * query = "SELECT VIS FROM % WHERE ID = ?";
    query = concat(query, 1, tb_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
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
        printf("Entry in Table: SQL Error: %s\n", sqlite3_errmsg(PM_INST->db) );
        sqlite3_finalize(statement);
        return -1;
}

int _recover_or_forget(pm_inst * PM_INST, char * name, int op ){
    char * opstrings[] = { "Forget", "Recover" };
    if (name == NULL){
        printf("%s: a name is required.\n", opstrings[op]);
        return -1;
    }
    int name_len = strlen(name);
    int exists = _entry_in_table(PM_INST, PM_INST->table_name, name);
    if ( exists == 0 ) {
        printf("No entry %s:%s to %s.\n", PM_INST->table_name, name, opstrings[op]);
        return -1;
    } else if ( op + 1 == exists ) {
        printf("%s:%s %s operaton already complete\n", PM_INST->table_name, name, opstrings[op]);
    }

    char * query = "UPDATE % SET VIS = ? WHERE ID = ?";
    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
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
        printf("%s: SQL Error: %s\n", opstrings[op], sqlite3_errmsg(PM_INST->db) );
        sqlite3_finalize(statement);
        return -1;
}

int _delete(pm_inst * PM_INST, char * name, int name_len) {
    char * query = "DELETE FROM % WHERE ID = ?";
    query = concat( query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK ) {
        printf("Del: Error compiling SQL query: %s\n", sqlite3_errmsg(PM_INST->db) );
        return -1;
    }

    ecode = sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC);
    if ( ecode != SQLITE_OK ) {
        printf("Del: Error binding SQL query: %s\n", sqlite3_errmsg(PM_INST->db) );
        return -1;
    }

    ecode = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if ( ecode == SQLITE_DONE ) {
        return 0;
    }

    printf("Del: a backend error prevented the deletion of %s : %s\n", name,
        sqlite3_errmsg(PM_INST->db) );
    return -1;
}

int _delete_val( pm_inst * PM_INST, char * name ) {
    char * bquery = "DROP TABLE %s";
    size_t buffsize = strlen(bquery) + 16;
    char * query = malloc(buffsize);
    snprintf(query, buffsize, bquery, name);

    sqlite3_stmt * stmt;
    pmsql_stmt pmsql = { SQLITE_STATIC, PM_INST->db, stmt, 0 };

    int ecode = sqlite3_prepare_v2(PM_INST->db, query, strlen(query), &stmt, NULL);
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
        printf("Backend error prevented vault deletion: SQL Error: %s\n", sqlite3_errmsg(PM_INST->db) );
        sqlite3_finalize(pmsql.stmt);
        return -1;
}

char * * _find_by_key( pm_inst * PM_INST, char * tb, char * key, int numres ) {
    char * bquery = "SELECT ID FROM %s WHERE VIS = 1";
    size_t buffsz = strlen(bquery) + strlen(tb);
    char * query = malloc( buffsz );
    snprintf(query, buffsz, bquery, tb );

    sqlite3_stmt * stmt;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, strlen(query), &stmt, NULL);
    free(query);
    if ( ecode )
        goto find_sql_fail;

    char * * results = malloc( 65 * sizeof(char *) );
    void * * rescodes = malloc( 65 * sizeof(void *) );
    char * * s_results = results;
    void * * s_rescodes = rescodes;

    int i = 0;
    ecode = sqlite3_step(stmt);
    while ( ecode == SQLITE_ROW ) {
        if ( i && (i % 64 == 0) ) {
            results[64] = malloc( 65 * sizeof(char *) );
            rescodes[64] = malloc( 65 * sizeof(void *) );
            results = (char * *) results[64];
            rescodes = (void * *) rescodes[64];
            i++;
            continue;
        }
        size_t sz = sqlite3_column_bytes(stmt, 0);
        char * this = malloc(sz+1);
        memcpy( this, sqlite3_column_text(stmt, 0), sz);
        this[sz] = 0;

        uint32_t res = o_search(this,key);
        rescodes[i%64] = (void *) res;
        results[i%64] = this;
        //printf("%s : %i\n", results[i%64], rescodes[i%64] );
        i++;
        ecode = sqlite3_step(stmt);
    }
    if ( ecode != SQLITE_DONE )
        goto find_sql_fail;
    sqlite3_finalize(stmt);
    //now, a sort to find numents of the top matches
    if ( (i - i/64) < numres )
        numres = i - i/64;
    else if ( (i - i/64) == numres ) {
        //handle base case
    }
    //printf("numres: %i\n", numres );
    char * * tops = malloc( ( numres + 1) * sizeof(char * ) );
    memset(tops, 0, ( numres + 1) * sizeof(char *) );
    int * top_vals = malloc(numres * sizeof(int ) );
    memset(top_vals, 0, numres * sizeof(int) );
    // crawl along the hybrid linked list and sort insert and free
    for (int j = 0; j < i; j++ ) { /* this is like 3 lines in python :\ */
        if ( j && (j % 64 == 0) ) {
            char * * oldc = s_results;
            void * * oldi = s_rescodes;
            s_results = s_results[64];
            s_rescodes = s_rescodes[64];
            free(oldc);
            free(oldi);
            oldc = NULL;
            oldi = NULL;
            continue;
        }
        //printf("%s : %i\n", s_results[j%64], s_rescodes[j%64] );
        if ( s_rescodes[j%64] <= top_vals[numres-1] ) {
            //printf("skipping %s\n", s_results[j%64] );
            continue;
        } // find index of our new entry into top
        int ci = numres - 1;
        while ( (s_rescodes[j%64] > top_vals[ci]) && ( ci )  ) {
            ci--;
            //printf("%i\n", ci );
        } // now insert at rightful index
        for ( int k = numres - 2; k >= ci; k-- ) {
            if ( !tops[k] )
                continue;
            //printf("Moving %s down\n", tops[k] );
            tops[k+1] = tops[k];
            top_vals[k+1] = tops[k];
        }
        //tops[ci] = s_results[j%64];
        tops[ci] = malloc( strlen(s_results[j%64]) ) ;
        strcpy(tops[ci], s_results[j%64]);
        top_vals[ci] = s_rescodes[j%64];
        for ( int i = 0; i++; i < numres ) {
            if ( !tops[i] )
                break;
            //printf("%i : %s\n",i, tops[i] );
        }
    }
    if ( s_results ) {
        free(s_results);
        free(s_rescodes);
    }
    free(top_vals);
    return tops;
    find_sql_fail:
        printf("Find: SQL error: %s\n", sqlite3_errmsg(PM_INST->db) );
        return NULL;
}
