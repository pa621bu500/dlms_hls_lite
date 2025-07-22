#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h> /* memset */
#include <unistd.h>
#include <strings.h>

#include "../include/communication.h"
#include "../include/connection.h"
// #include "../../development/include/gxserializer.h"
#include "../include/poll_result.h"
#include "../../development/include/bytebuffer.h"

#define GET_METER_SN 100
#define POLL_ITEM_TOTAL_ACTIVE_ENERGY 1300
#define GET_RELAY_STATUS 5500
#define SET_RELAY_ON 5600
#define SET_RELAY_OFF 5700
#define METER_SN_OBIS "0.0.96.1.0.255:2"
#define TOTAL_ACTIVE_ENERGY_OBIS "0.0.96.1.0.255:2,1.0.1.8.0.255:2"
#define RELAY_STATUS_OBIS "0.0.96.3.10.255:2"
#define RELAY_ON_OBIS "0.0.96.3.10.255:1"
#define RELAY_OFF_OBIS "0.0.96.3.10.255:1"
#define READING_DATA_FOLDER "./reading_data"

const int supported_batches[] = {2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025};
const char *invalid_arg_prompt = "Invalid arguments. Usage: %s --model=<4char> --batch=<yyyy> --mac=<int> --comm_item=<int> [--usb_offset=<int>]\n";
bool is_dev = false;
char model[5] = "";
bool is_supported_model = false;
bool is_supported_batch = false;
int batch_year = -1;
int dest_mac = -1;
int comm_item = -1;
int usb_offset = 0;
bool arg_found_model = false;
bool arg_found_batch = false;
bool arg_found_mac = false;
bool arg_found_comm_item = false;
bool arg_found_usb_offset = false;
bool arg_found_env = false;

int parse_named_arg(char* arg, const char* prefix) {
    if (strncmp(arg, prefix, strlen(prefix)) != 0) {
        return -1; // Not matching prefix
    }
    char* val_str = arg + strlen(prefix);
    for (int i = 0; val_str[i] != '\0'; i++) {
        if (!isdigit(val_str[i])) {
            return -1; // Not a valid number
        }
    }
    return atoi(val_str);
 }


