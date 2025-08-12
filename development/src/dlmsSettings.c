#include "../include/gxmem.h"
#include "../include/dlmssettings.h"

// Server sender frame sequence starting number.
static const unsigned char SERVER_START_SENDER_FRAME_SEQUENCE = 0x1E;
// Server receiver frame sequence starting number.
static const unsigned char SERVER_START_RECEIVER_FRAME_SEQUENCE = 0xFE;
// Client sender frame sequence starting number.
static const unsigned char CLIENT_START_SENDER_FRAME_SEQUENCE = 0xFE;
// Client receiver frame sequence starting number.
static const unsigned char CLIENT_START_RCEIVER_FRAME_SEQUENCE = 0xE;

void cl_init(
    dlmsSettings *settings,
    unsigned char useLogicalNameReferencing,
    uint16_t clientAddress,
    uint32_t serverAddress,
    DLMS_AUTHENTICATION authentication,
    const char *password,
    DLMS_INTERFACE_TYPE interfaceType)
{
    settings->autoIncreaseInvokeID = 0;
    settings->qualityOfService = 0;
    settings->protocolVersion = 0;
    settings->preEstablishedSystemTitle = NULL;
    settings->blockIndex = 1;
    settings->clientAddress = clientAddress;
    settings->serverAddress = serverAddress;
    settings->dlmsVersionNumber = 6;
    settings->useLogicalNameReferencing = useLogicalNameReferencing;
    settings->interfaceType = interfaceType;
    settings->authentication = authentication;
    BYTE_BUFFER_INIT(&settings->password);
    bb_addString(&settings->password, password);
    memset(settings->sourceSystemTitle, 0, sizeof(settings->sourceSystemTitle));
    BYTE_BUFFER_INIT(&settings->kek);

    settings->maxServerPDUSize = 1024;
    settings->maxPduSize = 0xFFFF;
    settings->server = 0;
    if (useLogicalNameReferencing)
    {
        settings->proposedConformance = (DLMS_CONFORMANCE)(DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_ACTION |
                                                           DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_SET_OR_WRITE |
                                                           DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_GET_OR_READ |
                                                           DLMS_CONFORMANCE_SET |
                                                           DLMS_CONFORMANCE_SELECTIVE_ACCESS |
                                                           DLMS_CONFORMANCE_ACTION |
                                                           DLMS_CONFORMANCE_MULTIPLE_REFERENCES |
                                                           DLMS_CONFORMANCE_GET |
                                                           DLMS_CONFORMANCE_GENERAL_PROTECTION);
    }
    settings->longInvokeID = 0;
    settings->maxInfoTX = settings->maxInfoRX = 0x80;
    settings->windowSizeTX = settings->windowSizeRX = 1;
    settings->connected = DLMS_CONNECTION_STATE_NONE;
    oa_init(&settings->objects);
    settings->connected = DLMS_CONNECTION_STATE_NONE;
    settings->customChallenges = 0;
    settings->invokeID = 1;
    BYTE_BUFFER_INIT(&settings->ctoSChallenge);
    BYTE_BUFFER_INIT(&settings->stoCChallenge);
    settings->priority = DLMS_PRIORITY_HIGH;
    settings->serviceClass = DLMS_SERVICE_CLASS_CONFIRMED;
#ifndef DLMS_IGNORE_HIGH_GMAC
    cip_init(&settings->cipher);
#endif // DLMS_IGNORE_HIGH_GMAC
    settings->userId = -1;
    resetFrameSequence(settings);
    settings->serializedPdu = NULL;
    oa_init(&settings->releasedObjects);
    settings->expectedSecurityPolicy = 0xFF;
    settings->expectedSecuritySuite = 0xFF;
    settings->expectedInvocationCounter = NULL;
    settings->expectedClientSystemTitle = NULL;
#ifndef DLMS_IGNORE_PLC
    plc_reset(settings);
#endif // DLMS_IGNORE_PLC
    oa_init(&settings->internalObjects);
}

