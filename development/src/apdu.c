#include "../include/gxmem.h"
#include "../include/enums.h"
#include "../include/helpers.h"
#include "../include/apdu.h"
#include "../include/errorcodes.h"
#include "../include/ciphering.h"

unsigned char useDedicatedKey(dlmsSettings *settings)
{
#ifndef DLMS_IGNORE_MALLOC
    if (settings->cipher.dedicatedKey == NULL)
    {
        return 0;
    }
    return settings->cipher.dedicatedKey->size == 16;
#else
    return memcmp(settings->cipher.dedicatedKey, EMPTY_KEY, sizeof(EMPTY_KEY)) != 0;
#endif // DLMS_IGNORE_MALLOC
}
int apdu_getAuthenticationString(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    int ret = 0;
    gxByteBuffer *callingAuthenticationValue = NULL;
    if (settings->authentication != DLMS_AUTHENTICATION_NONE
#ifndef DLMS_IGNORE_HIGH_GMAC
        || settings->cipher.security != DLMS_SECURITY_NONE
#endif // DLMS_IGNORE_HIGH_GMAC
    )
    {
        unsigned char p[] = {0x60, 0x85, 0x74, 0x05, 0x08, 0x02};
        // Add sender ACSE-requirements field component.
        if ((ret = bb_setUInt8(data, (uint16_t)BER_TYPE_CONTEXT | (char)PDU_TYPE_SENDER_ACSE_REQUIREMENTS)) != 0 ||
            (ret = bb_setUInt8(data, 2)) != 0 ||
            (ret = bb_setUInt8(data, BER_TYPE_BIT_STRING | BER_TYPE_OCTET_STRING)) != 0 ||
            (ret = bb_setUInt8(data, 0x80)) != 0 ||
            (ret = bb_setUInt8(data, (uint16_t)BER_TYPE_CONTEXT | (char)PDU_TYPE_MECHANISM_NAME)) != 0 ||
            // Len
            (ret = bb_setUInt8(data, 7)) != 0 ||
            // OBJECT IDENTIFIER
            (ret = bb_set(data, p, 6)) != 0 ||
            (ret = bb_setUInt8(data, settings->authentication)) != 0)
        {
            // Error code is returned at the end of the function.
        }
    }
    // If authentication is used.
    if (settings->authentication != DLMS_AUTHENTICATION_NONE)
    {
        // Add Calling authentication information.
        if (settings->authentication == DLMS_AUTHENTICATION_LOW)
        {
            if (settings->password.size != 0)
            {
                callingAuthenticationValue = &settings->password;
            }
        }
        else
        {
            callingAuthenticationValue = &settings->ctoSChallenge;
        }
        // 0xAC
        if ((ret = bb_setUInt8(data, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLING_AUTHENTICATION_VALUE)) == 0 &&
            // Len
            (ret = bb_setUInt8(data, (unsigned char)(2 + bb_size(callingAuthenticationValue)))) == 0 &&
            // Add authentication information.
            (ret = bb_setUInt8(data, BER_TYPE_CONTEXT)) == 0 &&
            // Len.
            (ret = bb_setUInt8(data, (unsigned char)bb_size(callingAuthenticationValue))) == 0)
        {
            if (callingAuthenticationValue != NULL)
            {
                ret = bb_set(data, callingAuthenticationValue->data, bb_size(callingAuthenticationValue));
            }
        }
    }
    return ret;
}

/**
 * Parse User Information from PDU.
 */
int apdu_parseUserInformation(
    dlmsSettings *settings,
    gxByteBuffer *data,
    unsigned char ciphered,
    unsigned char *command)
{
    int ret;
    unsigned char response;
    uint16_t pduSize;
    unsigned char ch, len, tag;
    uint32_t v;
    if ((ret = bb_getUInt8(data, &len)) != 0)
    {
        return ret;
    }
    if (data->size - data->position < len)
    {
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    // Encoding the choice for user information
    if ((ret = bb_getUInt8(data, &tag)) != 0)
    {
        return ret;
    }
    if (tag != 0x4)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(data, &len)) != 0)
    {
        return ret;
    }
    // Tag for xDLMS-Initate.response
    if ((ret = bb_getUInt8(data, &tag)) != 0)
    {
        return ret;
    }
