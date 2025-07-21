#ifndef CLIENT_H
#define CLIENT_H

#include "dlms.h"
#include "dlmssettings.h"
#include "message.h"


int cl_snrmRequest(
dlmsSettings* settings,
message* messages);

int cl_receiverReady(
dlmsSettings* settings,
DLMS_DATA_REQUEST_TYPES type,
gxByteBuffer* message);

#endif