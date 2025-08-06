
#include "../include/client.h"

#include "../include/gxdefine.h"
#include "../include/dlms.h"
#include "../include/cosem.h"
#include "../include/gxmem.h"
#include "../include/dlmssettings.h"
#include "../include/message.h"
#include "../include/errorcodes.h"
#include "../include/bytebuffer.h"
#include "../include/parameters.h"
#include "../include/gxvalueeventargs.h"

int cl_snrmRequest(dlmsSettings *settings, message *messages)
{
    int ret;
    gxByteBuffer *reply;
    gxByteBuffer *pData;
    mes_clear(messages);
    // Save default values.
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

    resetFrameSequence(settings);
#ifdef DLMS_IGNORE_MALLOC
    // EVS2 NOT SUPPORTED
#else
    reply = (gxByteBuffer *)gxmalloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(reply);
    gxByteBuffer bb;
    BYTE_BUFFER_INIT(&bb);
    if ((ret = bb_capacity(&bb, 30)) != 0)
    {
        return ret;
    }
    pData = &bb;
#endif // DLMS_IGNORE_MALLOC

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
#endif // DLMS_IGNORE_MALLOC
            return ret;
        }
    }
    bb_clear(pData);
#ifndef DLMS_IGNORE_MALLOC
    ret = mes_push(messages, reply);
#endif // DLMS_IGNORE_MALLOC
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

int cl_parseUAResponse(dlmsSettings *settings, gxByteBuffer *data)
{
    int ret = dlms_parseSnrmUaResponse(settings, data);
    if (ret == 0 && bb_size(data) != 0)
    {
        settings->connected = DLMS_CONNECTION_STATE_HDLC;
    }
    return ret;
}


/**
* Generates a release request.
*
* @return Release request, as byte array.
*/
int cl_releaseRequest2(dlmsSettings* settings, message* packets, unsigned char useProtectedRelease)
{
    int ret;
    gxByteBuffer bb;
    mes_clear(packets);
    // If connection is not established, there is no need to send
    // DisconnectRequest.
    if ((settings->connected & DLMS_CONNECTION_STATE_DLMS) == 0)
    {
        return 0;
    }
    settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
    BYTE_BUFFER_INIT(&bb);
    if (!useProtectedRelease)
    {
        if ((ret = bb_setUInt8(&bb, 0x3)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x80)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x01)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x0)) != 0)
        {
            return ret;
        }
    }
    else
    {
        // Length.
        if ((ret = bb_setUInt8(&bb, 0x0)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x80)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x01)) != 0 ||
            (ret = bb_setUInt8(&bb, 0x0)) != 0)
        {
            return ret;
        }
        apdu_generateUserInformation(settings, &bb);
        bb.data[0] = (unsigned char)(bb.size - 1);
    }
    if (settings->useLogicalNameReferencing)
    {
        gxLNParameters p;
        params_initLN(&p, settings, 0,
            DLMS_COMMAND_RELEASE_REQUEST, DLMS_SET_COMMAND_TYPE_NORMAL,
            &bb, NULL, 0xff, DLMS_COMMAND_NONE, 0, 0);
        ret = dlms_getLnMessages(&p, packets);
    }

    bb_clear(&bb);
    //Restore default values.
    settings->maxPduSize = settings->initializePduSize;
    return ret;
}

int cl_aarqRequest(
    dlmsSettings *settings,
    message *messages)
{
    if (settings->proposedConformance == 0)
    {
        // Invalid conformance.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if (dlms_usePreEstablishedConnection(settings))
    {
        // Invalid conformance.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }

    // Save default values.
    settings->initializePduSize = settings->maxPduSize;
    int ret;
    gxByteBuffer *pdu;
#ifdef DLMS_IGNORE_MALLOC
    // EVS2 NOT SUPPORTED
#else
    gxByteBuffer buff;
#ifdef GX_DLMS_MICROCONTROLLER
    // EVS2 NOT SUPPORTED
#else
    BYTE_BUFFER_INIT(&buff);
    if ((ret = bb_capacity(&buff, 100)) != 0)
    {
        return ret;
    }
    pdu = &buff;
#endif
#endif // DLMS_IGNORE_MALLOC

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
            // EVS2 NOT SUPPORTED
        }
    }
    settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
    bb_clear(pdu);
    return ret;
}

int cl_getData2(
    dlmsSettings *settings,
    gxByteBuffer *reply,
    gxReplyData *data,
    gxReplyData *notify,
    unsigned char *isNotify)
{
    return dlms_getData3(settings, reply, data, notify, 0, isNotify);
}

