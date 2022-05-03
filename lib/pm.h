#ifndef PM
#define PM

#include "common.h"

pm_inst * __init__();

int val_pad(char *);

sqlite3_stmt * pm_sqlite3_make_stmt( char *, ... );

// void init_guardian( pm_inst * );
// void guardian_clear_pm( pm_inst *);
// void guardian_sleep10();

#endif
