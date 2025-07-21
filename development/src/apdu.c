#include "../include/gxmem.h"
#include "../include/enums.h"
#include "../include/helpers.h"
#include "../include/apdu.h"
#include "../include/errorcodes.h"
#include "../include/ciphering.h"

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