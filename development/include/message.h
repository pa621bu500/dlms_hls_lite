#ifndef MESSAGE_H
#define MESSAGE_H

#define MESSAGE_CAPACITY 2
#include "bytebuffer.h"
typedef struct
{
    gxByteBuffer** data;
    unsigned char capacity;
    unsigned char size;
#ifndef DLMS_IGNORE_MALLOC
    unsigned char attached;
#endif //DLMS_IGNORE_MALLOC
} message;


void mes_init(message* mes);
void mes_clear(
    message* mes);

    int mes_push(
    message* mes,
    gxByteBuffer* item);

#endif