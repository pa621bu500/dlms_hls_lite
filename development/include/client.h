#ifndef CLIENT_H
#define CLIENT_H

#include "dlms.h"
#include "dlmssettings.h"
#include "message.h"


int cl_snrmRequest(
dlmsSettings* settings,
message* messages);

int dlms_parseSnrmUaResponse(
dlmsSettings* settings,
gxByteBuffer* data);
#endif

int cl_aarqRequest(
dlmsSettings* settings,
message* messages);


int cl_parseUAResponse(
dlmsSettings* settings,
gxByteBuffer* data);