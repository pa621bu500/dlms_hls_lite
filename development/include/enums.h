#ifndef ENUMS_H
#define ENUMS_H
 
 typedef enum
    {
        DLMS_AUTHENTICATION_NONE = 0,
        DLMS_AUTHENTICATION_HIGH_GMAC = 5
    } DLMS_AUTHENTICATION;

     typedef enum
    {
        DLMS_INTERFACE_TYPE_HDLC = 0,
        DLMS_INTERFACE_TYPE_PLC,
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

} DLMS_OBJECT_TYPE;


typedef enum
    {
        DLMS_DATA_TYPE_NONE = 0,
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
        DLMS_COMMAND_INITIATE_REQUEST = 0x1,
        DLMS_COMMAND_INITIATE_RESPONSE = 0x8,
        DLMS_COMMAND_SNRM = 0x93,
    } DLMS_COMMAND;


    


       typedef enum {
        //Connection is not made for the meter.
        DLMS_CONNECTION_STATE_NONE = 0,
        //Connection is made for DLMS level.
        DLMS_CONNECTION_STATE_DLMS = 2,

    }DLMS_CONNECTION_STATE;

#endif