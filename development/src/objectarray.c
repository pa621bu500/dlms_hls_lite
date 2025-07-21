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