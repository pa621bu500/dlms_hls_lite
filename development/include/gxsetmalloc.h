#ifndef COSEM_SET_MALLOC_H
#define COSEM_SET_MALLOC_H

#include "gxobjects.h"
#include "dlmssettings.h"
#include "gxvalueeventargs.h"

#ifndef DLMS_IGNORE_DATA
    int cosem_setData(gxValueEventArg* e);
#endif //DLMS_IGNORE_DATA
#ifndef DLMS_IGNORE_REGISTER
    int cosem_setRegister(gxRegister* object, unsigned char index, dlmsVARIANT* value);
#endif //DLMS_IGNORE_REGISTER

#endif