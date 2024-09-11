#ifndef PM
#define PM

#include "common.h"

// pm opts
typedef uint8_t pm_options_t; 

#define NUM_FLAGS 5 /* flags defined in pm.c and accessed from getters declared here */   
enum pm_opts { PIPETOSTDOUT=1, SKIPVAL=2, UKEY=4, SHOW_INVIS=8, NOCONFIRM=16,
 	WARNNOVAL=32, DEFVAULT=64, PRETTYOUT=128}; 

int val_pad(char *);
char * * get_pm_flags(); 
int * get_flag_bits();  

#endif
