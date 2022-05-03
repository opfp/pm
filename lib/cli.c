# include "cli.h"

int main( int argc, char * * argv ) {
    //if ( argc < 2 || argv[1][0] != '-' || strlen(argv[1]) < 2 ) {
    if ( argc < 2 ) {
        printf("PM: Invocation error. [pm help] for help, or read the docs.\n");
        //help(void, void);
        exit(-1);
    }

    char opts = 2;
    pm_inst * PM_INST = __init__();

    char * db_path;
    char * vault;

    if ( argc > 2 ) {
        char * flags[] = { "e", "noval", "q" };
        for ( int i = 0; i < 3; i++ ) {
            for ( int j = 2; j < argc; j++ ) {
                if ( strlen(argv[j]) < 2 || argv[j][1] != '-' )
                    continue;
                if ( strcmp(flags[i], argv[j]+1)) {
                    opts = opts ^ ( 1 << (i - 1) );
                }
            }
        }
        char * opts[] = { "db", "v" };
        void * text_boxes[] = { db_path, vault };
    }

    char * verbs[] = { "set", "get", "forg", "rec", "del", "loc", "help" };
    int verbs_len = 7;
    void (*functions[])(pm_inst *, char *) = { set, get, forget, recover, delete, locate, help };

    char * verb = argv[1];
    int i = 0;
    for(/*i*/; i < verbs_len; i++ ) {
        if ( strcmp(verb, verbs[i]) == 0 )
            (*functions[i])(PM_INST, argv[2]);
    }

    if (i == verbs_len - 1 ) {
        help(NULL, NULL);
        exit(0);
    }
}

