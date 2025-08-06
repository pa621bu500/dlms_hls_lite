#ifndef COSEM_H
#define COSEM_H

#include "gxobjects.h"
#include "dlmssettings.h"

int cosem_init(
    gxObject *object,
    DLMS_OBJECT_TYPE type,
    const char *logicalNameString);

int cosem_init2(
    gxObject *object,
    DLMS_OBJECT_TYPE type,
    const unsigned char *ln);

int cosem_init3(
    gxObject *object,
    const uint16_t expectedSize,
    DLMS_OBJECT_TYPE type,
    const unsigned char *ln);

int cosem_init4(
    void *object,
    const uint16_t expectedSize,
    DLMS_OBJECT_TYPE type,
    const unsigned char *ln);

int cosem_getOctetString(gxByteBuffer *bb, gxByteBuffer *value);

#endif