int connectMeter(int argc, char *argv[])
{
    connection con;
    gxByteBuffer item;
           printf("reached here");
    bb_init(&item);
    con_init(&con, GX_TRACE_LEVEL_INFO);
    // Initialize settings using Logical Name referencing and HDLC.
    cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
    con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
    int ret, opt = 0;
    int port = 0;
    char *address = NULL;
    char *serialPort = NULL;
    char *p, *readObjects = NULL, *outputFile = NULL;
    int index, a, b, c, d, e, f;
    char *invocationCounter = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "--env=", 6) == 0)
        {
            const char *env_value = argv[i] + 6;
            if (strcmp(env_value, "dev") == 0)
            {
                is_dev = true;
            }
            else if (strcmp(env_value, "prod") == 0)
            {
                is_dev = false; // or set is_prod = true;
            }
            else
            {
                printf("invalid env, valid options : 1.dev 2.prod \n");
                return 1;
            };
        }
        else if (strncmp(argv[i], "--model=", 8) == 0)
        {
            strncpy(model, argv[i] + 8, sizeof(model) - 1);
            model[sizeof(model) - 1] = '\0'; // Null-terminate
            arg_found_model = true;
            if (is_dev)
            {
                printf("model:%s\n", model);
            };
        }
        else if (strncmp(argv[i], "--batch=", 8) == 0)
        {
            batch_year = parse_named_arg(argv[i], "--batch=");
            arg_found_batch = true;
            if (is_dev)
            {
                printf("batch year:%d\n", batch_year);
            };
        }
        else if (strncmp(argv[i], "--mac=", 6) == 0)
        {
            dest_mac = parse_named_arg(argv[i], "--mac=");
            arg_found_mac = true;
            if (is_dev)
            {
                printf("mac:%d\n", dest_mac);
            };
        }
        else if (strncmp(argv[i], "--comm_item=", 12) == 0)
        {
            comm_item = parse_named_arg(argv[i], "--comm_item=");
            arg_found_comm_item = true;
            if (is_dev)
            {
                printf("comm_item:%d\n", comm_item);
            };
        }
        else if (strncmp(argv[i], "--usb_offset=", 13) == 0)
        {
            // use_offset is optional
            usb_offset = parse_named_arg(argv[i], "--usb_offset=");
            arg_found_usb_offset = true;
            if (is_dev)
            {
                printf("usb_offset:%d\n", usb_offset);
            };
        }
    }
    if (!arg_found_model)
    {
        printf("❌ model not found in args\n");
        printf(invalid_arg_prompt, argv[0]);
        return 1;
    }
    if (!arg_found_batch)
    {
        printf("❌ batch not found in args\n");
        printf(invalid_arg_prompt, argv[0]);
        return 1;
    }
    if (!arg_found_mac)
    {
        printf("❌ mac not found in args\n");
        printf(invalid_arg_prompt, argv[0]);
        return 1;
    }
    if (!arg_found_comm_item)
    {
        printf("❌ comm_item not found in args\n");
        printf(invalid_arg_prompt, argv[0]);
        return 1;
    }

    for (int i = 0; i < sizeof(supported_batches) / sizeof(supported_batches[0]); i++)
    {
        if (batch_year == supported_batches[i])
        {
            is_supported_batch = true;
            break;
        }
    }

    const int com_port_number_default = 16;
    int com_port_number = -1;
    com_port_number = com_port_number_default + usb_offset;
    con.settings.serverAddress = dest_mac;
    con.settings.serverAddress = cl_getServerAddress(1, (unsigned short)con.settings.serverAddress, 0);
    con.settings.useLogicalNameReferencing = 1;
    con.settings.clientAddress = 111;
    // serialPort="/dev/ttyUSB0";
    // usb com port adjustment
    if(is_dev){
      printf("use com port number:%d\n", com_port_number);
    };
    con.settings.authentication = DLMS_AUTHENTICATION_HIGH_GMAC;
    bb_init(&con.settings.password);
    bb_addString(&con.settings.password, "00000000");
    con.trace = GX_TRACE_LEVEL_INFO;
    con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;
    bb_clear(&con.settings.cipher.authenticationKey);
    bb_addHexString(&con.settings.cipher.authenticationKey, "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF");
    bb_clear(&con.settings.cipher.blockCipherKey);
    bb_addHexString(&con.settings.cipher.blockCipherKey, "000102030405060708090A0B0C0D0E0F");
    invocationCounter="0.0.43.1.3.255";
    con.settings.autoIncreaseInvokeID = 1;
    bb_clear(&con.settings.cipher.systemTitle);
    bb_addHexString(&con.settings.cipher.systemTitle, "5753453030303031");

    t_poll_result *poll_result;
    poll_result = (t_poll_result *)malloc(sizeof(t_poll_result));
    memset((void *)poll_result, 0x00, sizeof(t_poll_result));
    poll_result->kwh = -1;
    poll_result->relay_status = -1;
    poll_result->parse_stage=0;

    switch (comm_item)
    {
        case GET_METER_SN:
            readObjects = METER_SN_OBIS;
            break;
        case POLL_ITEM_TOTAL_ACTIVE_ENERGY:
            readObjects = TOTAL_ACTIVE_ENERGY_OBIS;
            break;
        case GET_RELAY_STATUS:
            readObjects = RELAY_STATUS_OBIS;
            break;
        case SET_RELAY_ON:
            readObjects = RELAY_ON_OBIS;
            break;
        case SET_RELAY_OFF:
            readObjects = RELAY_OFF_OBIS;
            break;
        default:
            printf("invalid comm item, please input one of the comm item below\n");
            printf("100 - get meter sn\n");
            printf("1300 - get kwh \n");
            printf("5500 - get relay status \n");
            printf("5600 - set relay on \n");
            printf("5700 - set relay off \n");
            return 1;
    }

    switch (com_port_number) {
        case 16:
            serialPort = "/dev/ttyUSB0";
            break;
        case 17:
            serialPort = "/dev/ttyUSB1";
            break;
        case 18:
            serialPort = "/dev/ttyUSB2";
            break;
        case 19:
            serialPort = "/dev/ttyUSB3";
            break;
        case 20:
            serialPort = "/dev/ttyUSB4";
            break;
        case 24:
            serialPort = "/dev/ttyACM0";
            break;
        default:
            printf("Invalid serial port: com_port_number = %d\n", com_port_number);
            printf("Use --offset=0 for /dev/ttyUSB0 (com_port_number = 16)\n");
            printf("Use --offset=8 for /dev/ttyACM0 (com_port_number = 24)\n");
            return 1;
    }

    if (serialPort != NULL)
    {
        ret = readSerialPort(&con, serialPort, readObjects, invocationCounter, outputFile,poll_result);
        if (ret != 0)
        {
            printf("error:rx_timeout\n");
            return 1;
        }
        // if (comm_item == POLL_ITEM_TOTAL_ACTIVE_ENERGY){
        //     save_meter_reading(poll_result->meter_identity, poll_result->t, poll_result->kwh);
        // }
        printf("reached here");
    }
    else
    {
        printf("Error, serial port is Null\n");
        return 1;
    }
  
    // cl_clear(&con.settings);
    return 0;
}

/*Read DLMS meter using serial port connection.*/
int readSerialPort(
    connection *connection,
    const char *port,
    char *readObjects,
    const char *invocationCounter,
    const char *outputFile,
    t_poll_result *poll_result)
{
    int ret;
    ret = com_open(connection, port);
    //--------additional code to workaround the fix array issue ------
    char obisCopy[64]; 
    strncpy(obisCopy, readObjects, sizeof(obisCopy) - 1);
    obisCopy[sizeof(obisCopy) - 1] = '\0'; 
    //-------------
    if (ret == 0 && readObjects != NULL)
    {
         if ((ret = com_updateInvocationCounter(connection, invocationCounter)) == 0 
        && (ret = com_initializeConnection(connection)) == 0
        )
        {
           
        }
    }else{
         printf("Failed to Open port\n");
         return 1;
    }

    return ret;
}

int main(int argc, char* argv[])
{
    int ret = connectMeter(argc, argv);
    return ret;
}