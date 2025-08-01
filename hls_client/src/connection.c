#include "../include/connection.h"

//Initialize connection buffers.
void con_initializeBuffers(connection* connection, int size)
{
    if (size == 0)
    {
        bb_clear(&connection->data);
    }
    else
    {
        bb_capacity(&connection->data, size);
    }
}

void con_init(connection* con, GX_TRACE_LEVEL trace)
{
    con->trace = trace;
    //Reply wait time is 5 seconds.
    con->waitTime = 5000;
    #if defined(_WIN32) || defined(_WIN64)//If Windows
    //EVS2 NOT SUPPORTED
    #else
        con->comPort = -1;
    #endif
    con->socket = -1;
    con->receiverThread = -1;
    con->closing = 0;
    bb_init(&con->data);
    bb_capacity(&con->data, 500);
}

void con_close(connection* con)
{
#if defined(_WIN32) || defined(_WIN64)//If Windows
    con->comPort = INVALID_HANDLE_VALUE;
#else
    if (con->comPort != -1)
    {
        int ret = close(con->comPort);
        if (ret < 0)
        {
            printf("Failed to close port.\r\n");
        }
        con->comPort = -1;
    }
#endif
    con->socket = -1;
    bb_clear(&con->data);
    con->closing = 0;
    con_initializeBuffers(con, 0);
}

