

#ifndef VARIANT_H
#define VARIANT_H
#include "enums.h"
#include "bytebuffer.h"
#include <stdint.h>
#include "bitarray.h"
#include <assert.h>

#define V_VT(X) ((X)->vt)
#define GX_UNION(X, Y, Z) \
    V_VT(X) = Z;          \
    (X)->Y
#define GX_INT8(X) GX_UNION(&X, cVal, DLMS_DATA_TYPE_INT8)

typedef struct
{
#ifdef DLMS_IGNORE_MALLOC
    void *data;
#else
    void **data;
#endif // DLMS_IGNORE_MALLOC
    uint16_t capacity;
    uint16_t size;
} variantArray;

typedef struct tagdlmsVARIANT
{
    DLMS_DATA_TYPE vt;
    union
    {
        unsigned char bVal;
        signed char cVal;
        int16_t iVal;
        int32_t lVal;
        int64_t llVal;
#ifndef DLMS_IGNORE_FLOAT32
        float fltVal;
#endif // DLMS_IGNORE_FLOAT32
#ifndef DLMS_IGNORE_FLOAT64
        double dblVal;
#endif // DLMS_IGNORE_FLOAT64
        unsigned char boolVal;
        uint16_t uiVal;
        uint32_t ulVal;
        uint64_t ullVal;
#ifndef DLMS_IGNORE_MALLOC
        gxByteBuffer *strVal;
        gxByteBuffer *strUtfVal;
#endif // DLMS_IGNORE_MALLOC
        variantArray *Arr;
        gxByteBuffer *byteArr;
        bitArray *bitArr;
        unsigned char *pbVal;
        signed char *pcVal;
        int16_t *piVal;
        int32_t *plVal;
        int64_t *pllVal;
#ifndef DLMS_IGNORE_FLOAT32
        float *pfltVal;
#endif // DLMS_IGNORE_FLOAT32
#ifndef DLMS_IGNORE_FLOAT64
        double *pdblVal;
#endif // DLMS_IGNORE_FLOAT64
        unsigned char *pboolVal;
        uint16_t *puiVal;
        uint32_t *pulVal;
        uint64_t *pullVal;
        void *pVal;
    };
#ifdef DLMS_IGNORE_MALLOC
    uint16_t size;
    uint16_t capacity;
#endif // DLMS_IGNORE_MALLOC
} dlmsVARIANT;

typedef dlmsVARIANT *dlmsVARIANT_PTR;

int var_init(
    dlmsVARIANT *data);

int var_addBytes(
    dlmsVARIANT *data,
    const unsigned char *value,
    uint16_t count);

int va_getByIndex(
    variantArray *arr,
    int index,
    dlmsVARIANT_PTR *item);

int va_push(
    variantArray *arr,
    dlmsVARIANT *item);

// Clear variant.
int var_clear(
    dlmsVARIANT *data);

int var_changeType(
    dlmsVARIANT *value,
    DLMS_DATA_TYPE newType);

int var_toInteger(
    dlmsVARIANT *data);

int var_toString(
    dlmsVARIANT *item,
    gxByteBuffer *value);

int var_getBytes(
    dlmsVARIANT *data,
    gxByteBuffer *ba);

// Get bytes from variant value.
int var_getBytes2(
    dlmsVARIANT *data,
    DLMS_DATA_TYPE type,
    gxByteBuffer *ba);

// Get bytes from variant value without data type.
int var_getBytes3(
    dlmsVARIANT *data,
    DLMS_DATA_TYPE type,
    gxByteBuffer *ba,
    unsigned char addType);

// Get bytes from variant value.
int var_getBytes4(
    dlmsVARIANT *data,
    DLMS_DATA_TYPE type,
    gxByteBuffer *ba,
    unsigned char addType,
    unsigned char addArraySize,
    unsigned char addStructureSize);

int var_changeType(
    dlmsVARIANT *value,
    DLMS_DATA_TYPE newType);

int var_copy(
    dlmsVARIANT *target,
    dlmsVARIANT *source);

#endif