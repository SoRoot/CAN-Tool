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
 * File Name    : hw_can.c
 * Description  : Hardware related  LLD functions for the CAN HAL
 **********************************************************************************************************************/

#include "r_can_api.h"
#include "hw_can_private.h"
#include "bsp_api.h"
#include "r_can_cfg.h"
#include "r_cgc_api.h"
#include "r_cgc.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define OPERATE_TIMEOUT (5000U)      // 5000 microseconds
#define HALT_TIMEOUT    (5000U)      // 5000 microseconds
#define SLEEP_TIMEOUT   (5000U)      // 5000 microseconds
#define RESET_TIMEOUT   (5000U)      // 5000 microseconds

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static bool HW_CAN_Halt_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Operate_Mode (R_CAN0_Type * p_can_regs);

static void timeout_set (volatile uint32_t * const p_timer, uint32_t timer_value);

static bool timeout_get (volatile uint32_t * const p_timer);

static bool HW_CAN_Sleep_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Exit_Sleep_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Reset_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Listen_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Loopback_Internal_Mode (R_CAN0_Type * p_can_regs);

static bool HW_CAN_Loopback_External_Mode (R_CAN0_Type * p_can_regs);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to halt mode.
 * @param[in]  channel
 * @retval     false if timed out, true if no timeout
 **********************************************************************************************************************/

