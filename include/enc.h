#ifndef ENC
#define ENC

#include "common.h"

int enc_plaintext( pm_inst *, char *);
int dec_ciphertext( pm_inst * /*, char * */);
unsigned long mix(unsigned long, unsigned long, unsigned long);

#endif
