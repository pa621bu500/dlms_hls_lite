
// #include "variant.h"
#include "bytebuffer.h"
#include <stdint.h>
#include "variant.h"
#include "enums.h"
#include <time.h>
#ifndef GXREPLYDATA_H
#define GXREPLYDATA_H
typedef struct
    {
        /**
        * Is more data available.
        */
        DLMS_DATA_REQUEST_TYPES moreData;
//         /**
//          * Received command.
//          */
        DLMS_COMMAND command;
//         /**
//          * Encrypted command.
//          */
        DLMS_COMMAND encryptedCommand;

        unsigned char commandType;

//         /**
//         * Received data.
//         */
        gxByteBuffer data;

//         /**
//          * Is frame complete.
//          */
        unsigned char complete;

//         /**
//          * Read value.
//          */
        dlmsVARIANT dataValue;

//         /**
//          * Expected count of element in the array.
//          */
        uint16_t totalCount;


        uint32_t readPosition;
//         /**
//         * Packet length.
//         */
        uint32_t packetLength;

//         /**
//          * Try Get value.
//          */
        unsigned char peek;

//         /**
//         * Value is not try to parse. This is used in data collector.
//         */
        unsigned char ignoreValue;

        DLMS_DATA_TYPE dataType;

//         /**
//         * Cipher index is position where data is decrypted.
//         */
        uint16_t cipherIndex;

//         /**
//          * Data notification date time.
//          */

        struct tm time;

//         /**
//         * Pre-established connection.
//         */
        unsigned char preEstablished;
        unsigned char invokeId;

        //GBT block number.
        uint16_t blockNumber;
        //GBT block number ACK.
        uint16_t blockNumberAck;
        //GBT is streaming used.
        unsigned streaming;
        //GBT window size
        unsigned windowSize;
        //Server address.
        uint16_t serverAddress;
        //Client address.
        uint16_t clientAddress;
    } gxReplyData;

void reply_init(gxReplyData* reply);
void reply_clear(gxReplyData* reply);
unsigned char reply_isMoreData(gxReplyData* reply);

#endif