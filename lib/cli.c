# include "cli.h"

void cli_main( int argc, char * * argv, pm_inst * PM_INST) {
    if ( argc < 1 ) {
        printf("pm: Not enough arguments. [pm help] for help, or read the docs.\n");
        //help(void, void);
        return;
    }
    const int num_verbs = 8;
    char * verbs[num_verbs] = { "set", "get", "mkvault", "forg", "rec", "del", "ls",
          "help" };
    // See idocs for explanation of verb_type
    char verb_type[num_verbs] = { 0, 0, 0, 1, 1, 1, 2, 0};
    void (*functions[])(pm_inst *, char *) = { set, get, mkvault, forget, recover,
        delete, ls, help };

    int i = 0;
    for(/*i*/; i < num_verbs; i++ ) {
        if ( !strcmp(argv[0], verbs[i]) )
            break;
    }
    if ( i > (num_verbs - 2) ) {
        help(NULL, NULL);
        return;
    }

    char * vault = NULL;
    char * name = NULL;
    if ( verb_type[i] == 0 ) {
        if ( argc < 2 ) {
            printf("%s: Name required. [pm help].\n", verbs[i] );
            return;
        }
        name = ( argc > 2 ) ? argv[2] : argv[1];
        vault = ( argc > 2 ) ? argv[1] : NULL;
    } else if ( verb_type[i] == 1 ) {
        if ( argc < 2 ) {
            printf("%s: Vault name required. [pm help].\n", verbs[i] );
            return;
        }
        name = argv[1];
    } else if ( verb_type[i] == 2 ) {
        name = ( argc > 1 ) ? argv[1] : NULL;
    }
    // code for testing
    // char * option_names[] = { "Echo", "Skip pswd hash val", "Use ukey vault",
    //     "vault" , "Confirm cipertexts" , "Warn on no validation" , "Use only default vaults"};
    // unsigned char opts = PM_INST->pm_opts;
    // for ( int i = 0; i < 7; i++) {
    //     printf("%i : %s\n", ( opts & 1 ), option_names[i]);
    //     opts = opts >> 1;
    // }
    // ---
    if (vault) {
        if ( !pmsql_safe_in(vault) || strlen(vault) > 15 ) {
            printf("Illegal vault name: %s\n", vault);
            return;
        }
        if ( PM_INST->pm_opts & 0x40 ) {
            printf("Vault specified, but pm configured to only use defaults."
                " [ pm chattr def_vaults 0 ] to enable user created vaults.\n");
            return;
        }
        if ( !_entry_in_table(PM_INST, "_index", argv[1]) && !verb_type[i] ) {
            printf("No vault named %s. [pm mkvault] to make a vault.\n", argv[1]);
            return;
        }
    } else {
        vault = ( PM_INST->pm_opts & 4 ) ? "_main_ukey" : "_main_cmk" ;
    }
    strcpy(PM_INST->table_name, vault);

    if (name) {
        if ( !pmsql_safe_in(name) || strlen(name) > 31 ) {
            printf("Illegal name: %s\n", name);
            return;
        }
    }
    //printf("%s : %s {%s}\n", verbs[i], name, PM_INST->table_name );
    (*functions[i])(PM_INST, name);
    return;
}

