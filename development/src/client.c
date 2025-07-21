
#include "../include/client.h"


#include "../include/dlms.h"
#include "../include/cosem.h"
#include "../include/gxmem.h"
#include "../include/dlmssettings.h"
#include "../include/message.h"
#include "../include/errorcodes.h"
#include "../include/bytebuffer.h"
#include "../include/parameters.h"

int cl_snrmRequest(dlmsSettings* settings, message* messages)
{
    int ret;
    gxByteBuffer* reply;
    gxByteBuffer* pData;
    mes_clear(messages);
    //Save default values.
    settings->initializeMaxInfoTX = settings->maxInfoTX;
    settings->initializeMaxInfoRX = settings->maxInfoRX;
    settings->initializeWindowSizeTX = settings->windowSizeTX;
    settings->initializeWindowSizeRX = settings->windowSizeRX;
    settings->connected = DLMS_CONNECTION_STATE_NONE;
    settings->isAuthenticationRequired = 0;

    if (settings->interfaceType != DLMS_INTERFACE_TYPE_HDLC && settings->interfaceType != DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E)
    {
        return 0;
    }
    if (settings->interfaceType == DLMS_INTERFACE_TYPE_WRAPPER)
    {
        return DLMS_ERROR_CODE_OK;
    }
    resetFrameSequence(settings);
#ifdef DLMS_IGNORE_MALLOC
    //EVS2 NOT SUPPORTED
#else
    reply = (gxByteBuffer*)gxmalloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(reply);
    gxByteBuffer bb;
    BYTE_BUFFER_INIT(&bb);
    if ((ret = bb_capacity(&bb, 30)) != 0)
    {
        return ret;
    }
    pData = &bb;
#endif //DLMS_IGNORE_MALLOC

    // FromatID
    if ((ret = bb_setUInt8(pData, 0x81)) == 0 &&
        // GroupID
        (ret = bb_setUInt8(pData, 0x80)) == 0 &&
        // Length is updated later.
        (ret = bb_setUInt8(pData, 0)) == 0)
    {
        // If custom HDLC parameters are used.
        if (ret == 0 &&
            (DEFAULT_MAX_INFO_TX != settings->maxInfoTX ||
                DEFAULT_MAX_INFO_RX != settings->maxInfoRX ||
                DEFAULT_MAX_WINDOW_SIZE_TX != settings->windowSizeTX ||
                DEFAULT_MAX_WINDOW_SIZE_RX != settings->windowSizeRX))
        {
            // if ((ret = bb_setUInt8(pData, HDLC_INFO_MAX_INFO_TX)) != 0 ||
            //     (ret = dlms_appendHdlcParameter(pData, settings->maxInfoTX)) != 0 ||
            //     (ret = bb_setUInt8(pData, HDLC_INFO_MAX_INFO_RX)) != 0 ||
            //     (ret = dlms_appendHdlcParameter(pData, settings->maxInfoRX)) != 0 ||
            //     (ret = bb_setUInt8(pData, HDLC_INFO_WINDOW_SIZE_TX)) != 0 ||
            //     (ret = bb_setUInt8(pData, 4)) != 0 ||
            //     (ret = bb_setUInt32(pData, settings->windowSizeTX)) != 0 ||
            //     (ret = bb_setUInt8(pData, HDLC_INFO_WINDOW_SIZE_RX)) != 0 ||
            //     (ret = bb_setUInt8(pData, 4)) != 0 ||
            //     (ret = bb_setUInt32(pData, settings->windowSizeRX)) != 0)
            // {
            //     //Error is returned in the end of this method.
            // }
        }
        // If default HDLC parameters are not used.
        if (ret == 0)
        {
            if (pData->size != 3)
            {
                // Length.
                ret = bb_setUInt8ByIndex(pData, 2, (unsigned char)(pData->size - 3));
            }
            else
            {
                bb_clear(pData);
            }
        }
        if (ret == 0 && (ret = dlms_getHdlcFrame(settings, DLMS_COMMAND_SNRM, pData, reply)) != 0)
        {
            bb_clear(pData);
            bb_clear(reply);
        #ifndef DLMS_IGNORE_MALLOC
                    gxfree(reply);
        #endif //DLMS_IGNORE_MALLOC
                    return ret;
                }
            }
            bb_clear(pData);
        #ifndef DLMS_IGNORE_MALLOC
            ret = mes_push(messages, reply);
        #endif //DLMS_IGNORE_MALLOC
    return ret;
}

// int cl_receiverReady(dlmsSettings* settings, DLMS_DATA_REQUEST_TYPES type, gxByteBuffer* reply)
// {
//     return dlms_receiverReady(settings, type, reply);
// }


