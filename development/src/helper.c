#include <stdint.h>
#include <stddef.h>
#include "../include/errorcodes.h"
#include "../include/helpers.h"
#include "../include/gxmem.h"

int hlp_hexToBytes(
    const char* str,
    unsigned char** buffer,
    uint16_t* count)
{
    *count = 0;
    if (buffer != NULL && *buffer != NULL)
    {
        gxfree(*buffer);
    }
    if (str == NULL)
    {
        return 0;
    }
    int len = (int)strlen(str);
    if (len == 0)
    {
        return 0;
    }
    unsigned char* tmp = (unsigned char*)gxmalloc(len / 2);
    if (tmp == NULL)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *buffer = tmp;
    int lastValue = -1;
    for (int pos = 0; pos != len; ++pos)
    {
        if (*str >= '0' && *str < 'g')
        {
            if (lastValue == -1)
            {
                lastValue = hlp_getValue(*str);
            }
            else if (lastValue != -1)
            {
                tmp[*count] = (unsigned char)(lastValue << 4 | hlp_getValue(*str));
                lastValue = -1;
                ++*count;
            }
        }
        else if (lastValue != -1)
        {
            tmp[*count] = hlp_getValue(*str);
            lastValue = -1;
            ++*count;
        }
        ++str;
    }
    if (len / 2 != *count)
    {
#ifdef gxrealloc
        //If compiler supports realloc.
        * buffer = gxrealloc(*buffer, *count);
        if (tmp == NULL)
        {
            return DLMS_ERROR_CODE_OUTOFMEMORY;
        }
#else
        //A few extra bytes are returned if compiler doesn't support realloc.
#endif // gxrealloc  
    }
    return 0;
}

static uint16_t lfsr = 0xACE1u;
unsigned bit;
unsigned char hlp_rand(void)
{
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    return (unsigned char)(lfsr = (uint16_t)((lfsr >> 1) | (bit << 15)));
}


unsigned char hlp_getValue(char c)
{
    unsigned char value;
    if (c > '9')
    {
        if (c > 'Z')
        {
            value = (c - 'a' + 10);
        }
        else
        {
            value = (c - 'A' + 10);
        }
    }
    else
    {
        value = (c - '0');
    }
    return value;
}


// Set count of items.
int hlp_setObjectCount(uint32_t count, gxByteBuffer* buff)
{
    int ret;
    if (count < 0x80)
    {
        ret = bb_setUInt8(buff, (unsigned char)count);
    }
    // else if (count < 0x100)
    // {
    //     if ((ret = bb_setUInt8(buff, 0x81)) == 0)
    //     {
    //         ret = bb_setUInt8(buff, (unsigned char)count);
    //     }
    // }
    // else if (count < 0x10000)
    // {
    //     if ((ret = bb_setUInt8(buff, 0x82)) == 0)
    //     {
    //         ret = bb_setUInt16(buff, (uint16_t)count);
    //     }
    // }
    // else
    // {
    //     if ((ret = bb_setUInt8(buff, 0x84)) == 0)
    //     {
    //         ret = bb_setUInt32(buff, count);
    //     }
    // }
    return ret;
}


unsigned char hlp_swapBits(unsigned char value)
{
    unsigned char ret = 0, pos;
    for (pos = 0; pos != 8; ++pos)
    {
        ret = (unsigned char)((ret << 1) | (value & 0x01));
        value = (unsigned char)(value >> 1);
    }
    return ret;
}

int hlp_getObjectCount2(gxByteBuffer* buff, uint16_t* count)
{
    int ret;
    unsigned char ch;
    ret = bb_getUInt8(buff, &ch);
    if (ret != 0)
    {
        return ret;
    }
    else
    {
        *count = ch;
    }
    return ret;
}


int hlp_getLogicalNameToString(const unsigned char value[6], char* ln)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64)
#if _MSC_VER > 1000
    ret = sprintf_s(ln, 25, "%d.%d.%d.%d.%d.%d", value[0], value[1], value[2], value[3], value[4], value[5]);
#else
    ret = sprintf(ln, "%d.%d.%d.%d.%d.%d", value[0], value[1], value[2], value[3], value[4], value[5]);
#endif  
    if (ret != -1)
    {
        ret = 0;
    }
#else
    ret = hlp_intToString(ln, 25, value[0], 0, 1);
    if (ret != -1)
    {
        ln += ret;
        *ln = '.';
        ++ln;
        ret = hlp_intToString(ln, 25, value[1], 0, 1);
        if (ret != -1)
        {
            ln += ret;
            *ln = '.';
            ++ln;
            ret = hlp_intToString(ln, 25, value[2], 0, 1);
            if (ret != -1)
            {
                ln += ret;
                *ln = '.';
                ++ln;
                ret = hlp_intToString(ln, 25, value[3], 0, 1);
                if (ret != -1)
                {
                    ln += ret;
                    *ln = '.';
                    ++ln;
                    ret = hlp_intToString(ln, 25, value[4], 0, 1);
                    if (ret != -1)
                    {
                        ln += ret;
                        *ln = '.';
                        ++ln;
                        ret = hlp_intToString(ln, 25, value[5], 0, 1);
                    }
                }
            }
        }
    }
    if (ret != -1)
    {
        ret = 0;
    }
#endif //WIN32
    return ret;
}