#ifndef DLMS_IGNORE_HIGH_GMAC
    DLMS_SECURITY security;
    DLMS_SECURITY_SUITE suite;
    uint64_t invocationCounter;

    if (tag == DLMS_COMMAND_GLO_INITIATE_RESPONSE ||
        tag == DLMS_COMMAND_GLO_INITIATE_REQUEST ||
        tag == DLMS_COMMAND_GENERAL_GLO_CIPHERING)
    {
        *command = (unsigned char)tag;
        --data->position;

        if ((ret = cip_decrypt(&settings->cipher,
                               settings->sourceSystemTitle,
                               &settings->cipher.blockCipherKey,
                               data,
                               &security,
                               &suite,
                               &invocationCounter)) != 0)
        {
            return ret;
        }

        if (settings->expectedSecurityPolicy != 0xFF && security != settings->expectedSecurityPolicy << 4)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }

#endif // DLMS_INVOCATION_COUNTER_VALIDATOR
       // If client system title doesn't match.
        if (settings->expectedClientSystemTitle != NULL &&
            memcmp(settings->expectedClientSystemTitle, EMPTY_SYSTEM_TITLE, 8) != 0 &&
            memcmp(settings->sourceSystemTitle, settings->expectedClientSystemTitle, 8) != 0)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }

        settings->cipher.security = security;
        settings->cipher.suite = suite;
        if ((ret = bb_getUInt8(data, &tag)) != 0)
        {
            return ret;
        }
    }
    response = tag == DLMS_COMMAND_INITIATE_RESPONSE;
    if (response)
    {
        // Optional usage field of the negotiated quality of service
        // component
        if ((ret = bb_getUInt8(data, &tag)) != 0)
        {
            return ret;
        }
        if (tag != 0)
        {
            if ((ret = bb_getUInt8(data, &settings->qualityOfService)) != 0)
            {
                return ret;
            }
        }
    }
    else if (tag == DLMS_COMMAND_INITIATE_REQUEST)
    {
        // Optional usage field of the negotiated quality of service
        // component
        if ((ret = bb_getUInt8(data, &tag)) != 0)
        {
            return ret;
        }
        // Dedicated key.
#ifdef DLMS_IGNORE_HIGH_GMAC
#else
    if (tag != 0)
    {
        // Return error if ciphering is not used.
        if (!ciphered || settings->cipher.security != DLMS_SECURITY_AUTHENTICATION_ENCRYPTION)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        if ((ret = bb_getUInt8(data, &len)) != 0)
        {
            return ret;
        }
        if (len != 16)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
#ifndef DLMS_IGNORE_MALLOC
        if (settings->cipher.dedicatedKey == NULL)
        {
            settings->cipher.dedicatedKey = gxmalloc(sizeof(gxByteBuffer));
            BYTE_BUFFER_INIT(settings->cipher.dedicatedKey);
        }
        else
        {
            bb_clear(settings->cipher.dedicatedKey);
        }
        bb_set2(settings->cipher.dedicatedKey, data, data->position, len);
#else
        hlp_setObjectCount(8, data);
        bb_get(data, settings->cipher.dedicatedKey, len);
#endif // DLMS_IGNORE_MALLOC
    }
    else
    {
#ifndef DLMS_IGNORE_MALLOC
        if (settings->cipher.dedicatedKey != NULL)
        {
            bb_clear(settings->cipher.dedicatedKey);
            gxfree(settings->cipher.dedicatedKey);
            settings->cipher.dedicatedKey = NULL;
        }
#else
        memset(settings->cipher.dedicatedKey, 0, 8);
#endif // DLMS_IGNORE_MALLOC
    }
#endif // DLMS_IGNORE_HIGH_GMAC
       //  Optional usage field of the negotiated quality of service
       //  component
        if ((ret = bb_getUInt8(data, &tag)) != 0)
        {
            return ret;
        }
        // Skip if used.
        if (tag != 0)
        {
            if ((ret = bb_getUInt8(data, &tag)) != 0)
            {
                return ret;
            }
        }
        // Optional usage field of the proposed quality of service component
        if ((ret = bb_getUInt8(data, &tag)) != 0)
        {
            return ret;
        }
        if (tag != 0)
        {
            if ((ret = bb_getUInt8(data, &settings->qualityOfService)) != 0)
            {
                return ret;
            }
        }
    }
    else
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }

    if ((ret = bb_getUInt8(data, &ch)) != 0)
    {
        return ret;
    }
    if (ch < 6)
    {
        // Invalid DLMS version number.
        return DLMS_ERROR_CODE_INVALID_VERSION_NUMBER;
    }

    // Tag for conformance block
    if ((ret = bb_getUInt8(data, &tag)) != 0)
    {
        return ret;
    }
    if (tag != 0x5F)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    // Old Way...
    tag = data->data[data->position];
    if (tag == 0x1F)
    {
        data->position = (data->position + 1);
    }
    if ((ret = bb_getUInt8(data, &len)) != 0)
    {
        return ret;
    }
    // The number of unused bits in the bit string.
    if ((ret = bb_getUInt8(data, &tag)) != 0)
    {
        return ret;
    }
    bitArray ba;
    ba_attach(&ba, data->data + data->position, 24, 24);
    data->position += 3;
    if ((ret = ba_toInteger(&ba, &v)) != 0)
    {
        return ret;
    }

    settings->negotiatedConformance = (DLMS_CONFORMANCE)v;

    if ((ret = bb_getUInt16(data, &pduSize)) != 0)
    {
        return ret;
    }
    settings->maxPduSize = pduSize;

    if (response)
    {
        // VAA Name
        uint16_t vaa;
        if ((ret = bb_getUInt16(data, &vaa)) != 0)
        {
            return ret;
        }
        if (vaa == 0x0007)
        {
            // If LN
            if (!settings->useLogicalNameReferencing)
            {
                // Invalid VAA.
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        }
        else if (vaa == 0xFA00)
        {
            // If SN
            if (settings->useLogicalNameReferencing)
            {
                // Invalid VAA.
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        }
        else
        {
            // Unknown VAA.
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    return 0;
}

int apdu_handleResultComponent(unsigned char value)
{
    int ret;
    switch (value)
    {
    case DLMS_SOURCE_DIAGNOSTIC_NO_REASON_GIVEN:
        ret = DLMS_ERROR_CODE_NO_REASON_GIVEN;
        break;
    case DLMS_SOURCE_DIAGNOSTIC_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED:
        ret = DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED;
        break;
    case DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_FAILURE:
        ret = DLMS_ERROR_CODE_AUTHENTICATION_FAILURE;
        break;
    default:
        // OK.
        ret = 0;
        break;
    }
    return ret;
}

int apdu_generateAarq(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
#if defined(GX_DLMS_BYTE_BUFFER_SIZE_32) || (!defined(GX_DLMS_MICROCONTROLLER) && (defined(_WIN32) || defined(_WIN64) || defined(__linux__)))
    uint32_t offset;
#else
    uint16_t offset;
#endif
    int ret;
    // Length is updated later.
    offset = data->size + 1;
    // AARQ APDU Tag
    if ((ret = bb_setUInt8(data, BER_TYPE_APPLICATION | BER_TYPE_CONSTRUCTED)) == 0 &&
        (ret = bb_setUInt8(data, 0)) == 0 &&
        ///////////////////////////////////////////
        // Add Application context name.
        (ret = apdu_generateApplicationContextName(settings, data)) == 0 &&
        (ret = apdu_getAuthenticationString(settings, data)) == 0 &&
        (ret = apdu_generateUserInformation(settings, data)) == 0)
    {
        return bb_setUInt8ByIndex(data, offset, (unsigned char)(data->size - offset - 1));
    }
    return ret;
}

int apdu_updateAuthentication(
    dlmsSettings *settings,
    gxByteBuffer *buff)
{
    int ret;
    unsigned char ch;
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }

    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x60)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x85)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x74)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x05)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x08)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x02)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    switch (ch)
    {
    case DLMS_AUTHENTICATION_NONE:
    case DLMS_AUTHENTICATION_LOW:
#ifndef DLMS_IGNORE_HIGH_GMAC
    case DLMS_AUTHENTICATION_HIGH_GMAC:
#endif // DLMS_IGNORE_HIGH_GMAC
        break;
    default:
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    settings->authentication = (DLMS_AUTHENTICATION)ch;
    return 0;
}

