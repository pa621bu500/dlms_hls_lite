
#include "../include/client.h"


#include "../include/dlms.h"
#include "../include/cosem.h"
#include "../include/gxmem.h"
#include "../include/dlmssettings.h"
#include "../include/message.h"
#include "../include/errorcodes.h"
#include "../include/bytebuffer.h"

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