void set(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    char exists = _entry_in_table(PM_INST, PM_INST->table_name, name);
    if ( exists == 2 ) {
        printf("Set: an entry with name %s already exists in %s.\n", name, PM_INST->table_name );
        return;
    } else if ( exists == 1 ) { //Entry exists but is in trash, will be overwritten.
        if ( _delete(PM_INST, name) )
            printf("Set: backend error in _delete: %s\n", sqlite3_errmsg(PM_INST->db) );
    }

    char * rprompt = "Enter the ciphertext for the entry %:\n";
    char * prompt = malloc(strlen(rprompt) + strlen(name) + 1);
    sprintf(prompt, rprompt, name);
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
    char * rquery;
    void * data;
    int * data_sz;
    int * data_tp;

    char ukey = (PM_INST->pm_opts & 4) >> 2;
    if ( ukey ) {
        rquery = "INSERT INTO %s (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?,?)";
        data = (void *[6]) { name, salt, master_key, PM_INST->ciphertext, 1, validate };
        data_sz = (int [6]) { 0, 0, M_KEYSIZE, CIPHERSIZE, 0, 0 };
        data_tp = (int [6]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_BLOB,
            PMSQL_INT, PMSQL_INT };
    } else {
        rquery = "INSERT INTO %s (ID,SALT,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?)";
        data = (void *[5]) { name, salt, PM_INST->ciphertext, 1, validate };
        data_sz = (int [5]) { 0, 0, CIPHERSIZE, 0, 0 };
        data_tp = (int [5]) { PMSQL_TEXT, PMSQL_TEXT, PMSQL_BLOB, PMSQL_INT, PMSQL_INT};
    }

    size_t query_len = strlen(rquery) + strlen(PM_INST->table_name);
    char * query = malloc(query_len+1);
    snprintf(query, query_len, rquery, PM_INST->table_name);

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
        printf("Get: No entry %s exists in %s. Perhaps look elsewhere? [pm help]"
        " for help on searching.\n", name, PM_INST->table_name);
        return;
    } else if ( exists == 1 ) {
        printf("Get: %s is in the trash. [pm rec %s] to recover so the entry may be retreived\n", name, name);
        return;
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
    data = ( void * [1] ) {PM_INST->table_name};
    data_tp = ( int [1] ) { PMSQL_TEXT };
    pmsql = &(pmsql_stmt) { SQLITE_STATIC, PM_INST->db, stmt, NULL };

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
    size_t query_len = strlen(rquery) + strlen(PM_INST->table_name);
    char * query = malloc(query_len+1);
    snprintf(query, query_len, rquery, PM_INST->table_name);

    if (( ecode = pmsql_compile( pmsql, query, 1, data, NULL, data_tp ) ))
        goto get_sql_fail;

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
    char * table = (PM_INST->pm_opts & 8) ? "_index" : PM_INST->table_name ;
    _recover_or_forget(PM_INST, table, name, 0);
}

void recover(pm_inst * PM_INST, char * name) {
    char * table = (PM_INST->pm_opts & 8) ? "_index" : PM_INST->table_name ;
    //printf("%i : %s\n", (PM_INST->pm_opts & 8), table  );
    _recover_or_forget(PM_INST, table, name, 1);
}

void delete(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    char vault = (PM_INST->pm_opts & 8) >> 3;
    char * table = vault ? "_index" : PM_INST->table_name ;

    if ( ! _entry_in_table(PM_INST, table, name) ) {
        printf("Del: no entry to delete\n");
    }
    printf("This will permanently delete %s. This action cannot be undone."
        " Use forget [pm help forg] to mark as forgotten, which can be undone"\
        " (until an entry of the same name is set).\nProceed with permanent"\
        " deletion? (y/n)\n", name);
    if ( fgetc(stdin) != 'y' ) {
        printf("Del: aborting\n");
        return;
    }

    if ( vault )
        _delete_val(PM_INST, name);
    else
        _delete(PM_INST, name);
}

