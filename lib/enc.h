#ifndef ENC
#define ENC

#include "common.h"
#include <crypt.h> 

int enc_plaintext( pm_inst *);
int dec_ciphertext( pm_inst *);
unsigned long mix(unsigned long, unsigned long, unsigned long);

#endif
