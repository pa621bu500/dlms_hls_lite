
#ifndef PARAMETERS_H
#define PARAMETERS_H
#include "dlmssettings.h"

  typedef struct
    {
        /**
         * DLMS settings.
         */
        dlmsSettings *settings;
        /**
         * DLMS command.
         */
        DLMS_COMMAND command;
        /**
        * Encrypted DLMS command.
        */
        DLMS_COMMAND encryptedCommand;
        /**
         * Request type.
         */
		unsigned char requestType;
        /**
         * Attribute descriptor.
         */
        gxByteBuffer* attributeDescriptor;
        /**
         * Data.
         */
        gxByteBuffer* data;
        /**
         * Send date and time. This is used in Data notification messages.
         */
        #ifdef DLMS_USE_EPOCH_TIME
                uint32_t time;
        #else
                struct tm* time;
        #endif // DLMS_USE_EPOCH_TIME
        /**
         * Reply status.
         */
        unsigned char status;
        /**
         * Are there more data to send or more data to receive.
         */
        unsigned char multipleBlocks;
        /**
         * Is this last block in send.
         */
        unsigned char lastBlock;
        /**
         * Block index.
         */
        uint32_t blockIndex;
        /**
        * Received invoke ID.
        */
        unsigned char invokeId;
        //Serialize data to this PDU.
        gxByteBuffer* serializedPdu;
    } gxLNParameters;

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
unsigned char lastBlock);

#endif // PARAMETERS_H