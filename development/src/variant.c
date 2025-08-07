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

static int convert(dlmsVARIANT* item, DLMS_DATA_TYPE type)
{
    int ret, fromSize, toSize;
    uint16_t pos;
    char buff[250];
    dlmsVARIANT tmp, tmp3;
    dlmsVARIANT* it;
    if (item->vt == type)
    {
        return DLMS_ERROR_CODE_OK;
    }
     var_init(&tmp);
    var_init(&tmp3);
    ret = var_copy(&tmp, item);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    var_clear(item);
    if (type == DLMS_DATA_TYPE_STRING)
    {
          item->strVal = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
        BYTE_BUFFER_INIT(item->strVal);
        switch (tmp.vt)
        {
            case DLMS_DATA_TYPE_OCTET_STRING:
            {
                #ifndef DLMS_IGNORE_STRING_CONVERTER
                    if (tmp.byteArr != NULL)
                    {
                        char* str = bb_toHexString(tmp.byteArr);
                        bb_addString(item->strVal, str);
                        gxfree(str);
                    }
                    item->vt = type;
                    var_clear(&tmp);
                    return DLMS_ERROR_CODE_OK;
                #else
                    return DLMS_ERROR_CODE_INVALID_PARAMETER;
                #endif //DLMS_IGNORE_STRING_CONVERTER
            }
            case DLMS_DATA_TYPE_DELTA_UINT32:
            {
            hlp_uint64ToString(buff, 250, tmp.ulVal, 0);
            if ((ret = bb_addString(item->strVal, buff)) == 0)
            {
                item->vt = type;
            }
            var_clear(&tmp);
            return ret;
        }
            default:
                return DLMS_ERROR_CODE_NOT_IMPLEMENTED;
        }
    }
    fromSize = var_getSize(tmp.vt);
    toSize = var_getSize(item->vt);
    //If we try to change bigger valut to smaller check that value is not too big.
    //Example Int16 to Int8.
    if (fromSize > toSize)
    {
        unsigned char* pValue = &tmp.bVal;
        for (pos = (unsigned char)toSize; pos != (unsigned char)fromSize; ++pos)
        {
            if (pValue[pos] != 0)
            {
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        }
    }
    if (fromSize > toSize)
    {
        memcpy(&item->bVal, &tmp.bVal, toSize);
    }
    else
    {
        memset(&item->bVal, 0, toSize);
        memcpy(&item->bVal, &tmp.bVal, fromSize);
    }
    item->vt = type;
    var_clear(&tmp);
    return DLMS_ERROR_CODE_OK;
}

int var_changeType(dlmsVARIANT* value, DLMS_DATA_TYPE newType)
{
    if (newType == value->vt)
    {
        return DLMS_ERROR_CODE_OK;
    }
    if (newType == DLMS_DATA_TYPE_NONE)
    {
        return var_clear(value);
    }
     if (value->vt == DLMS_DATA_TYPE_STRING)
    {
        return convert(value, newType);
    }
     switch (newType)
    {
    case DLMS_DATA_TYPE_STRING:
    case DLMS_DATA_TYPE_UINT32:
    case DLMS_DATA_TYPE_INT8:
    case DLMS_DATA_TYPE_INT16:
    case DLMS_DATA_TYPE_UINT8:
    case DLMS_DATA_TYPE_INT64:
    case DLMS_DATA_TYPE_DELTA_UINT32:
        return convert(value, newType);
    default:
            //Handled later.
            break;
    
    }
    switch (value->vt)
    {
        case DLMS_DATA_TYPE_OCTET_STRING:
            switch (newType)
            {
            default:
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        

    }
}

int va_getByIndex(variantArray* arr, int index, dlmsVARIANT_PTR* item)
{
    if (index >= arr->size)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }

    #ifdef DLMS_IGNORE_MALLOC
        dlmsVARIANT_PTR p = (dlmsVARIANT_PTR)arr->data;
        *item = &p[index];
        return DLMS_ERROR_CODE_OK;
    #else
        dlmsVARIANT** p = (dlmsVARIANT**)arr->data;
        *item = p[index];
        return DLMS_ERROR_CODE_OK;
    #endif //DLMS_IGNORE_MALLOC
}





int var_toString(dlmsVARIANT* item, gxByteBuffer* value)
{
    int ret = DLMS_ERROR_CODE_OK;
    uint16_t pos;
    if (item->vt == DLMS_DATA_TYPE_ARRAY || item->vt == DLMS_DATA_TYPE_STRUCTURE)
    {
        dlmsVARIANT* it;
        bb_setInt8(value, item->vt == DLMS_DATA_TYPE_ARRAY ? '{' : '[');
        for (pos = 0; pos != item->Arr->size; ++pos)
        {
            if (pos != 0)
            {
                bb_setInt8(value, ',');
                bb_setInt8(value, ' ');
            }
            if ((ret = va_getByIndex(item->Arr, pos, &it)) != 0 ||
                (ret = var_toString(it, value)) != 0)
            {
                break;
            }
        }
        bb_setInt8(value, item->vt == DLMS_DATA_TYPE_ARRAY ? '}' : ']');
    }
    else
    {
        dlmsVARIANT tmp;
        var_init(&tmp);
        ret = var_copy(&tmp, item);
        if (ret == 0)
        {
            ret = var_changeType(&tmp, DLMS_DATA_TYPE_STRING);
            if (ret == 0 && tmp.strVal != NULL)
            {
                bb_set(value, tmp.strVal->data, tmp.strVal->size);
            }
        }
        var_clear(&tmp);
    }
    return ret;
}

/**
* Convert octetstring to DLMS bytes.
*
* buff
*            Byte buffer where data is write.
* value
*            Added value.
*/
int var_setOctetString(gxByteBuffer* buff, dlmsVARIANT* value)
{
    if (value->vt == DLMS_DATA_TYPE_STRING)
    {
        gxByteBuffer bb;
        BYTE_BUFFER_INIT(&bb);
        bb_addHexString(&bb, (char*)value->strVal->data);
        hlp_setObjectCount(bb.size, buff);
        bb_set2(buff, &bb, 0, bb.size);
    }
    else if (value->vt == DLMS_DATA_TYPE_OCTET_STRING)
    {
        if (value->byteArr == NULL)
        {
            printf("reached var_setOctetString if clause");
            // hlp_setObjectCount(0, buff);
        }
        else
        {
            hlp_setObjectCount(value->byteArr->size, buff);
            bb_set(buff, value->byteArr->data, value->byteArr->size);
        }
    }
    else if (value->vt == DLMS_DATA_TYPE_NONE)
    {
        hlp_setObjectCount(0, buff);
    }
    else
    {
        // Invalid data type.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return 0;
}

int var_getBytes2(
    dlmsVARIANT* data,
    DLMS_DATA_TYPE type,
    gxByteBuffer* ba)
{
    return var_getBytes3(data, type, ba, 1);
}

int var_getBytes3(
    dlmsVARIANT* data,
    DLMS_DATA_TYPE type,
    gxByteBuffer* ba,
    unsigned char addType)
{
    return var_getBytes4(data, type, ba, addType, 1, 1);
}


int var_getBytes4(
    dlmsVARIANT* data,
    DLMS_DATA_TYPE type,
    gxByteBuffer* ba,
    unsigned char addType,
    unsigned char addArraySize,
    unsigned char addStructureSize)
{
    int ret = 0, pos;
    if ((type & DLMS_DATA_TYPE_BYREF) != 0)
    {
        return var_getBytes3(data, type & ~DLMS_DATA_TYPE_BYREF, ba, addType);
    }
    if (addType)
    {
        if ((ret = bb_setUInt8(ba, type)) != 0)
        {
            return ret;
        }
    }
    switch (type)
    {
    case DLMS_DATA_TYPE_OCTET_STRING:
            ret = var_setOctetString(ba, data);
        break;
    }
    return ret;
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

int var_addBytes(dlmsVARIANT* data, const unsigned char* value, uint16_t count)
{
    if (data->vt != DLMS_DATA_TYPE_OCTET_STRING)
    {
#ifdef DLMS_IGNORE_MALLOC
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
#else
        var_clear(data);
        data->byteArr = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
        BYTE_BUFFER_INIT(data->byteArr);
        data->vt = DLMS_DATA_TYPE_OCTET_STRING;
#endif //DLMS_IGNORE_MALLOC
    }
#ifndef DLMS_IGNORE_MALLOC
    else
    {
        bb_clear(data->byteArr);
    }
#endif //DLMS_IGNORE_MALLOC
    return bb_set(data->byteArr, value, count);
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