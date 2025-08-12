#if defined(_WIN32) || defined(_WIN64)
#include <Ws2tcpip.h>
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <assert.h>
#include <stdio.h> //printf needs this or error is generated.
#endif
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif

#include "../include/gxobjects.h"
#ifndef DLMS_IGNORE_MALLOC
#include "../include/gxmem.h"
#ifndef DLMS_IGNORE_STRING_CONVERTER
#include <string.h>
#include "../include/objectarray.h"
#endif // DLMS_IGNORE_STRING_CONVERTER
#else
#include "../include/enums.h"
#endif // DLMS_IGNORE_MALLOC
#include <string.h>
#include "../include/helpers.h"
#include "../include/errorcodes.h"

int obj_DataToString(gxData *object, char **buff)
{
    int ret;
    gxByteBuffer ba;
    BYTE_BUFFER_INIT(&ba);
    if ((ret = bb_addString(&ba, GET_STR_FROM_EEPROM("Index: 2 Value: "))) == 0 &&
        (ret = var_toString(&object->value, &ba)) == 0 &&
        (ret = bb_addString(&ba, GET_STR_FROM_EEPROM("\n"))) == 0)
    {
        *buff = bb_toString(&ba);
    }
    bb_clear(&ba);
    return ret;
}

const char *obj_getUnitAsString(unsigned char unit)
{
    const char *ret;
    switch (unit)
    {
    case DLMS_UNIT_NONE:
        ret = GET_STR_FROM_EEPROM("None");
        break;
    default:
        ret = NULL;
        break;
    }
    return ret;
}

int obj_disconnectControlToString(gxDisconnectControl *object, char **buff)
{
    gxByteBuffer ba;
    BYTE_BUFFER_INIT(&ba);
    bb_addString(&ba, "Index: 2 Value: ");
    bb_addIntAsString(&ba, object->outputState);
    bb_addString(&ba, "\nIndex: 3 Value: ");
    bb_addIntAsString(&ba, object->controlState);
    bb_addString(&ba, "\nIndex: 4 Value: ");
    bb_addIntAsString(&ba, object->controlMode);
    bb_addString(&ba, "\n");
    *buff = bb_toString(&ba);
    bb_clear(&ba);
    return 0;
}

int obj_RegisterToString(gxRegister *object, char **buff)
{
    int ret;
    gxByteBuffer ba;
    BYTE_BUFFER_INIT(&ba);
    if ((ret = bb_addString(&ba, GET_STR_FROM_EEPROM("Index: 3 Value: Scaler: "))) == 0 &&
        (ret = bb_addDoubleAsString(&ba, hlp_getScaler(object->scaler))) == 0 &&
        (ret = bb_addString(&ba, GET_STR_FROM_EEPROM(" Unit: "))) == 0 &&
        (ret = bb_addString(&ba, obj_getUnitAsString(object->unit))) == 0 &&
        (ret = bb_addString(&ba, GET_STR_FROM_EEPROM("\nIndex: 2 Value: "))) == 0 &&
        (ret = var_toString(&object->value, &ba)) == 0 &&
        (ret = bb_addString(&ba, GET_STR_FROM_EEPROM("\n"))) == 0)
    {
        *buff = bb_toString(&ba);
    }
    bb_clear(&ba);
    return ret;
}

int obj_toString(gxObject *object, char **buff)
{
    int ret = 0;
    switch (object->objectType)
    {
    case DLMS_OBJECT_TYPE_DATA:
        ret = obj_DataToString((gxData *)object, buff);
        break;
    case DLMS_OBJECT_TYPE_REGISTER:
        ret = obj_RegisterToString((gxRegister *)object, buff);
        break;
    case DLMS_OBJECT_TYPE_DISCONNECT_CONTROL:
        ret = obj_disconnectControlToString((gxDisconnectControl *)object, buff);
        break;
    default: // Unknown type.
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