int apdu_generateApplicationContextName(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    int ret;
    // ProtocolVersion
    if (settings->protocolVersion != 0)
    {
        if ((ret = bb_setUInt8(data, BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_PROTOCOL_VERSION)) != 0 ||
            (ret = bb_setUInt8(data, 2)) != 0 ||
            // Un-used bits.
            (ret = bb_setUInt8(data, 2)) != 0 ||
            (ret = bb_setUInt8(data, settings->protocolVersion)) != 0)
        {
            return ret;
        }
    }
    unsigned char ciphered;
#ifndef DLMS_IGNORE_HIGH_GMAC
    ciphered = isCiphered(&settings->cipher);
#else
    ciphered = 0;
#endif // DLMS_IGNORE_HIGH_GMAC

    // Application context name tag
    if ((ret = bb_setUInt8(data, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_APPLICATION_CONTEXT_NAME))) != 0 ||
        // Len
        (ret = bb_setUInt8(data, 0x09)) != 0 ||
        (ret = bb_setUInt8(data, BER_TYPE_OBJECT_IDENTIFIER)) != 0 ||
        // Len
        (ret = bb_setUInt8(data, 0x07)) != 0 ||
        (ret = bb_setUInt8(data, 0x60)) != 0 ||
        (ret = bb_setUInt8(data, 0x85)) != 0 ||
        (ret = bb_setUInt8(data, 0x74)) != 0 ||
        (ret = bb_setUInt8(data, 0x05)) != 0 ||
        (ret = bb_setUInt8(data, 0x08)) != 0 ||
        (ret = bb_setUInt8(data, 0x01)) != 0)
    {
        return ret;
    }

    if (settings->useLogicalNameReferencing)
    {
        if ((ret = bb_setUInt8(data, ciphered ? 0x03 : 0x01)) != 0)
        {
            return ret;
        }
    }
    else
    {
        if ((ret = bb_setUInt8(data, ciphered ? 0x04 : 0x02)) != 0)
        {
            return ret;
        }
    }
    // Add system title.
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (!settings->server && (ciphered ||
                              settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC))
    {
#ifndef DLMS_IGNORE_MALLOC
        if (settings->cipher.systemTitle.size == 0)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
#endif // DLMS_IGNORE_MALLOC
       //  Add calling-AP-title
        if ((ret = bb_setUInt8(data, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | 6))) != 0 ||
            // LEN
            (ret = bb_setUInt8(data, (unsigned char)(2 + 8))) != 0 ||
            (ret = bb_setUInt8(data, BER_TYPE_OCTET_STRING)) != 0 ||
            // LEN
            (ret = bb_setUInt8(data, (unsigned char)8)) != 0 ||
#ifndef DLMS_IGNORE_MALLOC
            (ret = bb_set(data, settings->cipher.systemTitle.data, 8)) != 0)