static bool HW_CAN_Halt_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
    uint32_t timer;

    p_can_regs->CTLR_b.CANM = CAN_MODE_CONTROL_HALT;      // Switch to HALT mode.
    timeout_set(&timer, HALT_TIMEOUT);                          // Set timer for timeout.
    while ((!p_can_regs->STR_b.HLTST) && (timer != (uint32_t) 0x00) )                      // Wait for mode to switch.
    {
        if (timeout_get(&timer))                                // Check timer.
        {
            ret_val = false;                                    // Switch to HALT mode timed out.
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to normal operation mode.
 * @param[in]  channel
 * @retval     false if timed out, true if no timeout
 **********************************************************************************************************************/
static bool HW_CAN_Operate_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
    uint32_t timer;

    p_can_regs->CTLR_b.CANM = CAN_MODE_CONTROL_NORMAL;    // Switch to Normal operate mode.
    timeout_set(&timer, OPERATE_TIMEOUT);                       // Set timer for timeout.
    while ((p_can_regs->STR_b.RSTST) && (timer != (uint32_t) 0x00))                       // Wait for mode to switch.
    {
        if (timeout_get(&timer))                                // Check timer.
        {
            ret_val = false;                                    // Switch to Normal operate mode timed out.
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to sleep mode.
 * @param[in]  p_can_regs
 * @retval     false if timed out, true if no timeout
 **********************************************************************************************************************/
static bool HW_CAN_Sleep_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
    uint32_t timer;

            ret_val = HW_CAN_Halt_Mode(p_can_regs);
            if (true == ret_val)
            {
                p_can_regs->CTLR_b.SLPM = CAN_SLEEP_SLEEP;
                timeout_set(&timer, SLEEP_TIMEOUT);                 // Set timer for timeout.
                while ((!p_can_regs->STR_b.SLPST) && (timer != (uint32_t) 0x00))
                {
                    if (timeout_get(&timer))                        // Check timer.
                    {
                        ret_val = false;                            // Mode switch timed out.
                    }
                }
            }
    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to exit sleep mode.
 * @param[in]  p_can_regs
 * @retval     false if timed out, true if no timeout
**********************************************************************************************************************/
static bool HW_CAN_Exit_Sleep_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
    uint32_t timer;

            p_can_regs->CTLR_b.SLPM = CAN_SLEEP_AWAKEN;
            ret_val                       = HW_CAN_Operate_Mode(p_can_regs);
            if (true == ret_val)
            {
                timeout_set(&timer, SLEEP_TIMEOUT);                 // Set timer for timeout.
                while ((p_can_regs->STR_b.SLPST) && (timer != (uint32_t) 0x00))
                {
                    if (timeout_get(&timer))                        // Check timer.
                    {
                        ret_val = false;                            // Mode switch timed out.
                    }
                }
            }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to reset.
 * @param[in]  p_can_regs
 * @retval     false if timed out, true if no timeout
**********************************************************************************************************************/
static bool HW_CAN_Reset_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
    uint32_t timer;

            p_can_regs->CTLR_b.CANM = CAN_MODE_CONTROL_RESET;
            timeout_set(&timer, RESET_TIMEOUT);                     // Set timer for timeout.
            while ((!p_can_regs->STR_b.RSTST) && (timer != (uint32_t) 0x00))
            {
                if (timeout_get(&timer))                            // Check timer.
                {
                    ret_val = false;                                // Mode switch timed out.
                }
            }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to Listen mode.
 * @param[in]  p_can_regs
 * @retval     false if failed, true if succeeded
**********************************************************************************************************************/
static bool HW_CAN_Listen_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;
            ret_val = HW_CAN_Halt_Mode(p_can_regs);
            if (true == ret_val)
            {
                p_can_regs->TCR = CAN_TEST_LISTEN_ONLY;
                ret_val               = HW_CAN_Operate_Mode(p_can_regs);
            }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to Internal Loopback mode.
 * @param[in]  p_can_regs
 * @retval     false if failed, true if succeeded
**********************************************************************************************************************/
static bool HW_CAN_Loopback_Internal_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;

            ret_val = HW_CAN_Halt_Mode(p_can_regs);
            if (true == ret_val)
            {
                p_can_regs->TCR = CAN_TEST_LOOPBACK_INTERNAL;
                ret_val               = HW_CAN_Operate_Mode(p_can_regs);
            }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function causes the CAN peripheral to go to External Loopback mode.
 * @param[in]  p_can_regs
 * @retval     false if failed, true if succeeded
**********************************************************************************************************************/

static bool HW_CAN_Loopback_External_Mode (R_CAN0_Type * p_can_regs)
{
    bool     ret_val = true;

            ret_val = HW_CAN_Halt_Mode(p_can_regs);
            if (true == ret_val)
            {
                p_can_regs->TCR = CAN_TEST_LOOPBACK_EXTERNAL;
                ret_val               = HW_CAN_Operate_Mode(p_can_regs);
            }

    return ret_val;
}


/*******************************************************************************************************************//**
 * @brief      This function is used to switch the CAN peripheral operation and test modes.
 * @param[in]  channel
 * @param[in]  mode    - operation mode
 * @retval     false if timed out, true if no timeout
 **********************************************************************************************************************/

bool HW_CAN_Control (R_CAN0_Type * p_can_regs, can_mode_t mode)
{
    bool     ret_val;

    ret_val = true;
    switch (mode)
    {
        case CAN_MODE_NORMAL:                                       // Switch to Normal mode.
            p_can_regs->TCR = CAN_TEST_NORMAL;
            ret_val               = HW_CAN_Operate_Mode(p_can_regs);
            break;

        case CAN_MODE_HALT:                                         // Switch to HALT mode.
            ret_val = HW_CAN_Halt_Mode(p_can_regs);
            break;

        case CAN_MODE_SLEEP:                                        // Switch to SLEEP mode.
            ret_val = HW_CAN_Sleep_Mode (p_can_regs);
            break;

        case CAN_MODE_EXIT_SLEEP:                                   // Exit SLEEP mode.
            ret_val = HW_CAN_Exit_Sleep_Mode (p_can_regs);
            break;

        case CAN_MODE_RESET:                                        // Switch to RESET mode.
            ret_val = HW_CAN_Reset_Mode (p_can_regs);
            break;

        case CAN_MODE_LISTEN:                                       // Switch to LISTEN ONLY mode.
            ret_val = HW_CAN_Listen_Mode (p_can_regs);
            break;

        case CAN_MODE_LOOPBACK_INTERNAL:                            // Switch to INTERNAL LOOPBACK mode.
            ret_val = HW_CAN_Loopback_Internal_Mode (p_can_regs);
            break;

        case CAN_MODE_LOOPBACK_EXTERNAL:                            // Switch to EXTERNAL LOOPBACK mode.
            ret_val = HW_CAN_Loopback_External_Mode (p_can_regs);
            break;

        default:
            ret_val = false;                    // Invalid mode selected.
            break;
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the bus off recovery mode to CAN spec.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_BusRecoveryModeSet (R_CAN0_Type * p_can_regs)
{
    /* BOM:    Bus Off recovery mode acc. to IEC11898-1 */
    p_can_regs->CTLR_b.BOM = 0U;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the mode for all mailboxes to normal mailbox (not FIFO mode).
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_MailboxModeSet (R_CAN0_Type * p_can_regs)
{
    /* MBM: Select normal mailbox mode. */
    p_can_regs->CTLR_b.MBM = 0U;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the id for all receive mailboxes.
 * @param[in]  channel
 * @param[in]  count        - mailbox count
 * @param[in]  p_mailbox    - list of mailboxes
 * @param[in]  id_mode      - standard or extended id mode
 * @retval     false if invalid mailbox type, true if no errors.
 **********************************************************************************************************************/
bool HW_CAN_MailboxIdSet (R_CAN0_Type * p_can_regs, uint32_t count, can_mailbox_t * const p_mailbox, can_id_mode_t id_mode)
{
    bool     ret_val;
    uint32_t i;

    ret_val = true;
    if (count > CAN_MAX_NO_MAILBOXES)
    {
        ret_val = false;
    }
    else
    {
        /* Set mailbox ids for all receive mailboxes. */
        for (i = 0U; i < CAN_MAX_NO_MAILBOXES; i++)
        {
            if (CAN_MAILBOX_RECEIVE == (p_mailbox[i].mailbox_type))
            {
                p_can_regs->MCTLn_RX[i] = 0x00U;                               // Clear RX control register
                if (CAN_ID_MODE_STANDARD == id_mode)
                {
                    p_can_regs->MBn[i].MBn_ID_b.SID = (p_mailbox[i].mailbox_id & CAN_SID_MASK);
                }
                else
                {
                    p_can_regs->MBn[i].MBn_ID = (p_mailbox[i].mailbox_id & (~CAN_XID_MASK));
                }

                /* Always set IDE bit to 0, regardless of SID or XID (Not Mixed Mode). */
                p_can_regs->MBn[i].MBn_ID_b.IDE = 0U;
                p_can_regs->MBn[i].MBn_ID_b.RTR = p_mailbox[i].frame_type;    // Set receive mailbox for either
                                                                                    // Data or Remote frame type.
                p_can_regs->MCTLn_RX[i]         = CAN_MAILBOX_RX;             // Clear NEWDATA, Mailbox configured
                                                                                    // for receive
            }
            else if (CAN_MAILBOX_TRANSMIT == (p_mailbox[i].mailbox_type))
            {
                p_can_regs->MCTLn_TX[i] = 0x00U;                               // Clear NEWDATA, Mailbox
                                                                                    // not configured for receive
            }
            else
            {
                ret_val = false;
            }
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the ID for the selected transmit mailbox.
 * @param[in]  channel
 * @param[in]  mailbox      - mailbox number
 * @param[in]  id           - ID to send to
 * @param[in]  id_mode      - standard or extended id mode
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_MailboxSendIdSet (R_CAN0_Type * p_can_regs, uint32_t mailbox, can_id_t id, can_id_mode_t id_mode)
{
    if (CAN_ID_MODE_STANDARD == id_mode)
    {
        p_can_regs->MBn[mailbox].MBn_ID_b.SID = (id & CAN_SID_MASK);  // Set standard id.
    }
    else
    {
        p_can_regs->MBn[mailbox].MBn_ID = (id & (~CAN_XID_MASK));     // Set extended id.
    }
}

/*******************************************************************************************************************//**
 * @brief      This function sets the mask for all receive mailboxes.
 * @param[in]  channel
 * @param[in]  id_mode      - standard or extended id mode
 * @retval     false if invalid mailbox count, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_MailboxMaskSet (R_CAN0_Type * p_can_regs, uint32_t count, uint32_t * const p_mailbox_mask, can_id_mode_t id_mode)
{
    bool     ret_val;
    uint32_t i;
    uint32_t mask;
    uint32_t group_mask;
    uint32_t invalid_mask;

    ret_val = true;
    if (count > CAN_MAX_NO_MAILBOXES)                   // Return error if number is out of range.
    {
        ret_val = false;
    }
    else
    {
        group_mask   = 0xfffffff0UL;                      // Initialize masks.
        invalid_mask = 0xffffffffUL;
        /* Set mask for each group of 8 mailboxes. */
        for (i = 0U; i < (CAN_MAX_NO_MAILBOXES / 4); i++)
        {
            mask = p_mailbox_mask[i];                       // Read user configured mask for group.
            if (mask < 0x1fffffffUL)
            {
                if (CAN_ID_MODE_STANDARD == id_mode)
                {
                    p_can_regs->MKRn_b[i].SID = (mask & CAN_SID_MASK);    // Set standard ID mask.
                }
                else
                {
                    p_can_regs->MKRn[i] = (mask & (~CAN_XID_MASK));       // Set extended ID mask.
                }

                invalid_mask &= group_mask;
            }

            group_mask <<= 4;                               // Shift group mask for next group.
        }

        if (invalid_mask != 0xffffffffUL)
        {
            p_can_regs->MKIVLR = invalid_mask;        // enable mask for all mailboxes in group of 4
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the id mode for all mailboxes.
 * @param[in]  channel
 * @param[in]  id_mode      - standard or extended id mode
 * @retval     false if invalid id mode, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_IdModeSet (R_CAN0_Type * p_can_regs, can_id_mode_t id_mode)
{
    bool ret_val;
    ret_val = true;
    /* IDFM: Select Frame ID mode. */
    if ((CAN_ID_MODE_STANDARD != id_mode) && (CAN_ID_MODE_EXTENDED != id_mode))
    {
        ret_val = false;                            // Return error if invalid mode.
    }
    else
    {
        p_can_regs->CTLR_b.IDFM = id_mode;    // Set ID mode.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the mailbox message mode to overrun or overwrite mode.
 * @param[in]  channel
 * @param[in]  message_mode      - overrun or overwrite mode
 * @retval     false if invalid message mode, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_MessageModeSet (R_CAN0_Type * p_can_regs, can_message_mode_t message_mode)
{
    bool ret_val;
    ret_val = true;

    /*  0 = Overwrite mode: Latest message overwrites old.
     *  1 = Overrun mode: Latest message discarded. */
    if ((CAN_MESSAGE_MODE_OVERWRITE != message_mode) && (CAN_MESSAGE_MODE_OVERRUN != message_mode))
    {
        ret_val = false;                                // Return error if invalid mode.
    }
    else
    {
        p_can_regs->CTLR_b.MLM = message_mode;    // Set message mode.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the priority to ID, the CAN standard.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_PriorityModeSet (R_CAN0_Type * p_can_regs)
{
    /* TPM: ID priority mode. */
    p_can_regs->CTLR_b.TPM = 0U;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the timestamp update period.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_TimeStampSet (R_CAN0_Type * p_can_regs)
{
    /* TSRC: Only to be set to 1 in operation mode */
    p_can_regs->CTLR_b.TSRC = 0U;
    /* TSPS: Update every 8 bit times */
    p_can_regs->CTLR_b.TSPS = 3U;
}

/*******************************************************************************************************************//**
 * @brief      This function resets the timestamp.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
bool HW_CAN_TimeStampReset (R_CAN0_Type * p_can_regs)
{
    bool     ret_val;
    uint32_t timer;

    ret_val                       = true;
    p_can_regs->CTLR_b.TSRC = 1U;
    timeout_set(&timer, RESET_TIMEOUT);                     // Set timer for timeout.
    while ((p_can_regs->CTLR_b.TSRC) && (timer != (uint32_t) 0x00))
    {
        if (timeout_get(&timer))                            // Check timer.
        {
            ret_val = false;                                // Reset timed out.
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the CAN source clock.
 * @param[in]  channel
 * @retval     false if cfg values out of range, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_ClockSet (R_CAN0_Type * p_can_regs, can_extended_cfg_t * const p_cfg)
{
    bool ret_val;
    ret_val = true;
    const cgc_api_t * pcgc = &g_cgc_on_cgc;
    ssp_err_t err;

    /* Set clock source. */
    if (p_cfg->clock_source > CAN_CLOCK_SOURCE_CANMCLK)
    {
        ret_val = false;                // Return error if invalid source.
    }

    bsp_feature_can_t can_feature = {0U};
    R_BSP_FeatureCanGet(&can_feature);
    if (can_feature.mclock_only)
    {
        p_cfg->clock_source = CAN_CLOCK_SOURCE_CANMCLK;
    }

    if (true == ret_val)
    {
        if (CAN_CLOCK_SOURCE_CANMCLK == p_cfg->clock_source)
        {
            err = pcgc->clockCheck(CGC_CLOCK_MAIN_OSC);
            if ( SSP_ERR_CLOCK_INACTIVE == err )
            {
                ret_val = false;                // Return error if invalid source.
            }
        }
        if (true == ret_val)
        {
            p_can_regs->BCR_b.CCLKS = p_cfg->clock_source;
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets CAN bit timing.
 * @param[in]  channel
 * @retval     false if cfg values out of range, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_BitTimingSet (R_CAN0_Type * p_can_regs, can_bit_timing_cfg_t * const p_timing)
{
    bool ret_val;
    ret_val = true;

    /* Set baud Rate Prescaler. */
    if (p_timing->baud_rate_prescaler > CAN_BAUD_RATE_PRESCALER_MAX)
    {
        ret_val = false;                // Return error if invalid prescaler.
    }

    /* Set Time Segment 1. */
    if ((p_timing->time_segment_1 < CAN_TIME_SEGMENT1_MIN) || (p_timing->time_segment_1 > CAN_TIME_SEGMENT1_MAX))
    {
        ret_val = false;                // Return error if invalid Time segment.
    }

    /* Set Time Segment 2. */
    if ((p_timing->time_segment_2 < CAN_TIME_SEGMENT2_MIN) || (p_timing->time_segment_2 > CAN_TIME_SEGMENT2_MAX))
    {
        ret_val = false;                // Return error if invalid Time segment.
    }

    /* Set Synchronization Jump Width. */
    if ((p_timing->synchronization_jump_width < CAN_SYNC_JUMP_WIDTH_MIN) ||
        (p_timing->synchronization_jump_width > CAN_SYNC_JUMP_WIDTH_MAX))
    {
        ret_val = false;                // Return error if invalid width.
    }

    /* Commit configuration to hardware */
    if (true == ret_val)
    {
        p_can_regs->BCR_b.BRP   = ((p_timing->baud_rate_prescaler - 1U) & 0x3fU);
        p_can_regs->BCR_b.TSEG1 = p_timing->time_segment_1;
        p_can_regs->BCR_b.TSEG2 = p_timing->time_segment_2;
        p_can_regs->BCR_b.SJW   = p_timing->synchronization_jump_width;
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the bit rate.
 * @param[in]  channel
 * @retval     false if errors, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_BitRateGet (R_CAN0_Type * p_can_regs, uint32_t * const p_frequency)
{
    ssp_err_t       err;

    uint32_t        clock_frequency = {0U};
    uint32_t        time_segment;
    bool            ret_val;
#ifndef CAN_MCLOCK_ONLY
    const cgc_api_t * pcgc = &g_cgc_on_cgc;
#endif
    ret_val = true;
    err     = SSP_SUCCESS;

#ifndef CAN_MCLOCK_ONLY
    if (CAN_CLOCK_SOURCE_PCLKB == p_can_regs->BCR_b.CCLKS)
    {
        err = pcgc->systemClockFreqGet(CGC_SYSTEM_CLOCKS_PCLKB, &clock_frequency);
    }
    else        // Clock source is Main Oscillator (S7 and S3 only).
    {
        clock_frequency = BSP_CFG_XTAL_HZ;
    }

#else /* ifndef CAN_MCLOCK_ONLY */
    clock_frequency = BSP_CFG_XTAL_HZ;
#endif /* ifndef CAN_MCLOCK_ONLY */
    if (SSP_SUCCESS == err)
    {
        time_segment  = (p_can_regs->BCR_b.TSEG1 + 1U);
        time_segment += (p_can_regs->BCR_b.TSEG2 + 1U);
        time_segment += 1U;
        *p_frequency  = (clock_frequency / (p_can_regs->BCR_b.BRP + 1U)) / time_segment;
    }
    else
    {
        ret_val = false;            // Return error if frequency value is invalid.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the mask enable default (off) for all receive mailboxes.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_MaskEnableDefautSet (R_CAN0_Type * p_can_regs)
{
    /* Mask invalid for all mailboxes by default. */
    p_can_regs->MKIVLR = 0x1FFFFFFFU;
}

/*******************************************************************************************************************//**
 * @brief      This function clears all mailboxes.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_MailboxesClear (R_CAN0_Type * p_can_regs)
{
    uint32_t i;
    uint32_t j;
    for (i = 0U; i < CAN_MAX_NO_MAILBOXES; i++)
    {
        p_can_regs->MBn[i].MBn_ID       = 0x00U;       // Clear ID.
        p_can_regs->MBn[i].MBn_DL_b.DLC = 0x0000U;     // Clear data length code.
        for (j = 0U; j < CAN_MAX_DATA_LENGTH; j++)
        {
            p_can_regs->MBn[i].MBn_D[j] = 0x00U;       // Clear data.
        }

        for (j = 0U; j < 2U; j++)
        {
            p_can_regs->MBn[i].MBn_TS = 0x0000U;       // Clear timestamp.
        }
    }
}

/*******************************************************************************************************************//**
 * @brief      This function returns the received id from the designated mailbox.
 * @param[in]  channel
 * @param[in]  id_mode      - standard or extended id mode
 * @param[in]  mailbox      - mailbox number
 * @retval     ID value of message
 **********************************************************************************************************************/
can_id_t HW_CAN_ReceiveIdGet (R_CAN0_Type * p_can_regs, can_id_mode_t id_mode, uint32_t mailbox)
{
    can_id_t id;

    if (CAN_ID_MODE_STANDARD == id_mode)                    // Read standard id or extended id?
    {
        id = p_can_regs->MBn[mailbox].MBn_ID_b.SID;   // Read standard id.
    }
    else
    {
        id = p_can_regs->MBn[mailbox].MBn_ID;         // Read extended id.
    }

    return id;
}

/*******************************************************************************************************************//**
 * @brief      This function gets the type (send or receive) for the selected mailbox.
 * @param[in]  channel
 * @param[in]  mailbox      - mailbox number
 * @retval     CAN_MAILBOX_RECEIVE if receive mailbox, CAN_MAILBOX_TRANSMIT if transmit mailbox.
 **********************************************************************************************************************/
can_mailbox_send_receive_t HW_CAN_MailboxTypeGet (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    can_mailbox_send_receive_t ret_val;
    ret_val = CAN_MAILBOX_RECEIVE;

    /* Check for setup as a receive mailbox. */
    if (CAN_MAILBOX_RX != (p_can_regs->MCTLn_TX[mailbox] & CAN_MAILBOX_RX))
    {
        ret_val = CAN_MAILBOX_TRANSMIT;     // Mailbox is a transmit mailbox.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function reports whether receive data is available or not.
 * @param[in]  channel
 * @param[in]  mailbox      - mailbox number
 * @retval     false if no data, true if data available
 **********************************************************************************************************************/
bool HW_CAN_ReceiveDataAvailable (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    bool ret_val;
    ret_val = false;
    if ((p_can_regs->MCTLn_RX_b[mailbox].NEWDATA) && (!(p_can_regs->MCTLn_RX_b[mailbox].INVALDATA)))
    {
        ret_val = true;             // Return affirmative value when data is available.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function reports whether receive message is lost or not.
 * @param[in]  channel
 * @param[in]  mailbox      - mailbox number
 * @retval     false if no loss, true if data lost
 **********************************************************************************************************************/
bool HW_CAN_MessageLostGet (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    bool ret_val;
    ret_val = false;
    if (p_can_regs->MCTLn_RX_b[mailbox].MSGLOST)
    {
        ret_val = true;             // Return affirmative value when message is lost.
    }

    p_can_regs->MCTLn_RX[mailbox] = CAN_MAILBOX_RX_CLEAR;       // Clear NEWDATA and MSGLOST
    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the receive data count.
 * @param[in]  channel
 * @retval     data length
 **********************************************************************************************************************/
uint8_t HW_CAN_ReceiveDataCountGet (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    return (uint8_t) (p_can_regs->MBn[mailbox].MBn_DL_b.DLC & 0xFU);     // Return Data Length Count
}

/*******************************************************************************************************************//**
 * @brief      This function reads the receive mailbox data to the supplied buffer pointer.
 * @param[in]  channel
 * @param[in]  mailbox      - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_ReceiveDataGet (R_CAN0_Type * p_can_regs, uint32_t mailbox, can_id_mode_t id_mode, can_frame_t  * const p_frame)
{
    uint32_t i;

    p_frame->type             = (can_frame_type_t) p_can_regs->MBn[mailbox].MBn_ID_b.RTR;
    p_frame->id               = HW_CAN_ReceiveIdGet(p_can_regs, id_mode, mailbox);
    p_frame->data_length_code = p_can_regs->MBn[mailbox].MBn_DL_b.DLC;

    /* Be sure to check data_length_code in calling function */
    for (i = 0U; i < p_frame->data_length_code; i++)
    {
        p_frame->data[i] = p_can_regs->MBn[mailbox].MBn_D[i];        // Copy receive data to buffer.
    }

    /* Clear rx data flag */
    p_can_regs->MCTLn_RX[mailbox] = CAN_MAILBOX_RX_CLEAR;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the transmit data ready flag.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     false if transmit not ready, true if transmit ready.
 **********************************************************************************************************************/
bool HW_CAN_TransmitDataReady (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    bool ret_val;
    ret_val = true;

    if (1U == p_can_regs->MCTLn_TX_b[mailbox].TRMREQ)          // Check for send enabled.
    {
        if (0U == p_can_regs->MCTLn_TX_b[mailbox].SENTDATA)    // Check for data send complete.
        {
            ret_val = false;
        }
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function clears the transmit ready flag.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_TransmitDataClear (R_CAN0_Type * p_can_regs, uint32_t mailbox)
{
    /* Clear SENTDATA.
     * Do a byte-write to avoid read-modify-write with HW writing another bit in between.
     * TRMREQ must be set to 0 (or will send again).
     * Do it twice since "Bits SENTDATA and TRMREQ cannot be set to 0 simultaneously." */
    p_can_regs->MCTLn_TX[mailbox] = 0U;
    p_can_regs->MCTLn_TX[mailbox] = 0U;
}

/*******************************************************************************************************************//**
 * @brief      This function sends a mailbox frame.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_TransmitFrameSend (R_CAN0_Type * p_can_regs,
                               uint32_t             mailbox,
                               can_frame_t  * const p_frame)
{
    uint32_t i;

    /* Put mailbox data length code */
    p_can_regs->MBn[mailbox].MBn_DL_b.DLC = (p_frame->data_length_code & 0x0fU);

    /* Put mailbox data. */
    for (i = 0U; i < p_frame->data_length_code; i++)
    {
        p_can_regs->MBn[mailbox].MBn_D[i] = p_frame->data[i];
    }

    /* Set frame type to Data or Remote frame type. */
    p_can_regs->MBn[mailbox].MBn_ID_b.RTR = p_frame->type;

    /* Clear SentData flag since we are about to send anew.
     * Do a byte-write to avoid read-modify-write with HW writing another bit in between.
     * TRMREQ/RECREQ already 0 after can_wait_tx_rx(). (No need to write twice).
     */
    p_can_regs->MCTLn_TX[mailbox]          = 0U;
    p_can_regs->MCTLn_TX_b[mailbox].TRMREQ = 1U;               // Send data
}

/*******************************************************************************************************************//**
 * @brief      This function finds the lowest numbered mailbox with a message lost.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_ErrorMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox)
{
    p_can_regs->MSMR = CAN_ERROR_SEARCH;               // search for lowest numbered mailbox with message lost
    *mailbox               = p_can_regs->MSSR_b.MBNST; // get mailbox number
}

/*******************************************************************************************************************//**
 * @brief      This function finds the lowest numbered mailbox with a message received.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_ReceiveMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox)
{
    p_can_regs->MSMR = CAN_RECEIVE_SEARCH;             // search for lowest numbered mailbox with message received
    *mailbox               = p_can_regs->MSSR_b.MBNST; // get mailbox number
}

/*******************************************************************************************************************//**
 * @brief      This function finds the lowest numbered mailbox with a message transmitted.
 * @param[in]  channel
 * @param[in]  mailbox - mailbox number
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_TransmitMailboxGet (R_CAN0_Type * p_can_regs, uint32_t * mailbox)
{
    p_can_regs->MSMR = CAN_TRANSMIT_SEARCH;            // search for lowest numbered mailbox with message
                                                             // transmitted
    *mailbox               = p_can_regs->MSSR_b.MBNST; // get mailbox number
}

/*******************************************************************************************************************//**
 * @brief      This function enables the interrupts.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_ErrorInterruptEnable (R_CAN0_Type * p_can_regs)
{
    p_can_regs->EIER_b.EPIE  = 1U;         // Error-Passive interrupt enabled
    p_can_regs->EIER_b.BOEIE = 1U;         // Bus-Off Entry interrupt enabled
    p_can_regs->EIER_b.BORIE = 1U;         // Bus-Off Recovery interrupt enabled
    p_can_regs->EIER_b.ORIE  = 1U;         // Overrun interrupt enabled
}

/*******************************************************************************************************************//**
 * @brief      This function enables interrupts for all mailboxes, transmit and receive.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_SendReceiveInterruptsEnable (R_CAN0_Type * p_can_regs)
{
    p_can_regs->MIER = 0xffffffffUL;      // Enable interrupts for all mailboxes, transmit and receive.
}

/*******************************************************************************************************************//**
 * @brief      This function disables interrupts for all mailboxes, transmit and receive.
 * @param[in]  channel
 * @retval     none
 **********************************************************************************************************************/
void HW_CAN_SendReceiveInterruptsDisable (R_CAN0_Type * p_can_regs)
{
    p_can_regs->MIER = 0x0U;             // Disable interrupts for all mailboxes, transmit and receive.
}

/*******************************************************************************************************************//**
 * @brief      This function returns the errors from the error interrupt.
 * @param[in]  channel
 * @retval     false if no errors, true if errors
 **********************************************************************************************************************/
bool HW_CAN_InterruptErrorGet (R_CAN0_Type * p_can_regs, can_interrrupt_status_t * const p_status)
{
    uint8_t read_val;
    bool    ret_val;

    ret_val  = false;
    read_val = p_can_regs->EIFR;                  // Read Error Interrupt Factor Judge register.
    /* Set status if errors occurred. */
    if (0U != read_val)
    {
        p_status->status       = (uint32_t) read_val;
        p_can_regs->EIFR = 0x00U;                  // Clear Error Interrupt Factor Judge register.
        ret_val                = true;                  // True return value indicates errors are valid.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the status of the CAN channel.
 * @param[in]  channel
 * @retval     false if errors, true if no errors
 **********************************************************************************************************************/
bool HW_CAN_StatusGet (R_CAN0_Type * p_can_regs, can_info_t * const p_info)
{
    uint32_t bit_rate;
    bool     ret_val;
    uint8_t  clear_val;
    ret_val  = true;
    bit_rate = 0U;

    /* Report Status register data. */
    p_info->status.status = (uint32_t) p_can_regs->STR;

    /* Report test modes. */
    p_info->status.status_b.listen_only       = (p_can_regs->TCR == CAN_TEST_LISTEN_ONLY) ?  1U : 0U;
    p_info->status.status_b.internal_loopback = (p_can_regs->TCR == CAN_TEST_LOOPBACK_INTERNAL) ?  1U : 0U;
    p_info->status.status_b.external_loopback = (p_can_regs->TCR == CAN_TEST_LOOPBACK_EXTERNAL) ?  1U : 0U;

    p_info->error_count_receive               = p_can_regs->RECR;                     // Report receive error count.
    p_info->error_count_transmit              = p_can_regs->TECR;                     // Report transmit error count.
    p_info->error_code.error                  = (uint32_t) p_can_regs->ECSR;          // Report error code.

    /* Clear error code */
    clear_val              = (uint8_t) p_info->error_code.error;      // Get value read from error register.
    clear_val             &= CAN_ERROR_MASK;                          // Clear all but EDPM.
 //   p_can_regs->ECSR = clear_val;                               // Clear register.

    if (true != HW_CAN_BitRateGet(p_can_regs, &bit_rate))
    {
        ret_val = false;                        // Error occurred.
    }
    else
    {
        p_info->bit_rate = bit_rate;            // Bit rate is correct value.
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the error display mode.
 * @param[in]  channel
 * @retval     None.
 **********************************************************************************************************************/

void HW_CAN_ErrorModeSet (R_CAN0_Type * p_can_regs)
{
    p_can_regs->ECSR = CAN_ERROR_DISPLAY_MODE;         // Set error display mode.
}

/*******************************************************************************************************************//**
 * @brief      This function returns CAN errors.
 * @param[in]  channel
 * @retval     false if no errors, true if errors
 **********************************************************************************************************************/
bool HW_CAN_ErrorsGet (R_CAN0_Type * p_can_regs)
{
    bool ret_val;
    ret_val = false;

    /* Check Error Status FLag */
    if (p_can_regs->STR_b.EST != 0U)
    {
        ret_val = true;
    }

    /* Clear Error Interrupt Factor Judge Register. */
    if (p_can_regs->EIFR != 0U)
    {
        ret_val                = true;
        p_can_regs->EIFR = 0x0U;
    }

    /* Clear Error Code Store Register. */
    if ((p_can_regs->ECSR & (~ CAN_ERROR_MASK)) != 0U)
    {
        ret_val                = true;
        p_can_regs->ECSR = 0x0U;
    }

    return ret_val;
}

/*******************************************************************************************************************//**
 *   Set a timeout value.
 *   @param[in] p_timer     Pointer to a timer.
 *   @param[in] timer_value Value to set timer to.
 *   @retval none
 **********************************************************************************************************************/
static void timeout_set (volatile uint32_t * const p_timer, uint32_t timer_value)
{
    *p_timer = timer_value;                         // Set timer initial value.
}

/*******************************************************************************************************************//**
 *   Get a timeout value.
 *   @param[in] p_timer     Pointer to a timer.
 *   @retval true           Timeout reached.
 *   @retval false          Timeout not reached.
 *
 **********************************************************************************************************************/
static bool timeout_get (volatile uint32_t * const p_timer)
{
    R_BSP_SoftwareDelay((uint32_t) 1, BSP_DELAY_UNITS_MICROSECONDS);
    return (0U == (--(*(p_timer))));
}
