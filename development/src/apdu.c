#include "../include/gxmem.h"
#include "../include/enums.h"
#include "../include/helpers.h"
#include "../include/apdu.h"
#include "../include/errorcodes.h"
#include "../include/ciphering.h"



unsigned char useDedicatedKey(dlmsSettings* settings)
{
#ifndef DLMS_IGNORE_MALLOC
    if (settings->cipher.dedicatedKey == NULL)
    {
        return 0;
    }
    return settings->cipher.dedicatedKey->size == 16;
#else
    return memcmp(settings->cipher.dedicatedKey, EMPTY_KEY, sizeof(EMPTY_KEY)) != 0;
#endif //DLMS_IGNORE_MALLOC
}
int apdu_getAuthenticationString(
    dlmsSettings* settings,
    gxByteBuffer* data)
{
    int ret = 0;
    gxByteBuffer* callingAuthenticationValue = NULL;
    if (settings->authentication != DLMS_AUTHENTICATION_NONE
    #ifndef DLMS_IGNORE_HIGH_GMAC
            || settings->cipher.security != DLMS_SECURITY_NONE
    #endif //DLMS_IGNORE_HIGH_GMAC
            )
    {
        // unsigned char p[] = { 0x60, 0x85, 0x74, 0x05, 0x08, 0x02 };
        // // Add sender ACSE-requirements field component.
        // if ((ret = bb_setUInt8(data, (uint16_t)BER_TYPE_CONTEXT | (char)PDU_TYPE_SENDER_ACSE_REQUIREMENTS)) != 0 ||
        //     (ret = bb_setUInt8(data, 2)) != 0 ||
        //     (ret = bb_setUInt8(data, BER_TYPE_BIT_STRING | BER_TYPE_OCTET_STRING)) != 0 ||
        //     (ret = bb_setUInt8(data, 0x80)) != 0 ||
        //     (ret = bb_setUInt8(data, (uint16_t)BER_TYPE_CONTEXT | (char)PDU_TYPE_MECHANISM_NAME)) != 0 ||
        //     // Len
        //     (ret = bb_setUInt8(data, 7)) != 0 ||
        //     // OBJECT IDENTIFIER
        //     (ret = bb_set(data, p, 6)) != 0 ||
        //     (ret = bb_setUInt8(data, settings->authentication)) != 0)
        // {
        //     //Error code is returned at the end of the function.
        // }
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

int apdu_generateAarq(
    dlmsSettings* settings,
    gxByteBuffer* data)
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

int apdu_generateApplicationContextName(
    dlmsSettings* settings,
    gxByteBuffer* data)
{
    int ret;
    //ProtocolVersion
    if (settings->protocolVersion != 0)
    {
        if ((ret = bb_setUInt8(data, BER_TYPE_CONTEXT | (unsigned char)PDU_TYPE_PROTOCOL_VERSION)) != 0 ||
            (ret = bb_setUInt8(data, 2)) != 0 ||
            //Un-used bits.
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
#endif //DLMS_IGNORE_HIGH_GMAC

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
        settings->authentication == DLMS_AUTHENTICATION_HIGH_GMAC ))
    {
#ifndef DLMS_IGNORE_MALLOC
        if (settings->cipher.systemTitle.size == 0)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
#endif //DLMS_IGNORE_MALLOC
        // Add calling-AP-title
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
#endif //DLMS_IGNORE_MALLOC
        {
            return ret;
        }
    }
#endif
    //Add CallingAEInvocationId.
    if (!settings->server && settings->userId != -1 && settings->cipher.security != DLMS_SECURITY_NONE)
    {
        if ((ret = bb_setUInt8(data, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_CALLING_AE_INVOCATION_ID))) != 0 ||
            //LEN
            (ret = bb_setUInt8(data, 3)) != 0 ||
            (ret = bb_setUInt8(data, BER_TYPE_INTEGER)) != 0 ||
            //LEN
            (ret = bb_setUInt8(data, 1)) != 0 ||
            (ret = bb_setUInt8(data, (unsigned char)settings->userId)) != 0)
        {
            return ret;
        }
    }
    return 0;
}


int apdu_generateUserInformation(
    dlmsSettings* settings,
    gxByteBuffer* data)
{
    int ret = 0;
    bb_setUInt8(data, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | (unsigned char)PDU_TYPE_USER_INFORMATION);
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (!isCiphered(&settings->cipher))
#endif //DLMS_IGNORE_HIGH_GMAC
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
#endif //DLMS_IGNORE_MALLOC

        if ((ret = apdu_getInitiateRequest(settings, &crypted)) != 0)
        {
            return ret;
        }
#ifndef DLMS_IGNORE_MALLOC
        // ret = cip_encrypt(
        //     &settings->cipher,
        //     settings->cipher.security,
        //     DLMS_COUNT_TYPE_PACKET,
        //     settings->cipher.invocationCounter,
        //     DLMS_COMMAND_GLO_INITIATE_REQUEST,
        //     settings->cipher.systemTitle.data,
        //     &settings->cipher.blockCipherKey,
        //     &crypted);
#else
        ret = cip_encrypt(
            &settings->cipher,
            settings->cipher.security,
            DLMS_COUNT_TYPE_PACKET,
            settings->cipher.invocationCounter,
            DLMS_COMMAND_GLO_INITIATE_REQUEST,
            settings->cipher.systemTitle,
            settings->cipher.blockCipherKey,
            &crypted);
#endif //DLMS_IGNORE_MALLOC
        if (ret == 0)
        {
            // Length for AARQ user field
            if ((ret = bb_setUInt8(data, (unsigned char)(2 + crypted.size))) != 0 ||
                // Coding the choice for user-information (Octet string, universal)
                (ret = bb_setUInt8(data, BER_TYPE_OCTET_STRING)) != 0 ||
                (ret = bb_setUInt8(data, (unsigned char)crypted.size)) != 0 ||
                (ret = bb_set2(data, &crypted, 0, crypted.size)) != 0)
            {
                //Error code is returned at the end of the function.
            }
        }
#ifndef DLMS_IGNORE_MALLOC
        bb_clear(&crypted);
#endif //DLMS_IGNORE_MALLOC
    }
#endif //DLMS_IGNORE_HIGH_GMAC
    return ret;
}

int apdu_getInitiateRequest(
    dlmsSettings* settings,
    gxByteBuffer* data)
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
#endif //DLMS_IGNORE_MALLOC
    }
#endif //DLMS_IGNORE_HIGH_GMAC

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
