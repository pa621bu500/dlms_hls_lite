#include <stdint.h>    
#include "bytebuffer.h"
#include "../include/gxmem.h"

#define GETU32(pt) (((uint32_t)(pt)[0] << 24) | \
                    ((uint32_t)(pt)[1] << 16) | \
                    ((uint32_t)(pt)[2] <<  8) | \
                    ((uint32_t)(pt)[3]))
    
int hlp_hexToBytes(
const char* str,
unsigned char** arr,
uint16_t* count);

unsigned char hlp_getValue(char c);

unsigned char hlp_rand(void);

int hlp_setLogicalName(unsigned char ln[6], const char* name);

    int hlp_setObjectCount(
        uint32_t count,
        gxByteBuffer* buff);

            unsigned char hlp_swapBits(unsigned char value);

                int32_t hlp_stringToInt(
        const char* str);

            int hlp_intToString(
        char* str,
        int bufsize,
        int32_t value,
        unsigned char isSigned,
        unsigned char digits);



            /**
    * Convert string to integer.
    *
    * @param str
    *            Parsed string.
    * @return Value of string as integer.
    */
    int32_t hlp_stringToInt2(
        const char* str, const char* end);