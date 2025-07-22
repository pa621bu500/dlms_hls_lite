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