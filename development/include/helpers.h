#include <stdint.h>    
#include "bytebuffer.h"
#include "../include/gxmem.h"
    
int hlp_hexToBytes(
const char* str,
unsigned char** arr,
uint16_t* count);

unsigned char hlp_getValue(char c);

unsigned char hlp_rand(void);

    int hlp_setObjectCount(
        uint32_t count,
        gxByteBuffer* buff);

            unsigned char hlp_swapBits(unsigned char value);