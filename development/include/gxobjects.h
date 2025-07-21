
#include <stdint.h>
#include "bytebuffer.h"
#include "variant.h"

#ifndef GXOBJECTS_H
#define GXOBJECTS_H

   typedef struct
    {
        gxByteBuffer attributeAccessModes;
        gxByteBuffer methodAccessModes;
    }gxAccess;
   typedef struct
    {
        uint16_t objectType;
        unsigned char version;
#ifndef DLMS_IGNORE_ASSOCIATION_SHORT_NAME
        uint16_t shortName;
#endif // DLMS_IGNORE_ASSOCIATION_SHORT_NAME
        unsigned char logicalName[6];
        gxAccess* access;
    } gxObject;

    typedef struct
    {
        /*
        * Base class where class is derived.
        */
        gxObject base;
        dlmsVARIANT value;
    } gxData;

    typedef struct
    {
        gxObject** data;
        uint16_t capacity;
        uint16_t size;
#if !(defined(GX_DLMS_MICROCONTROLLER) || defined(DLMS_IGNORE_MALLOC))
        uint16_t position;
#endif //!(defined(GX_DLMS_MICROCONTROLLER) || defined(DLMS_IGNORE_MALLOC))
    } objectArray;


#endif