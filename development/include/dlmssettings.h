
#ifndef DLMS_SETTINGS_H
#define DLMS_SETTINGS_H
#include <stdint.h>
#include "ciphering.h"
#include "bytebuffer.h"
#include "enums.h"
#include "objectarray.h"

typedef enum
{
    /*
     * Reserved zero conformance bit.
     */
    DLMS_CONFORMANCE_NONE = 0,
    /*
     * Reserved zero conformance bit.
     */
    // DLMS_CONFORMANCE_RESERVED_ZERO = 0x1,

    /*
    * General protection conformance bit.
    // */
    DLMS_CONFORMANCE_GENERAL_PROTECTION = 0x2,

    // /*
    // * General block transfer conformance bit.
    // */
    DLMS_CONFORMANCE_GENERAL_BLOCK_TRANSFER = 0x4,
    // /*
    // * Read conformance bit.
    // */
    // DLMS_CONFORMANCE_READ = 0x8,
    // /*
    // * Write conformance bit.
    // */
    // DLMS_CONFORMANCE_WRITE = 0x10,
    // /*
    // * Un confirmed write conformance bit.
    // */
    // DLMS_CONFORMANCE_UN_CONFIRMED_WRITE = 0x20,
    // /*
    // Delta value encoding.
    // */
    // DLMS_CONFORMANCE_DELTA_VALUE_ENCODING = 0x40,
    // /*
    // * Reserved seven conformance bit.
    // */
    // DLMS_CONFORMANCE_RESERVED_SEVEN = 0x80,
    // /*
    // * Attribute 0 supported with set conformance bit.
    // */
    // DLMS_CONFORMANCE_ATTRIBUTE_0_SUPPORTED_WITH_SET = 0x100,
    // /*
    // * Priority mgmt supported conformance bit.
    // */
    // DLMS_CONFORMANCE_PRIORITY_MGMT_SUPPORTED = 0x200,
    // /*
    // * Attribute 0 supported with get conformance bit.
    // */
    // DLMS_CONFORMANCE_ATTRIBUTE_0_SUPPORTED_WITH_GET = 0x400,
    // /*
    // * Block transfer with get or read conformance bit.
    // */
    DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_GET_OR_READ = 0x800,
    // /*
    // * Block transfer with set or write conformance bit.
    // */
    DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_SET_OR_WRITE = 0x1000,
    // /*
    // * Block transfer with action conformance bit.
    // */
    DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_ACTION = 0x2000,
    // /*
    // * multiple references conformance bit.
    // */
    DLMS_CONFORMANCE_MULTIPLE_REFERENCES = 0x4000,
    // /*
    // * Information report conformance bit.
    // */
    // DLMS_CONFORMANCE_INFORMATION_REPORT = 0x8000,
    // /*
    // * Data notification conformance bit.
    // */
    // DLMS_CONFORMANCE_DATA_NOTIFICATION = 0x10000,
    // /*
    // * Access conformance bit.
    // */
    // DLMS_CONFORMANCE_ACCESS = 0x20000,
    // /*
    // * Parameterized access conformance bit.
    // */
    // DLMS_CONFORMANCE_PARAMETERIZED_ACCESS = 0x40000,
    // /*
    // * Get conformance bit.
    // */
    DLMS_CONFORMANCE_GET = 0x80000,
    // /*
    // * Set conformance bit.
    // */
    DLMS_CONFORMANCE_SET = 0x100000,
    // /*
    // * Selective access conformance bit.
    // */
    DLMS_CONFORMANCE_SELECTIVE_ACCESS = 0x200000,
    // /*
    // * Event notification conformance bit.
    // */
    // DLMS_CONFORMANCE_EVENT_NOTIFICATION = 0x400000,
    // /*
    // * Action conformance bit.
    // */
    DLMS_CONFORMANCE_ACTION = 0x800000
} DLMS_CONFORMANCE;

