#ifndef VALUE_EVENT_ARG_H
#define VALUE_EVENT_ARG_H

#include "gxobjects.h"
#include "errorcodes.h"

typedef struct
{
    /**
     * CGXDLMSVariant value.
     */
    dlmsVARIANT value;
#if !defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
    /**
     * Data type of the value.
     */
    DLMS_DATA_TYPE dataType;
#endif //! defined(DLMS_IGNORE_MALLOC) && !defined(DLMS_COSEM_EXACT_DATA_TYPES)
    /**
     * Is request handled.
     */
    unsigned char handled;
    /**
     * Target DLMS object
     */
    gxObject *target;

    /**
     * Attribute index.
     */
    unsigned char index;
    /**
     * Data index.
     */
    uint16_t dataIndex;
    /**
     * Optional selector.
     */
    unsigned char selector;
    /**
     * Optional parameters.
     */
    dlmsVARIANT parameters;
    /**
     * Occurred error.
     */
    DLMS_ERROR_CODE error;
    /**
     * Is action. This is reserved for internal use.
     */
    unsigned char action;

    /**
     * Is value added as byte array.
     */
    unsigned char byteArray;

    /**
     * Is value max PDU size skipped.
     */
    unsigned char skipMaxPduSize;

    /**
     *  Transaction begin index.
     */
    uint32_t transactionStartIndex;
    /**
     *  Transaction end index.
     */
    uint32_t transactionEndIndex;
    // It this transaction.
    uint16_t transaction;
} gxValueEventArg;

#endif