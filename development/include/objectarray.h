#ifndef OBJECTARRAY_H
#define OBJECTARRAY_H

#include "gxobjects.h"

#define OBJECT_ARRAY_CAPACITY 10

void oa_init(
    objectArray *arr);

int oa_push(
    objectArray *arr,
    gxObject *item);

void oa_clear(
    objectArray *arr,
    unsigned char releaseObjects);

int oa_findByLN(
    objectArray *objects,
    DLMS_OBJECT_TYPE type,
    const unsigned char *ln,
    gxObject **object);

#define OA_ATTACH(X, V) oa_attach(&X, (gxObject **)V, sizeof(V) / sizeof(V[0]))

#endif // OBJECTARRAY_H
