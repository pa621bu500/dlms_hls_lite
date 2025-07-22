#include "../include/gxmem.h"
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#if defined(_WIN64) || defined(_WIN32) || defined(__linux__)
#include <stdio.h>
#endif //defined(_WIN64) || defined(_WIN32) || defined(__linux__)
#include <string.h>
#include "../include/objectarray.h"
#include "../include/helpers.h"
#include "../include/errorcodes.h"

#if defined(_WIN64) || defined(_WIN32) || defined(__linux__)
#include <stdio.h>
#include "../include/helpers.h"
#endif //defined(_WIN64) || defined(_WIN32) || defined(__linux__)


//Initialize objectArray.
void oa_init(objectArray* arr)
{
    arr->capacity = 0;
    arr->data = NULL;
#if !(defined(GX_DLMS_MICROCONTROLLER) || defined(DLMS_IGNORE_MALLOC))
    arr->position = 0;
#endif //!(defined(GX_DLMS_MICROCONTROLLER) || defined(DLMS_IGNORE_MALLOC))
    arr->size = 0;
}

//Get item from object array by index.
int oa_getByIndex(
    const objectArray* arr,
    uint16_t index,
    gxObject** item)
{
    if (index >= arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    *item = (gxObject*)arr->data[index];
    return DLMS_ERROR_CODE_OK;
}

int oa_findByLN(
    objectArray* objects,
    DLMS_OBJECT_TYPE type,
    const unsigned char* ln,
    gxObject** object)
{
    uint16_t pos;
    int ret = DLMS_ERROR_CODE_OK;
    gxObject* obj = NULL;
    *object = NULL;
    if (ln == NULL)
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    for (pos = 0; pos != objects->size; ++pos)
    {
        if ((ret = oa_getByIndex(objects, pos, &obj)) != DLMS_ERROR_CODE_OK)
        {
            break;
        }
        if ((obj->objectType == type || DLMS_OBJECT_TYPE_NONE == type) && memcmp(obj->logicalName, ln, 6) == 0)
        {
            *object = obj;
            break;
        }
    }
    return ret;
}