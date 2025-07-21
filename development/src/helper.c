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