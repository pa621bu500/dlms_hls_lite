#ifndef APDU_H
#define APDU_H

#include "dlmssettings.h"
#include "bytebuffer.h"


int apdu_generateAarq(
    dlmsSettings* settings,
    gxByteBuffer* data);
int apdu_generateUserInformation(
    dlmsSettings* settings,
    gxByteBuffer* data);


#endif //APDU_H