#else
            (ret = bb_set(data, settings->cipher.systemTitle, 8)) != 0)
#endif // DLMS_IGNORE_MALLOC
        {
            return ret;
        }
    }
#endif
    // Add CallingAEInvocationId.
    if (!settings->server && settings->userId != -1 && settings->cipher.security != DLMS_SECURITY_NONE)
    {
        if ((ret = bb_setUInt8(data, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLING_AE_INVOCATION_ID))) != 0 ||
            // LEN
            (ret = bb_setUInt8(data, 3)) != 0 ||
            (ret = bb_setUInt8(data, BER_TYPE_INTEGER)) != 0 ||
            // LEN
            (ret = bb_setUInt8(data, 1)) != 0 ||
            (ret = bb_setUInt8(data, (unsigned char)settings->userId)) != 0)
        {
            return ret;
        }
    }
    return 0;
}

int apdu_generateUserInformation(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    int ret = 0;
    bb_setUInt8(data, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_USER_INFORMATION);
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (!isCiphered(&settings->cipher))
#endif // DLMS_IGNORE_HIGH_GMAC
    {
        // Length for AARQ user field
        bb_setUInt8(data, 0x10);
        // Coding the choice for user-information (Octet STRING, universal)
        bb_setUInt8(data, BER_TYPE_OCTET_STRING);
        // Length
        bb_setUInt8(data, 0x0E);
        if ((ret = apdu_getInitiateRequest(settings, data)) != 0)
        {
            return ret;
        }
    }
#ifndef DLMS_IGNORE_HIGH_GMAC
    else
    {
        gxByteBuffer crypted;
#ifndef DLMS_IGNORE_MALLOC
        BYTE_BUFFER_INIT(&crypted);
#else
        unsigned char tmp[25 + 12];
        bb_attach(&crypted, tmp, 0, sizeof(tmp));
#endif // DLMS_IGNORE_MALLOC

        if ((ret = apdu_getInitiateRequest(settings, &crypted)) != 0)
        {
            return ret;
        }

        ret = cip_encrypt(
            &settings->cipher,
            settings->cipher.security,
            DLMS_COUNT_TYPE_PACKET,
            settings->cipher.invocationCounter,
            DLMS_COMMAND_GLO_INITIATE_REQUEST,
            settings->cipher.systemTitle.data,
            &settings->cipher.blockCipherKey,
            &crypted);
        if (ret == 0)
        {
            // Length for AARQ user field
            if ((ret = bb_setUInt8(data, (unsigned char)(2 + crypted.size))) != 0 ||
                // Coding the choice for user-information (Octet string, universal)
                (ret = bb_setUInt8(data, BER_TYPE_OCTET_STRING)) != 0 ||
                (ret = bb_setUInt8(data, (unsigned char)crypted.size)) != 0 ||
                (ret = bb_set2(data, &crypted, 0, crypted.size)) != 0)
            {
                // Error code is returned at the end of the function.
            }
        }
#ifndef DLMS_IGNORE_MALLOC
        bb_clear(&crypted);
#endif // DLMS_IGNORE_MALLOC
    }
#endif // DLMS_IGNORE_HIGH_GMAC
    return ret;
}

