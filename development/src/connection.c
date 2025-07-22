

#include "../include/connection.h"
#include <stdlib.h> // malloc and free needs this or error is generated.
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)//Windows includes
#else
#include <string.h> // string function definitions
#include <unistd.h> // UNIX standard function definitions
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <termios.h> // POSIX terminal control definitions
#include <time.h>   // time calls
#endif

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