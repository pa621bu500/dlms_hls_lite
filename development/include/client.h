#ifndef CLIENT_H
#define CLIENT_H

#include "dlms.h"
#include "dlmssettings.h"
#include "message.h"

int cl_snrmRequest(
    dlmsSettings *settings,
    message *messages);

int cl_getData2(
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    gxReplyData *notify,
    unsigned char *isNotify);

int cl_parseUAResponse(
    dlmsSettings *settings,
    gxByteBuffer *data);

int cl_aarqRequest(
    dlmsSettings *settings,
    message *messages);

int cl_parseAAREResponse(
    dlmsSettings *settings,
    gxByteBuffer *data);


#endif