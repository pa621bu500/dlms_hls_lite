#include "../include/parameters.h"

void params_initLN(
    gxLNParameters *target,
    dlmsSettings* settings,
    unsigned char invokeId,
    DLMS_COMMAND command,
    unsigned char commandType,
    gxByteBuffer* attributeDescriptor,
    gxByteBuffer* data,
    unsigned char status,
    DLMS_COMMAND encryptedCommand,
    unsigned char multipleBlocks,
    unsigned char lastBlock)
{
    target->invokeId = invokeId;
    target->settings = settings;
    target->blockIndex = settings->blockIndex;
    target->command = command;
    target->encryptedCommand = encryptedCommand;
    target->requestType = commandType;
    target->attributeDescriptor = attributeDescriptor;
    target->data = data;
    target->time = 0;
    target->status = status;
    target->multipleBlocks = multipleBlocks;
    target->lastBlock = lastBlock;
    //Serialize data to this PDU.
    target->serializedPdu = settings->serializedPdu;
}