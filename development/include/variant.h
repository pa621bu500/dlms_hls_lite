

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
        DLMS_DATA_TYPE vt;
        union
        {
            int64_t llVal;
            gxByteBuffer* byteArr;
            gxByteBuffer* strUtfVal;
            gxByteBuffer* strVal;
            uint32_t ulVal;
            variantArray* Arr;
            void* pVal;
            uint32_t* pulVal;
            // bitArray* bitArr;
        };
    } dlmsVARIANT;

typedef dlmsVARIANT* dlmsVARIANT_PTR;


int var_init(
dlmsVARIANT* data);

//Clear variant.
int var_clear(
dlmsVARIANT* data);

int var_changeType(
dlmsVARIANT* value,
DLMS_DATA_TYPE newType);

int var_toInteger(
    dlmsVARIANT* data);


int var_copy(
dlmsVARIANT* target,
dlmsVARIANT* source);

#endif