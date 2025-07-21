
#include <stdio.h>
#include <string.h>
#include "../../development/include/client.h"
#include "../../development/include/cosem.h"
#include "connection.h"


int com_initializeOpticalHead(
    connection* connection);

int com_readDataBlock(
connection *connection,
message* messages,
gxReplyData* reply);


int readDLMSPacket(
    connection *connection,
    gxByteBuffer* data,
    gxReplyData* reply);


      typedef struct
    {
        //Is trace used.
        GX_TRACE_LEVEL trace;
        //Socked handle.
        int socket;
        //Serial port handle.
#if defined(_WIN32) || defined(_WIN64)// If Windows
        HANDLE comPort;
        OVERLAPPED		osWrite;
        OVERLAPPED		osReader;
#else //If Linux
        int comPort;
#endif
        unsigned long   waitTime;
        //Received data is read here from the serial port or TCP/IP socket.
        gxByteBuffer data;
        //Receiver thread handle.
        int receiverThread;
        //If receiver thread is closing.
        unsigned char closing;

        dlmsSettings settings;
    } clientConnection;