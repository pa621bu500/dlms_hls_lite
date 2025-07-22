#ifndef MEDIA_H
#define MEDIA_H
#include "bytebuffer.h"
#include "dlmssettings.h"


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
        //Received data.
        gxByteBuffer data;
        //Receiver thread handle.
        int receiverThread;
        //If receiver thread is closing.
        unsigned char closing;

        dlmsSettings settings;
    } connection;

void con_initializeBuffers(
    connection* connection,
    int size);


#endif