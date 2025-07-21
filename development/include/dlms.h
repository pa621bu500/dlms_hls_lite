
#ifndef DLMS_H
#define DLMS_H



#include "errorcodes.h"
#include "bytebuffer.h"
#include "message.h"
#include "helpers.h"
#include "dlmssettings.h"
#include "variant.h"
#include "replydata.h"
#include "parameters.h"
int dlms_getHdlcFrame(
dlmsSettings* settings,
int frame,
gxByteBuffer* data,
gxByteBuffer* reply);


int dlms_receiverReady(
dlmsSettings* settings,
DLMS_DATA_REQUEST_TYPES type,
gxByteBuffer* reply);

int dlms_checkInit(
dlmsSettings* settings);

int dlms_generateChallenge(
gxByteBuffer* challenge);

int dlms_getLnMessages(
gxLNParameters* p,
message* reply);

#endif