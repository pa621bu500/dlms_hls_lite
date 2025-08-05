#include "../include/variant.h"
#include "../include/errorcodes.h"
#include "../include/helpers.h"
#include <assert.h>

//Initialize variant.
int var_init(dlmsVARIANT* data)
{
    data->vt = DLMS_DATA_TYPE_NONE;
    data->byteArr = NULL;
    return DLMS_ERROR_CODE_OK;
}

//Get size in bytes.
int var_getSize(DLMS_DATA_TYPE vt)
{
    int nSize = -1;
    switch (vt)
    {
    case DLMS_DATA_TYPE_NONE:
        nSize = 0;
        break;
    case DLMS_DATA_TYPE_INT8:
    case DLMS_DATA_TYPE_UINT8:
        nSize = 1;
        break;
    case DLMS_DATA_TYPE_UINT32:
        nSize = 4;
        break;
    case DLMS_DATA_TYPE_INT16:
        nSize = 2;
        break;
    case DLMS_DATA_TYPE_INT64:
        nSize = 8;
        break;
    case DLMS_DATA_TYPE_BIT_STRING:
    case DLMS_DATA_TYPE_OCTET_STRING:
    case DLMS_DATA_TYPE_STRING:
        nSize = -1;
        break;
    default:
        break;
    }
    return nSize;
}



//copy variant.
int var_copy(dlmsVARIANT* target, dlmsVARIANT* source)
{
#ifndef DLMS_IGNORE_MALLOC
    dlmsVARIANT* it;
    dlmsVARIANT* item;
#endif //DLMS_IGNORE_MALLOC
    int ret = DLMS_ERROR_CODE_OK;
    if ((source->vt & DLMS_DATA_TYPE_BYREF) != 0)
    {
       //skip
        return 0;
    }
    if ((target->vt & DLMS_DATA_TYPE_BYREF) != 0)
    {
        //skip
        return 0;
    }

    unsigned char attaced = 0;

    ret = var_clear(target);
 
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    target->vt = source->vt;
    if (source->vt == DLMS_DATA_TYPE_STRING)
    {
        if (source->strVal != NULL)
        {
            target->strVal = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
            BYTE_BUFFER_INIT(target->strVal);
            bb_set(target->strVal, source->strVal->data, source->strVal->size);
        }
    }
    else if (source->vt == DLMS_DATA_TYPE_OCTET_STRING)
    {
        if (source->byteArr != 0)
        {
            target->byteArr = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
            BYTE_BUFFER_INIT(target->byteArr);
            bb_set(target->byteArr, source->byteArr->data, source->byteArr->size);
        }
    }
    else
    {
        ret = var_getSize(source->vt);
        if (ret > 0)
        {
            memcpy(&target->pVal, &source->pVal, ret);
        }
        ret = 0;
    }
    return ret;
}




int var_toInteger(dlmsVARIANT* data)
{
    int ret;
    switch (data->vt)
    {
    case DLMS_DATA_TYPE_NONE:
        ret = 0;
        break;
    case DLMS_DATA_TYPE_STRING:
        ret = hlp_stringToInt((const char*)data->strVal);
        break;
#ifndef DLMS_IGNORE_DELTA
    case DLMS_DATA_TYPE_DELTA_UINT32:
        ret = data->ulVal;
        break;
    case DLMS_DATA_TYPE_UINT32:
        ret = data->ulVal;
        break;
#endif //DLMS_IGNORE_DELTA
    default:
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
        assert(0);
#endif
        ret = 0;
        break;
    }
    return ret;
    return 0;
}




int var_clear(dlmsVARIANT* data)
{

    //Referenced values are not cleared. User must do it.
    if ((data->vt & DLMS_DATA_TYPE_BYREF) != 0)
    {
        return 0;
    }
    switch (data->vt)
    {
    case DLMS_DATA_TYPE_STRING:
        if (data->strVal != NULL)
        {
            bb_clear(data->strVal);
            gxfree(data->strVal);
        }
        break;
    default:
        data->llVal = 0;
        break;
    }
    data->vt = DLMS_DATA_TYPE_NONE;

    return DLMS_ERROR_CODE_OK;
}