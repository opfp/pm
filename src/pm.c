#include "pm.h"
#include "cli.h" 

const char * pm_flags[NUM_FLAGS] = {"unique-key", "skip-validate", "no-confirm","pretty-out"};  

int flag_bit[NUM_FLAGS] = { UKEY, SKIPVAL, NOCONFIRM, PRETTYOUT }; 

int val_pad(char * in) {
	char pad = 0;
	for ( int i = 0; i < DATASIZE; i++ ) {
		unsigned char c = in[i];
		if ( ( c > 0 && c < 32) || c == 127 || c == 59) {
			return -1;
		}
		if ( c == 0 ){
			in[i] = 59; // padding
		}
	}
	return 0;
}

char * * get_atts_conf( char * conf_str, int attc, char * * att_map, pm_options_t * opts_wb) {
	if ( !conf_str || !att_map )
		return NULL;

	/* process flags */ 
	char * rv = conf_str; //strstr(; 
	char * flag_name, * flag_end; 
	int f; 
	char c; 
	bool do_not_cont=false; 
	pm_options_t options_bmp =0; 

	while ( !do_not_cont && (rv = strstr(rv, "\nsetflag ") ) ){ 
		flag_name = rv+9; 	
		flag_end = flag_name; 
		while ( 1 ) { 
			c=*flag_end; 
			if ( !c ) { 
				do_not_cont=true; 
				break; 
			}
			if ( c == ' ' || c == '\n' )  
				break; 
			flag_end++; 
		}

		c=*flag_end; 
		*flag_end='\0'; 	

		for ( f = 0; f<NUM_FLAGS; f++ ) { 
			if ( !strcmp(flag_name, pm_flags[f] ) ) 
				break; 
		}

		if ( f == NUM_FLAGS ) { 
			//fprintf(stderr, "Unknown flag in conf file: %s\n", flag_name ); 
			printf( "Unknown flag in conf file: %s\n", flag_name ); 
		} else { 
			options_bmp |= flag_bit[f]; 	
		}		

		*flag_end=c; 
		rv = flag_end+1; 
	} 

	*opts_wb=options_bmp; 

	/* process attributes */ 
	char * * ret = malloc( attc * sizeof(char *) );
	for( int i = 0; i < attc; i++ ) {
		char * this_attr = strstr(conf_str, att_map[i]);
		_found_att_ck:; 
		if (!this_attr) { 
			ret[i] = NULL;
			continue; 
		}
		if ( *(this_attr+strlen(att_map[i])) != '=' || 
				( *(this_attr-1) != 1 && *(this_attr-1) != '\n' ) ) { 
			this_attr = strstr(this_attr + 1, att_map[i]);
			goto _found_att_ck; 
		} 

		char * eoa = this_attr; 
		char c; 
		while ( (c = *eoa++) && c != '\n' && c != ' ' ) { } 
		*(eoa-1) = 1; // flag for put 0 here later
		ret[i] = this_attr+strlen(att_map[i])+1;
	}
	
	/* seperate the old file buffer with nulltrms (each new buf is in att_map ) */ 
	char * rover = strchr(conf_str, 1);
	while (rover) {
		*rover++ = 0;
		rover = strchr(rover, 1);
	}


	return ret;
}

