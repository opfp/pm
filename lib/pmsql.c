#include "pmsql.h"

int pmsql_compile(pmsql_stmt * base_stmt, char * query, int num_binds,
    void * * bind_data, int * blob_lens, int * bind_types ) {
    char * error;
    if ( !base_stmt )
        return -1;
    int ret = -1; //msc error
    if ( !query ) {
        error = "Null query error.";
        goto pmsql_compile_reterr;
    }
    if ( base_stmt->dest != SQLITE_STATIC && base_stmt->dest != SQLITE_TRANSIENT ) {
        error = "Invalid destructor error";
        goto pmsql_compile_reterr;
    }
    // ensure num binds = num ?s in query
    int num_qs = 0;
    char * rover = query;
    char c;
    while (( c = *rover++ )) {
        if ( c == '?' )
            num_qs++;
    }

    if ( num_qs != num_binds ) {
        error = "Bind characters (?) in query does not match number of binds.";
        goto pmsql_compile_reterr;
    }
    // Now lets prepare so we can bind
    ret = sqlite3_prepare_v2(base_stmt->db, query, strlen(query)+1,
        &base_stmt->stmt, NULL);
    if ( ret != SQLITE_OK ) {
        printf("%s\n", query);
        error = "Statement preperation error.";
        ret *= -1;
        goto pmsql_compile_reterr;
    }
    ret = -1; //put back
    // bind
    //---
    int * bind_res = malloc( num_binds * sizeof(int) );
    for ( int i = 0; i < num_binds; i++ ) {
        if ( bind_types[i] == PMSQL_BLOB ) {
            bind_res[i] = sqlite3_bind_blob(base_stmt->stmt, i+1, bind_data[i],
                blob_lens[i], base_stmt->dest);
        } else if ( bind_types[i] == PMSQL_TEXT ) {
            bind_res[i] = sqlite3_bind_text(base_stmt->stmt, i+1,(char *) bind_data[i],
                strlen(bind_data[i]), base_stmt->dest);
        } else if ( bind_types[i] == PMSQL_INT ) {
            bind_res[i] = sqlite3_bind_int(base_stmt->stmt, i+1, (int) bind_data[i] );
        } else {
            error = "Invalid PMSQL_TYPE.";
            goto pmsql_compile_reterr;
        }
    }
    //--
    for ( int i = 0; i < num_binds; i++ ) {
        if ( bind_res[i] != SQLITE_OK ) {
            ret = -1 * bind_res[i] ;
            error = "Binding error";
            goto pmsql_compile_reterr;
        }
    }
    free(bind_res);
    //---
    return 0;
    pmsql_compile_reterr:;
        int elen = strlen(error);
        base_stmt->pmsql_error = malloc(elen+1);
        memcpy(base_stmt->pmsql_error, error, elen);
        base_stmt->pmsql_error[elen] = 0;
        return ret;
}

int pmsql_read(pmsql_stmt * base_stmt, int num_rows, void * * buffs,
    int * buff_lens, int * bind_types ) {
    int ret = -1;
    char * error;
    if ( !base_stmt )
        goto pmsql_read_reterr;
    if( !num_rows || !buffs || !buff_lens || !bind_types ) {
        error = "Null paramater error.";
        goto pmsql_read_reterr;
    }
    // copy buffer sizes so we can modify safely
    int * buff_lens_cpy = malloc(num_rows * sizeof(int));
    memcpy(buff_lens_cpy, buff_lens, num_rows * sizeof(int));
    // Check sizes
    for ( int i = 0; i < num_rows; i++ ) {
        if ( bind_types[i] == PMSQL_INT )
            continue;
        int retsz = sqlite3_column_bytes(base_stmt->stmt, i);
        if ( buff_lens_cpy[i] < 0) {
            if ( retsz == 0 )
                continue;
            buff_lens_cpy[i] *= -1;
        }
        if ( retsz != buff_lens_cpy[i] ) {
            char * berror = "Query returned a malformed object at column %i. Expected %i"\
                " bytes, returned %i";
            error = malloc(strlen(berror) + 4); // no pm tables have >10 columns
            sprintf(error, berror, i, buff_lens_cpy[i], retsz );
            goto pmsql_read_reterr;
        }
    }
    //---
    // do read
    for ( int i = 0; i < num_rows; i++ ) {
        if ( buff_lens_cpy[i] < 0 )
            continue;
        if ( bind_types[i] == PMSQL_BLOB ) {
            memcpy( buffs[i], sqlite3_column_blob(base_stmt->stmt, i), buff_lens_cpy[i] );
        } else if ( bind_types[i] == PMSQL_TEXT ) {
            memcpy( buffs[i], sqlite3_column_text(base_stmt->stmt, i), buff_lens_cpy[i] );
        } else if ( bind_types[i] == PMSQL_INT ) {
            *((int *) buffs[i]) = sqlite3_column_int(base_stmt->stmt, i);
        } else {
            error = "Invalid PMSQL_TYPE.";
            goto pmsql_read_reterr;
        }
    }
    free(buff_lens_cpy);
    return 0;
    pmsql_read_reterr:;
        int elen = strlen(error);
        base_stmt->pmsql_error = malloc(elen+1);
        memcpy(base_stmt->pmsql_error, error, elen);
        base_stmt->pmsql_error[elen] = 0;
        return ret;
    }

int pmsql_safe_in( char * in ) {
    unsigned char c;
    if (!in)
        return 0;
    if ( in[0] == '_' )
        return 0;
    while(( c = *in++ )) {
        if ( c < 48 || ( c > 57 && c < 65 ) || ( c > 90 && c < 95 ) || ( c > 122 ) ) {
            return 0;
        }
    }
    return 1;
}
