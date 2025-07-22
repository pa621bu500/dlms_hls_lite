#include <assert.h>
#include "../include/gxmem.h"
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include <string.h> /* memset */
#include "../include/enums.h"
#include "../include/dlms.h"
#include "../include/ciphering.h"
#include "../include/crc.h"
#include "../include/cosem.h"
#include "../include/gxobjects.h"
#include "../include/parameters.h"


static const unsigned char LLC_SEND_BYTES[3] = { 0xE6, 0xE6, 0x00 };
static const unsigned char LLC_REPLY_BYTES[3] = { 0xE6, 0xE7, 0x00 };
static const unsigned char HDLC_FRAME_START_END = 0x7E;

int dlms_getHdlcFrame(
    dlmsSettings* settings,
    int frame,
    gxByteBuffer* data,
    gxByteBuffer* reply)
{
    unsigned char tmp[4], tmp2[4];
    uint16_t crc;
    int ret;
    uint16_t frameSize;
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t len = 0;
#else
    uint16_t len = 0;
#endif
    gxByteBuffer primaryAddress, secondaryAddress;
    bb_clear(reply);
    bb_attach(&primaryAddress, tmp, 0, 4);
    bb_attach(&secondaryAddress, tmp2, 0, 4);
    if (settings->server)
    {
        if ((ret = bb_capacity(&primaryAddress, 1)) == 0 &&
            (ret = bb_capacity(&secondaryAddress, 4)) == 0)
        {
            // if ((frame == 0x13 || frame == 0x3) && ((dlmsServerSettings*)settings)->pushClientAddress != 0)
            // {
            //     ret = dlms_getAddressBytes(((dlmsServerSettings*)settings)->pushClientAddress, &primaryAddress);
            // }
            // else
            // {
            //     ret = dlms_getAddressBytes(settings->clientAddress, &primaryAddress);
            // }
            if (ret == 0)
            {
                ret = dlms_getAddressBytes(settings->serverAddress, &secondaryAddress);
            }
        }
    }
    else
    {
        ret = bb_capacity(&primaryAddress, 4);
        if (ret == 0)
        {
            ret = bb_capacity(&secondaryAddress, 1);
        }
        if (ret == 0 && (ret = dlms_getAddressBytes(settings->serverAddress, &primaryAddress)) == 0)
        {
            ret = dlms_getAddressBytes(settings->clientAddress, &secondaryAddress);
        }
    }

    // Add BOP
    if (ret == 0 && (ret = bb_capacity(reply, 8)) == 0)
    {
        ret = bb_setUInt8(reply, HDLC_FRAME_START_END);
    }

    frameSize = settings->maxInfoTX;
    if (data != NULL && data->position == 0)
    {
        frameSize -= 3;
    }
    // If no data
    if (ret == 0 && (data == NULL || data->size == 0))
    {
        len = 0;
        ret = bb_setUInt8(reply, 0xA0);
    }
    else if (ret == 0 && data->size - data->position <= frameSize)
    {
        // Is last packet.
        len = bb_available(data);
        ret = bb_setUInt8(reply, (unsigned char)(0xA0 | (((7 + primaryAddress.size + secondaryAddress.size + len) >> 8) & 0x7)));
    }
    else if (ret == 0)
    {
        // More data to left.
        len = frameSize;
        ret = bb_setUInt8(reply, (unsigned char)(0xA8 | (((7 + primaryAddress.size + secondaryAddress.size + len) >> 8) & 0x7)));
    }
    // Frame len.
    if (ret == 0 && len == 0)
    {
        ret = bb_setUInt8(reply, (unsigned char)(5 + primaryAddress.size + secondaryAddress.size + len));
    }
    else if (ret == 0)
    {
        if ((ret = bb_capacity(reply, (uint16_t)(11 + len))) == 0)
        {
            ret = bb_setUInt8(reply, (unsigned char)(7 + primaryAddress.size + secondaryAddress.size + len));
        }
    }
    // Add primary address.
    if (ret == 0 && (ret = bb_set2(reply, &primaryAddress, 0, primaryAddress.size)) == 0)
    {
        // Add secondary address.
        ret = bb_set2(reply, &secondaryAddress, 0, secondaryAddress.size);
    }

    // Add frame ID.
    if (ret == 0 && frame == 0)
    {
        // ret = bb_setUInt8(reply, getNextSend(settings, 1));
    }
    else if (ret == 0)
    {
        ret = bb_setUInt8(reply, (unsigned char)frame);
    }
    if (ret == 0)
    {
        // Add header CRC.
        crc = countCRC(reply, 1, (reply->size - 1));
        ret = bb_setUInt16(reply, crc);
    }
    if (ret == 0 && len != 0)
    {
        // Add data.
        if ((ret = bb_set2(reply, data, data->position, len)) == 0)
        {
            // Add data CRC.
            crc = countCRC(reply, 1, (reply->size - 1));
            ret = bb_setUInt16(reply, crc);
        }
    }
    // Add EOP
    if (ret == 0)
    {
        ret = bb_setUInt8(reply, HDLC_FRAME_START_END);
    }
    // Remove sent data in server side.
    if (ret == 0 && settings->server)
    {
        if (data != NULL)
        {
            //If all data is sent.
            if (data->size == data->position)
            {
                ret = bb_clear(data);
            }
            else
            {
                //Remove sent data.
                ret = bb_move(data, data->position, 0, data->size - data->position);
                data->position = 0;
            }
        }
    }
    bb_clear(&primaryAddress);
    bb_clear(&secondaryAddress);
    return ret;
}