int apdu_getInitiateRequest(
    dlmsSettings *settings,
    gxByteBuffer *data)
{
    int ret;
    // Tag for xDLMS-Initiate request
    bb_setUInt8(data, DLMS_COMMAND_INITIATE_REQUEST);
    // Usage field for the response allowed component.

#ifdef DLMS_IGNORE_HIGH_GMAC
    bb_setUInt8(data, 0);
#else
    // Usage field for dedicated-key component.
    if (settings->cipher.security == DLMS_SECURITY_NONE || !useDedicatedKey(settings))
    {
        bb_setUInt8(data, 0);
    }
    else
    {
        bb_setUInt8(data, 1);
#ifndef DLMS_IGNORE_MALLOC
        hlp_setObjectCount(settings->cipher.dedicatedKey->size, data);
        bb_set(data, settings->cipher.dedicatedKey->data, settings->cipher.dedicatedKey->size);
#else
        hlp_setObjectCount(settings->cipher.suite == DLMS_SECURITY_SUITE_V2 ? 32 : 16, data);
        bb_set(data, settings->cipher.dedicatedKey, settings->cipher.suite == DLMS_SECURITY_SUITE_V2 ? 32 : 16);
#endif // DLMS_IGNORE_MALLOC
    }
#endif // DLMS_IGNORE_HIGH_GMAC

    // encoding of the response-allowed component (bool DEFAULT TRUE)
    // usage flag (FALSE, default value TRUE conveyed)
    bb_setUInt8(data, 0);

    // Usage field of the proposed-quality-of-service component.
    if (settings->qualityOfService == 0)
    {
        // Not used
        bb_setUInt8(data, 0x00);
    }
    else
    {
        bb_setUInt8(data, 0x01);
        bb_setUInt8(data, settings->qualityOfService);
    }
    if ((ret = bb_setUInt8(data, settings->dlmsVersionNumber)) != 0 ||
        // Tag for conformance block
        (ret = bb_setUInt8(data, 0x5F)) != 0 ||
        (ret = bb_setUInt8(data, 0x1F)) != 0 ||
        (ret = bb_setUInt8(data, DLMS_DATA_TYPE_BIT_STRING)) != 0 ||
        // encoding the number of unused bits in the bit string
        (ret = bb_setUInt8(data, 0x00)) != 0 ||
        (ret = bb_setUInt8(data, hlp_swapBits((unsigned char)settings->proposedConformance))) != 0 ||
        (ret = bb_setUInt8(data, hlp_swapBits((unsigned char)(settings->proposedConformance >> 8)))) != 0 ||
        (ret = bb_setUInt8(data, hlp_swapBits((unsigned char)(settings->proposedConformance >> 16)))) != 0 ||
        (ret = bb_setUInt16(data, settings->maxPduSize)) != 0)
    {
        return ret;
    }
    return 0;
}

