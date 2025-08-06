#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <assert.h>
#endif

#include "../include/gxmem.h"
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include <string.h>
#include "../include/enums.h"
#include "../include/dlms.h"
#include "../include/cosem.h"
#include "../include/helpers.h"

int cosem_init(
    gxObject* object,
    DLMS_OBJECT_TYPE type,
    const char* logicalNameString)
{
    unsigned char ln[6];
    hlp_setLogicalName(ln, logicalNameString);
    return cosem_init2(object, type, ln);
}

int cosem_init2(
    gxObject* object,
    DLMS_OBJECT_TYPE type,
    const unsigned char* ln)
{
    return cosem_init3(object, 0, type, ln);
}

int cosem_init3(
    gxObject* object,
    const uint16_t expectedSize,
    DLMS_OBJECT_TYPE type,
    const unsigned char* ln)
{
    return cosem_init4((void*)object, expectedSize, type, ln);
}

uint16_t cosem_getObjectSize(DLMS_OBJECT_TYPE type)
{
    int size = 0;
    switch (type)
    {
        #ifndef DLMS_IGNORE_DATA
        case DLMS_OBJECT_TYPE_DATA:
            size = sizeof(gxData);
            break;
        #endif //DLMS_IGNORE_DATA
        #ifndef DLMS_IGNORE_REGISTER
        case DLMS_OBJECT_TYPE_REGISTER:
            size = sizeof(gxRegister);
            break;
        #endif //DLMS_IGNORE_REGISTER
        default:
           return 0;
        
    }
    return size;
}

int cosem_init4(
    void* object,
    const uint16_t expectedSize,
    DLMS_OBJECT_TYPE type,
    const unsigned char* ln)
{
    uint16_t size = cosem_getObjectSize(type);
    if (size == 0)
    {
        printf("error in cosem.c");
        // return DLMS_ERROR_CODE_UNAVAILABLE_OBJECT;
    }
    if (expectedSize != 0 && size != expectedSize)
    {
        printf("error in cosem.c");
        // return DLMS_ERROR_CODE_UNMATCH_TYPE;
    }
    memset(object, 0, size);
    ((gxObject*)object)->objectType = type;
    ((gxObject*)object)->logicalName[0] = ln[0];
    ((gxObject*)object)->logicalName[1] = ln[1];
    ((gxObject*)object)->logicalName[2] = ln[2];
    ((gxObject*)object)->logicalName[3] = ln[3];
    ((gxObject*)object)->logicalName[4] = ln[4];
    ((gxObject*)object)->logicalName[5] = ln[5];
    //Set default values, if any.
    switch (type)
    {
    case DLMS_OBJECT_TYPE_DATA:
        break;
    case DLMS_OBJECT_TYPE_REGISTER:
        break;
    default:
        break;
    }
    return 0;
}


int cosem_getOctetStringBase(gxByteBuffer* bb,
    gxByteBuffer* value,
    unsigned char type,
    unsigned char exact)
{
    int ret;
    unsigned char tmp;
    uint16_t count;
    if ((ret = bb_getUInt8(bb, &tmp)) != 0)
    {
        return ret;
    }
    if (tmp != type)
    {
        return DLMS_ERROR_CODE_UNMATCH_TYPE;
    }
    if ((ret = hlp_getObjectCount2(bb, &count)) != 0)
    {
        return ret;
    }
    if ((exact && count != bb_getCapacity(value)) ||
        //Octet-string is too big.
        count > bb_getCapacity(value))
    {
        return DLMS_ERROR_CODE_INCONSISTENT_CLASS_OR_OBJECT;
    }
    if ((ret = bb_clear(value)) != 0 ||
        (ret = bb_set2(value, bb, bb->position, count)) != 0)
    {
        return ret;
    }
    return 0;
}

int cosem_getOctetString(gxByteBuffer* bb, gxByteBuffer* value)
{
    return cosem_getOctetStringBase(bb, value, DLMS_DATA_TYPE_OCTET_STRING, 0);
}