int cl_parseAAREResponse(dlmsSettings *settings, gxByteBuffer *reply)
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
        // Invalid DLMS version number.
        return DLMS_ERROR_CODE_INVALID_VERSION_NUMBER;
    }
    return 0;
}

int cl_methodLN(
    dlmsSettings *settings,
    const unsigned char *name,
    DLMS_OBJECT_TYPE objectType,
    unsigned char index,
    dlmsVARIANT *value,
    message *messages)
{
    int ret = 0;
    gxLNParameters p;
    gxByteBuffer *pdu;
    gxByteBuffer data;
    if (index < 1)
    {
        // Invalid parameter
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
#ifdef DLMS_IGNORE_MALLOC
    if (settings->serializedPdu == NULL)
    {
        // Invalid parameter
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    pdu = settings->serializedPdu;
    // Use same buffer for header and data. Header size is 10 bytes.
    BYTE_BUFFER_INIT(&data);
    bb_clear(pdu);
#else
    gxByteBuffer bb;
    unsigned char GX_METHOD_PDU[10];
    bb_attach(&bb, GX_METHOD_PDU, 0, sizeof(GX_METHOD_PDU));
    pdu = &bb;
    BYTE_BUFFER_INIT(&data);
#endif // DLMS_IGNORE_MALLOC
    resetBlockIndex(settings);
    // CI
    if ((ret = bb_setUInt16(pdu, objectType)) == 0 &&
        // Add LN
        (ret = bb_set(pdu, name, 6)) == 0 &&
        // Attribute ID.
        (ret = bb_setUInt8(pdu, index)) == 0 &&
        // Is Method Invocation Parameters used.
        (ret = bb_setUInt8(pdu, value == NULL || value->vt == DLMS_DATA_TYPE_NONE ? 0 : 1)) == 0)
    {
#ifdef DLMS_IGNORE_MALLOC
        if (value != NULL && value->vt != DLMS_DATA_TYPE_NONE)
        {
            if (value->vt == DLMS_DATA_TYPE_OCTET_STRING)
            {
                ret = bb_set(pdu, value->byteArr->data, value->byteArr->size);
            }
            else
            {
                // ret = dlms_setData(pdu, value->vt, value);
            }
        }
#else
        if (value != NULL && value->vt != DLMS_DATA_TYPE_NONE)
        {
            if ((value->vt == DLMS_DATA_TYPE_ARRAY || value->vt == DLMS_DATA_TYPE_STRUCTURE) &&
                value->vt == DLMS_DATA_TYPE_OCTET_STRING)
            {
                ret = bb_set(&data, value->byteArr->data, value->byteArr->size);
            }
            else
            {
                if (value->vt == DLMS_DATA_TYPE_OCTET_STRING)
                {
                    // Space is allocated for type and size
                    ret = bb_capacity(&data, 5 + bb_size(value->byteArr));
                }
                if (ret == 0)
                {
                    ret = dlms_setData(&data, value->vt, value);
                }
            }
        }
#endif // DLMS_IGNORE_MALLOC
    }
    if (ret == 0)
    {
        params_initLN(&p, settings, 0,
                      DLMS_COMMAND_METHOD_REQUEST, DLMS_ACTION_COMMAND_TYPE_NORMAL,
                      pdu, &data, 0xff, DLMS_COMMAND_NONE, 0, 0);
        ret = dlms_getLnMessages(&p, messages);
    }
    bb_clear(&data);
    bb_clear(pdu);
    return ret;
}

int cl_getApplicationAssociationRequest(
    dlmsSettings *settings,
    message *messages)
{
    int ret;
    gxByteBuffer challenge;
    gxByteBuffer *pw;
    dlmsVARIANT data;
#if !defined(DLMS_IGNORE_HIGH_GMAC) || !defined(DLMS_IGNORE_HIGH_SHA256)
    gxByteBuffer pw2;
    BYTE_BUFFER_INIT(&pw2);
#endif // DLMS_IGNORE_HIGH_GMAC
#ifndef GX_DLMS_MICROCONTROLLER
    unsigned char APPLICATION_ASSOCIATION_REQUEST[64];
#else
    static unsigned char APPLICATION_ASSOCIATION_REQUEST[64];
#endif // DLMS_IGNORE_HIGH_GMAC
    bb_attach(&challenge, APPLICATION_ASSOCIATION_REQUEST, 0, sizeof(APPLICATION_ASSOCIATION_REQUEST));
    if (
#ifndef DLMS_IGNORE_HIGH_GMAC
        settings->authentication != DLMS_AUTHENTICATION_HIGH_GMAC &&
#endif // DLMS_IGNORE_HIGH_GMAC
        settings->password.size == 0)
    {
        // Password is invalid.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    resetBlockIndex(settings);
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
    {
#ifndef DLMS_IGNORE_MALLOC
        pw = &settings->cipher.systemTitle;
#else
        bb_attach(&pw2, settings->cipher.systemTitle, 8, 8);
        pw = &pw2;
#endif // DLMS_IGNORE_MALLOC
    }
#endif // DLMS_IGNORE_HIGH_GMAC
    else
    {
        pw = &settings->password;
    }
    ret = dlms_secure(settings,
    #ifndef DLMS_IGNORE_HIGH_GMAC
    settings->cipher.invocationCounter,
    #else
        0,
    #endif //DLMS_IGNORE_HIGH_GMAC
        & settings->stoCChallenge,
        pw,
        &challenge);
#if !defined(DLMS_IGNORE_HIGH_GMAC) || !defined(DLMS_IGNORE_HIGH_SHA256)
    bb_clear(&pw2);
#endif //! defined(DLMS_IGNORE_HIGH_GMAC) || !defined(DLMS_IGNORE_HIGH_SHA256)
    if (ret == 0)
    {
        var_init(&data);
        data.vt = DLMS_DATA_TYPE_OCTET_STRING;
        data.byteArr = &challenge;
        {
            if (settings->useLogicalNameReferencing)
            {
                static const unsigned char LN[6] = {0, 0, 40, 0, 0, 255};
                ret = cl_methodLN(settings, LN, DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME,
                                  1, &data, messages);
            }
        }
#ifndef DLMS_IGNORE_MALLOC
        var_clear(&data);
        bb_clear(&challenge);
#endif // DLMS_IGNORE_MALLOC
    }
    return ret;
}

int cl_parseApplicationAssociationResponse(
    dlmsSettings *settings,
    gxByteBuffer *reply)
{
    unsigned char empty, equals = 0;
    gxByteBuffer *secret;
    gxByteBuffer challenge;
#ifndef DLMS_IGNORE_HIGH_GMAC
    gxByteBuffer bb2;
#endif // DLMS_IGNORE_HIGH_GMAC
    int ret;
    uint32_t ic = 0;
    gxByteBuffer value;
    static unsigned char tmp[MAX_CHALLENGE_SIZE];
    static unsigned char CHALLENGE_BUFF[MAX_CHALLENGE_SIZE];
    bb_attach(&value, tmp, 0, sizeof(tmp));
    bb_attach(&challenge, CHALLENGE_BUFF, 0, sizeof(CHALLENGE_BUFF));
    if ((ret = cosem_getOctetString(reply, &value)) != 0)
    {
        settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
        //ParseApplicationAssociationResponse failed. Server to Client do not match.
        return DLMS_ERROR_CODE_AUTHENTICATION_FAILURE;
    }
    empty = value.size == 0;
    if (!empty)
    {
        if (settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
        {
            unsigned char ch;
            bb_attach(&bb2, settings->sourceSystemTitle, sizeof(settings->sourceSystemTitle), sizeof(settings->sourceSystemTitle));
            secret = &bb2;
            if ((ret = bb_set(&challenge, value.data, value.size)) != 0 ||
                (ret = bb_getUInt8(&challenge, &ch)) != 0 ||
                (ret = bb_getUInt32(&challenge, &ic)) != 0)
            {
                return ret;
            }
            bb_clear(&challenge);
        }
        // if ((ret = dlms_secure(
        //     settings,
        //     ic,
        //     &settings->ctoSChallenge,
        //     secret,
        //     &challenge)) != 0)
        // {
        //     return ret;
        // }
        equals = bb_compare(
            &challenge,
            value.data,
            value.size);
    }
    else
    {
        // Server did not accept CtoS.
    }
    if (!equals)
    {
        settings->connected &= ~DLMS_CONNECTION_STATE_DLMS;
        // ParseApplicationAssociationResponse failed. Server to Client do not match.
        return DLMS_ERROR_CODE_AUTHENTICATION_FAILURE;
    }
    settings->connected |= DLMS_CONNECTION_STATE_DLMS;
    return 0;
}

int cl_getObjectsRequest(dlmsSettings *settings, message *messages)
{
    int ret;
    if (settings->useLogicalNameReferencing)
    {
        static const unsigned char ln[] = {0, 0, 40, 0, 0, 0xFF};
        ret = cl_readLN(settings, ln, DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME, 2, NULL, messages);
    }
    return ret;
}

int cl_readLN(
    dlmsSettings *settings,
    const unsigned char *name,
    DLMS_OBJECT_TYPE objectType,
    unsigned char attributeOrdinal,
    gxByteBuffer *data,
    message *messages)
{
    int ret;
    gxLNParameters p;
    gxByteBuffer *pdu;
    if ((attributeOrdinal < 1))
    {
        // Invalid parameter
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
#ifdef DLMS_IGNORE_MALLOC
    if (settings->serializedPdu == NULL)
    {
        // Invalid parameter
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    pdu = settings->serializedPdu;
    bb_clear(pdu);
#else
    gxByteBuffer attributeDescriptor;
    BYTE_BUFFER_INIT(&attributeDescriptor);
    pdu = &attributeDescriptor;
#endif // DLMS_IGNORE_MALLOC
    resetBlockIndex(settings);
    // CI
    if ((ret = bb_setUInt16(pdu, objectType)) == 0 &&
        // Add LN
        (ret = bb_set(pdu, name, 6)) == 0 &&
        // Attribute ID.
        (ret = bb_setUInt8(pdu, attributeOrdinal)) == 0)
    {
        if (data == NULL || data->size == 0)
        {
            // Access selection is not used.
            ret = bb_setUInt8(pdu, 0);
        }
        else
        {
            // Access selection is used.
            if ((ret = bb_setUInt8(pdu, 1)) == 0)
            {
                // Add data.
                ret = bb_set2(pdu, data, 0, data->size);
            }
        }
    }
    if (ret == 0)
    {
        params_initLN(&p, settings, 0,
                      DLMS_COMMAND_GET_REQUEST, DLMS_GET_COMMAND_TYPE_NORMAL,
                      pdu, data, 0xFF, DLMS_COMMAND_NONE, 0, 0);
        ret = dlms_getLnMessages(&p, messages);
    }
    bb_clear(pdu);
    return ret;
}

int cl_updateValue(
    dlmsSettings* settings,
    gxObject* target,
    unsigned char attributeIndex,
    dlmsVARIANT* value)
{
    gxValueEventArg e;
    if (target == NULL)
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    e.target = target;
    e.index = attributeIndex;
    e.value = *value;
    return cosem_setValue(settings, &e);
}

int cl_read(
    dlmsSettings *settings,
    gxObject *object,
    unsigned char attributeOrdinal,
    message *messages)
{
    int ret;
    if (object == NULL)
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    else if (settings->useLogicalNameReferencing)
    {
        ret = cl_readLN(settings, object->logicalName, object->objectType, attributeOrdinal, NULL, messages);
    }
    return ret;
}

int cl_disconnectRequest(dlmsSettings *settings, message *packets)
{
    int ret = 0;
#ifndef DLMS_IGNORE_WRAPPER
    gxByteBuffer bb;
#endif // DLMS_IGNORE_WRAPPER
    gxByteBuffer *reply = NULL;
    mes_clear(packets);
    settings->maxPduSize = 0xFFFF;
    // If connection is not established, there is no need to send DisconnectRequest.
    if ((settings->connected & DLMS_CONNECTION_STATE_HDLC) == 0)
    {
        return ret;
    }
    settings->connected &= ~DLMS_CONNECTION_STATE_HDLC;
#ifdef DLMS_IGNORE_MALLOC
    reply = packets->data[0];
    ++packets->size;
    bb_clear(reply);
#else
    reply = (gxByteBuffer *)gxmalloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(reply);
#endif // DLMS_IGNORE_MALLOC
    switch (settings->interfaceType)
    {
#ifndef DLMS_IGNORE_HDLC
    case DLMS_INTERFACE_TYPE_HDLC:
    case DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E:
    {
        ret = dlms_getHdlcFrame(settings, DLMS_COMMAND_DISC, NULL, reply);
#ifndef DLMS_IGNORE_MALLOC
        if (ret == 0)
        {
            ret = mes_push(packets, reply);
        }
        else
        {
            gxfree(reply);
        }
#endif // DLMS_IGNORE_MALLOC
    }
    break;
#endif // DLMS_IGNORE_HDLC
    default:
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        break;
    }
#ifndef DLMS_IGNORE_HDLC
    if (dlms_useHdlc(settings->interfaceType))
    {
        // Restore default HDLC values.
        settings->maxInfoTX = settings->initializeMaxInfoTX;
        settings->maxInfoRX = settings->initializeMaxInfoRX;
        settings->windowSizeTX = settings->initializeWindowSizeTX;
        settings->windowSizeRX = settings->initializeWindowSizeRX;
    }
#endif // DLMS_IGNORE_HDLC
    // Restore default values.
    settings->maxPduSize = settings->initializePduSize;
    resetFrameSequence(settings);
    return ret;
}