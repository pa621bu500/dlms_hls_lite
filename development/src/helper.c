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
    else if (count < 0x100)
    {
        if ((ret = bb_setUInt8(buff, 0x81)) == 0)
        {
            ret = bb_setUInt8(buff, (unsigned char)count);
        }
    }
    else if (count < 0x10000)
    {
        if ((ret = bb_setUInt8(buff, 0x82)) == 0)
        {
            ret = bb_setUInt16(buff, (uint16_t)count);
        }
    }
    else
    {
        if ((ret = bb_setUInt8(buff, 0x84)) == 0)
        {
            ret = bb_setUInt32(buff, count);
        }
    }
    return ret;
}

char* hlp_bytesToHex(const unsigned char* bytes, int count)
{
    const char hexArray[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    unsigned char tmp;
    int pos;
    char* hexChars;
    if (count != 0)
    {
        hexChars = (char*)gxmalloc(3 * count);
        if (hexChars != NULL)
        {
            for (pos = 0; pos != count; ++pos)
            {
                tmp = bytes[pos] & 0xFF;
                hexChars[pos * 3] = hexArray[tmp >> 4];
                hexChars[pos * 3 + 1] = hexArray[tmp & 0x0F];
                hexChars[pos * 3 + 2] = ' ';
            }
            hexChars[(3 * count) - 1] = '\0';
        }
    }
    else
    {
        hexChars = (char*)gxmalloc(1);
        hexChars[0] = '\0';
    }
    return hexChars;
}

int hlp_setLogicalName(unsigned char ln[6], const char* name)
{
    char* ch;
    char* pOriginalBuff;
    char* pBuff;
    int val = 0, count = 0, size = (int)strlen(name);
    if (size < 11)
    {
        return -1;
    }
    pBuff = (char*)gxmalloc(size + 1);
    pOriginalBuff = pBuff;
    memcpy(pBuff, name, size);
    pBuff[size] = 0;
    //AVR compiler can't handle this if casting to char* is removed.
    while ((ch = (char*)strchr(pBuff, '.')) != NULL)
    {
        *ch = '\0';
        val = hlp_stringToInt(pBuff);
        if (val == -1)
        {
            gxfree(pOriginalBuff);
            return -1;
        }
        ln[count] = (unsigned char)val;
        pBuff = ch + sizeof(char);
        ++count;
    }
    if (count == 5)
    {
        val = hlp_stringToInt(pBuff);
        if (val == -1)
        {
            gxfree(pOriginalBuff);
            return -1;
        }
        ln[count] = (unsigned char)val;
        pBuff = ch + sizeof(char);
        ++count;
    }
    gxfree(pOriginalBuff);
    if (count != 6)
    {
        memset(ln, 0, 6);
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
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

int hlp_intToString(char* str, int bufsize, int32_t value, unsigned char isSigned, unsigned char digits)
{
    int cnt = 0;
    int32_t val = value;
    if (isSigned && value < 0)
    {
        if (bufsize < 1)
        {
            return -1;
        }
        *str = '-';
        ++str;
        --bufsize;
        value = -value;
        val = value;
        ++cnt;
    }
    if (digits != 0)
    {
        --digits;
    }
    //Find length.
    while ((val = (val / 10)) > 0)
    {
        ++str;
        if (digits != 0)
        {
            --digits;
        }
    }
    *(str + digits + 1) = '\0';
    while (digits != 0)
    {
        if (bufsize < 1)
        {
            return -1;
        }
        *str = '0';
        --digits;
        --bufsize;
        ++str;
        ++cnt;
    }
    do
    {
        if (bufsize < 1)
        {
            return -1;
        }
        *str = (value % 10) + '0';
        value /= 10;
        if (value != 0)
        {
            --str;
        }
        --bufsize;
        ++cnt;
    } while (value != 0);
    return cnt;
}


int32_t hlp_stringToInt2(const char* str, const char* end)
{
    if (str == NULL)
    {
        return -1;
    }
    int32_t value = 0;
    unsigned char minus = 0;
    if (*str == '-')
    {
        minus = 1;
        ++str;
    }
    while (*str != '\0' && str != end)
    {
        if (*str < '0' || *str > '9')
        {
            return -1;
        }
        value *= 10;
        value += *str - '0';
        ++str;
    }
    if (minus)
    {
        return -value;
    }
    return value;
}

int32_t hlp_stringToInt(const char* str)
{
    return hlp_stringToInt2(str, NULL);
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
    if (ch > 0x80)
    {
        if (ch == 0x81)
        {
            ret = bb_getUInt8(buff, &ch);
            *count = ch;
        }
        else if (ch == 0x82)
        {
            ret = bb_getUInt16(buff, count);
        }
        else if (ch == 0x84)
        {
            uint32_t value;
            ret = bb_getUInt32(buff, &value);
            *count = (uint16_t)value;
        }
        else
        {
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    else
    {
        *count = ch;
    }
    return ret;
}

int hlp_uint64ToString(char* str, int bufsize, uint64_t value, unsigned char digits)
{
    int cnt = 0;
    uint64_t val = value;
    if (digits != 0)
    {
        --digits;
    }
    //Find length.
    while ((val = (val / 10)) > 0)
    {
        ++str;
        if (digits != 0)
        {
            --digits;
        }
    }
    *(str + digits + 1) = '\0';
    while (digits != 0)
    {
        if (bufsize < 1)
        {
            return -1;
        }
        *str = '0';
        --digits;
        --bufsize;
        ++str;
        ++cnt;
    }
    do
    {
        if (bufsize < 1)
        {
            return -1;
        }
        *str = (value % 10) + '0';
        value /= 10;
        if (value != 0)
        {
            --str;
        }
        --bufsize;
        ++cnt;
    } while (value != 0);
    return cnt;
}


double hlp_getScaler(int scaler)
{
    //If OS
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
    return pow((float)10, scaler);
#else
    double ret = 1;
    if (scaler > 0)
    {
        while (scaler--)
        {
            ret *= 10;
        }
    }
    else if (scaler < 0)
    {
        while (scaler++)
        {
            ret /= 10;
        }
    }
    return ret;
#endif
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
