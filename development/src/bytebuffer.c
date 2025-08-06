
#include <stdio.h> //printf needs this or error is generated.
#include "../include/bytebuffer.h"
#include "../include/errorcodes.h"
#include "../include/gxmem.h"
#include "../include/helpers.h"
#include "../include/variant.h"
#include "../include/datainfo.h"
#include <assert.h>
#include <string.h>

int BYTE_BUFFER_INIT(
    gxByteBuffer *arr)
{
    arr->capacity = 0;
    arr->data = NULL;
    arr->position = 0;
    arr->size = 0;
    return 0;
}


int getUInt32(gxByteBuffer* buff, gxDataInfo* info, dlmsVARIANT* value)
{
    int ret;
    // If there is not enough data available.
    if (buff->size - buff->position < 4)
    {
        info->complete = 0;
        return 0;
    }
    if ((value->vt & DLMS_DATA_TYPE_BYREF) == 0)
    {
        value->vt = DLMS_DATA_TYPE_UINT32;
    }
    if ((ret = bb_getUInt32(buff, (value->vt & DLMS_DATA_TYPE_BYREF) == 0 ? &value->ulVal : value->pulVal)) != 0)
    {
        return ret;
    }
    return DLMS_ERROR_CODE_OK;
}

char bb_isAttached(gxByteBuffer *arr)
{
    if (arr == NULL)
    {
        return 0;
    }
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    // If byte buffer is attached.
    return (arr->capacity & 0x80000000) == 0x80000000;
#else
    return (arr->capacity & 0x8000) == 0x8000;
#endif
}

#ifndef DLMS_IGNORE_MALLOC
int bb_addHexString(
    gxByteBuffer *arr,
    const char *str)
{
    uint16_t count;
    int ret;
    unsigned char *buffer = NULL;
    ret = hlp_hexToBytes(str, &buffer, &count);
    if (ret != 0)
    {
        return ret;
    }
    if (buffer != NULL)
    {
        bb_set(arr, buffer, count);
        gxfree(buffer);
    }
    return 0;
}
#endif // DLMS_IGNORE_MALLOC

int bb_clear(

    gxByteBuffer *arr)
{
#ifndef DLMS_IGNORE_MALLOC
    // If byte buffer is attached.
    if (!bb_isAttached(arr))
    {
        if (arr->data != NULL)
        {
            // gxfree(arr->data);
            arr->data = NULL;
        }
        arr->capacity = 0;
    }
#endif // DLMS_IGNORE_MALLOC
    arr->size = 0;
    arr->position = 0;
    return 0;
}

char* bb_toHexString(
    gxByteBuffer* arr)
{
    char* buff = hlp_bytesToHex(arr->data, arr->size);
    return buff;
}

