#include "pm.h"

int main( int argc, char * * argv ) {
    if ( argc < 2 ) {
        printf("pm: headless invocation error. Has pm been installed properly?\n");
        return -1;
    }
    // args[1] is the path to pm.conf
    FILE * pm_conf = fopen(argv[1], "r");
    if ( pm_conf == NULL ) {
        printf("Your pm configuration file ( %s ) not found. PM has either not"\
            " been installed, or your install has been corrupted.\n", argv[1]);
        return -1;
    }
    fseek(pm_conf, 0, SEEK_END);
    int pm_conf_sz = ftell(pm_conf);
    rewind(pm_conf); //nesecary?

    if ( pm_conf_sz > 2048 ) {
        printf("Your pm configuration file ( %s ) is too long. Must be less than 2048 bytes\n",
            argv[1]);
        return -1;
    } else if ( pm_conf_sz == 0 ) {
        printf("Your pm configuration file ( %s ) appears to be empty.\n", argv[1]);
        return -1;
    }
    // echo, noval, unique key
    const int num_flags = 4;
    char flags[num_flags+1] = "enuv";
    char opts_chr = 0;

    if (argv[argc-1][0] == '-' ) {
        for( int i = 0; i < num_flags; i++ ) {
            char * match;
            if (( match = strchr(argv[argc-1], flags[i]) )) {
                //printf("%c in opts\n", flags[i] );
                opts_chr |= 1 << i ;
            }
        }
    }

    // if ( argv[argc-1][0] == '-' && strlen(argv[argc-1]) > 1 ) {
    //     flags_passed = 1;
    //     int i = 0;
    //     char c;
    //     while((c = argv[argc-1][i++])) {
    //         char * l;
    //         if (( l = strchr(flags, c) ))
    //              opts |= ( 1 << ( l - flags ) );
    //     }
    //     argv[argc-1] = NULL;
    // }

    //---
    char * pm_conf_str = malloc(pm_conf_sz+1);
    pm_conf_str[pm_conf_sz] = 0;
    fread( pm_conf_str, 1, pm_conf_sz, pm_conf);

    char * att_names[] = { "cooldown" , "db_path" , "confirm_cphr",
         "warn", "def_tables" };
    char * * atts = get_atts_conf( pm_conf_str, 5, att_names);

    if ( !atts ) {
        printf("Issue with %s. Has pm been installed / configured correctly?\n", argv[1]);
        return -1;
    }

    int cooldown = atoi(atts[0]);
    char * db_path = atts[1];

    if ( cooldown > 600 ) {
        cooldown = 600;
        printf("Maximum configurable cooldown is 600s. Edit your cooldown settings"
            " in %s to avoid seeing this warning in the future.", argv[1] );
    }

    sqlite3 * db;
    if ( sqlite3_open(db_path, &db) ) {
        printf("Init: error connecting to database: %s. [ pm chattr db_path ] or edit %s\n",
            db_path, argv[1]);
        return -1;
    }
    // for( int i = num_flags; i < num_flags + 3; i++) {
    //     printf("%s : %s\n", att_names[i], atts[i]);
    //     opts_chr |= ( ( atoi(atts[i]) & 1 ) << i );
    // }
    for( int i = 2; i < 5; i++ ) {
        opts_chr |= ( atoi(atts[i]) & 1 ) << ( i + 2 );
    }
    free(atts);
    free(pm_conf_str);

    if ( hydro_init() != 0 ) {
        printf("Init: error initilizing hydrogen\n");
        return -1;
    }

    pm_inst * PM_INST = malloc( sizeof(pm_inst));
    memset(PM_INST, 0, sizeof(pm_inst));

    PM_INST->pm_opts = opts_chr;
    PM_INST->cooldown = cooldown;

    char table_name[] = "test";
    strncpy(PM_INST->table_name, table_name, 15);

    PM_INST->db = db;

    cli_main(argc - 2 - ( argv[argc-1][0] == '-' ), &argv[2], PM_INST);
}

int val_pad(char * in) {
    char pad = 0;
    for ( int i = 0; i < DATASIZE; i++ ) {
        unsigned char c = in[i];
        if ( ( c > 0 && c < 32)  || c == 127 || c == 59) {
            return -1;
        }
        if ( c == 0 ){
            in[i] = 59; // padding
        }
    }
    return 0;
}

char * * get_atts_conf( char * conf_str, int attc, char * * att_map) {
    if ( !conf_str || !att_map )
        return NULL;

    char * * ret = malloc( attc * sizeof(char *) );
    for( int i = 0; i < attc; i++ ) {
        char * this_attr = strstr(conf_str, att_map[i]);
        if (!this_attr)
            return NULL;
        // all this is so keys which are also values don't cause trouble
        while ( *(this_attr+strlen(att_map[i])) != '=' ) {
            printf("%s\n", this_attr );
            this_attr = strstr(this_attr + 1, att_map[i]);
            if ( !this_attr )
                return NULL;
        }
        this_attr = strchr(this_attr, '=')+1;
        *(strchr(this_attr, '\n')) = 1; // flag for put 0 here later
        ret[i] = this_attr;
    }
    char * rover = strchr(conf_str, 1);
    while (rover) {
        *rover++ = 0;
        rover = strchr(rover, 1);
    }
    return ret;
}