int dlms_getAddressBytes(
    uint32_t value,
    gxByteBuffer* bytes)
{
    int ret, size;
    uint32_t address;
    if ((ret = dlms_getAddress(value, &address, &size)) != 0)
    {
        return ret;
    }
    if (size == 1)
    {
        bb_setUInt8(bytes, (unsigned char)address);
    }
    else if (size == 2)
    {
        bb_setUInt16(bytes, (uint16_t)address);
    }
    // else if (size == 4)
    // {
    //     bb_setUInt32(bytes, address);
    // }
    else
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_getAddress(int32_t value, uint32_t* address, int* size)
{
    if (value < 0x80)
    {
        *address = (unsigned char)(value << 1 | 1);
        *size = 1;
        return 0;
    }
    else if (value < 0x4000)
    {
        *address = (uint16_t)((value & 0x3F80) << 2 | (value & 0x7F) << 1 | 1);
        *size = 2;
    }
    else if (value < 0x10000000)
    {
        *address = (uint32_t)((value & 0xFE00000) << 4 | (value & 0x1FC000) << 3
            | (value & 0x3F80) << 2 | (value & 0x7F) << 1 | 1);
        *size = 4;
    }
    else
    {
        //Invalid address
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_generateChallenge(
    gxByteBuffer* challenge)
{
    // Random challenge is 8 to 64 bytes.
    // Texas Instruments accepts only 16 byte long challenge.
    // For this reason challenge size is 16 bytes at the moment.
    int ret = 0, pos, len = 16;
    bb_clear(challenge);
    for (pos = 0; pos != len; ++pos)
    {
        if ((ret = bb_setUInt8(challenge, hlp_rand())) != 0)
        {
            break;
        }
    }
    return ret;
}

// int dlms_getLnMessages(
//     gxLNParameters* p,
//     message* messages)
// {
//     int ret;
//     gxByteBuffer* pdu;
//     gxByteBuffer* it;
// #ifndef DLMS_IGNORE_HDLC
//     unsigned char frame = 0;
//     if (p->command == DLMS_COMMAND_DATA_NOTIFICATION ||
//         p->command == DLMS_COMMAND_EVENT_NOTIFICATION)
//     {
//         frame = 0x13;
//     }
// #endif //DLMS_IGNORE_HDLC
// #ifdef DLMS_IGNORE_MALLOC
//     pdu = p->serializedPdu;
// #else
//     gxByteBuffer reply;
//     if (p->serializedPdu == NULL)
//     {
//         BYTE_BUFFER_INIT(&reply);
//         pdu = &reply;
//     }
//     else
//     {
//         pdu = p->serializedPdu;
//     }
// #endif //DLMS_IGNORE_MALLOC
//     do
//     {
//         if ((ret = dlms_getLNPdu(p, pdu)) == 0)
//         {
//             p->lastBlock = 1;
//             if (p->attributeDescriptor == NULL)
//             {
//                 ++p->settings->blockIndex;
//             }
//         }
//         while (ret == 0 && pdu->position != pdu->size)
//         {
// #ifdef DLMS_IGNORE_MALLOC
//             if (!(messages->size < messages->capacity))
//             {
//                 ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
//                 break;
//             }
//             it = messages->data[messages->size];
//             ++messages->size;
//             bb_clear(it);
// #else
//             if (messages->attached)
//             {
//                 if (messages->size < messages->capacity)
//                 {
//                     it = messages->data[messages->size];
//                     ++messages->size;
//                     bb_clear(it);
//                 }
//                 else
//                 {
//                     ret = DLMS_ERROR_CODE_OUTOFMEMORY;
//                 }
//             }
//             else
//             {
//                 it = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
//                 if (it == NULL)
//                 {
//                     ret = DLMS_ERROR_CODE_OUTOFMEMORY;
//                 }
//                 else
//                 {
//                     BYTE_BUFFER_INIT(it);
//                     ret = mes_push(messages, it);
//                 }
//             }
//             if (ret != 0)
//             {
//                 break;
//             }
// #endif //DLMS_IGNORE_MALLOC
//             switch (p->settings->interfaceType)
//             {
// #ifndef DLMS_IGNORE_WRAPPER
//             case DLMS_INTERFACE_TYPE_WRAPPER:
//                 ret = dlms_getWrapperFrame(p->settings, p->command, pdu, it);
//                 break;
// #endif //DLMS_IGNORE_WRAPPER
// #ifndef DLMS_IGNORE_HDLC
//             case DLMS_INTERFACE_TYPE_HDLC:
//             // case DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E:
//             //     ret = dlms_getHdlcFrame(p->settings, frame, pdu, it);
//             //     if (ret == 0 && pdu->position != pdu->size)
//             //     {
//             //         frame = getNextSend(p->settings, 0);
//             //     }
//             //     break;
// #endif //DLMS_IGNORE_HDLC
//             case DLMS_INTERFACE_TYPE_PDU:
//                 ret = bb_set2(it, pdu, 0, pdu->size);
//                 break;
// #ifndef DLMS_IGNORE_PLC
//             // case DLMS_INTERFACE_TYPE_PLC:
//             //     ret = dlms_getPlcFrame(p->settings, 0x90, pdu, it);
//             //     break;
//             // case DLMS_INTERFACE_TYPE_PLC_HDLC:
//             //     ret = dlms_getMacHdlcFrame(p->settings, frame, 0, pdu, it);
//             //     break;
// #endif //DLMS_IGNORE_PLC
//             default:
//                 ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
//             }
//             if (ret != 0)
//             {
//                 break;
//             }
//         }
//         bb_clear(pdu);
// #ifndef DLMS_IGNORE_HDLC
//         frame = 0;
// #endif //DLMS_IGNORE_HDLC
//     } while (ret == 0 && p->data != NULL && p->data->position != p->data->size);
//     return ret;
// }

int dlms_checkInit(dlmsSettings* settings)
{
    if (settings->clientAddress == 0)
    {
        return DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS;
    }
    if (settings->serverAddress == 0)
    {
        return DLMS_ERROR_CODE_INVALID_SERVER_ADDRESS;
    }
    if (settings->maxPduSize < 64)
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}