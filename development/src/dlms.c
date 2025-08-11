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
#include "../include/variant.h"
#include "../include/datainfo.h"

static const unsigned char LLC_SEND_BYTES[3] = {0xE6, 0xE6, 0x00};
static const unsigned char LLC_REPLY_BYTES[3] = {0xE6, 0xE7, 0x00};
static const unsigned char HDLC_FRAME_START_END = 0x7E;
#define GET_AUTH_TAG(s)(s.broadcast ? 0x40 : 0) | DLMS_SECURITY_AUTHENTICATION | s.suite


int getOctetString(gxByteBuffer *buff, gxDataInfo *info, unsigned char knownType, dlmsVARIANT *value)
{
    uint16_t len;
    int ret = 0;
    if (knownType)
    {
        len = (uint16_t)buff->size;
    }
    else
    {
        if (hlp_getObjectCount2(buff, &len) != 0)
        {
            return DLMS_ERROR_CODE_OUTOFMEMORY;
        }
        // If there is not enough data available.
        if (buff->size - buff->position < len)
        {
            info->complete = 0;
            return 0;
        }
    }
#ifdef DLMS_IGNORE_MALLOC
    if (value->vt != (DLMS_DATA_TYPE_OCTET_STRING | DLMS_DATA_TYPE_BYREF))
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if (value->capacity < len)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    value->size = len;
    memcpy(value->pVal, buff->data + buff->position, len);
    buff->position += len;
#else
    if (len == 0)
    {
        var_clear(value);
    }
    else
    {
        ret = var_addBytes(value, buff->data + buff->position, len);
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
        buff->position += (uint32_t)len;
#else
        buff->position += (uint16_t)len;
#endif
    }
#endif // DLMS_IGNORE_MALLOC
    return ret;
}

