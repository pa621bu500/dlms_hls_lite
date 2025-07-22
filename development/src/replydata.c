#include <string.h> /* memset */
#include "../include/gxmem.h"
#include "../include/replydata.h"


void reply_init(gxReplyData* reply)
{
    reply->invokeId = 0;
    reply->commandType = 0;
    reply->moreData = DLMS_DATA_REQUEST_TYPES_NONE;
    reply->encryptedCommand = reply->command = DLMS_COMMAND_NONE;
    BYTE_BUFFER_INIT(&reply->data);
    reply->complete = 0;
    var_init(&reply->dataValue);
    reply->totalCount = 0;
    reply->readPosition = 0;
    reply->packetLength = 0;
    reply->peek = 0;
    reply->ignoreValue = 0;
    reply->dataType = DLMS_DATA_TYPE_NONE;
    reply->cipherIndex = 0;
    memset(&reply->time, 0, sizeof(struct tm));
    reply->preEstablished = 0;
    reply->blockNumber = 0;
    reply->blockNumberAck = 0;
    reply->streaming = 0;
    reply->windowSize = 0;
    reply->serverAddress = 0;
    reply->clientAddress = 0;
}

unsigned char reply_isMoreData(gxReplyData* reply)
{
    return reply->moreData != DLMS_DATA_REQUEST_TYPES_NONE;
}
void reply_clear2(gxReplyData* reply, unsigned char clearData)
{
    reply->invokeId = 0;
    reply->moreData = DLMS_DATA_REQUEST_TYPES_NONE;
    reply->encryptedCommand = reply->command = DLMS_COMMAND_NONE;
    if (clearData)
    {
        bb_clear(&reply->data);
        reply->preEstablished = 0;
    }
    reply->complete = 0;
    // var_clear(&reply->dataValue);
    reply->totalCount = 0;
    reply->readPosition = 0;
    reply->packetLength = 0;
    reply->peek = 0;
    reply->ignoreValue = 0;
    reply->dataType = DLMS_DATA_TYPE_NONE;
    reply->cipherIndex = 0;
#ifdef DLMS_USE_EPOCH_TIME
    reply->time = 0;
#else
    memset(&reply->time, 0, sizeof(struct tm));
#endif // DLMS_USE_EPOCH_TIME
    reply->blockNumber = 0;
    reply->blockNumberAck = 0;
    reply->streaming = 0;
    reply->windowSize = 0;
    reply->serverAddress = 0;
    reply->clientAddress = 0;
}

void reply_clear(gxReplyData* reply)
{
    reply_clear2(reply, 1);
}

