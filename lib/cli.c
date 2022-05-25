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
        if ( strlen(argv[1]) > 15 || !_sql_safe_in(argv[1]) ) {
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

    char * verbs[] = { "set", "get", "forg", "rec", "del", "loc", "help" };
    char need_name[] = { 1, 1, 1, 1, 1, 1, 0 };
    int verbs_len = 7;
    void (*functions[])(pm_inst *, char *) = { set, get, forget, recover, delete,
        locate, help };

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
        }
    }
    if ( i == verbs_len - 1 )
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
        _delete(PM_INST, name, name_len);
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
        return; // We're free to just return because we've already destroyed our sensitive data
    } else if ( enc_code == -4) {
        printf("Set: Commkey / ukey mismatch. Toggle -u to fix\n");
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

    char ukey = (PM_INST->pm_opts & 4) >> 2;
    char * query;

    if ( ukey )
        query = "INSERT INTO % (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?,?)";
    else
        query = "INSERT INTO % (ID,SALT,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?)";

    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * stmt_handle;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &stmt_handle, NULL);
    free(query);

    if ( ecode != SQLITE_OK ) {
        printf("Set: SQL query compilation error: %s\n", sqlite3_errmsg(PM_INST->db));
        return;
    }
    // Bind real values to compiled parameterized query
    int binds[6]; // (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE)
    binds[0] = sqlite3_bind_text(stmt_handle, 1, name, name_len, SQLITE_STATIC/*?*/);
    binds[1] = sqlite3_bind_text(stmt_handle, 2, salt, 9, SQLITE_STATIC);
    if ( ukey )
        binds[2] = sqlite3_bind_blob(stmt_handle, 3, master_key, M_KEYSIZE, SQLITE_STATIC);
    binds[2+ukey] = sqlite3_bind_blob(stmt_handle, 3+ukey, PM_INST->ciphertext, DATASIZE +
        hydro_secretbox_HEADERBYTES, SQLITE_STATIC);
    binds[3+ukey] = sqlite3_bind_int( stmt_handle, 4+ukey, 1);
    binds[4+ukey] = sqlite3_bind_int( stmt_handle, 5+ukey, validate);

    for ( int i = 0; i < (5+ukey); i++ ) {
        if (binds[i] != SQLITE_OK ) {
            printf("Set: SQL query (%i) binding error %i: %s\n", i, binds[i], sqlite3_errmsg(PM_INST->db));
            return;
        }
    }

    int eval = sqlite3_step(stmt_handle); //Execute the statement
    if ( eval != SQLITE_DONE ) {
        printf("Set: SQL query evaluation error %i: %s\n", eval, sqlite3_errmsg(PM_INST->db));
        return;
    }
    sqlite3_finalize(stmt_handle);

    set_cleanup:
        return;
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
    // Check if vault is ukey or commkey
    char * query2 = "SELECT UKEY, SALT, MASTER_KEY FROM _index WHERE ID = ?";

    int query_len = strlen(query2);
    sqlite3_stmt * statement2;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query2, query_len, &statement2, NULL);

    if ( ecode != SQLITE_OK )
        goto get_sql_fail;

    ecode = sqlite3_bind_text(statement2, 1, PM_INST->table_name, strlen(PM_INST->table_name),
        SQLITE_STATIC);
    if ( ecode != SQLITE_OK )
        goto get_sql_fail;

    ecode = sqlite3_step(statement2);
    if ( ecode != SQLITE_ROW )
        goto get_sql_fail;

    char ukey = (char) sqlite3_column_int(statement2, 0) & 1;
    char * query;
    int * lens;

    if ( ukey ) {
        query = "SELECT SALT, MASTER_KEY, CIPHER, VALIDATE FROM % WHERE ID = ?";
        lens = (int[4]) {SALTSIZE, M_KEYSIZE, CIPHERSIZE, 0};
    } else {
        query = "SELECT SALT, CIPHER, VALIDATE FROM % WHERE ID = ?";
        lens = (int[3]) { SALTSIZE, CIPHERSIZE, 0};
    }

    query = concat(query, 1, PM_INST->table_name);
    query_len = strlen(query);

    sqlite3_stmt * statement;
    ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK )
        goto get_sql_fail;

    ecode = sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC/*?*/);
    if ( ecode != SQLITE_OK )
        goto get_sql_fail;

    ecode = sqlite3_step(statement);
    if ( ecode == SQLITE_DONE ) {
        printf("Get: No entry named: %s\n", name);
        return;
    } else if ( ecode != SQLITE_ROW )
        goto get_sql_fail;

    int i = 0;
    while ( lens[i] ) {
        ecode = sqlite3_column_bytes(statement, i);
        if ( ecode != lens[i] ) {
            printf("Get: SQL returned a malformed object at column %i. Expected:"
                " %i, returned: %i bytes.\n", i, lens[i], ecode );
            return;
        }
        i++;
    }

    memcpy( PM_INST->master_key, sqlite3_column_text(statement, 0) , SALTSIZE);
    memcpy( PM_INST->ciphertext, sqlite3_column_text(statement, (1+ukey) ) , CIPHERSIZE);

    char prompt[] = "Get: Enter the passphrase you used to encrypt your entry:\n";
    char * pswd = getpass(prompt);
    int pswd_len = strlen(pswd);

    if ( pswd_len >= DATASIZE ){
        printf("Get: pm is configured to accept passphrases up to %i characters \
            long. Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
        goto get_cleanup;
    } else if ( pswd_len == 0 ) {
        printf("Get: no passphrases entered\n");
        goto get_cleanup;
    }

    memcpy( PM_INST->plaintext, pswd, DATASIZE);
    memset(pswd, 0, pswd_len);

    char entry_val = (char) sqlite3_column_int(statement, 2+ukey);
    if ( entry_val && ukey ) {
        memcpy( PM_INST->master_key + SALTSIZE, sqlite3_column_text(statement, 1),
            M_KEYSIZE);
    } else if ( entry_val ) {
        // if comkey, check against masterkey from _index query, then turn off val
        // so dec doesnt try to val with wrong salt
        char c_mkey[M_KEYSIZE+1] = {0};
        char c_salt[SALTSIZE+1] = {0};

        memcpy(c_mkey, sqlite3_column_text(statement2, 2), M_KEYSIZE);
        memcpy(c_salt, sqlite3_column_text(statement2, 1), SALTSIZE);

        char * r_mkey =  crypt(PM_INST->plaintext, c_salt);

        if ( strcmp(c_mkey, r_mkey /*+ SALTSIZE, M_KEYSIZE*/ ) ) {
            printf("Invalid passphrase (newcheck)\n");
            // printf("c_mkey:\n");
            // for ( int i = 0; i < SALTSIZE + M_KEYSIZE; i++ ) {
            //     printf("[%u]", c_mkey[i]);
            // }
            // printf("\nr_mkey:\n");
            // for ( int i = 0; i < SALTSIZE + M_KEYSIZE; i++ ) {
            //     printf("[%u]", r_mkey[i]);
            // }
            return;
        }
        // turn off val
        PM_INST->pm_opts |= 2;
    }

    if ( !entry_val ) {
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
        printf("Get: SQL query compilation/execution error %i: %s\n", ecode,
            sqlite3_errmsg(PM_INST->db));
}

void forget(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 0);
}

