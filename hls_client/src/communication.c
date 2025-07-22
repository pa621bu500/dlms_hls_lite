#define INVALID_HANDLE_VALUE -1
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>      //Add support for sockets
#include <netdb.h>      //Add support for sockets
#include <sys/types.h>  //Add support for sockets
#include <sys/socket.h> //Add support for sockets
#include <netinet/in.h> //Add support for sockets
#include <arpa/inet.h>  //Add support for sockets
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../development/include/errorcodes.h"
#include "../include/communication.h"
#include "../include/connection.h"
#include "../../development/include/cosem.h"
#include "../../development/include/message.h"
#include "../../development/include/replydata.h"
#include "../../development/include/gxobjects.h"
#include "../../development/include/bytebuffer.h"
#include "../../development/include/client.h"

int com_open(
    connection *connection,
    const char *port)
{
    int ret = 0;
    // In Linux serial port name might be very long.

    // read/write | not controlling term | don't wait for DCD line signal.
    connection->comPort = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (connection->comPort == -1) // if open is unsuccessful.
    {
        ret = errno;
        printf("Failed to open serial port: %s\n", port);
        // return DLMS_ERROR_TYPE_COMMUNICATION_ERROR | ret;
        return 1;
    }
    if (!isatty(connection->comPort))
    {
        ret = errno;
        printf("Failed to Open port %s. This is not a serial port.\n", port);
        // return DLMS_ERROR_TYPE_COMMUNICATION_ERROR | ret;
        return 1;
    }

    return ret;
}

int com_readSerialPort(
    clientConnection *connection,
    unsigned char eop)
{
    // Read reply data.
    int ret, cnt = 1, pos;
    unsigned char eopFound = 0;
    int lastReadIndex = 0;
#if defined(_WIN32) || defined(_WIN64) // Windows
    //EVS2 NOT SUPPORTED
#else
    unsigned short bytesRead = 0;
    unsigned short readTime = 0;
#endif
    do
    {
#if defined(_WIN32) || defined(_WIN64) // Windows
        // EVS2 NOT SUPPORTED
#else
        // Get bytes available.
        ret = ioctl(connection->comPort, FIONREAD, &cnt);
        // If driver is not supporting this functionality.
        if (ret < 0)
        {
            cnt = RECEIVE_BUFFER_SIZE;
        }
        else if (cnt == 0)
        {
            // Try to read at least one byte.
            cnt = 1;
        }
        // If there is more data than can fit to buffer.
        if (cnt > RECEIVE_BUFFER_SIZE)
        {
            cnt = RECEIVE_BUFFER_SIZE;
        }
        bytesRead = read(connection->comPort, connection->data.data + connection->data.size, cnt);
        if (bytesRead == 0xFFFF)
        {
            // If there is no data on the read buffer.
            if (errno == EAGAIN)
            {
                if (readTime > connection->waitTime)
                {
                    return DLMS_ERROR_CODE_RECEIVE_FAILED;
                }
                readTime += 100;
                bytesRead = 0;
            }
            // If connection is closed.
            else if (errno == EBADF)
            {
                return DLMS_ERROR_CODE_RECEIVE_FAILED;
            }
            else
            {
                return DLMS_ERROR_CODE_RECEIVE_FAILED;
            }
        }
#endif
        connection->data.size += (unsigned short)bytesRead;
        // Note! Some USB converters can return true for ReadFile and Zero as bytesRead.
        // In that case wait for a while and read again.
        if (bytesRead == 0)
        {
#if defined(_WIN32) || defined(_WIN64) // Windows
            Sleep(100);
#else
            usleep(100000);
#endif
            continue;
        }
        // Search eop.
        if (connection->data.size > 5)
        {
            // Some optical strobes can return extra bytes.
            for (pos = connection->data.size - 1; pos != lastReadIndex; --pos)
            {
                if (connection->data.data[pos] == eop)
                {
                    eopFound = 1;
                    break;
                }
            }
            lastReadIndex = pos;
        }
    } while (eopFound == 0);
    return DLMS_ERROR_CODE_OK;
}

