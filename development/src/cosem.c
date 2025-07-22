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