typedef struct
{
    // gxByteBuffer systemTitle;

    /**
     * Initial credit (IC) tells how many times the frame must be repeated.
     * Maximum value is 7.
     */
    unsigned char initialCredit;
    /**
     * The current credit (CC) initial value equal to IC and automatically
     * decremented by the MAC layer after each repetition. Maximum value is 7.
     */
    unsigned char currentCredit;

    /**
     * Delta credit (DC) is used by the system management application entity
     * (SMAE) of the Client for credit management, while it has no meaning for a
     * Server or a REPEATER. It represents the difference(IC-CC) of the last
     * communication originated by the system identified by the DA address to
     * the system identified by the SA address. Maximum value is 3.
     */
    unsigned char deltaCredit;
    /**
     * Source MAC address.
     */
    uint16_t macSourceAddress;
    /**
     * Destination MAC address.
     */
    uint16_t macDestinationAddress;
    /**
     * Response probability.
     */
    unsigned char responseProbability;
    /**
     * Allowed time slots.
     */
    uint16_t allowedTimeSlots;
    /**
     * Server saves client system title.
     */
    // gxByteBuffer clientSystemTitle;
} gxPlcSettings;




 typedef struct
    {
        // Is custom challenges used. If custom challenge is used new challenge is
        // not generated if it is Set. This is for debugging purposes.
        unsigned char customChallenges;

        // Client to server challenge.
        gxByteBuffer ctoSChallenge;

        // Server to Client challenge.
        gxByteBuffer stoCChallenge;

        unsigned char sourceSystemTitle[8];

        // Invoke ID.
        unsigned char invokeID;

        //Long Invoke ID.
        int longInvokeID;

        // Priority.
        DLMS_PRIORITY priority;

        // Service class.
        DLMS_SERVICE_CLASS serviceClass;

        // Client address.
        uint16_t clientAddress;
        //Server address.
        uint32_t serverAddress;
        unsigned char useLogicalNameReferencing;
        DLMS_INTERFACE_TYPE interfaceType;
        DLMS_AUTHENTICATION authentication;
        gxByteBuffer password;
#ifndef DLMS_IGNORE_MALLOC
        gxByteBuffer kek;
#else
        unsigned char kek[16];
#endif //DLMS_IGNORE_MALLOC
        /**
        * DLMS version number.
        */
        unsigned char dlmsVersionNumber;

        /**
        * Max PDU size used in communicating.
        */
        uint16_t maxPduSize;

        /**
        * The PDU size proposed by the client.
        */
        uint16_t clientPduSize;

        /**
        * Max PDU size that server uses. Client can ask anything, but server will decide.
        */
        uint16_t maxServerPDUSize;

        /**
        * HDLC sender frame sequence number.
        */
        unsigned char senderFrame;

        /**
        * HDLC receiver block sequence number.
        */
        unsigned char receiverFrame;
        unsigned char server;
        unsigned char isAuthenticationRequired;

        // The client side defines which services the client wants to use.
        // The server side defines available services.
        DLMS_CONFORMANCE proposedConformance;

        // Defines the services agreed upon between the client and the server, 
        // based on the client's needs and the server's capabilities.
        DLMS_CONFORMANCE negotiatedConformance;

        // On the server side defines what conformances the client proposed.
        // This information is needed with secure release.
        DLMS_CONFORMANCE clientProposedConformance;

        //Used max info TX.
        uint16_t maxInfoTX;
        //Used max info RX.
        uint16_t maxInfoRX;
        //Used max window size in TX.
        unsigned char windowSizeTX;
        //Used max window size in RX.
        unsigned char windowSizeRX;

        // Initialize PDU size that is restored after the connection is closed.
        uint16_t initializePduSize;
        //Initialized max info TX.
        uint16_t initializeMaxInfoTX;
        //Initialized max info RX.
        uint16_t initializeMaxInfoRX;
        //Initialized max window size in TX.
        unsigned char initializeWindowSizeTX;
        //Initialized max window size in RX.
        unsigned char initializeWindowSizeRX;
#ifndef DLMS_IGNORE_PLC
        //PLC settings.
        gxPlcSettings plcSettings;
#endif //DLMS_IGNORE_PLC

        //List of internal COSEM objects.
        //Objects in this list are not added to assocaition view.
        //Objects can be used to save internal data.
        objectArray internalObjects;

        //List of COSEM objects.
        objectArray objects;

        // Block packet index.
        uint32_t blockIndex;
        //Is connected to the meter.
        DLMS_CONNECTION_STATE connected;

#ifndef DLMS_IGNORE_HIGH_GMAC
        ciphering cipher;
#endif //DLMS_IGNORE_HIGH_GMAC

        int16_t userId;

        /**
        *  Protocol version.
        */
        unsigned char protocolVersion;

        unsigned char qualityOfService;
        //Pre-established Application Associations system title.

#ifndef DLMS_IGNORE_MALLOC
        gxByteBuffer* preEstablishedSystemTitle;
#else
        unsigned char preEstablishedSystemTitle[8];
#endif //DLMS_IGNORE_MALLOC

        //Client serializes data to this PDU when malloc is not used or heap size is limited.
        gxByteBuffer* serializedPdu;
        //Auto increase Invoke ID.
        unsigned char autoIncreaseInvokeID;
        //Client adds objects that are not found from the association view here so they are released when client is clear.
        objectArray releasedObjects;

        /////////////////////////////////////////////////////////////////////////
        // Expected security suite.
        // If Expected security suite is set client can't connect with other security suite.
        unsigned char expectedSecuritySuite;
        /////////////////////////////////////////////////////////////////////////
        // Expected security policy.
        // If Expected security policy is set client can't connect with other security policies.
        unsigned char expectedSecurityPolicy;
        /////////////////////////////////////////////////////////////////////////
        // Expected Invocation(Frame) counter value.
        // Expected Invocation counter is not check if value is zero.
#ifdef DLMS_COSEM_INVOCATION_COUNTER_SIZE64
        uint64_t* expectedInvocationCounter;
#else
        uint32_t* expectedInvocationCounter;
#endif //DLMS_COSEM_INVOCATION_COUNTER_SIZE64
        /////////////////////////////////////////////////////////////////////////
        // Expected client system title.
        unsigned char* expectedClientSystemTitle;
    } dlmsSettings;

typedef struct
{
    dlmsSettings base;
} dlmsServerSettings;


void plc_reset(
    dlmsSettings *settings);

unsigned char getNextSend(
    dlmsSettings *settings, unsigned char first);

void resetFrameSequence(
    dlmsSettings *settings);

void cl_init(
    dlmsSettings *settings,
    unsigned char useLogicalNameReferencing,
    uint16_t clientAddress,
    uint32_t serverAddress,
    DLMS_AUTHENTICATION authentication,
    const char *password,
    DLMS_INTERFACE_TYPE interfaceType);

void cl_clear(
    dlmsSettings *settings);
void cip_clear(ciphering *target);

unsigned char isCiphered(
    ciphering *cipher);

#endif