int readData(connection* connection, gxByteBuffer* data, int* index)
{
    int ret = 0;
    if (connection->comPort != INVALID_HANDLE_VALUE)
    {
        if ((ret = com_readSerialPort(connection, 0x7E)) != 0)
        {
            return ret;
        }
    }
    // else
    // {
    //     uint32_t cnt = connection->data.capacity - connection->data.size;
    //     if (cnt < 1)
    //     {
    //         return DLMS_ERROR_CODE_OUTOFMEMORY;
    //     }
    //     if ((ret = recv(connection->socket, (char*)connection->data.data + connection->data.size, cnt, 0)) == -1)
    //     {
    //         return DLMS_ERROR_CODE_RECEIVE_FAILED;
    //     }
    //     connection->data.size += ret;
    // }
    // if (connection->trace > GX_TRACE_LEVEL_INFO)
    // {
    //     char* hex = hlp_bytesToHex(connection->data.data + *index, connection->data.size - *index);
    //     if (*index == 0)
    //     {
    //         printf("\nRX:\t %s", hex);
    //     }
    //     else
    //     {
    //         printf(" %s", hex);
    //     }
    //     free(hex);
    //     *index = connection->data.size;
    // }
    return 0;
}

int readDLMSPacket(
    connection* connection,
    gxByteBuffer* data,
    gxReplyData* reply)
{
    char* hex;
    int index = 0, ret;
    if (data->size == 0)
    {
        return DLMS_ERROR_CODE_OK;
    }
    reply->complete = 0;
    connection->data.size = 0;
    connection->data.position = 0;

    if ((ret = sendData(connection, data)) != 0)
    {
        return ret;
    }
    //Loop until packet is complete.
    unsigned char pos = 0;
    unsigned char isNotify;
    gxReplyData notify;
    reply_init(&notify);
    do
    {
         if ((ret = readData(connection, &connection->data, &index)) != 0)
        {
            if (ret != DLMS_ERROR_CODE_RECEIVE_FAILED || pos == 3)
            {
                break;
            }
            ++pos;
            printf("\nData send failed. Try to resend %d/3\n", pos);
            if ((ret = sendData(connection, data)) != 0)
            {
                break;
            }
        }
        else
        {
            ret = cl_getData2(&connection->settings, &connection->data, reply, &notify, &isNotify);
            if (ret != 0 && ret != DLMS_ERROR_CODE_FALSE)
            {
                break;
            }
            // if (isNotify)
            // {
            //     gxByteBuffer bb;
            //     bb_init(&bb);
            //     var_toString(&notify.dataValue, &bb);
            //     char* tmp = bb_toString(&bb);
            //     printf("Notification received: %s", tmp);
            //     free(tmp);
            //     bb_clear(&bb);
            // }
        }
    } while (reply->complete == 0);
    reply_clear(&notify);
    if (connection->trace == GX_TRACE_LEVEL_VERBOSE)
    {
        printf("\n");
    }
    return ret;
}

int sendData(connection *connection, gxByteBuffer *data)
{
    int ret = 0;
    #if defined(_WIN32) || defined(_WIN64) // Windows
        //EVS2 NOT SUPPORTED
    #endif
    if (connection->comPort != INVALID_HANDLE_VALUE)
    {
        #if defined(_WIN32) || defined(_WIN64) // Windows
            //EVS2 NOT SUPPORTED
        #else
                ret = write(connection->comPort, data->data, data->size);
                if (ret != data->size)
                {
                    ret = errno;
                    return DLMS_ERROR_TYPE_COMMUNICATION_ERROR | ret;
                }
        #endif
    }
    else
    {
        if (send(connection->socket, (const char *)data->data, data->size, 0) == -1)
        {
            #if defined(_WIN32) || defined(_WIN64) // If Windows
                    //EVS2 NOT SUPPORTED
            #else
                ret = errno;
            #endif
                return DLMS_ERROR_TYPE_COMMUNICATION_ERROR | ret;
        }
    }
    return 0;
}

