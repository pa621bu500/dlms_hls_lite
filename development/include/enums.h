#ifndef ENUMS_H
#define ENUMS_H
 
 typedef enum
    {
        DLMS_AUTHENTICATION_NONE = 0,
        DLMS_AUTHENTICATION_LOW = 1,
        DLMS_AUTHENTICATION_HIGH_GMAC = 5
    } DLMS_AUTHENTICATION;

     typedef enum
    {
        DLMS_INTERFACE_TYPE_HDLC = 0,
        DLMS_INTERFACE_TYPE_PLC,
        DLMS_INTERFACE_TYPE_PDU,
        DLMS_INTERFACE_TYPE_PLC_HDLC,
        DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E,
        DLMS_INTERFACE_TYPE_WRAPPER = 0x1,
    } DLMS_INTERFACE_TYPE;

        typedef enum
    {
        // Initiator.
    DLMS_PLC_HDLC_SOURCE_ADDRESS_INITIATOR = 0xC01,
    }DLMS_PLC_HDLC_SOURCE_ADDRESS;

       typedef enum
    {
        DLMS_PLC_DESTINATION_ADDRESS_ALL_PHYSICAL = 0xFFF
    }DLMS_PLC_DESTINATION_ADDRESS;


    typedef enum
    {
        HDLC_INFO_MAX_INFO_TX = 0x5,
        HDLC_INFO_MAX_INFO_RX = 0x6,
        HDLC_INFO_WINDOW_SIZE_TX = 0x7,
        HDLC_INFO_WINDOW_SIZE_RX = 0x8
    } HDLC_INFO;

    typedef enum
    {
        DEFAULT_MAX_INFO_TX = 128,
        DEFAULT_MAX_INFO_RX = 128,
        DEFAULT_MAX_WINDOW_SIZE_TX = 1,
        DEFAULT_MAX_WINDOW_SIZE_RX = 1
    } DEFAULT_MAX;

    /*
    * Specifies trace levels.
    *
    */
    typedef enum {
        GX_TRACE_LEVEL_OFF,

        /*
        * Output error-handling messages.
        */
        GX_TRACE_LEVEL_ERROR,

        /*
        * Output warnings and error-handling messages.
        */
        GX_TRACE_LEVEL_WARNING,

        /*
        * Output informational messages, warnings, and error-handling messages.
        */
        GX_TRACE_LEVEL_INFO,

        /*
        * Output all debugging and tracing messages.
        */
        GX_TRACE_LEVEL_VERBOSE
    }GX_TRACE_LEVEL;

typedef enum
{
    /*
    * Authentication and Encryption security are used.
    */
     DLMS_SECURITY_NONE = 0,

    DLMS_SECURITY_AUTHENTICATION_ENCRYPTION = 0x30,
} DLMS_SECURITY;

typedef enum
{

        DLMS_SECURITY_SUITE_V0 = 0,
        /*
         AES-GCM-128 authenticated encryption, ECDSA P-256 digital signature, ECDH P-256 key agreement, SHA-256 hash, V.44 compression and AES-128 key wrap.
        */
        DLMS_SECURITY_SUITE_V1 = 1,
        /*
            AES-GCM-256 authenticated encryption, ECDSA P-384 digital signature, ECDH P-384 key agreement, SHA-384 hash, V.44 compression and AES-256 key wrap
        */
        DLMS_SECURITY_SUITE_V2 = 2

} DLMS_SECURITY_SUITE;


typedef enum
{
DLMS_SECURITY_POLICY_NOTHING = 0,
    DLMS_SECURITY_POLICY_AUTHENTICATED_ENCRYPTED = 3,
   
} DLMS_SECURITY_POLICY;
typedef enum tagDLMS_OBJECT_TYPE
{
    DLMS_OBJECT_TYPE_NONE = 0,
    DLMS_OBJECT_TYPE_DATA = 1,
    DLMS_OBJECT_TYPE_REGISTER = 3,
    DLMS_OBJECT_TYPE_ASSOCIATION_SHORT_NAME = 12,
    DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME = 15,

} DLMS_OBJECT_TYPE;


typedef enum
    {
        DLMS_DATA_TYPE_NONE = 0,
        DLMS_DATA_TYPE_BIT_STRING = 4,
        DLMS_DATA_TYPE_OCTET_STRING = 9,
        DLMS_DATA_TYPE_STRING = 10,
        DLMS_DATA_TYPE_INT8 = 15,
        DLMS_DATA_TYPE_INT16 = 16,
        DLMS_DATA_TYPE_UINT8 = 17,
        DLMS_DATA_TYPE_INT64 = 20,

    } DLMS_DATA_TYPE;
typedef enum
    {
        DLMS_DATA_REQUEST_TYPES_NONE = 0x0,
        DLMS_DATA_REQUEST_TYPES_FRAME = 0x1,
        DLMS_DATA_REQUEST_TYPES_BLOCK = 0x2,
        // DLMS_DATA_REQUEST_TYPES_GBT = 0x4
    } DLMS_DATA_REQUEST_TYPES;

typedef enum
    {
        DLMS_MESSAGE_TYPE_COSEM_APDU = 0,
        // DLMS_MESSAGE_TYPE_COSEM_APDU_XML = 1,
        // DLMS_MESSAGE_TYPE_MANUFACTURER_SPESIFIC = 128
    } DLMS_MESSAGE_TYPE;

       typedef enum
    {
        // Normal priority.
        DLMS_PRIORITY_NORMAL = 0,

        // High priority.
        DLMS_PRIORITY_HIGH = 1
    } DLMS_PRIORITY;


        typedef enum
    {
        DLMS_SERVICE_CLASS_UN_CONFIRMED = 0,
        DLMS_SERVICE_CLASS_CONFIRMED = 1
    } DLMS_SERVICE_CLASS;


