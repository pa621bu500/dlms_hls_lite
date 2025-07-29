#include "../include/gxmem.h"
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <assert.h>
#endif

#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include <string.h>
#include "../include/errorcodes.h"
#include "../include/bitarray.h"
#include "../include/helpers.h"

void ba_attach(
    bitArray* arr,
    unsigned char* value,
    uint16_t count,
    uint16_t capacity)
{
    arr->data = value;
    arr->capacity = (uint16_t)(0x8000 | capacity);
    arr->size = count;
    #ifndef GX_DLMS_MICROCONTROLLER
        arr->position = 0;
    #endif //GX_DLMS_MICROCONTROLLER
}

//Return byte index where bit is saved.
int getByteIndex(int bitCount)
{
    double d = bitCount;
    d /= 8;
    return (int)d;
}


int ba_getByIndex(bitArray* arr, int index, unsigned char* value)
{
    char ch;
    if (index >= arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    ch = arr->data[getByteIndex(index)];
    *value = (ch & (1 << (7 - (index % 8)))) != 0;
    return 0;
}

int ba_toInteger(bitArray* arr, uint32_t* value)
{
    *value = 0;
    unsigned char ch;
    int pos, ret;
    for (pos = 0; pos != arr->size; ++pos)
    {
        if ((ret = ba_getByIndex(arr, pos, &ch)) != 0)
        {
            return ret;
        }
        *value |= ch << pos;
    }
    return 0;
}