int com_readDataBlock(
    connection* connection,
    message* messages,
    gxReplyData* reply)
{
    gxByteBuffer rr;
    int pos, ret = DLMS_ERROR_CODE_OK;
    //If there is no data to send.
    if (messages->size == 0)
    {
        return DLMS_ERROR_CODE_OK;
    }
    bb_init(&rr);
    //Send data.
    for (pos = 0; pos != messages->size; ++pos)
    {
        //Send data.
        if ((ret = readDLMSPacket(connection, messages->data[pos], reply)) != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
        //Check is there errors or more data from server
        while (reply_isMoreData(reply))
        {
            // if ((ret = cl_receiverReady(&connection->settings, reply->moreData, &rr)) != DLMS_ERROR_CODE_OK)
            // {
            //     bb_clear(&rr);
            //     return ret;
            // }
            // if ((ret = readDLMSPacket(connection, &rr, reply)) != DLMS_ERROR_CODE_OK)
            // {
            //     bb_clear(&rr);
            //     return ret;
            // }
            // bb_clear(&rr);
        }
    }
    return ret;
}

int com_updateInvocationCounter(
    connection *connection,
    const char *invocationCounter)
{
    int ret = DLMS_ERROR_CODE_OK;
    // Read frame counter if GeneralProtection is used.
    if (invocationCounter != NULL && connection->settings.cipher.security != DLMS_SECURITY_NONE)
    {
        if ((ret = com_initializeOpticalHead(connection)) != 0)
        {
            return ret;
        }
        message messages;
        gxReplyData reply;
        unsigned short add = connection->settings.clientAddress;
        DLMS_AUTHENTICATION auth = connection->settings.authentication;
        DLMS_SECURITY security = connection->settings.cipher.security;
        gxByteBuffer *preEstablishedSystemTitle = connection->settings.preEstablishedSystemTitle;
        gxByteBuffer challenge;
        bb_init(&challenge);
        bb_set(&challenge, connection->settings.ctoSChallenge.data, connection->settings.ctoSChallenge.size);
        connection->settings.clientAddress = 16;
        connection->settings.preEstablishedSystemTitle = NULL;
        connection->settings.authentication = DLMS_AUTHENTICATION_NONE;
        connection->settings.cipher.security = DLMS_SECURITY_NONE;
        if (connection->trace > GX_TRACE_LEVEL_WARNING)
        {
            printf("updateInvocationCounter\r\n");
        }
        mes_init(&messages);
        reply_init(&reply);
        // Get meter's send and receive buffers size.
        if (ret = cl_snrmRequest(&connection->settings, &messages) != 0 ||
        (ret = com_readDataBlock(connection, &messages, &reply)) != 0 
        || 
         (ret = cl_parseUAResponse(&connection->settings, &reply.data)) != 0
        )
        {
            printf("Sss");
        }
        mes_clear(&messages);
        reply_clear(&reply);
        if ((ret = cl_aarqRequest(&connection->settings, &messages)) != 0 ||
            (ret = com_readDataBlock(connection, &messages, &reply)) != 0 
            ||
            (ret = cl_parseAAREResponse(&connection->settings, &reply.data)) != 0
        )
        {
            bb_clear(&challenge);
            mes_clear(&messages);
            reply_clear(&reply);
            if (ret == DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
            {
                if (connection->trace > GX_TRACE_LEVEL_OFF)
                {
                    printf("Use Logical Name referencing is wrong. Change it!\r\n");
                }
                return ret;
            }
            // if (connection->trace > GX_TRACE_LEVEL_OFF)
            // {
            //     printf("AARQRequest failed %s\r\n", hlp_getErrorMessage(ret));
            // }
            return ret;
        }
         mes_clear(&messages);
        reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }
         mes_clear(&messages);
        reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }
        mes_clear(&messages);
        reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }
         reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }
    
        mes_clear(&messages);
        reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            // con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }
        gxData d;
        cosem_init(BASE(d), DLMS_OBJECT_TYPE_DATA, invocationCounter);
        if ((ret = com_read(connection, BASE(d), 2)) == 0)
        {
            connection->settings.cipher.invocationCounter = 1 + var_toInteger(&d.value);
            if (connection->trace > GX_TRACE_LEVEL_WARNING)
            {
                // printf("Invocation counter: %u (0x%X)\r\n",
                // connection->settings.cipher.invocationCounter,
                // connection->settings.cipher.invocationCounter);
            }
            // It's OK if this fails.
            com_disconnect(connection);
            connection->settings.clientAddress = add;
            connection->settings.authentication = auth;
            connection->settings.cipher.security = security;
            bb_clear(&connection->settings.ctoSChallenge);
            bb_set(&connection->settings.ctoSChallenge, challenge.data, challenge.size);
            bb_clear(&challenge);
            connection->settings.preEstablishedSystemTitle = preEstablishedSystemTitle;  
        }
    }
    return ret;
}

