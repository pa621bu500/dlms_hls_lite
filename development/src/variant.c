#include "../include/variant.h"
#include "../include/errorcodes.h"
#include "../include/helpers.h"


//Initialize variant.
int var_init(dlmsVARIANT* data)
{
    // data->vt = DLMS_DATA_TYPE_NONE;
    data->byteArr = NULL;
    return DLMS_ERROR_CODE_OK;
}

// int var_clear(dlmsVARIANT* data)
// {
// #ifdef DLMS_IGNORE_MALLOC
//     //Referenced values are not cleared. User must do it.
//     if ((data->vt & DLMS_DATA_TYPE_BYREF) == 0)
//     {
//         data->llVal = 0;
//         data->vt = DLMS_DATA_TYPE_NONE;
//     }
//     data->size = 0;
// #else
//     //Referenced values are not cleared. User must do it.
//     if ((data->vt & DLMS_DATA_TYPE_BYREF) != 0)
//     {
//         return 0;
//     }
//     switch (data->vt)
//     {
//     case DLMS_DATA_TYPE_OCTET_STRING:
//         if (data->byteArr != NULL)
//         {
//             bb_clear(data->byteArr);
//             if (!bb_isAttached(data->byteArr))
//             {
//                 gxfree(data->byteArr);
//                 data->byteArr = NULL;
//             }
//         }
//         break;
//     case DLMS_DATA_TYPE_STRING_UTF8:
//         if (data->strUtfVal != NULL)
//         {
//             bb_clear(data->strUtfVal);
//             if (!bb_isAttached(data->strUtfVal))
//             {
//                 gxfree(data->strUtfVal);
//                 data->strUtfVal = NULL;
//             }
//         }
//         break;
//     case DLMS_DATA_TYPE_STRING:
//         if (data->strVal != NULL)
//         {
//             bb_clear(data->strVal);
//             gxfree(data->strVal);
//         }
//         break;
//     case DLMS_DATA_TYPE_ARRAY:
//     case DLMS_DATA_TYPE_STRUCTURE:
//     case DLMS_DATA_TYPE_COMPACT_ARRAY:
//         if (data->Arr != NULL)
//         {
//             va_clear(data->Arr);
//             gxfree(data->Arr);
//             data->Arr = NULL;
//         }
//         break;
//     case DLMS_DATA_TYPE_BIT_STRING:
//         if (data->bitArr != NULL)
//         {
//             ba_clear(data->bitArr);
//             gxfree(data->bitArr);
//         }
//         break;
//     case DLMS_DATA_TYPE_DATETIME:
//     case DLMS_DATA_TYPE_DATE:
//     case DLMS_DATA_TYPE_TIME:
//         if (data->dateTime != NULL)
//         {
//             gxfree(data->dateTime);
//             data->dateTime = NULL;
//         }
//         break;
//     default:
//         data->llVal = 0;
//         break;
//     }
//     data->vt = DLMS_DATA_TYPE_NONE;
// #endif //DLMS_IGNORE_MALLOC
//     return DLMS_ERROR_CODE_OK;
// }