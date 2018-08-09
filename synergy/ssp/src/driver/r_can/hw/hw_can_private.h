/***********************************************************************************************************************
 * Copyright [2015-2017] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 * 
 * This file is part of Renesas SynergyTM Software Package (SSP)
 *
 * The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
 * and/or its licensors ("Renesas") and subject to statutory and contractual protections.
 *
 * This file is subject to a Renesas SSP license agreement. Unless otherwise agreed in an SSP license agreement with
 * Renesas: 1) you may not use, copy, modify, distribute, display, or perform the contents; 2) you may not use any name
 * or mark of Renesas for advertising or publicity purposes or in connection with your use of the contents; 3) RENESAS
 * MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED
 * "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF
 * CONTRACT OR TORT, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents
 * included in this file may be subject to different terms.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : hw_can_private.h
 * Description  : TODO_CAN
 **********************************************************************************************************************/

#ifndef HW_CAN_PRIVATE_H
#define HW_CAN_PRIVATE_H

/**********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
#include "r_can.h"
#include "../r_can_private_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define CAN_MAX_DATA_LENGTH         (8U)
#define CAN_MAX_NO_MAILBOXES        (32U)
#define CAN_TIME_SEGMENT1_MIN       CAN_TIME_SEGMENT1_TQ4
#define CAN_TIME_SEGMENT1_MAX       CAN_TIME_SEGMENT1_TQ16
#define CAN_TIME_SEGMENT2_MIN       CAN_TIME_SEGMENT2_TQ2
#define CAN_TIME_SEGMENT2_MAX       CAN_TIME_SEGMENT2_TQ8
#define CAN_SYNC_JUMP_WIDTH_MIN     CAN_SYNC_JUMP_WIDTH_TQ1
#define CAN_SYNC_JUMP_WIDTH_MAX     CAN_SYNC_JUMP_WIDTH_TQ4
#define CAN_BAUD_RATE_PRESCALER_MIN (0U)
#define CAN_BAUD_RATE_PRESCALER_MAX (1023U)

#define CAN_SID_MASK                (0x000007FFU)
#define CAN_XID_MASK                (0xE0000000UL)
#define CAN_MAILBOX_RX              (0x40U)
#define CAN_MAILBOX_RX_CLEAR        (0x40U)
#define CAN_ERROR_MASK              (0x80U)
#define CAN_ERROR_DISPLAY_MODE      (0x80U)
#define CAN_ERROR_SEARCH            (2U)
#define CAN_RECEIVE_SEARCH          (0U)
#define CAN_TRANSMIT_SEARCH         (1U)

/* CAN1 Control Register (CTLR) b9, b8 CANM[1:0] CAN Operating Mode Select. */
typedef enum e_can_mode_control
{
    CAN_MODE_CONTROL_NORMAL,             //CAN operation mode
    CAN_MODE_CONTROL_RESET,              //CAN reset mode
    CAN_MODE_CONTROL_HALT,               //CAN halt mode
    CAN_MODE_CONTROL_RESET_FORCED        //CAN reset mode (forced transition)
} can_mode_control_t;
/* Local sleep mode for CAN module */

typedef enum e_can_sleep
{
    CAN_SLEEP_AWAKEN,
    CAN_SLEEP_SLEEP,
} can_sleep_t;

#define    CAN_TEST_NORMAL            (0U)
#define    CAN_TEST_LISTEN_ONLY       (3U)
#define    CAN_TEST_LOOPBACK_EXTERNAL (5U)
#define    CAN_TEST_LOOPBACK_INTERNAL (7U)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
bool HW_CAN_Control (R_CAN0_Type * p_can_regs, can_mode_t mode);

void HW_CAN_BusRecoveryModeSet (R_CAN0_Type * p_can_regs);

void HW_CAN_MailboxModeSet (R_CAN0_Type * p_can_regs);

bool HW_CAN_MailboxIdSet (R_CAN0_Type   *       p_can_regs,
                          uint32_t              count,
                          can_mailbox_t * const p_mailbox,
                          can_id_mode_t         id_mode);

