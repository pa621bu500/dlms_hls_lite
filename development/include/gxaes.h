#ifndef GXAES
#define GXAES

#ifndef DLMS_IGNORE_AES
#include "bytebuffer.h"
#include "enums.h"

void gxaes_keyExpansion(
    const DLMS_AES eas,
    const unsigned char *key,
    unsigned char *roundKeys);

#endif // DLMS_IGNORE_AES
#endif // GXAES
