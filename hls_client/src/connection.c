#include "../include/connection.h"

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