int bb_getUInt8(
    gxByteBuffer *arr,
    unsigned char *value)
{
    if (arr->position >= arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *value = ((unsigned char *)arr->data)[arr->position];
    ++arr->position;
    return 0;
}

int bb_getUInt8ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    unsigned char *value)
{
    if (index >= arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *value = ((unsigned char *)arr->data)[index];
    return 0;
}

int bb_getUInt16(
    gxByteBuffer *arr,
    uint16_t *value)
{
    if (arr->position + 2 > arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *value = (uint16_t)(((unsigned char *)arr->data)[arr->position] << 8 |
                        ((unsigned char *)arr->data)[arr->position + 1]);
    arr->position += 2;
    return 0;
}

int bb_setUInt8ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    unsigned char item)
{
    if (arr == NULL)
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    int ret = bb_allocate(arr, index, 1);
    if (ret == 0)
    {
        arr->data[index] = item;
    }
    return ret;
}

int bb_getUInt16ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    uint16_t *value)
{
    if (index + 2 > arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *value = (uint16_t)(((unsigned char *)arr->data)[index] << 8 |
                        ((unsigned char *)arr->data)[index + 1]);
    return 0;
}

int bb_getUInt32(
    gxByteBuffer *arr,
    uint32_t *value)
{

    if (arr->position + 4 > arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *value = GETU32(arr->data + arr->position);
    arr->position += 4;
    return 0;
}

int bb_setUInt8(
    gxByteBuffer *arr,
    unsigned char item)
{
    int ret = bb_setUInt8ByIndex(arr, bb_size(arr), item);
    if (ret == 0)
    {
        ++arr->size;
    }
    return ret;
}


int bb_get(
    gxByteBuffer* bb,
    unsigned char* value,
    uint32_t count)
{
    if (bb == NULL || value == NULL || bb->size - bb->position < count)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    memcpy(value, bb->data + bb->position, count);
    bb->position += count;
    return 0;
}


uint32_t bb_size(gxByteBuffer *arr)
{
    return arr != NULL ? arr->size : 0;
}

int bb_attach(
    gxByteBuffer *arr,
    unsigned char *value,
    uint32_t count,
    uint32_t capacity)
{
    // If capacity is 1 value is cast t
    if (value == NULL || capacity < count)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    arr->data = value;
    if (capacity >= 0x80000000)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    arr->capacity = (0x80000000 | capacity);
    arr->size = count;
    arr->position = 0;
    return 0;
}

uint32_t bb_available(gxByteBuffer *arr)
{
    if (arr == NULL)
    {
        return 0;
    }
    return arr->size - arr->position;
}

int bb_set2(
    gxByteBuffer *arr,
    gxByteBuffer *data,
    uint32_t index,
    uint32_t count)

{
    if (data != NULL && count != 0)
    {

        if (count == (uint32_t)-1)

        {
            count = data->size - index;
        }
        int ret = bb_set(arr, data->data + index, count);
        if (ret == 0)
        {
            data->position += count;
        }
        return ret;
    }
    return 0;
}

// int bb_getUInt8ByIndex(
//     gxByteBuffer* arr,
//     uint32_t index,
//     unsigned char* value)
// {
//     if (index >= arr->size)
//     {
//         return DLMS_ERROR_CODE_OUTOFMEMORY;
//     }
//     *value = ((unsigned char*)arr->data)[index];
//     return 0;
// }
int bb_setUInt16(
    gxByteBuffer *arr,
    uint16_t item)
{
    int ret = bb_setUInt16ByIndex(arr, arr->size, item);
    if (ret == 0)
    {
        arr->size += 2;
    }
    return ret;
}

int bb_addString(
    gxByteBuffer *arr,
    const char *value)
{
    if (value != NULL)
    {
        int len = (int)strlen(value);
        if (len > 0)
        {
            int ret = bb_set(arr, (const unsigned char *)value, (uint16_t)(len + 1));
            if (ret == 0)
            {
                // Add end of string, but that is not added to the length.
                arr->data[arr->size - 1] = '\0';
                --arr->size;
            }
            return ret;
        }
    }
    return 0;
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_setUInt32ByIndex(
    gxByteBuffer* arr,
    uint32_t index,
    uint32_t item)
#else
int bb_setUInt32ByIndex(
    gxByteBuffer* arr,
    uint16_t index,
    uint32_t item)
#endif //defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
{
    int ret = bb_allocate(arr, index, 4);
    if (ret == 0)
    {
        PUT32(arr->data + index, item);
    }
    return ret;
}


int bb_setUInt32(
    gxByteBuffer* arr,
    uint32_t item)
{
    int ret = bb_setUInt32ByIndex(arr, arr->size, item);
    if (ret == 0)
    {
        arr->size += 4;
    }
    return ret;
}



#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_capacity(
    gxByteBuffer *arr,
    uint32_t capacity)
#else
int bb_capacity(
    gxByteBuffer *arr,
    uint16_t capacity)
#endif
{
#ifndef DLMS_IGNORE_MALLOC
    // Capacity can't change if it's attached.
    if (!bb_isAttached(arr))
    {
        if (capacity == 0)
        {
            if (arr->data != NULL)
            {
                gxfree(arr->data);
                arr->data = NULL;
                arr->size = 0;
            }
        }
        else
        {
            if (arr->capacity == 0)
            {
                arr->data = (unsigned char *)gxmalloc(capacity);
                if (arr->data == NULL)
                {
                    return DLMS_ERROR_CODE_OUTOFMEMORY;
                }
            }
            else
            {
                unsigned char *old = arr->data;
#ifdef gxrealloc

                // If compiler supports realloc.
                arr->data = (unsigned char *)gxrealloc(arr->data, capacity);
                // If not enought memory available.
                if (arr->data == NULL)
                {
                    arr->data = old;
                    return DLMS_ERROR_CODE_OUTOFMEMORY;
                }
#else
                // If compiler doesn't support realloc.
                arr->data = (unsigned char *)gxmalloc(capacity);
                // If not enought memory available.
                if (arr->data == NULL)
                {
                    arr->data = old;
                    return DLMS_ERROR_CODE_OUTOFMEMORY;
                }
                memcpy(arr->data, old, arr->size);
                gxfree(old);
#endif // gxrealloc
            }
            if (arr->size > capacity)
            {
                arr->size = capacity;
            }
        }
        arr->capacity = capacity;
    }
#endif // DLMS_IGNORE_MALLOC
    if (bb_getCapacity(arr) < capacity)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    return DLMS_ERROR_CODE_OK;
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
unsigned char bb_compare(
    gxByteBuffer *bb,
    unsigned char *buff,
    uint32_t length)
#else
unsigned char bb_compare(
    gxByteBuffer *bb,
    unsigned char *buff,
    uint16_t length)
#endif

{
    unsigned char equal;
    if (bb_available(bb) != length)
    {
        return 0;
    }
    equal = memcmp(bb->data + bb->position, buff, length) == 0;
    if (equal)
    {
        bb->position += length;
    }
    return equal;
}

uint32_t bb_getCapacity(gxByteBuffer *arr)
{

    return arr->capacity & 0x7FFFFFFF;
}

int bb_set(
    gxByteBuffer *arr,
    const unsigned char *pSource,
    uint32_t count)
{
    if (count != 0)
    {
        int ret = bb_allocate(arr, arr->size, count);
        if (ret == 0)
        {
            if (arr->size + count > arr->capacity)
            {
                return DLMS_ERROR_CODE_OUTOFMEMORY;
            }
            memcpy(arr->data + arr->size, pSource, count);
            arr->size += count;
        }
        return ret;
    }
    else
    {
        return 0;
    }
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_allocate(
    gxByteBuffer *arr,
    uint32_t index,
    uint32_t dataSize)
#else
// EVS2 NOT SUPPORTED
#endif
{
#ifndef DLMS_IGNORE_MALLOC
    if (!bb_isAttached(arr) && (arr->capacity == 0 || index + dataSize > arr->capacity))
    {
        unsigned char empty = arr->capacity == 0;
        // If data is append fist time.
        if (!(dataSize > VECTOR_CAPACITY || arr->capacity == 0))
        {
            dataSize = VECTOR_CAPACITY;
        }
        arr->capacity += dataSize;
        if (empty)
        {
            arr->data = (unsigned char *)gxmalloc(arr->capacity);
            if (arr->data == NULL)
            {
                arr->capacity -= dataSize;
                return DLMS_ERROR_CODE_OUTOFMEMORY;
            }
        }
        else
        {
            unsigned char *old = arr->data;
#ifdef gxrealloc
            // If compiler supports realloc.
            arr->data = (unsigned char *)gxrealloc(arr->data, arr->capacity);
            if (arr->data == NULL)
            {
                arr->capacity -= dataSize;
                arr->data = old;
                return DLMS_ERROR_CODE_OUTOFMEMORY;
            }
#else
            // EVS2 NOT SUPPORTED
#endif // gxrealloc
        }
    }
#endif // DLMS_IGNORE_MALLOC
    if (bb_getCapacity(arr) < index + dataSize)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    return 0;
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_setUInt16ByIndex(
    gxByteBuffer *arr,
    uint32_t index,
    uint16_t item)
#else
int bb_setUInt16ByIndex(
    gxByteBuffer *arr,
    uint16_t index,
    uint16_t item)
#endif
{
    int ret = 0;
    if (index + 2 > arr->size)
    {
        ret = bb_allocate(arr, arr->size, 2);
    }
    if (ret == 0)
    {
        arr->data[index] = (item >> 8) & 0xFF;
        arr->data[index + 1] = item & 0xFF;
    }
    return ret;
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_move(
    gxByteBuffer *bb,
    uint32_t srcPos,
    uint32_t destPos,
    uint32_t count)
#else
int bb_move(
    gxByteBuffer *bb,
    uint16_t srcPos,
    uint16_t destPos,
    uint16_t count)
#endif
{
    // If items are removed.
    if (srcPos > destPos)
    {
        if (bb->size < destPos + count)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    else
    {
        // Append data.
        if (bb_getCapacity(bb) < count + destPos)
        {
            int ret;
            if (bb_isAttached(bb))
            {
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
            if ((ret = bb_capacity(bb, count + destPos)) != 0)
            {
                return ret;
            }
        }
    }
    if (count != 0)
    {
        // Do not use memcpy here!
        memmove(bb->data + destPos, bb->data + srcPos, count);
        bb->size = (destPos + count);
        if (bb->position > bb->size)
        {
            bb->position = bb->size;
        }
    }
    return DLMS_ERROR_CODE_OK;
}

#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int bb_insert(const unsigned char *src,
              uint32_t count,
              gxByteBuffer *target,
              uint32_t index)
#else
int bb_insert(const unsigned char *src,
              uint16_t count,
              gxByteBuffer *target,
              uint16_t index)
#endif
{
    int ret;
    if (target->size == 0)
    {
        ret = bb_set(target, src, count);
    }
    else
    {
        if ((ret = bb_capacity(target, target->size + count)) == 0 &&
            (ret = bb_move(target, index, index + count, target->size - index)) == 0)
        {
            // Do not use memcpy here!
            memmove(target->data + index, src + index, count);
        }
    }
    return ret;
}