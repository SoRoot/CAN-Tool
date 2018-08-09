#include "can_transmitter.h"

/* Method from the UART-Module */
extern can_frame_t * create_can(void);

void can_transmitter_entry(void);

#define TRANSMIT_MESSAGE 0x1

#define FLAG_CAN_EVENT_TX_COMPLETE (1u << 1)
#define FLAG_CAN_EVENT_ERR_BUS_OFF (1u << 2)
#define FLAG_CAN_EVENT_BUS_RECOVERY (1u << 3)
#define FLAG_CAN_EVENT_ERR_PASSIVE (1u << 4)
#define FLAG_CAN_EVENT_MAILBOX_OVERWRITE_OVERRUN (1u << 5)
#define FLAG_CAN_UNEXPECTED_EVENT (1u << 6)

/* CAN master entry function */
void can_transmitter_entry(void)
{
    ULONG currentFlags = 0;
    ULONG err = 0;
    ssp_err_t status;

    can_frame_t * transmitFrame = create_can();

    status = g_can0.p_api->open(g_can0.p_ctrl, g_can0.p_cfg);
    if(status != SSP_SUCCESS) while (1);

    while (1)
    {
        /* set the flag for transmitting message */
        tx_event_flags_set(&g_start_transmit_message,TRANSMIT_MESSAGE, TX_OR);

        /* pend on the flag forever and clear after received */
        err = tx_event_flags_get(&g_start_transmit_message, TRANSMIT_MESSAGE, TX_OR_CLEAR, &currentFlags, TX_WAIT_FOREVER);
        if(err != TX_SUCCESS) while (1);

        /* send message every 100ms */
        tx_thread_sleep (10);

        /* transmit message with the configured msg (tranmitFrame) */
        status = g_can0.p_api->write(g_can0.p_ctrl, 0, transmitFrame);
        if(status != SSP_SUCCESS) while (1);

        /* wait for transmission to be completed */
        err = tx_event_flags_get(&g_can_status, FLAG_CAN_EVENT_TX_COMPLETE, TX_OR_CLEAR, &currentFlags, TX_WAIT_FOREVER);
        if(err != TX_SUCCESS) while (1);

    }
}

volatile bool unexpectedEvent = false;

void can_transmitter_callback(can_callback_args_t * p_args) {


    if(p_args->channel != 0) {
        while(1);
    }

    switch(p_args->event) {

        case CAN_EVENT_TX_COMPLETE:
            tx_event_flags_set(&g_can_status,FLAG_CAN_EVENT_TX_COMPLETE, TX_OR);
            break;

        case CAN_EVENT_ERR_BUS_OFF:
            tx_event_flags_set(&g_can_status,FLAG_CAN_EVENT_ERR_BUS_OFF, TX_OR);
            break;

        case CAN_EVENT_BUS_RECOVERY:
            tx_event_flags_set(&g_can_status,FLAG_CAN_EVENT_BUS_RECOVERY, TX_OR);
            break;

        case CAN_EVENT_ERR_PASSIVE:
            tx_event_flags_set(&g_can_status,FLAG_CAN_EVENT_ERR_PASSIVE, TX_OR);
            break;

        case CAN_EVENT_MAILBOX_OVERWRITE_OVERRUN:
            tx_event_flags_set(&g_can_status,FLAG_CAN_EVENT_MAILBOX_OVERWRITE_OVERRUN, TX_OR);
            break;

        default:
            tx_event_flags_set(&g_can_status,FLAG_CAN_UNEXPECTED_EVENT, TX_OR);
            break;
    };

}