int parse_pm_conf(pm_inst * pm_monolith/*, char * conf_path*/ ) { 
	char * conf_path = pm_monolith->conf_path; 

	FILE * pm_conf = fopen(conf_path, "r");
	if ( pm_conf == NULL ) {
		printf("Your pm configuration file ( %s ) not found. PM has either not"\
			" been installed, or your install has been corrupted.\n", conf_path);
		return -1;
	}

	fseek(pm_conf, 0, SEEK_END);
	int pm_conf_sz = ftell(pm_conf);
	rewind(pm_conf); //nesecary?

	if ( pm_conf_sz > 2048 ) {
		printf("Your pm configuration file ( %s ) is too long. Must be less than 2048 bytes\n",
			conf_path);
		return -1;
				// will this occur even in "empty" files? don't unix files need to end in a newline? 
	} else if ( pm_conf_sz == 0 ) {
				printf("Your pm configuration file ( %s ) appears to be empty.\n", conf_path);
		return -1;
	}

	char * pm_conf_str = malloc(pm_conf_sz+2);
	pm_conf_str[0] = '\n'; 
	pm_conf_str[pm_conf_sz+1] = '\0';
	fread( pm_conf_str+1, 1, pm_conf_sz, pm_conf); 

	char * att_names[] = { /*"cooldown" ,*/ "db_path" , "confirm_cphr",
		 "warn", "def_tables" };

	char * * atts = get_atts_conf( pm_conf_str, 4, att_names, &pm_monolith->pm_opts );

	// for ( int i = 0; i < 5; i++ ) { 
	//	 printf("%s : '%s'\n", att_names[i], atts[i] ); 
	// } 

	if ( !atts ) {
		printf("Issue with %s. Has pm been installed / configured correctly?\n", conf_path);
		return -1;
	}

	//int cooldown = atoi(atts[0]);
	char * db_path = atts[0];

	//if ( cooldown > 600 ) {
	//	cooldown = 600;
	//	printf("Maximum configurable cooldown is 600s. Edit your cooldown settings"
	//		" in %s to avoid seeing this warning in the future.", conf_path );
	//}

	sqlite3 * db;
	if ( sqlite3_open(db_path, &db) != SQLITE_OK ) {
		printf("Init: error connecting to database: %s. [ pm chattr db_path ] or edit %s\n",
			db_path, conf_path);
		return -1;
	}
	/* check if db is well formed */ 
	char * queries[] = {  
		"SELECT name FROM sqlite_master WHERE type='table' AND name='_index'", 
		"SELECT name FROM sqlite_master WHERE type='table' AND name='_main_cmk'", 
		"SELECT name FROM sqlite_master WHERE type='table' AND name='_main_ukey'" 
	};

	int ecode; 
	sqlite3_stmt * stmt;
	for ( int i = 0; i<3; i++ ) { 
		if ( sqlite3_prepare_v2( db, queries[i], -1, &stmt, NULL ) != SQLITE_OK )  
			goto _db_chk_sql_error; 

		ecode = sqlite3_step( stmt ); 
		if ( ecode == SQLITE_DONE ) 
			goto _db_chk_trap; 
		else if ( ecode != SQLITE_ROW ) 
			goto _db_chk_sql_error; 

		sqlite3_finalize(stmt); 
		continue; 

		_db_chk_trap:;  
			printf("Error: database %s is malformed, check your configuration file %s\n", 
					db_path, pm_monolith->conf_path );  
			return -1; 

		_db_chk_sql_error:; 	
			printf("Error preparing or executing statment to check db: %s\n", queries[i] ); 
			return -1; 
		}
		

	// for( int i = num_flags; i < num_flags + 3; i++) {
	//	 printf("%s : %s\n", att_names[i], atts[i]);
	//	 opts_chr |= ( ( atoi(atts[i]) & 1 ) << i );
	// }
	//for( int i = 2; i < 5; i++ ) {
	//	opts_chr |= ( atoi(atts[i]) & 1 ) << ( i + 2 );
	//}
	free(atts);
	free(pm_conf_str);

	if ( hydro_init() != 0 ) {
		printf("Init: error initilizing hydrogen\n");
		return -1;
	}

	//pm_monolith->pm_opts = opts_chr;
	//pm_monolith->cooldown = cooldown;

	//char table_name[] = "test";
	//strncpy(pm_monolith->table_name, table_name, 15);

	pm_monolith->db = db;
	return 0; 
}

int main( int argc, char * * argv ) {
	// printf("av2: %s\n", argv[2] );
	if ( argc < 2 ) {
		printf("pm: headless invocation error. Has pm been installed properly?\n");
		return -1;
	}

	pm_inst * pm_monolith = calloc( sizeof(pm_inst), 1);

	size_t conf_sz = strlen(argv[1]); 
	char * pm_conf_path = malloc(conf_sz+1);
	strncpy(pm_conf_path, argv[1], conf_sz+1); 
	pm_monolith->conf_path = pm_conf_path;

	if ( parse_pm_conf(pm_monolith )) {   
			return -1; 
	}

	return cli_new( argc-2, argv+2, pm_monolith);  

	//if ( parse_cli_args(pm_monolith, argv+2 )) {
	//	return -1; 
	//} 

	// args[1] is the path to pm.conf
		/*

	// echo, noval, unique key, vault
	#ifndef NUM_FLAGS 
	#define NUM_FLAGS 4 
	#endif
	char flags[NUM_FLAGS+1] = "enuv";
	char opts_chr = 0;

	if (argv[argc-1][0] == '-' ) {
		for( int i = 0; i < NUM_FLAGS; i++ ) {
			char * match;
			if (( match = strchr(argv[argc-1], flags[i]) )) {
				//printf("%c in opts\n", flags[i] );
				opts_chr |= 1 << i ;
			}
		}
	}
		*/

	// if ( argv[argc-1][0] == '-' && strlen(argv[ar=gc-1]) > 1 ) {
	//	 flags_passed = 1;
	//	 int i = 0;
	//	 char c;
	//	 while((c = argv[argc-1][i++])) {
	//		 char * l;
	//		 if (( l = strchr(flags, c) ))
	//			  opts |= ( 1 << ( l - flags ) );
	//	 }
	//	 argv[argc-1] = NULL;
	// }

	//---

	//cli_main(argc - 2 - ( argv[argc-1][0] == '-' ), &argv[2], PM_INST);
}

char * * get_pm_flags() { return pm_flags; } 
int * get_flag_bits() { return flag_bit; } 
