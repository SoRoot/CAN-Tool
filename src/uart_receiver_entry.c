/* UART Receiver entry function */
#include "uart_receiver.h"
#include "stdio.h"
#include "string.h"
#include "r_can_api.h"

can_frame_t transmitFrame;

/* temporary structure for reading the can-config */
typedef struct
{
    uint8_t id[4];
    uint8_t data_length_code[2];
    struct byte{
        uint8_t hex[3];
    } data[8];
    can_frame_type_t type;
}can_conf;

void uart_receiver_entry(void);

void uart_receiver_entry(void)
{
    //open UART
}


// Get conf settings from user
can_conf read_uart(void)
{
    can_conf conf;

    static uint8_t msg[100];

    /* CAN-Type is always DATA-Type, REMOTE-Type is not used */
    conf.type = CAN_FRAME_TYPE_DATA;


    /*Ask for the CAN-parameters */
    sprintf (&msg, "Set ID in hex (without 0x in front): \r\n");
    g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
    /*Receive the CAN-parameter */
    g_sf_comms0.p_api->read (g_sf_comms0.p_ctrl, conf.id, sizeof(conf.id)-1, TX_WAIT_FOREVER);
    memset(&msg, NULL, 100);
    /*display the received parameter for the user */
    sprintf (&msg, "ID: 0x%s \r\n", conf.id);
    g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
    memset(&msg, NULL, 100);


    sprintf (&msg, "Set DLC: \r\n");
    g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
    g_sf_comms0.p_api->read (g_sf_comms0.p_ctrl, conf.data_length_code, sizeof(conf.data_length_code)-1, TX_WAIT_FOREVER);
    memset(&msg, NULL, 100);
    sprintf (&msg, "DLC: %s \r\n", conf.data_length_code);
    g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
    memset(&msg, NULL, 100);


    /*depending on the data length code, the DATA is requested */
    int i;
    for (int i = 0; i < conf.data_length_code[0] - '0'; i++) {

        sprintf (&msg, "Set %d. Data-Byte in hex (without 0x in front): \r\n", i);
        g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
        g_sf_comms0.p_api->read (g_sf_comms0.p_ctrl, conf.data[i].hex, sizeof(conf.data[i].hex)-1, TX_WAIT_FOREVER);
        memset(&msg, NULL, 100);
        sprintf (&msg, "%d. Data-Byte: 0x%s \r\n", i, conf.data[i].hex);
        g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);
        memset(&msg, NULL, 100);

    }

    return conf;
}


can_frame_t * create_can(void) {

    /* open the UART-Module */
    g_sf_comms0.p_api->open (g_sf_comms0.p_ctrl, g_sf_comms0.p_cfg);

    can_conf conf = read_uart();

    /* Convert the Data from can_conf into can_frame_t */
    uint32_t id;
    id = strtol(conf.id, NULL, 16);
    transmitFrame.id = id;

    uint8_t dlc;
    dlc = strtol(conf.data_length_code, NULL, 10);
    transmitFrame.data_length_code = dlc;

    int i;
    for (i = 0; i < dlc; i++) {
        uint8_t data;
        data = strtol(conf.data[i].hex, NULL, 16);
        transmitFrame.data[i] = data;
    }

    transmitFrame.type = conf.type;

    /*write message for user that CAN-message has been transmitted */
    const uint8_t msg[32];
    sprintf (&msg, "CAN-Frame is been written! \r\n");
    g_sf_comms0.p_api->write (g_sf_comms0.p_ctrl, msg, sizeof(msg), TX_16_ULONG);


    return &transmitFrame;
}




