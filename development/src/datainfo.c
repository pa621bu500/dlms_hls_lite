#include "../include/datainfo.h"

void di_init(gxDataInfo *info)
{
    info->index = 0;
    info->count = 0;
    info->type = DLMS_DATA_TYPE_NONE;
    info->complete = 1;
}