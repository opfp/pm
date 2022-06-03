#ifndef CLI
#define CLI

#include <pwd.h>
#include <unistd.h>

#include "common.h"

int cli_main(int, char * *, pm_inst *);

void set(pm_inst *, char *);
void get(pm_inst *, char *);
void forget(pm_inst *, char *);
void recover(pm_inst *, char *);
void delete(pm_inst *, char *);
void locate(pm_inst *, char *);
void mkvault(pm_inst *, char *);
void rmvault(pm_inst *, char *);
void help(pm_inst *, char *);

int _entry_in_table( pm_inst *, char *, char *);
//int _entry_in_index( pm_inst *, char *);
int _recover_or_forget(pm_inst *, char *, int);
int _delete(pm_inst *, char *, int);
int _delete_val(pm_inst *, char *);

#endif
