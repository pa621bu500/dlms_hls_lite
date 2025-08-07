#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <stdint.h>
#define VECTOR_CAPACITY 50
#define BYTE_BUFFER_INIT bb_init
#define BB_ATTACH(X, V, S) bb_attach(&X, V, S, sizeof(V))
typedef struct
{
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    unsigned char *data;
    uint32_t capacity;
    uint32_t size;
    uint32_t position;
#else
    unsigned char *data;
    uint16_t capacity;
    uint16_t size;
    uint16_t position;
#endif
} gxByteBuffer;

char *bb_toHexString(
    gxByteBuffer *bb);

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
unsigned char bb_compare(
    gxByteBuffer *bb,
    unsigned char *buff,
    uint32_t length);
#else
unsigned char bb_compare(
    gxByteBuffer *bb,
    unsigned char *buff,
    uint16_t length);
#endif

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_insert(
    const unsigned char *src,
    uint32_t count,
    gxByteBuffer *target,
    uint32_t index);

#else
int bb_insert(
    const unsigned char *src,
    uint16_t count,
    gxByteBuffer *target,
    uint16_t index);
#endif

char bb_isAttached(
    gxByteBuffer *arr);

char *bb_toString(
    gxByteBuffer *bb);

uint32_t bb_getCapacity(
    gxByteBuffer *arr);

int bb_clear(
    gxByteBuffer *bb);

int bb_setUInt8(
    gxByteBuffer *bb,
    unsigned char item);

int bb_setInt8(
    gxByteBuffer *bb,
    char item);

int bb_getUInt8(
    gxByteBuffer *bb,
    unsigned char *value);

int bb_getUInt8ByIndex(
    gxByteBuffer *bb,
    uint32_t index,
    unsigned char *value);

int bb_getUInt16(
    gxByteBuffer *bb,
    uint16_t *value);

int bb_getUInt32(
    gxByteBuffer *bb,
    uint32_t *value);

int bb_setUInt32(
    gxByteBuffer *bb,
    uint32_t item);

int bb_get(
    gxByteBuffer *bb,
    unsigned char *value,
    uint32_t count);

int bb_setUInt32ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    uint32_t item);

int bb_getUInt16ByIndex(
    gxByteBuffer *bb,
    uint32_t index,
    uint16_t *value);

uint32_t bb_size(
    gxByteBuffer *bb);

int bb_getUInt8ByIndex(
    gxByteBuffer *bb,
    uint32_t index,
    unsigned char *value);

int bb_capacity(
    gxByteBuffer *bb,
    uint32_t capacity);

#ifndef DLMS_IGNORE_MALLOC
// Add hex string to byte buffer.
int bb_addHexString(
    gxByteBuffer *arr,
    const char *str);
#endif // DLMS_IGNORE_MALLOC

int bb_attach(
    gxByteBuffer *arr,
    unsigned char *value,
    uint32_t count,
    uint32_t capacity);

uint32_t bb_available(gxByteBuffer *arr);

int bb_set2(
    gxByteBuffer *bb,
    gxByteBuffer *data,
    uint32_t index,
    uint32_t count);

int BYTE_BUFFER_INIT(
    gxByteBuffer *bb);

int bb_addString(
    gxByteBuffer *bb,
    const char *value);

int bb_set(
    gxByteBuffer *bb,
    const unsigned char *pSource,
    uint32_t count);

int bb_setUInt16(
    gxByteBuffer *bb,
    uint16_t item);

int bb_setUInt16ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    uint16_t item);

int bb_move(
    gxByteBuffer *ba,
    uint32_t srcPos,
    uint32_t destPos,
    uint32_t count);

#endif