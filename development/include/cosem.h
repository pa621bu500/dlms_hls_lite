#ifndef COSEM_H
#define COSEM_H

#include "gxobjects.h"
#include "dlmssettings.h"
  int cosem_init(
      gxObject* object,
      DLMS_OBJECT_TYPE type,
      const char* logicalNameString);


      #endif