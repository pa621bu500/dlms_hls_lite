
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

    typedef enum
    {
        /*
         * The output_state is set to false and the consumer is disconnected.
         */
        DLMS_CONTROL_STATE_DISCONNECTED = 0,
        /*
         * The output_state is set to 1 and the consumer is connected.
        */
        DLMS_CONTROL_STATE_CONNECTED,
        /*
         * The output_state is set to false and the consumer is disconnected.
        */
        DLMS_CONTROL_STATE_READY_FOR_RECONNECTION
    } DLMS_CONTROL_STATE;

    typedef enum
    {
        /*
         * The disconnect control object is always in 'connected' state,
        */
        DLMS_CONTROL_MODE_NONE = 0,
        /*
         * Disconnection: Remote (b, c), manual (f), local (g)
         * Reconnection: Remote (d), manual (e).
        */
        DLMS_CONTROL_MODE_MODE_1,
        /*
         * Disconnection: Remote (b, c), manual (f), local (g)
         * Reconnection: Remote (a), manual (e).
        */
        DLMS_CONTROL_MODE_MODE_2,
        /*
         * Disconnection: Remote (b, c), manual (-), local (g)
         * Reconnection: Remote (d), manual (e).
        */
        DLMS_CONTROL_MODE_MODE_3,
        /*
         * Disconnection: Remote (b, c), manual (-), local (g)
         * Reconnection: Remote (a), manual (e)
        */
        DLMS_CONTROL_MODE_MODE_4,
        /*
         * Disconnection: Remote (b, c), manual (f), local (g)
         * Reconnection: Remote (d), manual (e), local (h),
        */
        DLMS_CONTROL_MODE_MODE_5,
        /*
         * Disconnection: Remote (b, c), manual (-), local (g)
         * Reconnection: Remote (d), manual (e), local (h)
        */
        DLMS_CONTROL_MODE_MODE_6,
        /*
         * Disconnection: Remote(b, c), manual(-), local(g)
         * Reconnection: Remote (a, i), manual (e), local (h)
         */
        DLMS_CONTROL_MODE_MODE_7,
    } DLMS_CONTROL_MODE;


    typedef struct
    {
        /*
        * Base class where class is derived.
        */
        gxObject base;
        unsigned char outputState;
        DLMS_CONTROL_STATE controlState;
        DLMS_CONTROL_MODE controlMode;
    } gxDisconnectControl;

    typedef struct
    {
        /*
        * Base class where class is derived.
        */
        gxObject base;
        dlmsVARIANT value;
        signed char scaler;
        unsigned char unit;
        unsigned char unitRead;
    } gxRegister;

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
    

     void obj_clear(gxObject* object);
     
#if _CVI_ //If LabWindows/CVI
#define BASE(X) &X.base
#define INIT_OBJECT(X, Y, Z) cosem_init4(&X, sizeof(X), Y, Z)
#else
#define BASE(X) &X.base
#define INIT_OBJECT(X, Y, Z) cosem_init4(&X.base, sizeof(X), Y, Z)
#endif //_CVI_

#endif