
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
    dlmsSettings *settings,
    int frame,
    gxByteBuffer *data,
    gxByteBuffer *reply);

// int dlms_receiverReady(
// dlmsSettings* settings,
// DLMS_DATA_REQUEST_TYPES type,
// gxByteBuffer* reply);

/**
 * Get next logical name PDU.
 *
 * @param p
 *            LN parameters.
 * @param reply
 *            Generated message.
 */
int dlms_getLNPdu(
    gxLNParameters *p,
    gxByteBuffer *reply);

unsigned char dlms_useHdlc(DLMS_INTERFACE_TYPE type);

int dlms_addLLCBytes(
    dlmsSettings *settings,
    gxByteBuffer *data);

int dlms_getData2(
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    unsigned char first);

int dlms_setData(
    gxByteBuffer *data,
    DLMS_DATA_TYPE type,
    dlmsVARIANT *value);

int dlms_getData3(
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    gxReplyData *notify,
    unsigned char first,
    unsigned char *isNotify);

int dlms_parseSnrmUaResponse(
    dlmsSettings *settings,
    gxByteBuffer *data);

int dlms_checkInit(
    dlmsSettings *settings);

int dlms_getLnMessages(
    gxLNParameters *p,
    message *reply);

/**
 * This function returns true, if pre-established connection is used.
 */
unsigned char dlms_usePreEstablishedConnection(dlmsSettings *settings);

#endif