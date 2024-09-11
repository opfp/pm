#ifndef CLI
#define CLI

#include <pwd.h>
#include <unistd.h>

#include "common.h"

//void cli_main(int, char * *, pm_inst *);
int cli_new(int, char * *, pm_inst *);

enum keywords_enum { KW_VAULT, KW_CIPHER, KW_PASS, KW_NEW_PASS, KW_NAME, KW_NUM };  

enum eit_rets { NO_ENTRY=0, FORG_ENTRY=1, VIS_ENTRY=2 }; 
int _entry_in_table( pm_inst *, char *, char *);

/*
void _ls_find(pm_inst *, char *);
int _entry_in_table( pm_inst *, char *, char *);
int _recover_or_forget(pm_inst *, char *, char *, int);
int _delete(pm_inst *, char *);
int _delete_val(pm_inst *, char *);
char * * _find_by_key( pm_inst *, char *, char *, int);
int _fbk_compare( const void *, const void *);
char * * _all_in_table(pm_inst *, char *);
*/
#endif
