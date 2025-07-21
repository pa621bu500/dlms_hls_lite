


#ifndef DLMS_IGNORE_MALLOC
#include "../include/gxmem.h"
#endif //DLMS_IGNORE_MALLOC

#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include <string.h>
#include "../include/message.h"
#include "../include/errorcodes.h"


void mes_init(message* mes)
{
    mes->capacity = MESSAGE_CAPACITY;
    mes->data = (gxByteBuffer**)gxmalloc(mes->capacity * sizeof(gxByteBuffer*));
    mes->size = 0;
    mes->attached = 0;
}

void mes_clear(message* mes)
{
    int pos;
#ifdef DLMS_IGNORE_MALLOC
    for (pos = 0; pos != mes->capacity; ++pos)
    {
        mes->data[pos]->size = mes->data[pos]->position = 0;
    }
#else
    if (!mes->attached)
    {
        if (mes->size != 0)
        {
            for (pos = 0; pos != mes->size; ++pos)
            {
                gxfree(mes->data[pos]->data);
                gxfree(mes->data[pos]);
            }
        }
        if (mes->data != NULL)
        {
            gxfree(mes->data);
            mes->data = NULL;
        }
        mes->capacity = 0;
    }
#endif //DLMS_IGNORE_MALLOC
    mes->size = 0;
}

int mes_push(message* mes, gxByteBuffer* item)
{
    if (mes->size == mes->capacity)
    {
        if (mes->attached)
        {
            return DLMS_ERROR_CODE_OUTOFMEMORY;
        }
        mes->capacity += MESSAGE_CAPACITY;
        if (mes->data == NULL)
        {
            mes->data = (gxByteBuffer**)gxmalloc(mes->capacity * sizeof(gxByteBuffer*));
            if (mes->data == NULL)
            {
                mes->capacity = 0;
                return DLMS_ERROR_CODE_OUTOFMEMORY;
            }
        }
        else
        {
            gxByteBuffer** old = mes->data;
        #ifdef gxrealloc
                    //If compiler supports realloc.
                    mes->data = (gxByteBuffer**)gxrealloc(mes->data, mes->capacity * sizeof(gxByteBuffer*));
                    if (mes->data == NULL)
                    {
                        mes->capacity -= MESSAGE_CAPACITY;
                        mes->data = old;
                        return DLMS_ERROR_CODE_OUTOFMEMORY;
                    }
        #else
                    //EVS2 NOT SUPPORTED
        #endif // gxrealloc      
                }
        }
    mes->data[mes->size] = item;
    ++mes->size;
    return 0;
}