void set(pm_inst * PM_INST, char * name) {
    if (name == NULL){
        printf("Set: a name is required for each entry. pm set [NAME]\n");
        return;
    }
    int namelen = strlen(name);
    if ( namelen > 31 ) {
        printf("Set: Name must be less than 32 characters long.");
        return;
    }
    //CHECK FOR ENTRY WITH NAME
    int exists = _entry_in_vault(PM_INST, name, namelen);
    if ( exists == 2 ) {
        printf("Set: an entry with name %s already exists in %s.\n", name, PM_INST->table_name );
        return;
    } else if ( exists == 1 ) { //Entry exists but is in trash, will be overwritten.
        delete(PM_INST, name);
    }

    char * prompt = "Enter the ciphertext for the entry %:\n";
    prompt = concat(prompt, 1, name);
    char * ctxt = getpass(prompt);
    free(prompt);
    int ctxt_len =  strlen(ctxt);

    if ( ctxt_len >= DATASIZE) {
        printf("Set: pm is configured to accept ciphers up to %i characters long."
        " Your entry was too long. See [YOUR CONF FILE]\n", DATASIZE );
        exit(0);
    }
    if ( ctxt == NULL ){
        printf("Set: no ciphertext entered\n");
        exit(0);
    }

    char context[DATASIZE];
    memset( context, 0, DATASIZE);
    memcpy(context, ctxt, ctxt_len);
    if ( val_pad(context) != 0 ) {
        printf("Set: your cipher contained disallowed characters. Only printable"
        " characters, except ;, are allowed.\n");
        exit(0);
    }

    //Add plaintext to pm_inst obj
    memcpy(PM_INST->plaintext, context, DATASIZE);

    int j = 0;
    while(ctxt[j] != 0) { //Destroy our copy of ciphertext
        ctxt[j] = 0;
        j++;
    }

    if ( enc_plaintext(PM_INST) != 0 ) {
        printf("Set: Error in Enc.\n");
        return;
    }

    // Build sqlite3 statement to insert the cipher, validate key etc
    uint8_t salt[SALTSIZE];
    memcpy(salt, PM_INST->master_key, SALTSIZE);
    uint8_t master_key[M_KEYSIZE];
    memset( master_key, 0, M_KEYSIZE);

    int validate = 0;
    if ( ( PM_INST->enc_flags & 1 ) == 1 ) {
        validate = 1;
        memcpy( master_key, PM_INST->master_key + 9, M_KEYSIZE);
    }

    char * query = "INSERT INTO % (ID,SALT,MASTER_KEY,CIPHER,VIS,VALIDATE) VALUES (?,?,?,?,?,?)";
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
    binds[0] = sqlite3_bind_text(stmt_handle, 1, name, namelen, SQLITE_STATIC/*?*/);
    binds[1] = sqlite3_bind_text(stmt_handle, 2, salt, 9, SQLITE_STATIC);
    binds[2] = sqlite3_bind_blob(stmt_handle, 3, master_key, M_KEYSIZE, SQLITE_STATIC);
    binds[3] = sqlite3_bind_blob(stmt_handle, 4, PM_INST->ciphertext, DATASIZE +
        hydro_secretbox_HEADERBYTES, SQLITE_STATIC);
    binds[4] = sqlite3_bind_int( stmt_handle, 5, 1);
    binds[5] = sqlite3_bind_int( stmt_handle, 6, validate);

    for ( int i = 0; i < 6; i++ ) {
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
}

void get(pm_inst * PM_INST, char * name) {
    if (name == NULL){
        printf("Get: a name is required for each entry. pm -s [NAME]\n");
        return;
    }
    int name_len = strlen(name);
    if ( name_len >= NAMELEN) {
        printf("Get: name must be less than %i characters long\n", NAMELEN);
        return;
    }
    // Check for exists / in trash
    int exists = _entry_in_vault(PM_INST, name, name_len);
    if ( exists == 0 ) {
        printf("Get: No entry %s exists in %s. Perhaps look elsewhere? [pm ls-vaults] to list vaults\n",
            name, PM_INST->table_name);
    } else if ( exists == 1 ) {
        printf("Get: %s is in the trash. [pm rec %s] to recover so the entry may be retreived\n", name, name);
    }

    char * query = "SELECT SALT, MASTER_KEY, CIPHER, VALIDATE FROM % WHERE ID = ?";
    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK ) {
        printf("Get: SQL query compilation error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return;
    }

    ecode = sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC/*?*/);
    if ( ecode != SQLITE_OK ) {
        printf("Get: SQL query binding error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return;
    }

    ecode = sqlite3_step(statement);
    if ( ecode == SQLITE_DONE ) {
        printf("Get: No entry named: %s\n", name);
        return;
    } else if ( ecode != SQLITE_ROW ) {
        printf("Get: SQL query execution error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return;
    }

    //SELECT SALT, MASTER_KEY, CIPHER, VALIDATE
    int lens[] = {SALTSIZE, M_KEYSIZE, CIPHERSIZE };
    for ( int i = 0; i < 3; i++ ){
        if ( sqlite3_column_bytes(statement, i) != lens[i] ) {
            printf("Get: SQL returned a malformed object at column %i\n", i );
        }
    }

    char validate = (char) sqlite3_column_int(statement, 3);

    memcpy( PM_INST->master_key, sqlite3_column_text(statement, 0) , SALTSIZE);
    if ( validate )
        memcpy( PM_INST->master_key + SALTSIZE, sqlite3_column_text(statement, 1) , M_KEYSIZE);
    memcpy( PM_INST->ciphertext, sqlite3_column_text(statement, 2) , CIPHERSIZE);

    PM_INST->dec_flags = 1;
    if ( validate == 0 ) {
        PM_INST->dec_flags = 0;
        if ( (PM_INST->enc_flags & 2) == 1 ) {
            printf("Warning: decryption validation disabled. PM will be unable to verify\
                the output for %s", name );
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
}

void forget(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 0);
}

void recover(pm_inst * PM_INST, char * name) {
    _recover_or_forget(PM_INST, name, 1);
}

void delete(pm_inst * PM_INST, char * name) {
    if (name == NULL) {
        printf("Del: a name is required for each entry. pm del [NAME]\n");
        return;
    }
    int name_len = strlen(name);
    if ( _entry_in_vault(PM_INST, name, name_len) == 0 ) {
        printf("Del: no entry to delete\n");
    }
    printf("This will permanently delete %s. This action cannot be undone."
    " Use forget [pm forg %s] to temporarilly delete.\nProceed? (y/n)\n", name, name);
    if ( fgetc(stdin) != 'y' ) {
        printf("Del: aborting\n");
        return;
    }
    _delete(PM_INST, name, name_len);
}

void locate(pm_inst * PM_INST, char * name) {
    if (name == NULL){
        printf("Locate: please enter a name. pm -l [NAME]\n");
        return;
    }
    int name_len = strlen(name);
    if ( name_len == 0 ) {
        printf("Locate: please enter a name. pm -l [NAME]\n");
        return;
    }
    int found = _entry_in_vault( PM_INST, name, name_len );
    if ( found == 2 ) {
        printf("%s:%s found.\n", PM_INST->table_name, name);
        return;
    } else if ( found == 1 ) {
        printf("%s is in the trash. [ pm rec %s ] to recover.\n", name, name);
        return;
    } else if ( found == 0 ) {
        printf("%s not found in %s. Perhaps look elsewhere? [ pm ls-vaults ] to list valults\n",
            name, PM_INST->table_name);
        return;
    } else {
        printf("Locate: error %i in _e_i_v: %s\n", found, sqlite3_errmsg(PM_INST->db) );
    }
}

void help(pm_inst * PM_INST, char * name) { // only takes these args for ease of calling. ignores them
    printf("In production, this is a help page.\n");
}

int _entry_in_vault(pm_inst * PM_INST, char * name, int name_len) {
    char * query = "SELECT VIS FROM % WHERE ID = ?";
    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK )
        return -1;

    ecode = sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC);
    if ( ecode != SQLITE_OK )
        return -2;

    ecode = sqlite3_step(statement);
    if ( ecode == SQLITE_DONE )
        return 0;
    else if ( ecode != SQLITE_ROW )
        return -3;

    ecode = sqlite3_column_int(statement, 0);
    return ecode + 1;
}

int _recover_or_forget(pm_inst * PM_INST, char * name, int op ){
    char * opstrings[] = { "Forget", "Recover" };
    if (name == NULL){
        printf("%s: a name is required.\n", opstrings[op]);
        return -1;
    }
    int name_len = strlen(name);
    int exists = _entry_in_vault(PM_INST, name, name_len);
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

int _delete(pm_inst * PM_INST, char * name, int name_len ){
    char * query = "DELETE FROM % WHERE NAME = ?";
    query = concat(query, 1, PM_INST->table_name);
    int query_len = strlen(query);

    sqlite3_stmt * statement;
    int ecode = sqlite3_prepare_v2(PM_INST->db, query, query_len, &statement, NULL);
    free(query);
    if ( ecode != SQLITE_OK ) {
        printf("Del: SQL query compilation error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return -1;
    }
    ecode = sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC);
    if ( ecode != SQLITE_OK ) {
        printf("Del: SQL query bining error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return -1;
    }
    ecode = sqlite3_step(statement);
    if ( ecode != SQLITE_OK ) {
        printf("Del: SQL query execution error %i: %s\n", ecode, sqlite3_errmsg(PM_INST->db));
        return -1;
    }
    return 0;
}