#ifndef DLMS_IGNORE_PLC
void plc_reset(
    dlmsSettings *settings)
{
    settings->plcSettings.initialCredit = 7;
    settings->plcSettings.currentCredit = 7;
    settings->plcSettings.deltaCredit = 0;
    // New device addresses are used.
    settings->plcSettings.macSourceAddress = DLMS_PLC_HDLC_SOURCE_ADDRESS_INITIATOR;
    settings->plcSettings.macDestinationAddress = DLMS_PLC_DESTINATION_ADDRESS_ALL_PHYSICAL;

    settings->plcSettings.allowedTimeSlots = 0x14;

    settings->plcSettings.responseProbability = 100;
}
#endif // DLMS_IGNORE_PLC

void cl_clear(
    dlmsSettings *settings)
{
    settings->protocolVersion = 0;
#ifndef DLMS_IGNORE_MALLOC
    if (settings->preEstablishedSystemTitle != NULL)
    {
        bb_clear(settings->preEstablishedSystemTitle);
        gxfree(settings->preEstablishedSystemTitle);
        settings->preEstablishedSystemTitle = NULL;
    }
#else
    memset(settings->preEstablishedSystemTitle, 0, 8);
#endif // DLMS_IGNORE_MALLOC
    memset(settings->sourceSystemTitle, 0, sizeof(settings->sourceSystemTitle));
    bb_clear(&settings->password);
#ifdef DLMS_IGNORE_MALLOC
    memset(settings->kek, 0, sizeof(settings->kek));
#else
    bb_clear(&settings->kek);
#endif // DLMS_IGNORE_MALLOC
    // oa_clear(&settings->objects, !settings->server);
    // settings->connected = DLMS_CONNECTION_STATE_NONE;
    settings->customChallenges = 0;
    settings->invokeID = 1;
    bb_clear(&settings->ctoSChallenge);
    bb_clear(&settings->stoCChallenge);
    // settings->priority = DLMS_PRIORITY_HIGH;
    // settings->serviceClass = DLMS_SERVICE_CLASS_CONFIRMED;
#ifndef DLMS_IGNORE_HIGH_GMAC
    cip_clear(&settings->cipher);
#endif // DLMS_IGNORE_HIGH_GMAC
    settings->maxPduSize = 0xFFFF;
    settings->userId = -1;
    // oa_clear(&settings->releasedObjects, 1);
    // oa_clear(&settings->internalObjects, 0);
    resetFrameSequence(settings);
    settings->expectedInvocationCounter = NULL;
}

void resetFrameSequence(
    dlmsSettings *settings)
{
    if (settings->server)
    {
        settings->senderFrame = SERVER_START_SENDER_FRAME_SEQUENCE;
        settings->receiverFrame = SERVER_START_RECEIVER_FRAME_SEQUENCE;
    }
    else
    {
        settings->senderFrame = CLIENT_START_SENDER_FRAME_SEQUENCE;
        settings->receiverFrame = CLIENT_START_RCEIVER_FRAME_SEQUENCE;
    }
}

void cip_clear(ciphering *target)
{
    target->invocationCounter = 1;
    target->security = DLMS_SECURITY_NONE;
    target->encrypt = 0;
#ifndef DLMS_IGNORE_MALLOC
    bb_clear(&target->blockCipherKey);
    bb_clear(&target->broadcastBlockCipherKey);
    bb_clear(&target->systemTitle);
    bb_clear(&target->authenticationKey);
    if (target->dedicatedKey != NULL)
    {
        bb_clear(target->dedicatedKey);
        gxfree(target->dedicatedKey);
        target->dedicatedKey = NULL;
    }
#else
    memset(target->blockCipherKey, 0, sizeof(DEFAULT_BLOCK_CIPHER_KEY));
    memset(target->broadcastBlockCipherKey, 0, sizeof(DEFAULT_BROADCAST_BLOCK_CIPHER_KEY));
    memset(target->systemTitle, 0, 8);
    memset(target->authenticationKey, 0, sizeof(DEFAULT_AUTHENTICATION_KEY));
    memset(target->dedicatedKey, 0, sizeof(DEFAULT_BLOCK_CIPHER_KEY));
#endif // DLMS_IGNORE_MALLOC
}

