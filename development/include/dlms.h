
#ifndef DLMS_H
#define DLMS_H



#include "errorcodes.h"
#include "bytebuffer.h"
#include "message.h"
#include "helpers.h"
#include "dlmssettings.h"
#include "variant.h"
#include "replydata.h"

int dlms_getHdlcFrame(
dlmsSettings* settings,
int frame,
gxByteBuffer* data,
gxByteBuffer* reply);

#endif