static int getBool(gxByteBuffer *buff, gxDataInfo *info, dlmsVARIANT *value)
{
    int ret;
    unsigned char ch;
    // If there is not enough data available.
    if (buff->size - buff->position < 1)
    {
        info->complete = 0;
        return 0;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if ((value->vt & DLMS_DATA_TYPE_BYREF) == 0)
    {
        value->vt = DLMS_DATA_TYPE_BOOLEAN;
        value->boolVal = ch != 0;
    }
    else
    {
        *value->pboolVal = ch != 0;
    }
    return 0;
}


int dlms_getData(gxByteBuffer* data, gxDataInfo* info, dlmsVARIANT* value)
{
    unsigned char ch, knownType;
    int ret = 0;
    uint32_t startIndex = data->position;
    var_clear(value);
    info->complete = 1;
    knownType = info->type != DLMS_DATA_TYPE_NONE;
    if (!knownType)
    {
        ret = bb_getUInt8(data, &ch);
        if (ret != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
        info->type = (DLMS_DATA_TYPE)ch;
    }
    switch (info->type & ~DLMS_DATA_TYPE_BYREF)
    {
        case DLMS_DATA_TYPE_UINT32:
            ret = getUInt32(data, info, value);
            break;
        case DLMS_DATA_TYPE_OCTET_STRING:
            ret = getOctetString(data, info, knownType, value);
            break;
        case DLMS_DATA_TYPE_BOOLEAN:
            {
                ret = getBool(data, info, value);
                break;
            }
    }
    if (ret == 0 && (value->vt & DLMS_DATA_TYPE_BYREF) == 0)
    {
        value->vt = info->type;
    }
     return ret;
}


int dlms_getValueFromData(dlmsSettings* settings,
    gxReplyData* reply)
{
    uint16_t index;
    int ret;
#if !defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
    int pos;
    dlmsVARIANT_PTR tmp;
#endif //!defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
    dlmsVARIANT value;
    gxDataInfo info;
    di_init(&info);
    var_init(&value);
    if (reply->dataValue.vt == DLMS_DATA_TYPE_ARRAY)
    {
        info.type = DLMS_DATA_TYPE_ARRAY;
        info.count = (uint16_t)reply->totalCount;
        info.index = (uint16_t)reply->data.size;
    }
    index = (uint16_t)(reply->data.position);
    reply->data.position = reply->readPosition;
    if ((ret = dlms_getData(&reply->data, &info, &value)) != 0)
    {
        var_clear(&value);
        return ret;
    }
    // If new data.
    if (value.vt != DLMS_DATA_TYPE_NONE)
    {
        if (value.vt != DLMS_DATA_TYPE_ARRAY && value.vt != DLMS_DATA_TYPE_STRUCTURE)
        {
            reply->dataType = info.type;
            reply->dataValue = value;
            reply->totalCount = 0;
            if (reply->command == DLMS_COMMAND_DATA_NOTIFICATION)
            {
                reply->readPosition = reply->data.position;
            }
        }else
        {
            if (reply->dataValue.vt == DLMS_DATA_TYPE_NONE)
            {
                reply->dataValue = value;
            }
            else
            {
            #if !defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
                            for (pos = 0; pos != value.Arr->size; ++pos)
                            {
                                if ((ret = va_getByIndex(value.Arr, pos, &tmp)) != 0)
                                {
                                    return ret;
                                }
                                // va_push(reply->dataValue.Arr, tmp);
                            }
            #endif //! defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
            }
        }

        reply->readPosition = reply->data.position;
        // Element count.
        reply->totalCount = info.count;
    }
    else if (info.complete
        && reply->command == DLMS_COMMAND_DATA_NOTIFICATION)
    {
        // If last item is null. This is a special case.
        reply->readPosition = reply->data.position;
    }
    reply->data.position = index;

    // If last data frame of the data block is read.
    if (reply->command != DLMS_COMMAND_DATA_NOTIFICATION
        && info.complete && reply->moreData == DLMS_DATA_REQUEST_TYPES_NONE)
    {
        // If all blocks are read.
        resetBlockIndex(settings);
        reply->data.position = 0;
    }
    return 0;
}


unsigned char dlms_useDedicatedKey(dlmsSettings *settings)
{
    return settings->cipher.dedicatedKey != NULL;
}

/**
 * Get used glo message.
 *
 * @param command
 *            Executed DLMS command.
 * @return Integer value of glo message.
 */
unsigned char dlms_getGloMessage(dlmsSettings *settings, DLMS_COMMAND command, DLMS_COMMAND encryptedCommand)
{
    unsigned char cmd;
    unsigned gp = ((settings->negotiatedConformance & DLMS_CONFORMANCE_GENERAL_PROTECTION) != 0 &&
                   (settings->connected & DLMS_CONNECTION_STATE_DLMS) != 0) ||
                  // If pre-established connection.
                  dlms_usePreEstablishedConnection(settings) != 0;
    unsigned ded = dlms_useDedicatedKey(settings) && (settings->connected & DLMS_CONNECTION_STATE_DLMS) != 0;
    if (encryptedCommand == DLMS_COMMAND_GENERAL_GLO_CIPHERING)
    {
        cmd = encryptedCommand;
    }
    else if (gp && encryptedCommand == DLMS_COMMAND_NONE)
    {
        cmd = DLMS_COMMAND_GENERAL_GLO_CIPHERING;
    }
    else
    {
        switch (command)
        {
            case DLMS_COMMAND_METHOD_REQUEST:
                cmd = DLMS_COMMAND_GLO_METHOD_REQUEST;
                break;
        }
    }
    return cmd;
}


/*
   - this function is used to build data link layer
   - attributes
        - data (Data to send in the frame)
        - reply (Output buffer to hold the final HDLC frame)
*/
int dlms_getHdlcFrame(
    dlmsSettings *settings,
    int frame,
    gxByteBuffer *data,
    gxByteBuffer *reply)
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
        ret = bb_setUInt8(reply, getNextSend(settings, 1));
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
            // If all data is sent.
            if (data->size == data->position)
            {
                ret = bb_clear(data);
            }
            else
            {
                // Remove sent data.
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
    gxByteBuffer *bytes)
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
    else if (size == 4)
    {
        bb_setUInt32(bytes, address);
    }
    else
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_getAddress(int32_t value, uint32_t *address, int *size)
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
        *address = (uint32_t)((value & 0xFE00000) << 4 | (value & 0x1FC000) << 3 | (value & 0x3F80) << 2 | (value & 0x7F) << 1 | 1);
        *size = 4;
    }
    else
    {
        // Invalid address
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_addLLCBytes(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    int ret;
    if (settings->server)
    {
        ret = bb_insert(LLC_REPLY_BYTES, 3, data, 0);
    }
    else
    {
        ret = bb_insert(LLC_SEND_BYTES, 3, data, 0);
    }
    return ret;
}

unsigned char dlms_useHdlc(DLMS_INTERFACE_TYPE type)
{
#ifndef DLMS_IGNORE_HDLC
    return type == DLMS_INTERFACE_TYPE_HDLC ||
           type == DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E ||
           type == DLMS_INTERFACE_TYPE_PLC_HDLC;
#else
    return 0;
#endif // DLMS_IGNORE_HDLC
}

void dlms_multipleBlocks(
    gxLNParameters *p,
    gxByteBuffer *reply,
    unsigned char ciphering)
{
    // Check is all data fit to one message if data is given.
    int len = bb_available(p->data);
    if (p->attributeDescriptor != NULL)
    {
        len += p->attributeDescriptor->size;
    }
    if (ciphering)
    {
        len += CIPHERING_HEADER_SIZE;
    }
    if (!p->multipleBlocks)
    {
        // Add command type and invoke and priority.
        p->multipleBlocks = 2 + reply->size + len > p->settings->maxPduSize;
    }
    if (p->lastBlock)
    {
        // Add command type and invoke and priority.
        p->lastBlock = !(8 + reply->size + len > p->settings->maxPduSize);
    }
}

unsigned char dlms_getInvokeIDPriority(dlmsSettings *settings, unsigned char increase)
{
    unsigned char value = 0;
    if (settings->priority == DLMS_PRIORITY_HIGH)
    {
        value |= 0x80;
    }
    if (settings->serviceClass == DLMS_SERVICE_CLASS_CONFIRMED)
    {
        value |= 0x40;
    }
    if (increase)
    {
        settings->invokeID = (unsigned char)((1 + settings->invokeID) & 0xF);
    }
    value |= settings->invokeID;
    return value;
}

int dlms_getLNPdu(
    gxLNParameters *p,
    gxByteBuffer *reply)
{
    int ret = 0;
#ifndef DLMS_IGNORE_HIGH_GMAC
    unsigned char ciphering = (p->command != DLMS_COMMAND_AARQ && p->command != DLMS_COMMAND_AARE &&
                               p->settings->cipher.security != DLMS_SECURITY_NONE) ||
                              p->encryptedCommand != DLMS_COMMAND_NONE;
#else
    unsigned char ciphering = 0;
#endif // DLMS_IGNORE_HIGH_GMAC
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t len = 0;
#else
    uint16_t len = 0;
#endif
    if (p->command == DLMS_COMMAND_AARQ)
    {
        // Data is already added to reply when malloc is not used.
#ifndef DLMS_IGNORE_MALLOC
        if ((ret = bb_set2(reply, p->attributeDescriptor, 0, p->attributeDescriptor->size)) != 0)
        {
            return ret;
        }
#endif // DLMS_IGNORE_MALLOC
    }
    else
    {
        gxByteBuffer header;
        gxByteBuffer *h;
        if (p->settings->server)
        {
            // bb_attach(&header, pduAttributes, 0, sizeof(pduAttributes));
            // h = &header;
        }
        else
        {
#ifdef DLMS_IGNORE_MALLOC
            bb_attach(&header, pduAttributes, 0, sizeof(pduAttributes));
            h = &header;
#else
            h = reply;
#endif // DLMS_IGNORE_MALLOC
        }

        if (p->command != DLMS_COMMAND_GENERAL_BLOCK_TRANSFER)
        {
            ret = bb_setUInt8(h, (unsigned char)p->command);
        }
        if (p->command != DLMS_COMMAND_RELEASE_REQUEST)
        {
            if (p->command != DLMS_COMMAND_GET_REQUEST && p->data != NULL && p->data->size != 0)
            {
                dlms_multipleBlocks(p, h, ciphering);
            }
            ret = bb_setUInt8(h, p->requestType);
            // Add Invoke Id And Priority.
            if (p->invokeId != 0)
            {
                ret = bb_setUInt8(h, p->invokeId);
            }
            else
            {
                ret = bb_setUInt8(h, dlms_getInvokeIDPriority(p->settings, p->settings->autoIncreaseInvokeID));
            }
        }
#ifndef DLMS_IGNORE_MALLOC
        // Add attribute descriptor.
        if (ret == 0 && p->attributeDescriptor != NULL)
        {
            ret = bb_set2(reply, p->attributeDescriptor, p->attributeDescriptor->position, p->attributeDescriptor->size);
        }
#endif // DLMS_IGNORE_MALLOC
        if (ret == 0 &&
            p->command != DLMS_COMMAND_EVENT_NOTIFICATION &&
            p->command != DLMS_COMMAND_DATA_NOTIFICATION &&
            (p->settings->negotiatedConformance & DLMS_CONFORMANCE_GENERAL_BLOCK_TRANSFER) == 0)
        {
            int totalLength;
            // If multiple blocks.
        }

        // Add data that fits to one block.
        if (ret == 0 && len == 0)
        {
            // Add status if reply.
            if (p->status != 0xFF)
            {
                if (p->status != 0 && (p->command == DLMS_COMMAND_GET_RESPONSE))
                {
                    ret = bb_setUInt8(h, 1);
                }
                ret = bb_setUInt8(h, p->status);
            }
             if (ret == 0 && p->data != NULL && p->data->size != 0)
            {
                len = bb_available(p->data);
                if (len + reply->size > p->settings->maxPduSize)
                {
                    len = (uint16_t)(p->settings->maxPduSize - h->size - p->data->size - p->data->position);
                }
    
                ret = bb_set2(reply, p->data, p->data->position, len);

            }

        }
#ifndef DLMS_IGNORE_HIGH_GMAC
        if (ret == 0 && ciphering && reply->size != 0 && p->command != DLMS_COMMAND_RELEASE_REQUEST)
        {
#ifndef DLMS_IGNORE_MALLOC
            gxByteBuffer *key;
#else
            unsigned char *key;
#endif // DLMS_IGNORE_MALLOC
            if (p->settings->cipher.broadcast)
            {
#ifndef DLMS_IGNORE_MALLOC
                key = &p->settings->cipher.broadcastBlockCipherKey;
#else
                key = p->settings->cipher.broadcastBlockCipherKey;
#endif // DLMS_IGNORE_MALLOC
            }
            // else if (dlms_useDedicatedKey(p->settings) && (p->settings->connected & DLMS_CONNECTION_STATE_DLMS) != 0)
            // {
            //     key = p->settings->cipher.dedicatedKey;
            // }
            else
            {
#ifndef DLMS_IGNORE_MALLOC
                key = &p->settings->cipher.blockCipherKey;
#else
                key = p->settings->cipher.blockCipherKey;
#endif // DLMS_IGNORE_MALLOC
            }
#ifdef DLMS_TRACE_PDU
            cip_tracePdu(1, reply);
#endif // DLMS_TRACE_PDU
                        ret = cip_encrypt(
                            &p->settings->cipher,
                            p->settings->cipher.security,
                            DLMS_COUNT_TYPE_PACKET,
                            p->settings->cipher.invocationCounter,
                            dlms_getGloMessage(p->settings, p->command, p->encryptedCommand),
            #ifndef DLMS_IGNORE_MALLOC
                            p->settings->cipher.systemTitle.data,
            #else
                            p->settings->cipher.systemTitle,
            #endif //DLMS_IGNORE_MALLOC
                            key,
                            reply);
        }
#endif // DLMS_IGNORE_HIGH_GMAC1
    }
#ifndef DLMS_IGNORE_HDLC
    if (ret == 0 && dlms_useHdlc(p->settings->interfaceType))
    {
        ret = dlms_addLLCBytes(p->settings, reply);
    }
#endif // DLMS_IGNORE_HDLC
    return ret;
}


#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
int dlms_getDataFromBlock(gxByteBuffer* data, uint32_t index)
#else
int dlms_getDataFromBlock(gxByteBuffer* data, uint16_t index)
#endif
{
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t pos, len = data->position - index;
#else
    //EVS2 NOT SUPPORTED
#endif
    if (data->size == data->position)
    {
        bb_clear(data);
        return 0;
    }
    pos = data->position;
    bb_move(data, data->position, data->position - len, data->size - data->position);
    data->position = pos - len;
    return 0;
}

int dlms_verifyInvokeId(dlmsSettings* settings, gxReplyData* reply)
{
    if (settings->autoIncreaseInvokeID && reply->invokeId != dlms_getInvokeIDPriority(settings, 0))
    {
        //Invalid invoke ID.
        return DLMS_ERROR_CODE_INVALID_INVOKE_ID;
    }
    return 0;
}




int dlms_handleGetResponse(
    dlmsSettings* settings,
    gxReplyData* reply,
    uint32_t index)
{
    int ret;
    uint16_t count;
    unsigned char ch;
    uint32_t number;
    short type;
    // Get type.
    if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
    {
        return ret;
    }
    type = ch;
    // Get invoke ID and priority.
    if ((ret = bb_getUInt8(&reply->data, &reply->invokeId)) != 0)
    {
        return ret;
    }
    if ((ret = dlms_verifyInvokeId(settings, reply)) != 0)
    {
        return ret;
    }
    // Response normal
    if (type == 1)
    {
        // Result
        if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
        {
            return ret;
        }
        if (ch != 0)
        {
            if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
            {
                return ret;
            }
            return ch;
        }
        ret = dlms_getDataFromBlock(&reply->data, 0);
    }
    else if (type == 2)
    {
        // GetResponsewithDataBlock
        // Is Last block.
        if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
        {
            return ret;
        }
        if (ch == 0)
        {
            reply->moreData = (DLMS_DATA_REQUEST_TYPES)(reply->moreData | DLMS_DATA_REQUEST_TYPES_BLOCK);
        }
        else
        {
            reply->moreData =
                (DLMS_DATA_REQUEST_TYPES)(reply->moreData & ~DLMS_DATA_REQUEST_TYPES_BLOCK);
        }
        // Get Block number.
        if ((ret = bb_getUInt32(&reply->data, &number)) != 0)
        {
            return ret;
        }
        // If meter's block index is zero based or Actaris is read.
        // Actaris SL7000 might return wrong block index sometimes.
        // It's not reseted to 1.
        if (number != 1 && settings->blockIndex == 1)
        {
            settings->blockIndex = number;
        }
        else if (number != settings->blockIndex)
        {
            return DLMS_ERROR_CODE_DATA_BLOCK_NUMBER_INVALID;
        }
        // Get status.
        if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
        {
            return ret;
        }
        if (ch != 0)
        {
            if ((ret = bb_getUInt8(&reply->data, &ch)) != 0)
            {
                return ret;
            }
            return ch;
        }
        else
        {
            // Get data size.
            if ((ret = hlp_getObjectCount2(&reply->data, &count)) != 0)
            {
                return ret;
            }
            // if whole block is read.
            if ((reply->moreData & DLMS_DATA_REQUEST_TYPES_FRAME) == 0)
            {
                // Check Block length.
                if (count > (uint16_t)(bb_available(&reply->data)))
                {
                    return DLMS_ERROR_CODE_OUTOFMEMORY;
                }
                reply->command = DLMS_COMMAND_NONE;
            }
            if (count == 0)
            {
                // If meter sends empty data block.
                reply->data.size = index;
            }
            else
            {
                if ((ret = dlms_getDataFromBlock(&reply->data, index)) != 0)
                {
                    return ret;
                }
            }
            // If last packet and data is not try to peek.
            if (reply->moreData == DLMS_DATA_REQUEST_TYPES_NONE)
            {
                if (!reply->peek)
                {
                    reply->data.position = 0;
                    resetBlockIndex(settings);
                }
            }
        }
    }
    else
    {
        //Invalid Get response.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}

int dlms_generateChallenge(
    gxByteBuffer *challenge)
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

int dlms_getLnMessages(
    gxLNParameters *p,
    message *messages)
{
    int ret;
    gxByteBuffer *pdu;
    gxByteBuffer *it;
#ifndef DLMS_IGNORE_HDLC
    unsigned char frame = 0;
    if (p->command == DLMS_COMMAND_DATA_NOTIFICATION ||
        p->command == DLMS_COMMAND_EVENT_NOTIFICATION)
    {
        frame = 0x13;
    }
#endif // DLMS_IGNORE_HDLC
#ifdef DLMS_IGNORE_MALLOC
    pdu = p->serializedPdu;
#else
    gxByteBuffer reply;
    if (p->serializedPdu == NULL)
    {
        BYTE_BUFFER_INIT(&reply);
        pdu = &reply;
    }
    else
    {
        pdu = p->serializedPdu;
    }
#endif // DLMS_IGNORE_MALLOC
    do
    {
        if ((ret = dlms_getLNPdu(p, pdu)) == 0)
        {
            p->lastBlock = 1;
            if (p->attributeDescriptor == NULL)
            {
                ++p->settings->blockIndex;
            }
        }
        while (ret == 0 && pdu->position != pdu->size)
        {
#ifdef DLMS_IGNORE_MALLOC
            if (!(messages->size < messages->capacity))
            {
                ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
                break;
            }
            it = messages->data[messages->size];
            ++messages->size;
            bb_clear(it);
#else
            if (messages->attached)
            {
                if (messages->size < messages->capacity)
                {
                    it = messages->data[messages->size];
                    ++messages->size;
                    bb_clear(it);
                }
                else
                {
                    ret = DLMS_ERROR_CODE_OUTOFMEMORY;
                }
            }
            else
            {
                it = (gxByteBuffer *)gxmalloc(sizeof(gxByteBuffer));
                if (it == NULL)
                {
                    ret = DLMS_ERROR_CODE_OUTOFMEMORY;
                }
                else
                {
                    BYTE_BUFFER_INIT(it);
                    ret = mes_push(messages, it);
                }
            }
            if (ret != 0)
            {
                break;
            }
#endif // DLMS_IGNORE_MALLOC
            switch (p->settings->interfaceType)
            {

#ifndef DLMS_IGNORE_HDLC
            case DLMS_INTERFACE_TYPE_HDLC:
            case DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E:
                ret = dlms_getHdlcFrame(p->settings, frame, pdu, it);
                if (ret == 0 && pdu->position != pdu->size)
                {
                    printf("reached dlms.c getlnmessage");
                    frame = getNextSend(p->settings, 0);
                }
                break;
#endif // DLMS_IGNORE_HDLC
            case DLMS_INTERFACE_TYPE_PDU:
                ret = bb_set2(it, pdu, 0, pdu->size);
                break;
#ifndef DLMS_IGNORE_PLC
                // case DLMS_INTERFACE_TYPE_PLC:
                //     ret = dlms_getPlcFrame(p->settings, 0x90, pdu, it);
                //     break;
                // case DLMS_INTERFACE_TYPE_PLC_HDLC:
                //     ret = dlms_getMacHdlcFrame(p->settings, frame, 0, pdu, it);
                //     break;
#endif // DLMS_IGNORE_PLC
            default:
                ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
            if (ret != 0)
            {
                break;
            }
        }
        bb_clear(pdu);
#ifndef DLMS_IGNORE_HDLC
        frame = 0;
#endif // DLMS_IGNORE_HDLC
    } while (ret == 0 && p->data != NULL && p->data->position != p->data->size);
    return ret;
}

int dlms_getHDLCAddress(
    gxByteBuffer *buff,
    uint32_t *address,
    unsigned char checkClientAddress)
{
    unsigned char ch;
    uint16_t s, pos;
    uint32_t l;
    int ret, size = 0;
    for (pos = (uint16_t)buff->position; pos != (uint16_t)buff->size; ++pos)
    {
        ++size;
        if ((ret = bb_getUInt8ByIndex(buff, pos, &ch)) != 0)
        {
            return ret;
        }
        if ((ch & 0x1) == 1)
        {
            break;
        }
    }
    // DLMS CCT test requires that client size is one byte.
    if (checkClientAddress && size != 1)
    {
        return DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS;
    }

    if (size == 1)
    {
        if ((ret = bb_getUInt8(buff, &ch)) != 0)
        {
            return ret;
        }
        *address = ((ch & 0xFE) >> 1);
    }
    else if (size == 2)
    {
        if ((ret = bb_getUInt16(buff, &s)) != 0)
        {
            return ret;
        }
        *address = ((s & 0xFE) >> 1) | ((s & 0xFE00) >> 2);
    }
    else if (size == 4)
    {
        if ((ret = bb_getUInt32(buff, &l)) != 0)
        {
            return ret;
        }
        *address = ((l & 0xFE) >> 1) | ((l & 0xFE00) >> 2) | ((l & 0xFE0000) >> 3) | ((l & 0xFE000000) >> 4);
    }
    else
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_handleMethodResponse(
    dlmsSettings *settings,
    gxReplyData *data)
{
    int ret;
    unsigned char ch, type;
    // Get type.
    if ((ret = bb_getUInt8(&data->data, &type)) != 0)
    {
        return ret;
    }
    // Get invoke ID and priority.
    if ((ret = bb_getUInt8(&data->data, &data->invokeId)) != 0)
    {
        return ret;
    }
    if ((ret = dlms_verifyInvokeId(settings, data)) != 0)
    {
        return ret;
    }
    // Action-Response-Normal
    if (type == 1)
    {
        // Get Action-Result
        if ((ret = bb_getUInt8(&data->data, &ch)) != 0)
        {
            return ret;
        }
        if (ch != 0)
        {
            return ch;
        }
        resetBlockIndex(settings);
        // Get data if exists. Some meters do not return here anything.
        if (data->data.position < data->data.size)
        {
            // Get-Data-Result.
            if ((ret = bb_getUInt8(&data->data, &ch)) != 0)
            {
                return ret;
            }
            // If data.
            if (ch == 0)
            {
                // return dlms_getDataFromBlock(&data->data, 0);
            }
            else if (ch == 1) // Data-Access-Result
            {
                // Get Data-Access-Result
                if ((ret = bb_getUInt8(&data->data, &ch)) != 0)
                {
                    return ret;
                }
                if (ch != 0)
                {
                    if (ch == 9)
                    {
                        // Handle Texas Instrument missing byte here.
                        if ((ret = bb_getUInt8ByIndex(&data->data, data->data.position, &ch)) != 0)
                        {
                            return ret;
                        }
                        if (ch == 16)
                        {
                            --data->data.position;
                            // return dlms_getDataFromBlock(&data->data, 0);
                        }
                    }
                    if ((ret = bb_getUInt8(&data->data, &ch)) != 0)
                    {
                        return ret;
                    }
                    return ch;
                }
                return dlms_getDataFromBlock(&data->data, 0);
            }
            else
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
        }
    }
    else
    {
        return DLMS_ERROR_CODE_INVALID_COMMAND;
    }
    return DLMS_ERROR_CODE_OK;
}


//Return DLMS_ERROR_CODE_FALSE if LLC bytes are not included.
int dlms_checkLLCBytes(dlmsSettings* settings, gxByteBuffer* data)
{
    if (settings->server)
    {
        //Check LLC bytes.
        if (memcmp(data->data + data->position, LLC_SEND_BYTES, 3) != 0)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    else
    {
        //Check LLC bytes.
        if (memcmp(data->data + data->position, LLC_REPLY_BYTES, 3) != 0)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    data->position += 3;
    return DLMS_ERROR_CODE_OK;
}


int dlms_getPdu(
    dlmsSettings* settings,
    gxReplyData* data,
    unsigned char first)
{
    int ret = DLMS_ERROR_CODE_OK;
#if !defined(DLMS_IGNORE_CLIENT)
    uint32_t index;
#endif //!defined(DLMS_IGNORE_CLIENT)
    unsigned char ch;
    DLMS_COMMAND cmd = data->command;
    // If header is not read yet or GBT message.
    if (cmd == DLMS_COMMAND_NONE)
    {
        // If PDU is missing.
        if (bb_available(&data->data) == 0)
        {
            // Invalid PDU.
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
#if !defined(DLMS_IGNORE_CLIENT)
        index = data->data.position;
#endif //!defined(DLMS_IGNORE_CLIENT)
        // Get command.
        if ((ret = bb_getUInt8(&data->data, &ch)) != 0)
        {
            return ret;
        }
        cmd = (DLMS_COMMAND)ch;
        data->command = cmd;
        switch (cmd)
        {
        #if !defined(DLMS_IGNORE_CLIENT)
        #if !defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
//         case DLMS_COMMAND_READ_RESPONSE:
//             if ((ret = dlms_handleReadResponse(settings, data, (uint16_t)index)) != 0)
//             {
//                 if (ret == DLMS_ERROR_CODE_FALSE)
//                 {
//                     return DLMS_ERROR_CODE_OK;
//                 }
//                 return ret;
//             }
//             break;
// #endif //!defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
        case DLMS_COMMAND_GET_RESPONSE:
            if ((ret = dlms_handleGetResponse(settings, data, index)) != 0)
            {
                if (ret == DLMS_ERROR_CODE_FALSE)
                {
                    return DLMS_ERROR_CODE_OK;
                }
                return ret;
            }
            break;
//         case DLMS_COMMAND_SET_RESPONSE:
//             ret = dlms_handleSetResponse(settings, data);
//             break;
// #if !defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
//         case DLMS_COMMAND_WRITE_RESPONSE:
//             ret = dlms_handleWriteResponse(data);
//             break;
// #endif //!defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
        case DLMS_COMMAND_METHOD_RESPONSE:
            ret = dlms_handleMethodResponse(settings, data);
            break;
//         case DLMS_COMMAND_GENERAL_BLOCK_TRANSFER:
//             ret = dlms_handleGbt(settings, data);
//             break;
//         case DLMS_COMMAND_CONFIRMED_SERVICE_ERROR:
//             ret = dlms_handleConfirmedServiceError(&data->data);
//             break;
//         case DLMS_COMMAND_EXCEPTION_RESPONSE:
//             ret = dlms_handleExceptionResponse(&data->data);
//             break;
        #endif //!defined(DLMS_IGNORE_CLIENT)
        case DLMS_COMMAND_AARQ:
        case DLMS_COMMAND_AARE:
            // This is parsed later.
            data->data.position -= 1;
            break;
//         case DLMS_COMMAND_RELEASE_RESPONSE:
//             break;
// #if !defined(DLMS_IGNORE_SERVER)
//         case DLMS_COMMAND_GET_REQUEST:
// #if !defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
//         case DLMS_COMMAND_READ_REQUEST:
//         case DLMS_COMMAND_WRITE_REQUEST:
// #endif //!defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME) && !defined(DLMS_IGNORE_MALLOC)
//         case DLMS_COMMAND_SET_REQUEST:
//         case DLMS_COMMAND_METHOD_REQUEST:
//         case DLMS_COMMAND_RELEASE_REQUEST:
//             // Server handles this.
//             if ((data->moreData & DLMS_DATA_REQUEST_TYPES_FRAME) != 0)
//             {
//                 break;
//             }
//             break;
// #endif //!defined(DLMS_IGNORE_SERVER)
// #ifndef DLMS_IGNORE_HIGH_GMAC
// #if !defined(DLMS_IGNORE_SERVER)
//         case DLMS_COMMAND_GLO_READ_REQUEST:
//         case DLMS_COMMAND_GLO_WRITE_REQUEST:
//         case DLMS_COMMAND_GLO_GET_REQUEST:
//         case DLMS_COMMAND_GLO_SET_REQUEST:
//         case DLMS_COMMAND_GLO_METHOD_REQUEST:
//         case DLMS_COMMAND_DED_GET_REQUEST:
//         case DLMS_COMMAND_DED_SET_REQUEST:
//         case DLMS_COMMAND_DED_METHOD_REQUEST:
//             ret = dlms_handleGloDedRequest(settings, data);
//             // Server handles this.
//             break;
// #endif //!defined(DLMS_IGNORE_SERVER)
#if !defined(DLMS_IGNORE_CLIENT)
        case DLMS_COMMAND_GLO_METHOD_RESPONSE:
            // If all frames are read.
            ret = dlms_handleGloDedResponse(settings, data, index);
            break;
#endif // !defined(DLMS_IGNORE_CLIENT)
//         case DLMS_COMMAND_GENERAL_GLO_CIPHERING:
//         case DLMS_COMMAND_GENERAL_DED_CIPHERING:
// #if !defined(DLMS_IGNORE_SERVER)
//             if (settings->server)
//             {
//                 ret = dlms_handleGloDedRequest(settings, data);
//             }
// #endif// !defined(DLMS_IGNORE_CLIENT)
// #if !defined(DLMS_IGNORE_CLIENT)
// #if !defined(DLMS_IGNORE_SERVER)
//             else
// #endif // !defined(DLMS_IGNORE_SERVER)
//             {
//                 ret = dlms_handleGloDedResponse(settings, data, index);
//             }
// #endif //!defined(DLMS_IGNORE_CLIENT)
//             break;
// #if !defined(DLMS_IGNORE_GENERAL_CIPHERING) && !defined(DLMS_IGNORE_HIGH_GMAC)
//         case DLMS_COMMAND_GENERAL_CIPHERING:
//             ret = dlms_handleGeneralCiphering(settings, data);
//             break;
// #endif //!defined(DLMS_IGNORE_GENERAL_CIPHERING) && !defined(DLMS_IGNORE_HIGH_GMAC)
// #endif //DLMS_IGNORE_HIGH_GMAC
//         case DLMS_COMMAND_DATA_NOTIFICATION:
//             ret = dlms_handleDataNotification(settings, data);
//             // Client handles this.
//             break;
//         case DLMS_COMMAND_EVENT_NOTIFICATION:
//             // Client handles this.
//             break;
//         case DLMS_COMMAND_INFORMATION_REPORT:
//             // Client handles this.
//             break;
//         default:
//             // Invalid command.
//             return DLMS_ERROR_CODE_INVALID_COMMAND;
//         }
//     }
//     else if ((data->moreData & DLMS_DATA_REQUEST_TYPES_FRAME) == 0)
//     {
//         // Is whole block is read and if last packet and data is not try to
//         // peek.
//         if (!data->peek && data->moreData == DLMS_DATA_REQUEST_TYPES_NONE)
//         {
//             if (!settings->server || data->command == DLMS_COMMAND_AARE || data->command == DLMS_COMMAND_AARQ)
//             {
//                 data->data.position = 0;
//             }
//             else
//             {
//                 data->data.position = 1;
//             }
//         }
//         if (cmd == DLMS_COMMAND_GENERAL_BLOCK_TRANSFER)
//         {
//             data->data.position = data->cipherIndex + 1;
//             ret = dlms_handleGbt(settings, data);
//             data->cipherIndex = (uint16_t)data->data.size;
//             data->command = DLMS_COMMAND_NONE;
//         }
//         // Get command if operating as a server.
// #ifndef DLMS_IGNORE_SERVER
//         if (settings->server)
//         {
// #ifndef DLMS_IGNORE_HIGH_GMAC
//             // Ciphered messages are handled after whole PDU is received.
//             switch (cmd)
//             {
//             case DLMS_COMMAND_GLO_READ_REQUEST:
//             case DLMS_COMMAND_GLO_WRITE_REQUEST:
//             case DLMS_COMMAND_GLO_GET_REQUEST:
//             case DLMS_COMMAND_GLO_SET_REQUEST:
//             case DLMS_COMMAND_GLO_METHOD_REQUEST:
//             case DLMS_COMMAND_GENERAL_GLO_CIPHERING:
//             case DLMS_COMMAND_GENERAL_DED_CIPHERING:
//             case DLMS_COMMAND_GENERAL_CIPHERING:
//                 data->command = DLMS_COMMAND_NONE;
//                 data->data.position = (data->cipherIndex);
//                 ret = dlms_getPdu(settings, data, 0);
//                 break;
//             default:
//                 break;
//             }
// #endif //DLMS_IGNORE_HIGH_GMAC
//         }
//         else
// #endif //DLMS_IGNORE_SERVER
//         {
//             // Client do not need a command any more.
//             data->command = DLMS_COMMAND_NONE;
// #ifndef DLMS_IGNORE_HIGH_GMAC
//             // Ciphered messages are handled after whole PDU is received.
//             switch (cmd)
//             {
//             case DLMS_COMMAND_GLO_READ_RESPONSE:
//             case DLMS_COMMAND_GLO_WRITE_RESPONSE:
//             case DLMS_COMMAND_GLO_GET_RESPONSE:
//             case DLMS_COMMAND_GLO_SET_RESPONSE:
//             case DLMS_COMMAND_GLO_METHOD_RESPONSE:
//             case DLMS_COMMAND_DED_GET_RESPONSE:
//             case DLMS_COMMAND_DED_SET_RESPONSE:
//             case DLMS_COMMAND_DED_METHOD_RESPONSE:
//             case DLMS_COMMAND_GENERAL_GLO_CIPHERING:
//             case DLMS_COMMAND_GENERAL_DED_CIPHERING:
//             case DLMS_COMMAND_GENERAL_CIPHERING:
//                 data->data.position = data->cipherIndex;
//                 ret = dlms_getPdu(settings, data, 0);
//                 break;
//             default:
//                 break;
//             }
#endif //DLMS_IGNORE_HIGH_GMAC
        }
    }

// #if !defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
// #if !defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME)
//     // Get data only blocks if SN is used. This is faster.
//     if (ret == 0 && cmd == DLMS_COMMAND_READ_RESPONSE
//         && data->commandType == DLMS_SINGLE_READ_RESPONSE_DATA_BLOCK_RESULT
//         && (data->moreData & DLMS_DATA_REQUEST_TYPES_FRAME) != 0)
//     {
//         return 0;
//     }
// #endif //!defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME)
//     // Get data if all data is read or we want to peek data.
    if (ret == 0 && !data->ignoreValue && data->data.position != data->data.size
        && (
#if !defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME)
            cmd == DLMS_COMMAND_READ_RESPONSE ||
#endif //!defined(DLMS_IGNORE_ASSOCIATION_SHORT_NAME)
            cmd == DLMS_COMMAND_GET_RESPONSE)
        && (data->moreData == DLMS_DATA_REQUEST_TYPES_NONE
            || data->peek))
    {
        ret = dlms_getValueFromData(settings, data);
    }
// #else
//     data->dataValue.byteArr = &data->data;
//     data->dataValue.vt = DLMS_DATA_TYPE_BYREF | DLMS_DATA_TYPE_OCTET_STRING;
// #endif //!defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
    return ret;
}


#if !defined(DLMS_IGNORE_CLIENT)
int dlms_handleGloDedResponse(dlmsSettings *settings,
                              gxReplyData *data, uint32_t index)
{
#ifdef DLMS_IGNORE_HIGH_GMAC
    return DLMS_ERROR_CODE_NOT_IMPLEMENTED;
#else
    int ret = 0;
    DLMS_SECURITY_SUITE suite;
    uint64_t invocationCounter;
    if ((data->moreData & DLMS_DATA_REQUEST_TYPES_FRAME) == 0)
    {
        DLMS_SECURITY security;
        --data->data.position;
        data->data.position = index;
        if ((settings->connected & DLMS_CONNECTION_STATE_DLMS) != 0 && dlms_useDedicatedKey(settings))
        {
            if ((ret = cip_decrypt(&settings->cipher,
                                   settings->sourceSystemTitle,
                                   settings->cipher.dedicatedKey,
                                   &data->data,
                                   &security,
                                   &suite,
                                   &invocationCounter)) != 0)
            {
                return ret;
            }
        }
        else
        {
            if ((ret = cip_decrypt(&settings->cipher,
                                   settings->sourceSystemTitle,
#ifndef DLMS_IGNORE_MALLOC
                                   &settings->cipher.blockCipherKey,
#else
                                   settings->cipher.blockCipherKey,
#endif // DLMS_IGNORE_MALLOC
                                   &data->data,
                                   &security,
                                   &suite,
                                   &invocationCounter)) != 0)
            {
                return ret;
            }
        }
#ifdef DLMS_TRACE_PDU
        if (ret == 0)
        {
            cip_tracePdu(0, &data->data);
        }
#endif // DLMS_TRACE_PDU
       // If target is sending data ciphered using different security policy.
        if (settings->cipher.security != security)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }
#ifdef DLMS_INVOCATION_COUNTER_VALIDATOR
        if (svr_validateInvocationCounter(settings, invocationCounter) != 0)
        {
            return DLMS_ERROR_CODE_INVOCATION_COUNTER_TOO_SMALL;
        }
#else
        if (settings->expectedInvocationCounter != NULL)
        {
            if (invocationCounter <= *settings->expectedInvocationCounter)
            {
                // return DLMS_ERROR_CODE_INVOCATION_COUNTER_TOO_SMALL;
            }
            // Update IC.
#ifdef DLMS_COSEM_INVOCATION_COUNTER_SIZE64
            *settings->expectedInvocationCounter = (invocationCounter);
#else
            *settings->expectedInvocationCounter = (uint32_t)(invocationCounter);
#endif // DLMS_COSEM_INVOCATION_COUNTER_SIZE64
        }
#endif // DLMS_INVOCATION_COUNTER_VALIDATOR
        data->command = DLMS_COMMAND_NONE;
        ret = dlms_getPdu(settings, data, 0);
        data->cipherIndex = (uint16_t)data->data.size;
    }
    return ret;
#endif // DLMS_IGNORE_HIGH_GMAC
}
#endif //! defined(DLMS_IGNORE_CLIENT)

int dlms_checkHdlcAddress(
    unsigned char server,
    dlmsSettings *settings,
    gxByteBuffer *reply,
    uint16_t index)
{
    unsigned char ch;
    int ret;
    uint32_t source, target;
    // Get destination and source addresses.
    if ((ret = dlms_getHDLCAddress(reply, &target, 0)) != 0)
    {
        return ret;
    }
    if ((ret = dlms_getHDLCAddress(reply, &source, server)) != 0)
    {
        return ret;
    }
    if (server)
    {
        // Check that server addresses match.
        // if (settings->serverAddress != 0 && settings->serverAddress != target)
        // {
        //     // Get frame command.
        //     if (bb_getUInt8ByIndex(reply, reply->position, &ch) != 0)
        //     {
        //         return DLMS_ERROR_CODE_INVALID_SERVER_ADDRESS;
        //     }
        //     return DLMS_ERROR_CODE_INVALID_SERVER_ADDRESS;
        // }
        // else
        // {
        //     settings->serverAddress = target;
        // }

        // // Check that client addresses match.
        // if (settings->clientAddress != 0 && settings->clientAddress != source)
        // {
        //     // Get frame command.
        //     if (bb_getUInt8ByIndex(reply, reply->position, &ch) != 0)
        //     {
        //         return DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS;
        //     }
        //     //If SNRM and client has not call disconnect and changes client ID.
        //     if (ch == DLMS_COMMAND_SNRM)
        //     {
        //         settings->clientAddress = (uint16_t)source;
        //     }
        //     else
        //     {
        //         return DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS;
        //     }
        // }
        // else
        // {
        //     settings->clientAddress = (uint16_t)source;
        // }
    }
    else
    {
        // Check that client addresses match.
        if (settings->clientAddress != target)
        {
            // If echo.
            if (settings->clientAddress == source && settings->serverAddress == target)
            {
                reply->position = index + 1;
            }
            return DLMS_ERROR_CODE_FALSE;
        }
        // Check that server addresses match.
        if (settings->serverAddress != source &&
            // If All-station (Broadcast).
            (settings->serverAddress & 0x7F) != 0x7F && (settings->serverAddress & 0x3FFF) != 0x3FFF)
        {
            // Check logical and physical address separately.
            // This is done because some meters might send four bytes
            // when only two bytes are needed.
            uint32_t readLogical, readPhysical, logical, physical;
            // dlms_getServerAddress(source, &readLogical, &readPhysical);
            // dlms_getServerAddress(settings->serverAddress, &logical, &physical);
            if (readLogical != logical || readPhysical != physical)
            {
                return DLMS_ERROR_CODE_FALSE;
            }
        }
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_getHdlcData(
    unsigned char server,
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    unsigned char *frame,
    unsigned char preEstablished,
    unsigned char first)
{
    int ret;
    unsigned char ch;
    uint16_t eopPos;
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t pos, frameLen = 0;
    uint32_t packetStartID = reply->position;
#else
    uint16_t pos, frameLen = 0;
    uint16_t packetStartID = (uint16_t)reply->position;
#endif
    uint16_t crc, crcRead;
    // If whole frame is not received yet.
    if (reply->size - reply->position < 9)
    {
        data->complete = 0;
        return 0;
    }
    data->complete = 1;
    // Find start of HDLC frame.
    for (pos = (uint16_t)reply->position; pos < reply->size; ++pos)
    {
        if ((ret = bb_getUInt8(reply, &ch)) != 0)
        {
            return ret;
        }
        if (ch == HDLC_FRAME_START_END)
        {
            packetStartID = pos;
            break;
        }
    }
    // Not a HDLC frame.
    // Sometimes meters can send some strange data between DLMS frames.
    if (reply->position == reply->size)
    {
        data->complete = 0;
        // Not enough data to parse;
        return 0;
    }
    if ((ret = bb_getUInt8(reply, frame)) != 0)
    {
        return ret;
    }
    if ((*frame & 0xF0) != 0xA0)
    {
        --reply->position;
        // If same data.
        return dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
    }
    // Check frame length.
    if ((*frame & 0x7) != 0)
    {
        frameLen = ((*frame & 0x7) << 8);
    }
    if ((ret = bb_getUInt8(reply, &ch)) != 0)
    {
        return ret;
    }
    // If not enough data.
    frameLen += ch;
    if ((reply->size - reply->position + 1) < frameLen)
    {
        data->complete = 0;
        reply->position = packetStartID;
        // Not enough data to parse;
        return 0;
    }
    eopPos = (uint16_t)(frameLen + packetStartID + 1);
    if ((ret = bb_getUInt8ByIndex(reply, eopPos, &ch)) != 0)
    {
        return ret;
    }
    if (ch != HDLC_FRAME_START_END)
    {
        reply->position -= 2;
        return dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
    }

    // Check addresses.
    ret = dlms_checkHdlcAddress(server, settings, reply, eopPos);
    if (ret != 0)
    {
        // If pre-established client address has change.
        if (ret == DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS)
        {
            return DLMS_ERROR_CODE_INVALID_CLIENT_ADDRESS;
        }
        // else
        // {
        //     if (ret == DLMS_ERROR_CODE_INVALID_SERVER_ADDRESS &&
        //         reply->position + 4 == reply->size)
        //     {
        //         data->packetLength = 0;
        //         bb_clear(reply);
        //         return ret;
        //     }
        //     if (ret == DLMS_ERROR_CODE_FALSE)
        //     {
        //         // If echo or reply to other meter.
        //         return dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
        //     }
        //     reply->position = packetStartID + 1;
        //     ret = dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
        //     return ret;
        // }
    }
    // Is there more data available.
    unsigned char moreData = (*frame & 0x8) != 0;
    // Get frame type.
    if ((ret = bb_getUInt8(reply, frame)) != 0)
    {
        return ret;
    }

    // Is there more data available.
    if (moreData)
    {
        data->moreData |= DLMS_DATA_REQUEST_TYPES_FRAME;
    }
    else
    {
        data->moreData = ((DLMS_DATA_REQUEST_TYPES)(data->moreData & ~DLMS_DATA_REQUEST_TYPES_FRAME));
    }

    if (!preEstablished
#ifndef DLMS_IGNORE_HDLC_CHECK
        && !checkFrame(settings, *frame)
#endif // DLMS_IGNORE_HDLC_CHECK
    )
    {
        // reply->position = eopPos + 1;
        // if (settings->server)
        // {
        //     return DLMS_ERROR_CODE_INVALID_FRAME_NUMBER;
        // }
        // return dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
    }
    // Check that header CRC is correct.
    crc = countCRC(reply, packetStartID + 1,
                   reply->position - packetStartID - 1);

    if ((ret = bb_getUInt16(reply, &crcRead)) != 0)
    {
        return ret;
    }

    if (crc != crcRead)
    {
        if (reply->size - reply->position > 8)
        {
            return dlms_getHdlcData(server, settings, reply, data, frame, preEstablished, first);
        }
#ifdef DLMS_DEBUG
        svr_notifyTrace("Invalid CRC. ", -1);
#endif // DLMS_DEBUG
        return DLMS_ERROR_CODE_WRONG_CRC;
    }
    // Check that packet CRC match only if there is a data part.
    if (reply->position != packetStartID + frameLen + 1)
    {
        crc = countCRC(reply, packetStartID + 1, frameLen - 2);
        if ((ret = bb_getUInt16ByIndex(reply, packetStartID + frameLen - 1, &crcRead)) != 0)
        {
            return ret;
        }
        if (crc != crcRead)
        {
#ifdef DLMS_DEBUG
            svr_notifyTrace("Invalid CRC. ", -1);
#endif // DLMS_DEBUG
            return DLMS_ERROR_CODE_WRONG_CRC;
        }
        // Remove CRC and EOP from packet length.
        data->packetLength = eopPos - 2;
    }
    else
    {
        data->packetLength = eopPos - 2;
    }

    if (*frame != 0x13 && *frame != 0x3 && (*frame & HDLC_FRAME_TYPE_U_FRAME) == HDLC_FRAME_TYPE_U_FRAME)
    {
        // Get Eop if there is no data.
        if (reply->position == packetStartID + frameLen + 1)
        {
            // Get EOP.
            if ((ret = bb_getUInt8(reply, &ch)) != 0)
            {
                return ret;
            }
        }
        data->command = (DLMS_COMMAND)*frame;
        switch (data->command)
        {
        case DLMS_COMMAND_SNRM:
        case DLMS_COMMAND_UA:
        case DLMS_COMMAND_DISCONNECT_MODE:
        case DLMS_COMMAND_REJECTED:
        case DLMS_COMMAND_DISC:
            break;
        default:
            // Unknown command.
            return DLMS_ERROR_CODE_REJECTED;
        }
    }
    // else if (*frame != 0x13 && *frame != 0x3 && (*frame & HDLC_FRAME_TYPE_S_FRAME) == HDLC_FRAME_TYPE_S_FRAME)
    // {
    //     // If S-frame
    //     int tmp = (*frame >> 2) & 0x3;
    //     // If frame is rejected.
    //     if (tmp == HDLC_CONTROL_FRAME_REJECT)
    //     {
    //         return DLMS_ERROR_CODE_REJECTED;
    //     }
    //     else if (tmp == HDLC_CONTROL_FRAME_RECEIVE_NOT_READY)
    //     {
    //         return DLMS_ERROR_CODE_REJECTED;
    //     }
    //     else if (tmp == HDLC_CONTROL_FRAME_RECEIVE_READY)
    //     {
    //     }
    //     // Get Eop if there is no data.
    //     if (reply->position == packetStartID + frameLen + 1)
    //     {
    //         // Get EOP.
    //         if ((ret = bb_getUInt8(reply, &ch)) != 0)
    //         {
    //             return ret;
    //         }
    //     }
    // }
    else
    {
        // I-frame
        // Get Eop if there is no data.
        if (reply->position == packetStartID + frameLen + 1)
        {
            // Get EOP.
            if ((ret = bb_getUInt8(reply, &ch)) != 0)
            {
                return ret;
            }
            if ((*frame & 0x1) == 0x1)
            {
                data->moreData = DLMS_DATA_REQUEST_TYPES_FRAME;
            }
        }   
        else
        {
            dlms_checkLLCBytes(settings, reply);
        }
    }
    if (settings->server && (first || data->command == DLMS_COMMAND_SNRM))
    {
        printf("reached dlmssettings gethdlc function");
    }
    return DLMS_ERROR_CODE_OK;
}

int dlms_getDataFromFrame(
    gxByteBuffer *reply,
    gxReplyData *data,
    unsigned char hdlc)
{
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t offset = data->data.size;
    uint32_t cnt;
#else
    uint16_t offset = data->data.size;
    uint16_t cnt;
#endif
    if (data->packetLength < reply->position)
    {
        cnt = 0;
    }
    else
    {
        cnt = data->packetLength - reply->position;
    }
    if (cnt != 0)
    {
        int ret;
        if ((ret = bb_capacity(&data->data, offset + cnt)) != 0 ||
            (ret = bb_set2(&data->data, reply, reply->position, cnt)) != 0)
        {
            return ret;
        }
        if (hdlc)
        {
            reply->position += 3;
        }
    }
    // Set position to begin of new data.
    data->data.position = offset;
    return 0;
}

int dlms_getData3(
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    gxReplyData *notify,
    unsigned char first,
    unsigned char *isNotify)
{
    int ret;
    unsigned char frame = 0;
    if (isNotify != NULL)
    {
        *isNotify = 0;
    }
    switch (settings->interfaceType)
    {
#ifndef DLMS_IGNORE_HDLC
    case DLMS_INTERFACE_TYPE_HDLC:
    case DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E:
        ret = dlms_getHdlcData(settings->server, settings, reply, data, &frame, data->preEstablished, first);
        break;
#endif // DLMS_IGNORE_HDLC
    case DLMS_INTERFACE_TYPE_PDU:
        data->packetLength = reply->size;
        data->complete = reply->size != 0;
        ret = 0;
        break;
    default:
        // Invalid Interface type.
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        break;
    }
    if (ret != 0)
    {
        return ret;
    }
    if (*isNotify && notify != NULL)
    {
        if (!notify->complete)
        {
            // If all data is not read yet.
            return 0;
        }
        data = notify;
    }
    else if (!data->complete)
    {
        // If all data is not read yet.
        return 0;
    }
    if (settings->interfaceType != DLMS_INTERFACE_TYPE_PLC_HDLC)
    {
        if ((ret = dlms_getDataFromFrame(reply, data, dlms_useHdlc(settings->interfaceType))) != 0)
        {
            return ret;
        }
    }
    // If keepalive or get next frame request.
    if (((frame != 0x13 && frame != 0x3) || data->moreData != DLMS_DATA_REQUEST_TYPES_NONE) && (frame & 0x1) != 0)
    {
        if (dlms_useHdlc(settings->interfaceType) && data->data.size != 0)
        {
            if (reply->position != reply->size)
            {
                reply->position += 3;
            }
        }
        if (data->command == DLMS_COMMAND_REJECTED)
        {
            return DLMS_ERROR_CODE_REJECTED;
        }
        return DLMS_ERROR_CODE_OK;
    }
    ret = dlms_getPdu(settings, data, first);
    return ret;
}

int dlms_parseSnrmUaResponse(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    uint32_t value;
    unsigned char ch, id, len;
    uint16_t tmp;
    int ret;
    // If default settings are used.
    if (data->size - data->position == 0)
    {
        return 0;
    }
    // Skip FromatID
    if ((ret = bb_getUInt8(data, &ch)) != 0)
    {
        return ret;
    }
    // Skip Group ID.
    if ((ret = bb_getUInt8(data, &ch)) != 0)
    {
        return ret;
    }
    // Skip Group len
    if ((ret = bb_getUInt8(data, &ch)) != 0)
    {
        return ret;
    }
    while (data->position < data->size)
    {
        if ((ret = bb_getUInt8(data, &id)) != 0 ||
            (ret = bb_getUInt8(data, &len)) != 0)
        {
            return ret;
        }
        switch (len)
        {
        case 1:
            ret = bb_getUInt8(data, &ch);
            value = ch;
            break;
        case 2:
            ret = bb_getUInt16(data, &tmp);
            value = tmp;
            break;
        case 4:
            ret = bb_getUInt32(data, &value);
            break;
        default:
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        if (ret != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
        // RX / TX are delivered from the partner's point of view =>
        // reversed to ours
        switch (id)
        {
        case HDLC_INFO_MAX_INFO_TX:
            if (value < settings->maxInfoRX)
            {
                settings->maxInfoRX = (uint16_t)value;
            }
            break;
        case HDLC_INFO_MAX_INFO_RX:
            if (value < settings->maxInfoTX)
            {
                settings->maxInfoTX = (uint16_t)value;
            }
            break;
        case HDLC_INFO_WINDOW_SIZE_TX:
            if (value < settings->windowSizeRX)
            {
                settings->windowSizeRX = (unsigned char)value;
            }
            break;
        case HDLC_INFO_WINDOW_SIZE_RX:
            if (value < settings->windowSizeTX)
            {
                settings->windowSizeTX = (unsigned char)value;
            }
            break;
        default:
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
            break;
        }
    }
    return ret;
}

int dlms_setData(gxByteBuffer *buff, DLMS_DATA_TYPE type, dlmsVARIANT *value)
{
#ifndef DLMS_IGNORE_MALLOC
    int ret;
    ret = var_changeType(value, type);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
#endif // DLMS_IGNORE_MALLOC
    return var_getBytes2(value, type, buff);
}

int dlms_secure(
    dlmsSettings *settings,
    int32_t ic,
    gxByteBuffer *data,
    gxByteBuffer *secret,
    gxByteBuffer *reply)
{
    int ret = 0;
    gxByteBuffer challenge;
    bb_clear(reply);

    BYTE_BUFFER_INIT(&challenge);

    // Get server Challenge.
    // Get shared secret
  if (settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
    {
        ret = cip_encrypt(
            &settings->cipher,
            DLMS_SECURITY_AUTHENTICATION,
            DLMS_COUNT_TYPE_TAG,
            ic,
            GET_AUTH_TAG(settings->cipher),
            secret->data,
            &settings->cipher.blockCipherKey,
            data);
        if (ret == 0)
        {
            if ((ret = bb_setUInt8(reply, (unsigned char)DLMS_SECURITY_AUTHENTICATION | (unsigned char)settings->cipher.suite)) != 0 ||
                (ret = bb_setUInt32(reply, ic)) != 0 ||
                (ret = bb_set(reply, data->data, 12)) != 0)
            {
            }
        }
        bb_clear(&challenge);
    }
    return ret;
}

unsigned char dlms_usePreEstablishedConnection(dlmsSettings *settings)
{
#ifndef DLMS_IGNORE_MALLOC
    return settings->preEstablishedSystemTitle != NULL;
#else
    return memcmp(settings->preEstablishedSystemTitle, EMPTY_SYSTEM_TITLE, 8) != 0;
#endif // DLMS_IGNORE_MALLOC
}

int dlms_checkInit(dlmsSettings *settings)
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