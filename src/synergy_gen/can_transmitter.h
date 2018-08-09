/* generated thread header file - do not edit */
#ifndef CAN_TRANSMITTER_H_
#define CAN_TRANSMITTER_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus 
extern "C" void can_transmitter_entry(void);
#else 
extern void can_transmitter_entry(void);
#endif
#include "r_can.h"
#include "r_can_api.h"
#ifdef __cplusplus
extern "C"
{
#endif
/** CAN on CAN Instance. */
extern const can_instance_t g_can0;
#ifndef can_transmitter_callback
void can_transmitter_callback(can_callback_args_t * p_args);
#endif
#define CAN_NO_OF_MAILBOXES_g_can0 (32)
extern TX_EVENT_FLAGS_GROUP g_start_transmit_message;
extern TX_EVENT_FLAGS_GROUP g_can_status;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CAN_TRANSMITTER_H_ */