void resetBlockIndex(
    dlmsSettings *settings)
{
    settings->blockIndex = 1;
}

unsigned char isCiphered(
    ciphering *cipher)
{
    return cipher->security != DLMS_SECURITY_NONE;
}

// Increase sender sequence.
//
// @param value
//            Frame value.
// Increased sender frame sequence.
unsigned char increaseSendSequence(
    unsigned char value)
{
    return (unsigned char)((value & 0xF0) | ((value + 0x2) & 0xE));
}

unsigned char increaseReceiverSequence(
    unsigned char value)
{
    return ((value + 0x20) | 0x10 | (value & 0xE));
}

unsigned char checkFrame(
    dlmsSettings *settings,
    unsigned char frame)
{
    // If notify
    if (frame == 0x13)
    {
        return 1;
    }
    // If U frame.
    if ((frame & 0x3) == 3)
    {
        if (frame == 0x93)
        {
            unsigned char isEcho = !settings->server && frame == 0x93 &&
                                   (settings->senderFrame == 0x10 || settings->senderFrame == 0xfe) &&
                                   settings->receiverFrame == 0xE;
            resetFrameSequence(settings);
            return !isEcho;
        }
        if (frame == 0x73 && !settings->server)
        {
            return settings->senderFrame == 0xFE && settings->receiverFrame == 0xE;
        }
        return 1;
    }
    if ((frame & 0x1) == 1)
    {
        // If echo.
        if (frame == (settings->senderFrame & 0xF1))
        {
            return 0;
        }
        settings->receiverFrame = increaseReceiverSequence(settings->receiverFrame);
        return 1;
    }
    unsigned char expected;
    if ((settings->senderFrame & 0x1) == 0)
    {
        expected = increaseReceiverSequence(increaseSendSequence(settings->receiverFrame));
        if (frame == expected)
        {
            settings->receiverFrame = frame;
            return 1;
        }
        // If the final bit is not set.
        if (frame == (expected & ~0x10) && settings->windowSizeRX != 1)
        {
            settings->receiverFrame = frame;
            return 1;
        }
        // If Final bit is not set for the previous message.
        if ((settings->receiverFrame & 0x10) == 0 && settings->windowSizeRX != 1)
        {
            expected = (unsigned char)(0x10 | increaseSendSequence(settings->receiverFrame));
            if (frame == expected)
            {
                settings->receiverFrame = frame;
                return 1;
            }
            // If the final bit is not set.
            if (frame == (expected & ~0x10))
            {
                settings->receiverFrame = frame;
                return 1;
            }
        }
    }
    else
    {
        expected = increaseSendSequence(settings->receiverFrame);
        if (frame == expected)
        {
            settings->receiverFrame = frame;
            return 1;
        }
        if (frame == (expected & ~0x10))
        {
            settings->receiverFrame = frame;
            return 1;
        }
        if (settings->windowSizeRX != 1)
        {
            // If HDLC window size is bigger than one.
            if (frame == (expected | 0x10))
            {
                settings->receiverFrame = frame;
                return 1;
            }
        }
    }
    // Pre-established connections needs this.
    if ((!settings->server && settings->receiverFrame == SERVER_START_RECEIVER_FRAME_SEQUENCE) ||
        (settings->server && settings->receiverFrame == CLIENT_START_RCEIVER_FRAME_SEQUENCE))
    {
        settings->receiverFrame = frame;
        return 1;
    }
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) // If Windows or Linux
    printf("Invalid frame %X. Expected %X.\r\n", frame, expected);
#endif
    return 0;
}

unsigned char getNextSend(
    dlmsSettings *settings, unsigned char first)
{
    if (first)
    {
        settings->senderFrame = increaseReceiverSequence(increaseSendSequence((unsigned char)settings->senderFrame));
    }
    else
    {
        settings->senderFrame = increaseSendSequence((unsigned char)settings->senderFrame);
    }
    return (unsigned char)settings->senderFrame;
}