int apdu_validateAare(
    dlmsSettings *settings,
    gxByteBuffer *buff)
{
    int ret;
    unsigned char tag;
    if ((ret = bb_getUInt8(buff, &tag)) != 0)
    {
        return ret;
    }
    if (settings->server)
    {
        if (tag != (BER_TYPE_APPLICATION | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_PROTOCOL_VERSION))
        {
            ret = DLMS_ERROR_CODE_INVALID_TAG;
        }
    }
    else
    {
        if (tag != (BER_TYPE_APPLICATION | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_APPLICATION_CONTEXT_NAME))
        {
            ret = DLMS_ERROR_CODE_INVALID_TAG;
        }
    }
    return ret;
}

int apdu_parseApplicationContextName(
    dlmsSettings *settings,
    gxByteBuffer *buff,
    unsigned char *ciphered)
{
    int ret;
    unsigned char len, ch;
    // Get length.
    if ((ret = bb_getUInt8(buff, &len)) != 0)
    {
        return ret;
    }
    if (buff->size - buff->position < len)
    {
        // Encoding failed. Not enough data->
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x6)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (settings->server)
    {
        settings->cipher.security = DLMS_SECURITY_NONE;
    }
#endif // DLMS_IGNORE_HIGH_GMAC
    // Object ID length.
    if ((ret = bb_getUInt8(buff, &len)) != 0)
    {
        return ret;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x60)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x85)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x74)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }

    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x05)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x08)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (ch != 0x01)
    {
        // Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    if ((ret = bb_getUInt8(buff, &ch)) != 0)
    {
        return ret;
    }
    if (settings->useLogicalNameReferencing)
    {
        *ciphered = ch == 3;
        if (ch == 1 || *ciphered)
        {
            return 0;
        }
        return DLMS_ERROR_CODE_FALSE;
    }
    *ciphered = ch == 4;
    if (ch == 2 || *ciphered)
    {
        return 0;
    }
    return DLMS_ERROR_CODE_FALSE;
}

