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

int cl_read(
dlmsSettings* settings,
gxObject* object,
unsigned char attributeOrdinal,
message* messages);

    int cl_readLN(
        dlmsSettings* settings,
        const unsigned char* name,
        DLMS_OBJECT_TYPE interfaceClass,
        unsigned char attributeOrdinal,
        gxByteBuffer* data,
        message* messages);


            int cl_updateValue(
        dlmsSettings* settings,
        gxObject* object,
        unsigned char attributeOrdinal,
        dlmsVARIANT* value);


int cl_parseAAREResponse(
    dlmsSettings *settings,
    gxByteBuffer *data);
    
int cl_parseApplicationAssociationResponse(
        dlmsSettings* settings,
        gxByteBuffer* reply);

#endif