int com_initializeConnection(
    connection *connection)
{
    message messages;
    gxReplyData reply;
    int ret = com_initializeOpticalHead(connection);
    if (ret != 0)
    {
        return ret;
    }
    if (connection->trace > GX_TRACE_LEVEL_WARNING)
    {
        // printf("InitializeConnection\r\n");
    }

    mes_init(&messages);
    reply_init(&reply);
    // Get meter's send and receive buffers size.
    if ((ret = cl_snrmRequest(&connection->settings, &messages)) != 0 ||
        (ret = com_readDataBlock(connection, &messages, &reply)) != 0 ||
        (ret = cl_parseUAResponse(&connection->settings, &reply.data)) != 0)
    {
        mes_clear(&messages);
        reply_clear(&reply);
        if (connection->trace > GX_TRACE_LEVEL_OFF)
        {
            // printf("SNRMRequest failed %s\r\n", hlp_getErrorMessage(ret));
        }
        return ret;
    }
    mes_clear(&messages);
    reply_clear(&reply);
    if (connection->settings.preEstablishedSystemTitle == NULL)
    {
        if ((ret = cl_aarqRequest(&connection->settings, &messages)) != 0 ||
            (ret = com_readDataBlock(connection, &messages, &reply)) != 0 ||
            (ret = cl_parseAAREResponse(&connection->settings, &reply.data)) != 0)
        {
            if (ret == DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
            {
                if (connection->trace > GX_TRACE_LEVEL_OFF)
                {
                    printf("Use Logical Name referencing is wrong. Change it!\r\n");
                }
            }
            else if (connection->trace > GX_TRACE_LEVEL_OFF)
            {
                if (ret == (DLMS_ERROR_TYPE_EXCEPTION_RESPONSE | DLMS_EXCEPTION_SERVICE_ERROR_INVOCATION_COUNTER_ERROR))
                {
                    // If invocation counter value is too low.
                    // Get invocation counter value.
                    uint32_t value = 0;
                    bb_getUInt32(&reply.data, &value);
                    printf("Connection failed. Expected invocation counter value: %u \r\n", value);
                }
                else
                {
                    printf("AARQRequest failed %s\r\n", hlp_getErrorMessage(ret));
                }
            }
            mes_clear(&messages);
            reply_clear(&reply);
            return ret;
        }
        mes_clear(&messages);
        reply_clear(&reply);
        if (connection->settings.maxPduSize == 0xFFFF)
        {
            con_initializeBuffers(connection, connection->settings.maxPduSize);
        }
        else
        {
            // Allocate 50 bytes more because some meters count this wrong and send few bytes too many.
            con_initializeBuffers(connection, 50 + connection->settings.maxPduSize);
        }

        // Get challenge Is HLS authentication is used.
        if (connection->settings.authentication > DLMS_AUTHENTICATION_LOW)
        {
            if ((ret = cl_getApplicationAssociationRequest(&connection->settings, &messages)) != 0 ||
                (ret = com_readDataBlock(connection, &messages, &reply)) != 0 ||
                (ret = cl_parseApplicationAssociationResponse(&connection->settings, &reply.data)) != 0)
            {
                mes_clear(&messages);
                reply_clear(&reply);
                return ret;
            }
            mes_clear(&messages);
            reply_clear(&reply);
        }
    }
    else
    {
        // Allocate buffers for pre-established connection.
        con_initializeBuffers(connection, connection->settings.maxPduSize);
    }
    return DLMS_ERROR_CODE_OK;
}


int com_loadHardcodedObjects(connection *connection)
{
    // Clear old objects from settings
    oa_clear(&connection->settings.objects, 1);

    // --- GXDLMSData: 0.0.96.1.0.255 (Serial Number) ---
    // initialize a new GxData object
    gxData *serialNumber = (gxData *)malloc(sizeof(gxData));
    if (serialNumber == NULL)
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    // clears (zeros out) the memory that was allocated for the gxData struct.
    memset(serialNumber, 0, sizeof(gxData));
    // Set the DLMS object type to DATA
    serialNumber->base.objectType = DLMS_OBJECT_TYPE_DATA;
    /*
        converts a string-form OBIS code (like "0.0.96.1.0.255") into a 6-byte logical name
        ln[0] = 0;
        ln[1] = 0;
        ln[2] = 96;
        ln[3] = 1;
        ln[4] = 0;
        ln[5] = 255;
     */
    hlp_setLogicalName(serialNumber->base.logicalName, "0.0.96.1.0.255");
    // add object to the settings.objects array
    oa_push(&connection->settings.objects, (gxObject *)serialNumber);

    // --- GXDLMSRegister: 1.0.1.8.0.255 (kWh) ---
    gxRegister *totalEnergy = (gxRegister *)malloc(sizeof(gxRegister));
    if (totalEnergy == NULL)
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    memset(totalEnergy, 0, sizeof(gxRegister));
    totalEnergy->base.objectType = DLMS_OBJECT_TYPE_REGISTER;
    hlp_setLogicalName(totalEnergy->base.logicalName, "1.0.1.8.0.255");
    oa_push(&connection->settings.objects, (gxObject *)totalEnergy);

    // --- GXDLMSDisconnectControl: 0.0.96.3.10.255 (Relay Status) ---
    gxDisconnectControl *relay = (gxDisconnectControl *)malloc(sizeof(gxDisconnectControl));
    if (relay == NULL)
        return DLMS_ERROR_CODE_OUTOFMEMORY;
    memset(relay, 0, sizeof(gxDisconnectControl));
    relay->base.objectType = DLMS_OBJECT_TYPE_DISCONNECT_CONTROL;
    hlp_setLogicalName(relay->base.logicalName, "0.0.96.3.10.255");
    oa_push(&connection->settings.objects, (gxObject *)relay);

    return DLMS_ERROR_CODE_OK;
}


int com_initializeOpticalHead(
    connection *connection)
{
    unsigned short baudRate;
    int ret = 0, len, pos;
    unsigned char ch;
    // In Linux serial port name might be very long.
    char buff[50];

    struct termios options;
    memset(&options, 0, sizeof(options));
    options.c_iflag = 0;
    options.c_oflag = 0;

    // 8n1, see termios.h for more information
    options.c_cflag = CS8 | CREAD | CLOCAL;
    /*
    options.c_cflag &= ~PARENB
    options.c_cflag &= ~CSTOPB
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    */
    // Set Baud Rates
    cfsetospeed(&options, B9600);
    cfsetispeed(&options, B9600);

    options.c_lflag = 0;
    options.c_cc[VMIN] = 1;
    // How long we are waiting reply charachter from serial port.
    options.c_cc[VTIME] = 5;

    // hardware flow control is used as default.
    // options.c_cflag |= CRTSCTS;
    if (tcsetattr(connection->comPort, TCSAFLUSH, &options) != 0)
    {
        printf("Failed to Open port. tcsetattr failed.\r");
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
