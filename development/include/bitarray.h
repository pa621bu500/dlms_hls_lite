#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include "bytebuffer.h"

typedef struct
{
    unsigned char *data;
    uint16_t capacity;
    uint16_t size;
#ifndef GX_DLMS_MICROCONTROLLER
    uint16_t position;
#endif // GX_DLMS_MICROCONTROLLER
} bitArray;

void ba_attach(
    bitArray *arr,
    unsigned char *value,
    uint16_t count,
    uint16_t capacity);

int ba_toInteger(
    bitArray *arr,
    uint32_t *value);

#endif