void HW_CAN_MailboxSendIdSet (R_CAN0_Type * p_can_regs,
                              uint32_t      mailbox,
                              can_id_t      id,
                              can_id_mode_t id_mode);

bool HW_CAN_MailboxMaskSet (R_CAN0_Type *    p_can_regs,
                            uint32_t         count,
                            uint32_t * const p_mailbox_mask,
                            can_id_mode_t    id_mode);

bool                       HW_CAN_IdModeSet (R_CAN0_Type * p_can_regs, can_id_mode_t id_mode);

bool                       HW_CAN_MessageModeSet (R_CAN0_Type * p_can_regs, can_message_mode_t message_mode);

void                       HW_CAN_PriorityModeSet (R_CAN0_Type * p_can_regs);

void                       HW_CAN_TimeStampSet (R_CAN0_Type * p_can_regs);

bool                       HW_CAN_TimeStampReset (R_CAN0_Type * p_can_regs);

bool                       HW_CAN_ClockSet (R_CAN0_Type * p_can_regs, can_extended_cfg_t * const p_cfg);

bool                       HW_CAN_BitTimingSet (R_CAN0_Type * p_can_regs, can_bit_timing_cfg_t * const p_timing);

bool                       HW_CAN_BitRateGet (R_CAN0_Type * p_can_regs, uint32_t * const p_frequency);

void                       HW_CAN_MaskEnableDefautSet (R_CAN0_Type * p_can_regs);

void                       HW_CAN_MailboxesClear (R_CAN0_Type * p_can_regs);

can_id_t                   HW_CAN_ReceiveIdGet (R_CAN0_Type * p_can_regs, can_id_mode_t id_mode, uint32_t mailbox);

can_mailbox_send_receive_t HW_CAN_MailboxTypeGet (R_CAN0_Type * p_can_regs, uint32_t mailbox);

bool                       HW_CAN_ReceiveDataAvailable (R_CAN0_Type * p_can_regs, uint32_t mailbox);

bool                       HW_CAN_MessageLostGet (R_CAN0_Type * p_can_regs, uint32_t mailbox);

uint8_t                    HW_CAN_ReceiveDataCountGet (R_CAN0_Type * p_can_regs, uint32_t mailbox);

void                       HW_CAN_ReceiveDataGet (R_CAN0_Type *        p_can_regs,
                                                  uint32_t             mailbox,
                                                  can_id_mode_t        id_mode,
                                                  can_frame_t  * const p_frame);

bool HW_CAN_TransmitDataReady (R_CAN0_Type * p_can_regs, uint32_t mailbox);

void HW_CAN_TransmitDataClear (R_CAN0_Type * p_can_regs, uint32_t mailbox);

void HW_CAN_TransmitFrameSend (R_CAN0_Type *       p_can_regs,
                               uint32_t            mailbox,
                               can_frame_t * const p_frame);

void HW_CAN_ErrorInterruptEnable (R_CAN0_Type * p_can_regs);

void HW_CAN_SendReceiveInterruptsEnable (R_CAN0_Type * p_can_regs);

void HW_CAN_SendReceiveInterruptsDisable (R_CAN0_Type * p_can_regs);

bool HW_CAN_InterruptErrorGet (R_CAN0_Type * p_can_regs, can_interrrupt_status_t * const p_status);

bool HW_CAN_StatusGet (R_CAN0_Type * p_can_regs, can_info_t * const p_info);

void HW_CAN_ErrorMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox);

void HW_CAN_ReceiveMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox);

void HW_CAN_TransmitMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox);

void HW_CAN_ErrorModeSet (R_CAN0_Type * p_can_regs);

bool HW_CAN_ErrorsGet (R_CAN0_Type * p_can_regs);

void HW_CAN_PowerOn (R_CAN0_Type * p_can_regs);

void HW_CAN_PowerOff (R_CAN0_Type * p_can_regs);

/**********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "common/hw_can_common.h"

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* HW_CAN_PRIVATE_H */
