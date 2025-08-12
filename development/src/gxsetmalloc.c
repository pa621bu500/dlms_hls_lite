#include <assert.h>
#include "../include/gxmem.h"
#include <string.h>
#include <math.h>

#include "../include/gxset.h"
#include "../include/dlms.h"
#include "../include/cosem.h"

#ifndef DLMS_IGNORE_DATA
int cosem_setData(gxValueEventArg *e)
{
    int ret;
    if (e->index == 2)
    {
        ret = var_copy(&((gxData *)e->target)->value, &e->value);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif // DLMS_IGNORE_DATA

int cosem_setRegister(gxRegister *object, unsigned char index, dlmsVARIANT *value)
{
    int ret = 0;
    if (index == 2)
    {
        ret = var_copy(&object->value, value);
    }

    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}

int cosem_setDisconnectControl(gxDisconnectControl *object, unsigned char index, dlmsVARIANT *value)
{
    if (index == 2)
    {
        object->outputState = value->boolVal;
    }
    // else if (index == 3)
    // {
    //     object->controlState = (DLMS_CONTROL_STATE)var_toInteger(value);
    // }
    // else if (index == 4)
    // {
    //     object->controlMode = (DLMS_CONTROL_MODE)var_toInteger(value);
    // }
    else
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}