

#ifndef VARIANT_H
#define VARIANT_H
#include "enums.h"
#include "bytebuffer.h"
#include <stdint.h>
#include "bitarray.h"
   typedef struct
    {
#ifdef DLMS_IGNORE_MALLOC
        void* data;
#else
        void** data;
#endif //DLMS_IGNORE_MALLOC
        uint16_t capacity;
        uint16_t size;
    } variantArray;
 typedef struct tagdlmsVARIANT
    {
        // DLMS_DATA_TYPE vt;
        union
        {
            gxByteBuffer* byteArr;
            gxByteBuffer* strUtfVal;
            gxByteBuffer* strVal;
            variantArray* Arr;
            // bitArray* bitArr;
        };
    } dlmsVARIANT;

    //  typedef enum
    // {
    //     // DLMS_DATA_TYPE_NONE = 0,
    //     DLMS_DATA_TYPE_ARRAY = 1,
    //     DLMS_DATA_TYPE_STRUCTURE = 2,
    //     // DLMS_DATA_TYPE_BOOLEAN = 3,
    //     DLMS_DATA_TYPE_BIT_STRING = 4,
    //     // DLMS_DATA_TYPE_INT32 = 5,
    //     // DLMS_DATA_TYPE_UINT32 = 6,
    //     // DLMS_DATA_TYPE_OCTET_STRING = 9,
    //     // DLMS_DATA_TYPE_STRING = 10,
    //     DLMS_DATA_TYPE_STRING_UTF8 = 12,
    //     // DLMS_DATA_TYPE_BINARY_CODED_DESIMAL = 13,
    //     // DLMS_DATA_TYPE_INT8 = 15,
    //     // DLMS_DATA_TYPE_INT16 = 16,
    //     // DLMS_DATA_TYPE_UINT8 = 17,
    //     // DLMS_DATA_TYPE_UINT16 = 18,
    //     DLMS_DATA_TYPE_COMPACT_ARRAY = 19,
    //     // DLMS_DATA_TYPE_INT64 = 20,
    //     // DLMS_DATA_TYPE_UINT64 = 21,
    //     // DLMS_DATA_TYPE_ENUM = 22,
    //     // DLMS_DATA_TYPE_FLOAT32 = 23,
    //     // DLMS_DATA_TYPE_FLOAT64 = 24,
    //     DLMS_DATA_TYPE_DATETIME = 25,
    //     DLMS_DATA_TYPE_DATE = 26,
    //     // DLMS_DATA_TYPE_TIME = 27,
    //     // DLMS_DATA_TYPE_DELTA_INT8 = 28,
    //     // DLMS_DATA_TYPE_DELTA_INT16 = 29,
    //     // DLMS_DATA_TYPE_DELTA_INT32 = 30,
    //     // DLMS_DATA_TYPE_DELTA_UINT8 = 31,
    //     // DLMS_DATA_TYPE_DELTA_UINT16 = 32,
    //     // DLMS_DATA_TYPE_DELTA_UINT32 = 33,
    //     DLMS_DATA_TYPE_BYREF = 0x80
    // } DLMS_DATA_TYPE;


int var_init(
dlmsVARIANT* data);

//Clear variant.
int var_clear(
dlmsVARIANT* data);


#endif