typedef enum
    {
        /*
        * No command to execute.
        */
        DLMS_COMMAND_NONE = 0,
        DLMS_ACTION_COMMAND_TYPE_NORMAL = 1,
        DLMS_COMMAND_INITIATE_REQUEST = 0x1,
        DLMS_COMMAND_INITIATE_RESPONSE = 0x8,
        DLMS_COMMAND_DISC = 0x53,
        DLMS_COMMAND_AARQ = 0x60,
        DLMS_COMMAND_AARE = 0x61,
        DLMS_COMMAND_RELEASE_REQUEST = 0x62,
        DLMS_COMMAND_SNRM = 0x93,
        DLMS_COMMAND_ACCESS_REQUEST = 0xD9,
        DLMS_COMMAND_GLO_GET_REQUEST = 0xC8,
        DLMS_COMMAND_GLO_INITIATE_REQUEST = 0x21,
        DLMS_COMMAND_GLO_SET_REQUEST = 0xC9,
        DLMS_COMMAND_DATA_NOTIFICATION = 0x0F,
        DLMS_COMMAND_EVENT_NOTIFICATION = 0xC2,
          DLMS_COMMAND_METHOD_REQUEST = 0xC3,
        DLMS_COMMAND_UA = 0x73,
        DLMS_COMMAND_DISCONNECT_MODE = 0x1f,
         DLMS_COMMAND_REJECTED = 0x97,
    } DLMS_COMMAND;


    


       typedef enum {
        //Connection is not made for the meter.
        DLMS_CONNECTION_STATE_NONE = 0,
        //Connection is made for DLMS level.
        DLMS_CONNECTION_STATE_HDLC = 1,
        DLMS_CONNECTION_STATE_DLMS = 2,

    }DLMS_CONNECTION_STATE;

    typedef enum
    {
         BER_TYPE_APPLICATION = 0x40,
         BER_TYPE_CONSTRUCTED = 0x20,
         BER_TYPE_CONTEXT = 0x80,
        BER_TYPE_OBJECT_IDENTIFIER = 0x6,
        BER_TYPE_OCTET_STRING = 0x4,
        BER_TYPE_INTEGER,
    };

        typedef enum
    {
        DLMS_ASSOCIATION_RESULT_ACCEPTED = 0,
        DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED = 1,
        DLMS_ASSOCIATION_RESULT_TRANSIENT_REJECTED = 2
    } DLMS_ASSOCIATION_RESULT;

      typedef enum
    {
        DLMS_SOURCE_DIAGNOSTIC_NONE = 0,
        DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_REQUIRED = 14
    } DLMS_SOURCE_DIAGNOSTIC;


     typedef enum
    {
        /*
        * IMPLICIT BIT STRING {version1  = 0} DEFAULT {version1}
        */
        PDU_TYPE_PROTOCOL_VERSION = 0,

        // /*
        // * Application-context-name
        // */
        PDU_TYPE_APPLICATION_CONTEXT_NAME = 1,

        // /*
        // * AP-title OPTIONAL
        // */
        // PDU_TYPE_CALLED_AP_TITLE = 2,

        // /*
        // * AE-qualifier OPTIONAL.
        // */
        // PDU_TYPE_CALLED_AE_QUALIFIER = 3,

        // /*
        // * AP-invocation-identifier OPTIONAL.
        // */
        // PDU_TYPE_CALLED_AP_INVOCATION_ID = 4,

        // /*
        // * AE-invocation-identifier OPTIONAL
        // */
        // PDU_TYPE_CALLED_AE_INVOCATION_ID = 5,

        // /*
        // * AP-title OPTIONAL
        // */
        // PDU_TYPE_CALLING_AP_TITLE = 6,

        // /*
        // * AE-qualifier OPTIONAL
        // */
        // PDU_TYPE_CALLING_AE_QUALIFIER = 7,

        // /*
        // * AP-invocation-identifier OPTIONAL
        // */
        // PDU_TYPE_CALLING_AP_INVOCATION_ID = 8,

        // /*
        // * AE-invocation-identifier OPTIONAL
        // */
        PDU_TYPE_CALLING_AE_INVOCATION_ID = 9,

        // /*
        // * The following field shall not be present if only the kernel is used.
        // */
        // PDU_TYPE_SENDER_ACSE_REQUIREMENTS = 10,

        // /*
        // * The following field shall only be present if the authentication
        // * functional unit is selected.
        // */
        // PDU_TYPE_MECHANISM_NAME = 11,

        // /*
        // * The following field shall only be present if the authentication
        // * functional unit is selected.
        // */
        PDU_TYPE_CALLING_AUTHENTICATION_VALUE = 12,

        // /*
        // * Implementation-data.
        // */
        // PDU_TYPE_IMPLEMENTATION_INFORMATION = 29,

        // /*
        // * Association-information OPTIONAL
        // */
        PDU_TYPE_USER_INFORMATION = 30
    } PDU_TYPE;


    typedef enum
    {

        HDLC_FRAME_TYPE_U_FRAME = 0x3
    } HDLC_FRAME_TYPE;

    typedef enum
    {
        DLMS_COUNT_TYPE_TAG = 0x1,
        DLMS_COUNT_TYPE_DATA = 2,
        DLMS_COUNT_TYPE_PACKET = 3
    } DLMS_COUNT_TYPE;


#endif