void recover(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 1);
}

void delete(pm_inst * PM_INST, char * name) {
    int name_len = strlen(name);
    if ( _entry_in_table(PM_INST, PM_INST->table_name, name) == 0 ) {
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
    } else if ( found == 0 ) {
        printf("%s not found in %s. Perhaps look elsewhere? [ pm ls-vaults ] to list valults.\n",
            name, PM_INST->table_name);
        return;
    } else {
        printf("Locate: error %i in _e_i_v: %s\n", found, sqlite3_errmsg(PM_INST->db) );
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
    return ecode + 1;

    eit_sql_fail:
        printf("Entry in Table: SQL Error: %s\n", sqlite3_errmsg(PM_INST->db) );
        return 0;
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
    if ( ecode != SQLITE_OK ) {
        printf("%s: Error compiling SQL query: %s\n", opstrings[op], sqlite3_errmsg(PM_INST->db) );
        return -1;
    }

    ecode = sqlite3_bind_int(statement, 1, op);
    if ( ecode != SQLITE_OK ) {
        printf("%s: Error binding SQL query: %s\n", opstrings[op], sqlite3_errmsg(PM_INST->db) );
        return -1;
    }
    ecode = sqlite3_bind_text(statement, 2, name, name_len, SQLITE_STATIC);
    if ( ecode != SQLITE_OK ) {
        printf("%s: Error binding SQL query: %s\n", opstrings[op], sqlite3_errmsg(PM_INST->db) );
        return -1;
    }

    ecode = sqlite3_step(statement);
    if ( ecode == SQLITE_DONE ) {
        return 0;
    }

    printf("%s: Error executing SQL query: %s\n", opstrings[op], sqlite3_errmsg(PM_INST->db) );
    return -2;
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
    if ( ecode == SQLITE_DONE ) {
        return 0;
    }

    printf("Del: a backend error prevented the deletion of %s : %s\n", name,
        sqlite3_errmsg(PM_INST->db) );
    return -1;
}

int _sql_safe_in( char * in ) {
    unsigned char c;
    if (!in)
        return 0;
    if ( in[0] == '_' )
        return 0;
    while(( c = *in++ )) {
        if ( c < 48 || ( c > 57 && c < 65 ) || ( c > 90 && c < 97 ) || ( c > 122 ) ) {
            return 0;
        }
    }
    return 1;
}
