/* generated thread source file - do not edit */
#include "can_transmitter.h"

TX_THREAD can_transmitter;
void can_transmitter_create(void);
static void can_transmitter_func(ULONG thread_input);
/** Alignment requires using pragma for IAR. GCC is done through attribute. */
#if defined(__ICCARM__)
#pragma data_alignment = BSP_STACK_ALIGNMENT
#endif
static uint8_t can_transmitter_stack[1024] BSP_PLACE_IN_SECTION(".stack.can_transmitter") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
can_bit_timing_cfg_t g_can0_bit_timing_cfg =
{ .baud_rate_prescaler = 5, .time_segment_1 = CAN_TIME_SEGMENT1_TQ15, .time_segment_2 = CAN_TIME_SEGMENT2_TQ8,
  .synchronization_jump_width = CAN_SYNC_JUMP_WIDTH_TQ1, };

uint32_t g_can0_mailbox_mask[CAN_NO_OF_MAILBOXES_g_can0 / 4] =
{ 0x1FFFFFFF,
#if CAN_NO_OF_MAILBOXES_g_can0 > 4
        0x1FFFFFFF,
#endif
#if CAN_NO_OF_MAILBOXES_g_can0 > 8
        0x1FFFFFFF,
        0x1FFFFFFF,
#endif
#if CAN_NO_OF_MAILBOXES_g_can0 > 16
        0x1FFFFFFF,
        0x1FFFFFFF,
        0x1FFFFFFF,
        0x1FFFFFFF,
#endif
    };

static const can_extended_cfg_t g_can0_extended_cfg =
{ .clock_source = CAN_CLOCK_SOURCE_CANMCLK, .p_mailbox_mask = g_can0_mailbox_mask, };

can_mailbox_t g_can0_mailbox[CAN_NO_OF_MAILBOXES_g_can0] =
{
{ .mailbox_id = 0, .mailbox_type = CAN_MAILBOX_TRANSMIT, .frame_type = CAN_FRAME_TYPE_REMOTE },
  { .mailbox_id = 1, .mailbox_type = CAN_MAILBOX_RECEIVE, .frame_type = CAN_FRAME_TYPE_DATA },
  { .mailbox_id = 2, .mailbox_type = CAN_MAILBOX_RECEIVE, .frame_type = CAN_FRAME_TYPE_DATA, },
  { .mailbox_id = 3, .mailbox_type = CAN_MAILBOX_RECEIVE, .frame_type = CAN_FRAME_TYPE_DATA },
#if CAN_NO_OF_MAILBOXES_g_can0 > 4
        {
            .mailbox_id = 4,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 5,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 6,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 7,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
#endif
#if CAN_NO_OF_MAILBOXES_g_can0 > 8
        {
            .mailbox_id = 8,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 9,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 10,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 11,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 12,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA,
        },
        {
            .mailbox_id = 13,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 14,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 15,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
#endif  
#if CAN_NO_OF_MAILBOXES_g_can0 > 16
        {
            .mailbox_id = 16,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },

        {
            .mailbox_id = 17,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 18,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 19,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 20,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 21,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 22,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA,
        },
        {
            .mailbox_id = 23,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 24,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 25,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 26,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 27,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 28,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 29,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 30,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        },
        {
            .mailbox_id = 31,
            .mailbox_type = CAN_MAILBOX_RECEIVE,
            .frame_type = CAN_FRAME_TYPE_DATA
        }
#endif  
    };

#if (6) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_can0) && !defined(SSP_SUPPRESS_ISR_CAN0)
SSP_VECTOR_DEFINE_CHAN(can_error_isr, CAN, ERROR, 0);
#endif
#endif
#if (7) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_can0) && !defined(SSP_SUPPRESS_ISR_CAN0)
SSP_VECTOR_DEFINE_CHAN(can_mailbox_rx_isr, CAN, MAILBOX_RX, 0);
#endif
#endif
#if (8) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_can0) && !defined(SSP_SUPPRESS_ISR_CAN0)
SSP_VECTOR_DEFINE_CHAN(can_mailbox_tx_isr, CAN, MAILBOX_TX, 0);
#endif
#endif

static can_instance_ctrl_t g_can0_ctrl;
static const can_cfg_t g_can0_cfg =
        { .channel = 0, .p_bit_timing = &g_can0_bit_timing_cfg, .id_mode = CAN_ID_MODE_STANDARD, .mailbox_count =
                  CAN_NO_OF_MAILBOXES_g_can0,
          .p_mailbox = g_can0_mailbox, .message_mode = CAN_MESSAGE_MODE_OVERWRITE, .p_callback =
                  can_transmitter_callback,
          .p_extend = &g_can0_extended_cfg, .p_context = &g_can0, .error_ipl = (6), .mailbox_tx_ipl = (8),
          .mailbox_rx_ipl = (7), };
/* Instance structure to use this module. */
const can_instance_t g_can0 =
{ .p_ctrl = &g_can0_ctrl, .p_cfg = &g_can0_cfg, .p_api = &g_can_on_can };
TX_EVENT_FLAGS_GROUP g_start_transmit_message;
TX_EVENT_FLAGS_GROUP g_can_status;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;
void g_hal_init(void);

void can_transmitter_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    tx_event_flags_create (&g_start_transmit_message, (CHAR *) "Transmit Flag");
    tx_event_flags_create (&g_can_status, (CHAR *) "CAN Status");

    tx_thread_create (&can_transmitter, (CHAR *) "Can Transmitter", can_transmitter_func, (ULONG) NULL,
                      &can_transmitter_stack, 1024, 1, 1, 1, TX_AUTO_START);
}

static void can_transmitter_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* First thread will take care of common initialization. */
    UINT err;
    err = tx_semaphore_get (&g_ssp_common_initialized_semaphore, TX_WAIT_FOREVER);

    while (TX_SUCCESS != err)
    {
        /* Check err, problem occurred. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR (0);
    }

    /* Only perform common initialization if this is the first thread to execute. */
    if (false == g_ssp_common_initialized)
    {
        /* Later threads will not run this code. */
        g_ssp_common_initialized = true;

        /* Perform common module initialization. */
        g_hal_init ();

        /* Now that common initialization is done, let other threads through. */
        /* First decrement by 1 since 1 thread has already come through. */
        g_ssp_common_thread_count--;
        while (g_ssp_common_thread_count > 0)
        {
            err = tx_semaphore_put (&g_ssp_common_initialized_semaphore);

            while (TX_SUCCESS != err)
            {
                /* Check err, problem occurred. */
                BSP_CFG_HANDLE_UNRECOVERABLE_ERROR (0);
            }

            g_ssp_common_thread_count--;
        }
    }

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    can_transmitter_entry ();
}