void mkvault(pm_inst * PM_INST, char * name) {
    // shouldn't happen, should be caught in cli_main.
    if ( !name || strlen(name) == 0 ) { // short circut ?
        printf("Mkvault: name required. [ pm mkvault NAME ]\n");
    }
    if ( !pmsql_safe_in(name) || strlen(name) > 15 ) {
        printf("Invalid vault name. Must contain only alphabetical characters and _."
            "Cannot start with _, and must be 15 characters or less\n");
            return;
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

// void rmvault(pm_inst * PM_INST, char * name) {
//     int vis;
//     if (!( vis = _entry_in_table(PM_INST, "_index", name) )) {
//         printf("Rmvault: No vault %s exists.\n", name);
//         return;
//     } else if ( vis == 1 ) {
//         printf("Rmvault: Vault %s already deleted.\n", name);
//         return;
//     }
//     //strncpy(PM_INST->table_name, "_index", 7);
//     if (( _recover_or_forget(PM_INST, "_index", name, 0 ) )) {
//         printf("A backend error prevented vault deletion. SQL : %s",
//             sqlite3_errmsg(PM_INST->db) );
//     }
// }

void ls(pm_inst * PM_INST, char * name) {
    _ls_find(PM_INST, name, (PM_INST->pm_opts & 8) >> 3 ) ;
}

// void lsv(pm_inst * PM_INST, char * name) {
//     _ls_find(PM_INST, name, 1);
// }
// only takes these args for ease of calling. ignores them
void help(pm_inst * PM_INST, char * name) {
    printf("In production, this is a help page.\n");
}

void _ls_find(pm_inst * PM_INST, char * name, char vault ) {
    char * table = vault ? "_index" : PM_INST->table_name ;
    //char * print = vault ? "The Vault" : name;
    if ( name ) {
        if ( _entry_in_table( PM_INST, table, name ) ) {
            printf("%s:%s found. Contents:\n", table, name );
            if ( vault ) {
                table = name;
                goto _ls_printall;
            }
            return;
        }
        char * * sims = _find_by_key(PM_INST, table, name, 8);

        if (!sims ) {
            printf("%s:%s not found. No similer entries were found either.\n",
                table, name);
            return;
        }
        printf("%s:%s not found. See these similar entries:\n", table, name);
        int i = 1;
        while(sims[i]) {
            printf("%i : %s\n",i, sims[i] );
            ++i;
        }
        free(sims);
        return;
    }
    _ls_printall:;
    char * * all = _all_in_table(PM_INST, table );
    if (!all ) {
        return;
    }
    int i = 1;
    while ( all[i] ) {
        if ( all[0][i] ) {
            if ( vault && name )
                printf("\t");
            printf("%s\n", all[i]);
        }
        ++i;
    }
}

int _entry_in_table(pm_inst * PM_INST, char * tb_name, char * ent_name) {
    char * rquery = "SELECT VIS FROM %s WHERE ID = ?";
    size_t query_len = strlen(rquery) + strlen(tb_name);
    char * query = malloc(query_len+1);
    snprintf(query, query_len, rquery, tb_name);
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
        printf("Entry in Vault: SQL Error: %s\n", sqlite3_errmsg(PM_INST->db) );
        sqlite3_finalize(statement);
        return -1;
}

int _recover_or_forget(pm_inst * PM_INST, char * table, char * name, int op ){
    char * opstrings[] = { "Forget", "Recover" };
    if (name == NULL){
        printf("%s: a name is required.\n", opstrings[op]);
        return -1;
    }
    int name_len = strlen(name);
    int exists = _entry_in_table(PM_INST, table, name);
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

int _delete(pm_inst * PM_INST, char * name) {
    char * rquery = "DELETE FROM %s WHERE ID = ?";
    size_t query_len = strlen(rquery) + strlen(PM_INST->table_name);
    char * query = malloc(query_len+1);
    snprintf(query, query_len, rquery, PM_INST->table_name);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK ) {
        printf("Del: Error compiling SQL query: %s\n", sqlite3_errmsg(PM_INST->db) );
        return -1;
    }

    ecode = sqlite3_bind_text(statement, 1, name, strlen(name), SQLITE_STATIC);
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
    char * rquery = "DROP TABLE %s";
    size_t query_len = strlen(rquery) + strlen(name);
    char * query = malloc(query_len+1);
    snprintf(query, query_len, rquery, name);

    sqlite3_stmt * stmt;
    pmsql_stmt pmsql = { SQLITE_STATIC, PM_INST->db, stmt, 0 };

    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &stmt, NULL);
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

char * FBK_KEY; // No way to get around glob unfortuantely
char * * _find_by_key( pm_inst * PM_INST, char * tb, char * key, int numres ) {
    char * * all = _all_in_table( PM_INST, tb );
    //---
    if ( !all )
        return NULL;

    int c = 1;
    while ( all[c] ) { // pick out vis=0 and nonsimilar ents
        //printf("%s\n",all[c] );
        if ( !all[0][c] || !o_search(all[c], key) ) {
            all[c] = all[c+1];
            int j = c+1;
            if ( ! all[j] )
                continue;
            while(( all[j] = all[j+1] )) { j++; };
        } else
            c++;
    }
    //---
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

char * * _all_in_table( pm_inst * PM_INST, char * tb_name ) {
    char * bquery = "SELECT COUNT(*) FROM %s";
    size_t query_len = strlen(bquery) + strlen(tb_name);
    char * query = malloc(query_len + 1);
    snprintf(query, query_len, bquery, tb_name);

    sqlite3_stmt * stmt;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &stmt, NULL);
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
    //     goto ait_sql_fail;
    sqlite3_finalize(stmt);

    char * * ids = malloc(rows * sizeof(char *) + 2 );
    char * vis = malloc(rows);
    ids[0] = vis;

    bquery = (char *) "SELECT ID, VIS from %s";
    query_len = strlen(bquery) + strlen(tb_name);
    query = malloc(query_len + 1);
    snprintf(query, query_len, bquery, tb_name);

    if (( ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &stmt, NULL) ))
        goto ait_sql_fail;
    //--
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
    ids[i] = NULL;
    if ( ecode != SQLITE_DONE )
        goto ait_sql_fail;
    return ids;

    ait_sql_fail:
        printf("all_in_table SQL error: %i : %s\n", ecode, sqlite3_errmsg(PM_INST->db)  );
        return NULL;
}
