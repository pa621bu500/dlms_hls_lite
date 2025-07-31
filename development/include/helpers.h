#include <stdint.h>    
#include "bytebuffer.h"
#include "../include/gxmem.h"

#define GETU32(pt) (((uint32_t)(pt)[0] << 24) | \
                    ((uint32_t)(pt)[1] << 16) | \
                    ((uint32_t)(pt)[2] <<  8) | \
                    ((uint32_t)(pt)[3]))

#define PUT32(ct, st) { \
(ct)[0] = (unsigned char)((st) >> 24); \
(ct)[1] = (unsigned char)((st) >> 16); \
(ct)[2] = (unsigned char)((st) >>  8); \
(ct)[3] = (unsigned char)(st); }

int hlp_hexToBytes(
const char* str,
unsigned char** arr,
uint16_t* count);

unsigned char hlp_getValue(char c);

unsigned char hlp_rand(void);

int hlp_setObjectCount(
uint32_t count,
gxByteBuffer* buff);

int hlp_setLogicalName(unsigned char ln[6], const char* name);

unsigned char hlp_swapBits(unsigned char value);

int hlp_getLogicalNameToString(const unsigned char value[6], char* ln);

int hlp_getObjectCount2(
gxByteBuffer* buff,
uint16_t* count);