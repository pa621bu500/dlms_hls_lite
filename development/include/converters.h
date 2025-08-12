
#ifndef CONVERTERRS_H
#define CONVERTERRS_H

#include "errorcodes.h"
#include "variant.h"
#include "gxobjects.h"
#include "enums.h"

int obj_toString(
    gxObject *object,
    char **buff);

const char *obj_getUnitAsString(
    unsigned char unit);

#endif