uint16_t cl_getServerAddress(uint16_t logicalAddress, uint16_t physicalAddress, unsigned char addressSize)
{
    uint16_t value;
    if (addressSize < 4 && physicalAddress < 0x80 && logicalAddress < 0x80)
    {
        value = (uint16_t)(logicalAddress << 7 | physicalAddress);
    }
    else if (physicalAddress < 0x4000 && logicalAddress < 0x4000)
    {
        value = (uint16_t)(logicalAddress << 14 | physicalAddress);
    }
    else
    {
        value = 0;
    }
    return value;
}


int dlms_parseSnrmUaResponse(
    dlmsSettings* settings,
    gxByteBuffer* data)
{
    uint32_t value;
    unsigned char ch, id, len;
    uint16_t tmp;
    int ret;
    //If default settings are used.
    // if (data->size - data->position == 0)
    // {
    //     return 0;
    // }
    // Skip FromatID
    // if ((ret = bb_getUInt8(data, &ch)) != 0)
    // {
    //     return ret;
    // }
    // Skip Group ID.
    // if ((ret = bb_getUInt8(data, &ch)) != 0)
    // {
    //     return ret;
    // }
    // // Skip Group len
    // if ((ret = bb_getUInt8(data, &ch)) != 0)
    // {
    //     return ret;
    // }
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
            // ret = bb_getUInt16(data, &tmp);
            // value = tmp;
            // break;
        case 4:
            // ret = bb_getUInt32(data, &value);
            // break;
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


int cl_parseUAResponse(dlmsSettings* settings, gxByteBuffer* data)
{
    int ret = dlms_parseSnrmUaResponse(settings, data);
    if (ret == 0 && bb_size(data) != 0)
    {
        settings->connected = DLMS_CONNECTION_STATE_HDLC;
    }
    return ret;
}

int cl_aarqRequest(
    dlmsSettings* settings,
    message* messages)
{
    if (settings->proposedConformance == 0)
    {
        //Invalid conformance.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    // if (dlms_usePreEstablishedConnection(settings))
    // {
    //     //Invalid conformance.
    //     return DLMS_ERROR_CODE_INVALID_PARAMETER;
    // }

    //Save default values.
    settings->initializePduSize = settings->maxPduSize;
    int ret;
    gxByteBuffer* pdu;
#ifdef DLMS_IGNORE_MALLOC
   //EVS2 NOT SUPPORTED
#else
    gxByteBuffer buff;
#ifdef GX_DLMS_MICROCONTROLLER
    //EVS2 NOT SUPPORTED
#else
    BYTE_BUFFER_INIT(&buff);
    if ((ret = bb_capacity(&buff, 100)) != 0)
    {
        return ret;
    }
    pdu = &buff;
#endif
#endif //DLMS_IGNORE_MALLOC

    settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
    resetBlockIndex(settings);
    mes_clear(messages);
    ret = dlms_checkInit(settings);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    bb_clear(&settings->stoCChallenge);
    if (settings->autoIncreaseInvokeID)
    {
        settings->invokeID = 0;
    }
    else
    {
        settings->invokeID = 1;
    }
    // If authentication or ciphering is used.
    if (settings->authentication > DLMS_AUTHENTICATION_LOW && settings->customChallenges == 0)
    {
        if ((ret = dlms_generateChallenge(&settings->ctoSChallenge)) != 0)
        {
            return ret;
        }
    }
    if ((ret = apdu_generateAarq(settings, pdu)) == 0)
    {
        if (settings->useLogicalNameReferencing)
        {
            gxLNParameters p;
            params_initLN(&p, settings, 0, DLMS_COMMAND_AARQ, 0, pdu, NULL, 0xFF, DLMS_COMMAND_NONE, 0, 0);
            ret = dlms_getLnMessages(&p, messages);
        }
        else
        {
            //EVS2 NOT SUPPORTED
        }
    }
    settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
    bb_clear(pdu);
    return ret;
}

int cl_parseAAREResponse(dlmsSettings* settings, gxByteBuffer* reply)
{
    int ret;
    unsigned char sd;
    DLMS_ASSOCIATION_RESULT result;
    unsigned char command = 0;
    if ((ret = apdu_parsePDU(settings, reply, &result, &sd, &command)) != 0)
    {
        return ret;
    }
    if (result != DLMS_ASSOCIATION_RESULT_ACCEPTED)
    {
        if (result == DLMS_ASSOCIATION_RESULT_TRANSIENT_REJECTED)
        {
            return DLMS_ERROR_CODE_REJECTED_TRANSIENT;
        }
        return DLMS_ERROR_CODE_REJECTED_PERMAMENT;
    }
    settings->isAuthenticationRequired = sd == DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_REQUIRED;
    if (!settings->isAuthenticationRequired)
    {
        settings->connected |= DLMS_CONNECTION_STATE_DLMS;
    }
    if (settings->dlmsVersionNumber != 6)
    {
        //Invalid DLMS version number.
        return DLMS_ERROR_CODE_INVALID_VERSION_NUMBER;
    }
    return 0;
}