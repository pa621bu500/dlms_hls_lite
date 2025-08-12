#ifndef APDU_H
#define APDU_H

#include "dlmssettings.h"
#include "bytebuffer.h"

int apdu_generateAarq(
    dlmsSettings *settings,
    gxByteBuffer *data);
int apdu_generateUserInformation(
    dlmsSettings *settings,
    gxByteBuffer *data);

int apdu_parsePDU(
    dlmsSettings *settings,
    gxByteBuffer *buff,
    DLMS_ASSOCIATION_RESULT *result,
    unsigned char *diagnostic,
    unsigned char *command);

#endif // APDU_H