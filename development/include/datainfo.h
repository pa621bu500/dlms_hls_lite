#ifndef GXDATA_INFO_H
#define GXDATA_INFO_H

#include <stdint.h>
#include "enums.h"


typedef struct
{
    // Last array index.
    uint16_t index;

    // Items count in array.
    uint16_t count;
    // Object data type.
    DLMS_DATA_TYPE type;
    // Is data parsed to the end.
    unsigned char complete;
} gxDataInfo;

void di_init(gxDataInfo *info);

#endif //GXDATA_INFO_H