int apdu_parsePDU(
    dlmsSettings *settings,
    gxByteBuffer *buff,
    DLMS_ASSOCIATION_RESULT *result,
    unsigned char *diagnostic,
    unsigned char *command)
{
    unsigned char ciphered = 0;
    uint16_t size;
    unsigned char len;
    unsigned char tag;
    int ret;
    settings->userId = -1;
    *result = DLMS_ASSOCIATION_RESULT_ACCEPTED;
    *diagnostic = DLMS_SOURCE_DIAGNOSTIC_NONE;
#ifndef DLMS_IGNORE_SERVER
    typedef enum
    {
        DLMS_AFU_MISSING_NONE = 0x0,
        DLMS_AFU_MISSING_SENDER_ACSE_REQUIREMENTS = 0x1,
        DLMS_AFU_MISSING_MECHANISM_NAME = 0x2,
        DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE = 0x4
    } DLMS_AFU_MISSING;
    DLMS_AFU_MISSING afu = DLMS_AFU_MISSING_NONE;
#endif // DLMS_IGNORE_SERVER
    // Get AARE tag and length
    if ((ret = apdu_validateAare(settings, buff)) != 0)
    {
        return ret;
    }
    if ((ret = hlp_getObjectCount2(buff, &size)) != 0)
    {
        return ret;
    }
    if (size > buff->size - buff->position)
    {
        // Encoding failed. Not enough data.
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    }
    while (ret == 0 && buff->position < buff->size)
    {
        if ((ret = bb_getUInt8(buff, &tag)) != 0)
        {
            break;
        }
        switch (tag)
        {
            // 0xA1
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_APPLICATION_CONTEXT_NAME:
        {
            if ((ret = apdu_parseApplicationContextName(settings, buff, &ciphered)) != 0)
            {

                return 0;
            }
#ifndef DLMS_IGNORE_SERVER
            if (ciphered)
            {
                afu = (DLMS_AFU_MISSING)(DLMS_AFU_MISSING_SENDER_ACSE_REQUIREMENTS | DLMS_AFU_MISSING_MECHANISM_NAME | DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE);
            }
#endif // DLMS_IGNORE_SERVER
        }
        break;
            // 0xA4 Result
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_USER_INFORMATION:
            // Check result component. Some meters are returning invalid user-information if connection failed.
            if (*result != DLMS_ASSOCIATION_RESULT_ACCEPTED && *diagnostic != DLMS_SOURCE_DIAGNOSTIC_NONE)
            {
                if ((ret = apdu_handleResultComponent(*diagnostic)) != 0)
                {
                }
                return ret;
            }
            if ((ret = apdu_parseUserInformation(settings, buff, ciphered, command)) != 0)
            {
                printf("reached apdu.parseUserInformation");
            }
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLED_AP_TITLE:
            // Get len.
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {

                break;
            }
            if (len != 3)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            // Choice for result.
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {

                break;
            }
            if (tag != BER_TYPE_INTEGER)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            // Get len.
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {
                break;
            }
            if (len != 1)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {
                break;
            }
            *result = (DLMS_ASSOCIATION_RESULT)tag;

            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLED_AE_QUALIFIER:
            if ((ret = bb_getUInt8(buff, &len)) != 0 ||
                (ret = bb_getUInt8(buff, &tag)) != 0 ||
                (ret = bb_getUInt8(buff, &len)) != 0)
            {
                break;
            }
            // Result source diagnostic component.
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {
                break;
            }
            if (tag != BER_TYPE_INTEGER)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {
                break;
            }
            if (len != 1)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {
                break;
            }
            *diagnostic = (DLMS_SOURCE_DIAGNOSTIC)tag;
            break;

        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLED_AP_INVOCATION_ID:
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {

                break;
            }
            if (len != 0xA)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            // Choice for result (Universal, Octet string type)
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {
                break;
            }
            if (tag != BER_TYPE_OCTET_STRING)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            // responding-AP-title-field
            // Get len.
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {
                break;
            }
            if ((ret = bb_get(buff, settings->sourceSystemTitle, len)) != 0)
            {
                break;
            }
            // If system title is invalid.
            if (len != 8)
            {
                memset(settings->sourceSystemTitle, 0, 8);
            }
            break;
        case (uint16_t)BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_SENDER_ACSE_REQUIREMENTS:
        case (uint16_t)BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_CALLING_AP_INVOCATION_ID:
            // Get sender ACSE-requirements field component.
            if ((ret = bb_getUInt8(buff, &len)) != 0)
            {

                break;
            }
            if (len != 2)
            {

                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {

                break;
            }

            // Get only value because client app is sending system title with LOW authentication.
            if ((ret = bb_getUInt8(buff, &tag)) != 0)
            {

                break;
            }
#ifndef DLMS_IGNORE_SERVER
            if (ciphered && tag == 0x80)
            {
                afu &= ~DLMS_AFU_MISSING_SENDER_ACSE_REQUIREMENTS;
            }
#endif // DLMS_IGNORE_SERVER
            break;
        case (uint16_t)BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_MECHANISM_NAME:
        case (uint16_t)BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_CALLING_AE_INVOCATION_ID:
            if ((ret = apdu_updateAuthentication(settings, buff)) != 0)
            {

                break;
            }
#ifndef DLMS_IGNORE_HIGH_GMAC
            unsigned char invalidSystemTitle;
            invalidSystemTitle = memcmp(settings->sourceSystemTitle, EMPTY_SYSTEM_TITLE, 8) == 0;
#ifndef DLMS_IGNORE_SERVER
            if (settings->server && settings->authentication > DLMS_AUTHENTICATION_LOW)
            {
                afu |= DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE;
            }
#endif // DLMS_IGNORE_SERVER
            if (settings->server && settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC && invalidSystemTitle)
            {
                *result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
                return 0;
            }
#endif // DLMS_IGNORE_HIGH_GMAC
#ifndef DLMS_IGNORE_SERVER
            if (ciphered)
            {
                afu &= ~DLMS_AFU_MISSING_MECHANISM_NAME;
            }
#endif // DLMS_IGNORE_SERVER
            break;

        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_SENDER_ACSE_REQUIREMENTS:
            if ((ret = bb_getUInt8(buff, &len)) != 0 ||
                (ret = bb_getUInt8(buff, &tag)) != 0 ||
                (ret = bb_getUInt8(buff, &len)) != 0 ||
                (ret = bb_clear(&settings->stoCChallenge)) != 0 ||
                (ret = bb_set2(&settings->stoCChallenge, buff, buff->position, len)) != 0)
            {
                break;
            }
            break;
        }
    }
    if (ret == 0)
    {
#ifndef DLMS_IGNORE_SERVER
        if (settings->server && afu != 0 &&
            *result == DLMS_ASSOCIATION_RESULT_ACCEPTED &&
            !(
                afu == DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE &&
                settings->authentication == DLMS_AUTHENTICATION_NONE))
        {
            *result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
            *diagnostic = DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_FAILURE;
            return 0;
        }
#endif // DLMS_IGNORE_SERVER
       // All meters don't send user-information if connection is failed.
       // For this reason result component is check again.
        if ((ret = apdu_handleResultComponent(*diagnostic)) != 0)
        {
#ifdef DLMS_DEBUG
            svr_notifyTrace(GET_STR_FROM_EEPROM("handleResultComponent."), ret);
#endif // DLMS_DEBUG
        }
#ifndef DLMS_IGNORE_HIGH_GMAC
        // Check that user is not trying to connect without ciphered connection.
        if (ret == 0 && settings->expectedSecurityPolicy != 0xFF)
        {
            if (settings->cipher.security != settings->expectedSecurityPolicy << 4)
            {
                return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
            }
        }
#endif // DLMS_IGNORE_HIGH_GMAC
    }
    return ret;
}