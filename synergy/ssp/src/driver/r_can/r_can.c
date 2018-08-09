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
 * File Name    : r_can.c
 * Description  : SSP CAN Driver.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "r_can.h"
#include "r_can_cfg.h"
#include "r_can_private_api.h"
#include "./hw/hw_can_private.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Macro for error logger. */
#ifndef CAN_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define CAN_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (err), &g_module_name[0], &g_can_version)
#endif

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static void rcan_irq_enable (can_instance_ctrl_t * p_ctrl);
static void rcan_irq_disable (can_instance_ctrl_t * p_ctrl);
static ssp_err_t can_open_parameters_check(can_ctrl_t * const p_ctrl, can_cfg_t const * const p_cfg);
static ssp_err_t can_write_parameters_check(can_ctrl_t * const p_ctrl, can_frame_t * const p_frame);
static ssp_err_t can_hardware_set(can_cfg_t const * const p_cfg, R_CAN0_Type * p_can_regs);
static void can_vectors_set(can_instance_ctrl_t * p_ctrl, can_cfg_t const * const p_cfg, ssp_feature_t * p_feature);
static ssp_err_t can_sleep_exit(R_CAN0_Type * p_can_regs, ssp_feature_t * p_feature, can_mode_t * p_mode);
static ssp_err_t can_halt_mode(R_CAN0_Type * p_can_regs, can_mode_t * p_mode);
static ssp_err_t can_reset_mode(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode);
static ssp_err_t can_mailbox_configure(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode);
static ssp_err_t can_operate_mode(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode);
static ssp_err_t  can_modes_switch(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, ssp_feature_t * p_feature, can_mode_t * p_mode);
/***********************************************************************************************************************
 * ISR prototypes
 **********************************************************************************************************************/

void rcan_error_interrupt (can_instance_ctrl_t * p_ctrl);
void can_error_isr (void);
void rcan_receive_interrupt (can_instance_ctrl_t * p_ctrl);
void can_mailbox_rx_isr (void);
void rcan_transmit_interrupt (can_instance_ctrl_t * p_ctrl);
void can_mailbox_tx_isr (void);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#if defined(__GNUC__)
/* This structure is affected by warnings from a GCC compiler bug. This pragma suppresses the warnings in this
 * structure only.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** Version data structure used by error logger macro. */
static const ssp_version_t g_can_version =
{
    .api_version_minor  = CAN_API_VERSION_MINOR,
    .api_version_major  = CAN_API_VERSION_MAJOR,
    .code_version_major = CAN_CODE_VERSION_MAJOR,
    .code_version_minor = CAN_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

/** Name of module used by error logger macro */
#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "can";
#endif

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/** CAN function pointers   */
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
const can_api_t g_can_on_can =
{
    .open       = R_CAN_Open,
    .close      = R_CAN_Close,
    .read       = R_CAN_Read,
    .write      = R_CAN_Write,
    .control    = R_CAN_Control,
    .infoGet    = R_CAN_InfoGet,
    .versionGet = R_CAN_VersionGet
};

/*******************************************************************************************************************//**
 * @addtogroup CAN
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/***************************************************************************************************************//**
 * @brief   Open and configure the CAN channel for operation.
 *
 * @retval SSP_SUCCESS                      Channel opened successfully
 * @retval SSP_ERR_INVALID_ARGUMENT         Invalid channel passed as argument.
 * @retval SSP_ERR_HW_LOCKED                Lock already owned by another user.
 * @retval SSP_ERR_CAN_MODE_SWITCH_FAILED   Channel failed to switch modes.
 * @retval SSP_ERR_CAN_INIT_FAILED          Channel failed to initialize.
 * @retval SSP_ERR_ASSERTION                Null pointer presented.
 *
 * @return See @ref Common_Error_Codes or functions called by this function for other possible return codes.
 *         This function calls:
 *                                   * fmi_api_t::productFeatureGet
 *                                   * fmi_api_t::eventInfoGet
 *****************************************************************************************************************/

ssp_err_t R_CAN_Open (can_ctrl_t * const p_ctrl, can_cfg_t const * const p_cfg)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t          err;
    can_mode_t         mode = {(can_mode_t)0U};
    ssp_feature_t feature = {{(ssp_ip_t) 0U}};
    R_CAN0_Type * p_can_regs = NULL;

    err = SSP_SUCCESS;

#if (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values. */
    err = can_open_parameters_check(p_ctrl, p_cfg);
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */
    /** Check for valid parameters. */
    if (SSP_SUCCESS == err)
    {
    /** Make sure the feature exists on this MCU. */
        feature.id = SSP_IP_CAN;
        feature.channel = (uint32_t) (p_cfg->channel & 0xFFFF);
        feature.unit = 0U;
        fmi_feature_info_t info = {0U};
        err = g_fmi_on_fmi.productFeatureGet(&feature, &info);
        p_internal_ctrl->p_reg = info.ptr;
        p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
    }

    if (SSP_SUCCESS == err)
    {
        /** Lookup and store IRQ numbers. */
        can_vectors_set(p_internal_ctrl, p_cfg, &feature);
        /** Try to get channel lock. */
        if (SSP_ERR_IN_USE == R_BSP_HardwareLock(&feature))
        {
            /** Channel is already open so return error */
            err = SSP_ERR_HW_LOCKED;
        }
    }

    if (SSP_SUCCESS == err)
    {
        /** Exit module stop state, reset, then go to halt mode.. */
        err = can_modes_switch(p_can_regs, p_cfg, &feature, &mode);
    }


    if (SSP_SUCCESS == err)
    {

        /** Configure mailboxes. */
        err = can_mailbox_configure(p_can_regs, p_cfg, &mode);
    }

    if (SSP_SUCCESS == err)
    {
        /** Go to normal operation. */
        err = can_operate_mode(p_can_regs, p_cfg, &mode);
    }

    /** Set channel, callback function, context, id mode, mailbox count, message mode, op mode and opened status. */
    if (SSP_SUCCESS == err)
    {
        p_internal_ctrl->channel        = p_cfg->channel;
        p_internal_ctrl->p_callback     = p_cfg->p_callback;
        p_internal_ctrl->p_context      = p_cfg->p_context;
        p_internal_ctrl->id_mode        = p_cfg->id_mode;
        p_internal_ctrl->mailbox_count  = p_cfg->mailbox_count;
        p_internal_ctrl->message_mode   = p_cfg->message_mode;
        p_internal_ctrl->operation_mode = mode;
        p_internal_ctrl->opened         = true;

        /** Configure CAN interrupts. */
        rcan_irq_enable(p_internal_ctrl);
    }
    else
    {
        /** Unlock channel if any errors occurred. */
        R_BSP_HardwareUnlock(&feature);
        /** Enter module stop state. */
        R_BSP_ModuleStop(&feature);

        /** Process errors before returning. */
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }


    return err;
}  /** End of function R_CAN_Open() */

/***************************************************************************************************************//**
 * @brief   Close the CAN channel.
 *
 * @retval SSP_SUCCESS          Channel closed successfully.
 * @retval SSP_ERR_NOT_OPEN     Port is not open.
 * @retval SSP_ERR_ASSERTION    Null pointer presented.
 *****************************************************************************************************************/
ssp_err_t R_CAN_Close (can_ctrl_t * const p_ctrl)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t   err;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values */
    if (NULL == p_ctrl)
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    ssp_feature_t feature = {{(ssp_ip_t) 0U}};

    if (SSP_SUCCESS == err)
    {
        feature.id = SSP_IP_CAN;
        feature.unit = 0U;
        feature.channel = (uint32_t) (p_internal_ctrl->channel & 0xFFFF);

        /** If channel is not open, return an error */
        if (false == p_internal_ctrl->opened)
        {
            err = SSP_ERR_NOT_OPEN;         // Channel is not open, so return error.
        }

        if (SSP_SUCCESS == err)
        {
            /** disable interrupts */
            rcan_irq_disable(p_internal_ctrl);

            /** enter module stop state */
            R_BSP_ModuleStop(&feature);

            /** Unlock channel */
            R_BSP_HardwareUnlock(&feature);
            p_internal_ctrl->opened = false;
        }
    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}  /* End of function R_CAN_Close() */

/***************************************************************************************************************//**
 * @brief  Read data from the CAN channel. Return up to eight bytes read from the channel mailbox.
 *
 * @retval SSP_SUCCESS                      Data successfully read.
 * @retval SSP_ERR_CAN_DATA_UNAVAILABLE     No data available.
 * @retval SSP_ERR_CAN_TRANSMIT_MAILBOX     Mailbox is not setup for receive.
 * @retval SSP_ERR_ASSERTION                Null pointer presented.
 *****************************************************************************************************************/
ssp_err_t R_CAN_Read (can_ctrl_t  * const p_ctrl,
                      uint32_t            mailbox,
                      can_frame_t * const p_frame)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t   err;
    uint8_t     byte_count;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values */
    if ((NULL == p_ctrl) || (NULL == p_frame))
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
    else
    {
        /** Make sure mailbox is setup for receive. */
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if (CAN_MAILBOX_RECEIVE != HW_CAN_MailboxTypeGet(p_can_regs, mailbox))
        {
            err = SSP_ERR_CAN_TRANSMIT_MAILBOX;
        }
    }

#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    if (SSP_SUCCESS == err)
    {
        /** Check for receive data */
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if (true == HW_CAN_ReceiveDataAvailable(p_can_regs, mailbox))
        {
            byte_count = HW_CAN_ReceiveDataCountGet(p_can_regs, mailbox);
            if (byte_count > CAN_MAX_DATA_LENGTH)
            {
                /** Invalid byte count, report no receive data available for this  mailbox. */
                err = SSP_ERR_CAN_DATA_UNAVAILABLE;
            }
            else
            {
                /** Get frame data. */
                HW_CAN_ReceiveDataGet(p_can_regs, mailbox, p_internal_ctrl->id_mode, p_frame);
                /* Check for message lost. */
            }
            if ( HW_CAN_MessageLostGet (p_can_regs, mailbox))
            {
                err = SSP_ERR_CAN_MESSAGE_LOST;
            }
        }
        else
        {
            err = SSP_ERR_CAN_DATA_UNAVAILABLE;
        }

    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}  /* End of function R_CAN_Read() */

/***************************************************************************************************************//**
 * @brief  Write data to the CAN channel. Write up to eight bytes to the channel mailbox.
 *
 * @retval SSP_SUCCESS                      Operation succeeded.
 * @retval SSP_ERR_CAN_TRANSMIT_NOT_READY   Transmit in progress, cannot write data at this time.
 * @retval SSP_ERR_CAN_RECEIVE_MAILBOX      Mailbox is setup for receive and cannot send.
 * @retval SSP_ERR_INVALID_ARGUMENT         Data length or frame type invalid.
 * @retval SSP_ERR_ASSERTION                Null pointer presented
 *****************************************************************************************************************/
ssp_err_t R_CAN_Write (can_ctrl_t  * const p_ctrl,
                       uint32_t            mailbox,
                       can_frame_t * const p_frame)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t   err;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values */
    err = can_write_parameters_check(p_ctrl, p_frame);

    if (SSP_SUCCESS == err)
    {
        /** Make sure mailbox is setup for transmit. */
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if (CAN_MAILBOX_TRANSMIT != HW_CAN_MailboxTypeGet(p_can_regs, mailbox))
        {
            err = SSP_ERR_CAN_RECEIVE_MAILBOX;
        }
        else if ((p_frame->data_length_code > CAN_MAX_DATA_LENGTH) || (p_frame->type > CAN_FRAME_TYPE_REMOTE))
        {
            err = SSP_ERR_INVALID_ARGUMENT;               // Report data length or frame type error.
        }
        else
        {
            /* Follow coding rules */
        }
    }
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    if (SSP_SUCCESS == err)
    {

        /** Check transmit ready flag. */
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if (true == HW_CAN_TransmitDataReady(p_can_regs, mailbox))
        {
            HW_CAN_TransmitDataClear(p_can_regs, mailbox);    // Transmit ready flag set, so clear it.
        }
        else
        {
            err = SSP_ERR_CAN_TRANSMIT_NOT_READY;       // Transmit ready flag is not set, return error/status.
        }

        if (SSP_SUCCESS == err)
        {
            /** Set transmit id  */
            HW_CAN_MailboxSendIdSet(p_can_regs, mailbox, p_frame->id, p_internal_ctrl->id_mode);
            /** Send transmit frame. */
            HW_CAN_TransmitFrameSend(p_can_regs, mailbox, p_frame);
        }
    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}

/***************************************************************************************************************//**
 * @brief  CAN Control is used to control extended features.
 *
 * @retval SSP_SUCCESS              Operation succeeded.
 * @retval SSP_ERR_INVALID_ARGUMENT Invalid command.
 * @retval SSP_ERR_ASSERTION        Null pointer presented
 *****************************************************************************************************************/
ssp_err_t R_CAN_Control (can_ctrl_t * const p_ctrl, can_command_t const command, void * p_data)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t   err;
    can_mode_t mode;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values */

    if ((NULL == p_ctrl) || (NULL == p_data))
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    if (SSP_SUCCESS == err)
    {
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if ( CAN_COMMAND_MODE_SWITCH == command )
        {
            /** Change operating mode. */
            mode = *((can_mode_t *) p_data);
            HW_CAN_Control(p_can_regs, mode);

            /** Save mode for diagnostic purposes. */
            p_internal_ctrl->operation_mode = mode;
        }
        else
        {
            err = SSP_ERR_INVALID_ARGUMENT;               // Report command error.
        }
    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}

/***************************************************************************************************************//**
 * @brief   Get CAN state and status information for the channel.
 *
 * @retval  SSP_SUCCESS                     Operation succeeded.
 * @retval  SSP_ERR_CAN_DATA_UNAVAILABLE    Channel failed to return info.
 * @retval  SSP_ERR_ASSERTION               Null pointer presented
 *****************************************************************************************************************/
ssp_err_t R_CAN_InfoGet (can_ctrl_t * const p_ctrl, can_info_t * const p_info)
{
    can_instance_ctrl_t * p_internal_ctrl = (can_instance_ctrl_t *) p_ctrl;
    ssp_err_t   err;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointers for NULL values */
    if ((NULL == p_ctrl) || (NULL == p_info))
    {
        err = SSP_ERR_ASSERTION;                          // return ASSERTION ERROR for NULL pointers.
    }
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    if (SSP_SUCCESS == err)
    {
        /** Get status for channel. */
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_internal_ctrl->p_reg;
        if (true != HW_CAN_StatusGet(p_can_regs, p_info))
        {
            err = SSP_ERR_CAN_DATA_UNAVAILABLE;         // Error encountered when retrieving info.
        }
        else
        {
            p_info->operation_mode = p_internal_ctrl->operation_mode;   // Populate operation mode.
        }
    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief   Get CAN module code and API versions.
 * @retval  SSP_SUCCESS             Operation succeeded.
 * @retval  SSP_ERR_ASSERTION       Null pointer presented
 * @note This function is reentrant.
 **********************************************************************************************************************/
ssp_err_t R_CAN_VersionGet (ssp_version_t * const p_version)
{
    ssp_err_t err;

    err = SSP_SUCCESS;

#if    (CAN_CFG_PARAM_CHECKING_ENABLE)
    /** Check pointer for NULL value */
    if (NULL == p_version)
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
#endif /* if    (CAN_CFG_PARAM_CHECKING_ENABLE) */

    if (SSP_SUCCESS == err)
    {
        p_version->version_id = g_can_version.version_id;       // Return module version information.
    }

    /** Process errors before returning. */
    if (SSP_SUCCESS != err)
    {
        /** Log error or assertion. */
        if (SSP_ERR_ASSERTION == err)
        {
            SSP_ASSERT_FAIL;
        }
        else
        {
            SSP_ERROR_LOG(err, g_module_name, g_can_version);
        }
    }

    return err;
}


/*******************************************************************************************************************//**
 * @} (end addtogroup CAN)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      CAN Common Error ISR.
 *
 * Gets errors, saves events and calls callback function, if used.
 *
 * @param[in]  p_ctrl    Pointer to CAN instance control block
 *
 **********************************************************************************************************************/

void rcan_error_interrupt (can_instance_ctrl_t * p_ctrl)
{
    can_callback_args_t     args = {0U};
    can_interrrupt_status_t status = {0U};
    uint32_t                mailbox = {0U};

    /** Get source of error interrupt. */
    R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_ctrl->p_reg;
    if (true == HW_CAN_InterruptErrorGet(p_can_regs, &status))
    {
        if (status.int_status_b.bus_off_entry)                      // Check for bus off error.
        {
            args.event = CAN_EVENT_ERR_BUS_OFF;                     // Set event argument to bus error.
        }
        else if (status.int_status_b.bus_off_recovery)              // Check for bus recovery error.
        {
            args.event = CAN_EVENT_BUS_RECOVERY;                    // Set event argument to bus recovery.
        }
        else if (status.int_status_b.error_passive)                 // Check for bus error passive.
        {
            args.event = CAN_EVENT_ERR_PASSIVE;                     // Set event argument to bus recovery.
        }
        else if (status.int_status_b.receive_overrun)               // Check for receive overwrite / overrrun.
        {
            args.event = CAN_EVENT_MAILBOX_OVERWRITE_OVERRUN;       // Set event to overwrite / overrrun error.
            HW_CAN_ErrorMailboxGet(p_can_regs, &mailbox);
        }
        else
        {
            /** Follow code rules */
        }
   }

    /** Get user the callback, if set in the open function. */
    if (NULL != p_ctrl->p_callback)
    {
        args.channel   = p_ctrl->channel;               // Populate callback arguments accordingly.
        args.p_context = p_ctrl->p_context;
        p_ctrl->p_callback(&args);                      // Call the user callback function.
    }
}

/*******************************************************************************************************************//**
 * @brief      Error ISR.
 *
 * Saves context if RTOS is used, clears interrupts, calls common error function, and restores context if RTOS is used.
 **********************************************************************************************************************/
void can_error_isr (void)
{
    /** Save context if RTOS is used */
    SF_CONTEXT_SAVE

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    can_instance_ctrl_t * p_ctrl = (can_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

#if CAN_CFG_PARAM_CHECKING_ENABLE
    if (NULL == p_ctrl)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
    if (NULL == p_ctrl->p_reg)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
#endif

    /** Clear interrupt */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    rcan_error_interrupt(p_ctrl);

    /** Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief      CAN Common Receive ISR.
 *
 * Identifies mailbox, saves receive event and calls callback function, if used.
 *
 * @param[in]  p_ctrl    Pointer to CAN instance control block
 *
 **********************************************************************************************************************/
void rcan_receive_interrupt (can_instance_ctrl_t * p_ctrl)
{
    /** Check CAN receive. */

    /** Get user the callback, if set in the open function. */
    if (NULL != p_ctrl->p_callback)
    {
        can_callback_args_t args;
        uint32_t            mailbox = 0U;
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_ctrl->p_reg;
        HW_CAN_ReceiveMailboxGet(p_can_regs, &mailbox);

        /** Save the receive mailbox number. */
        args.mailbox = mailbox;

        /**  Set event argument to receive complete. */
        args.event     = CAN_EVENT_RX_COMPLETE;
        args.channel   = p_ctrl->channel;               // Populate callback arguments accordingly.
        args.p_context = p_ctrl->p_context;
        p_ctrl->p_callback(&args);                      // Call the user callback function.
    }
}

/*******************************************************************************************************************//**
 * @brief      Receive ISR.
 *
 * Saves context if RTOS is used, clears interrupts, calls common receive function
 * and restores context if RTOS is used.
 **********************************************************************************************************************/
void can_mailbox_rx_isr (void)
{
    /** Save context if RTOS is used */
    SF_CONTEXT_SAVE

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    can_instance_ctrl_t * p_ctrl = (can_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

#if CAN_CFG_PARAM_CHECKING_ENABLE
    if (NULL == p_ctrl)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
    if (NULL == p_ctrl->p_reg)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
#endif

    /** Clear interrupt */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    rcan_receive_interrupt(p_ctrl);

    /** Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief      CAN Common Transmit ISR.
 *
 * Identifies mailbox, saves transmit event and calls callback function, if used.
 *
 * @param[in]  p_ctrl    Pointer to CAN instance control block
 *
 **********************************************************************************************************************/

void rcan_transmit_interrupt (can_instance_ctrl_t * p_ctrl)
{
    /** Check CAN transmit. */

    /** Get user the callback, if set in the open function. */
    if (NULL != p_ctrl->p_callback)
    {
        can_callback_args_t args;
        uint32_t            mailbox = 0U;;
        R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_ctrl->p_reg;
        HW_CAN_TransmitMailboxGet(p_can_regs, &mailbox);

        /** Save the transmit mailbox number. */
        args.mailbox = mailbox;

        /**  Set event argument to transmit complete. */
        args.event     = CAN_EVENT_TX_COMPLETE;
        args.channel   = p_ctrl->channel;                                   // Populate callback arguments accordingly.
        args.p_context = p_ctrl->p_context;
        p_ctrl->p_callback(&args);                      // Call the user callback function.
    }
}

/*******************************************************************************************************************//**
 * @brief      Transmit ISR.
 *
 * Saves context if RTOS is used, clears interrupts, calls common transmit function
 * and restores context if RTOS is used.
 **********************************************************************************************************************/
void can_mailbox_tx_isr (void)
{
    /** Save context if RTOS is used */
    SF_CONTEXT_SAVE

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    can_instance_ctrl_t * p_ctrl = (can_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

#if CAN_CFG_PARAM_CHECKING_ENABLE
    if (NULL == p_ctrl)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
    if (NULL == p_ctrl->p_reg)
    {
        /* The interrupt fired while the driver is not open. */
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
        return;
    }
#endif

    /** Clear interrupt */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    rcan_transmit_interrupt(p_ctrl);

    /** Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief      Enable ISRs.
 *
 * Enables CAN interrupts.
 *
 * @param[in]  p_ctrl      Instance Cntrl
 *
 **********************************************************************************************************************/
static void rcan_irq_enable (can_instance_ctrl_t * p_ctrl)
{
    R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_ctrl->p_reg;
    if (SSP_INVALID_VECTOR != p_ctrl->error_irq)
    {
        /** Enable error interrupt. */
        NVIC_EnableIRQ(p_ctrl->error_irq);
        HW_CAN_ErrorInterruptEnable(p_can_regs);
    }
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_rx_irq)
    {
        /** Enable error interrupt. */
        NVIC_EnableIRQ(p_ctrl->mailbox_rx_irq);
    }
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_tx_irq)
    {
        /** Enable error interrupt. */
        NVIC_EnableIRQ(p_ctrl->mailbox_tx_irq);
        HW_CAN_SendReceiveInterruptsEnable(p_can_regs);
    }
}

/*******************************************************************************************************************//**
 * @brief      Disable ISRs.
 *
 * Disables CAN interrupts.
 *
 * @param[in]  p_ctrl      Instance Cntrl
 *
 **********************************************************************************************************************/
static void rcan_irq_disable (can_instance_ctrl_t * p_ctrl)
{
    R_CAN0_Type * p_can_regs = (R_CAN0_Type *) p_ctrl->p_reg;
    if (SSP_INVALID_VECTOR != p_ctrl->error_irq)
    {
        /** Enable error interrupt. */
        NVIC_DisableIRQ(p_ctrl->error_irq);
        HW_CAN_ErrorInterruptEnable(p_can_regs);
    }
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_rx_irq)
    {
        /** Enable error interrupt. */
        NVIC_DisableIRQ(p_ctrl->mailbox_rx_irq);
    }
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_tx_irq)
    {
        /** Enable error interrupt. */
        NVIC_DisableIRQ(p_ctrl->mailbox_tx_irq);
        HW_CAN_SendReceiveInterruptsEnable(p_can_regs);
    }
}

/*******************************************************************************************************************//**
 * @brief      Check for Open function NULL pointers.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_open_parameters_check(can_ctrl_t * const p_ctrl, can_cfg_t const * const p_cfg)
{
    ssp_err_t err = SSP_SUCCESS;
    if (((NULL == p_ctrl) || (NULL == p_cfg)) || (NULL == p_cfg->p_extend))
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
    return err;
}
/*******************************************************************************************************************//**
 * @brief      Check for Write function NULL pointers.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_write_parameters_check(can_ctrl_t * const p_ctrl, can_frame_t * const p_frame)
{
    ssp_err_t err = SSP_SUCCESS;
    if ((NULL == p_ctrl) || (NULL == p_frame))
    {
        err = SSP_ERR_ASSERTION;        // return ASSERTION ERROR for NULL pointers.
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Set modes, clocks and timing for the CAN hardware.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_hardware_set(can_cfg_t const * const p_cfg, R_CAN0_Type * p_can_regs)
{
    ssp_err_t err = SSP_SUCCESS;
    can_extended_cfg_t  * extended_cfg = NULL;

    extended_cfg = (can_extended_cfg_t *) p_cfg->p_extend;

    /** BOM:    Bus Off recovery mode acc. to IEC11898-1 */
    HW_CAN_BusRecoveryModeSet(p_can_regs);

    /** MBM: Select normal mailbox mode. */
    HW_CAN_MailboxModeSet(p_can_regs);

    if (true != HW_CAN_IdModeSet(p_can_regs, p_cfg->id_mode))
    {
        err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
    }

    if (true != HW_CAN_MessageModeSet(p_can_regs, p_cfg->message_mode))
    {
        err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
    }

    HW_CAN_PriorityModeSet(p_can_regs);

    HW_CAN_TimeStampSet(p_can_regs);

    if (true != HW_CAN_ClockSet(p_can_regs, extended_cfg))
    {
        err = SSP_ERR_CLOCK_INACTIVE;               // Report error if channel failed to initialize.
    }
    if (true != HW_CAN_BitTimingSet (p_can_regs, p_cfg->p_bit_timing))
    {
        err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief      Set vectors.
 *
 *
 **********************************************************************************************************************/

static void can_vectors_set(can_instance_ctrl_t * p_ctrl, can_cfg_t const * const p_cfg, ssp_feature_t * p_feature)
{
    ssp_vector_info_t * p_vector_info;
    fmi_event_info_t event_info = {(IRQn_Type) 0};
    g_fmi_on_fmi.eventInfoGet(p_feature, SSP_SIGNAL_CAN_ERROR, &event_info);
    p_ctrl->error_irq = event_info.irq;
    if (SSP_INVALID_VECTOR != p_ctrl->error_irq)
    {
        R_SSP_VectorInfoGet(p_ctrl->error_irq, &p_vector_info);
        NVIC_SetPriority(p_ctrl->error_irq, p_cfg->error_ipl);
        *(p_vector_info->pp_ctrl) = p_ctrl;
    }
    g_fmi_on_fmi.eventInfoGet(p_feature, SSP_SIGNAL_CAN_MAILBOX_RX, &event_info);
    p_ctrl->mailbox_rx_irq = event_info.irq;
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_rx_irq)
    {
        R_SSP_VectorInfoGet(p_ctrl->mailbox_rx_irq, &p_vector_info);
        NVIC_SetPriority(p_ctrl->mailbox_rx_irq, p_cfg->mailbox_rx_ipl);
        *(p_vector_info->pp_ctrl) = p_ctrl;
    }
    g_fmi_on_fmi.eventInfoGet(p_feature, SSP_SIGNAL_CAN_MAILBOX_TX, &event_info);
    p_ctrl->mailbox_tx_irq = event_info.irq;
    if (SSP_INVALID_VECTOR != p_ctrl->mailbox_tx_irq)
    {
        R_SSP_VectorInfoGet(p_ctrl->mailbox_tx_irq, &p_vector_info);
        NVIC_SetPriority(p_ctrl->mailbox_tx_irq, p_cfg->mailbox_tx_ipl);
        *(p_vector_info->pp_ctrl) = p_ctrl;
    }
}

/*******************************************************************************************************************//**
 * @brief      Exit CAN sleep mode.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_sleep_exit(R_CAN0_Type * p_can_regs, ssp_feature_t * p_feature, can_mode_t * p_mode)
{
    SSP_PARAMETER_NOT_USED(p_feature);
    ssp_err_t err = SSP_SUCCESS;

    /** Exit Sleep mode. This will also take us from HALT mode to OPERATE_CANMODE. */
    if (true != HW_CAN_Control(p_can_regs, CAN_MODE_EXIT_SLEEP))
    {
        err = SSP_ERR_CAN_MODE_SWITCH_FAILED;       // Report error if mode failed to switch.
    }
    else
    {
        *p_mode = CAN_MODE_EXIT_SLEEP;                 // Save mode for diagnostic purposes.
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Go to CAN halt mode.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_halt_mode(R_CAN0_Type * p_can_regs, can_mode_t * p_mode)
{
    ssp_err_t err = SSP_SUCCESS;

    /** Reset -> HALT mode ************************************************************/
    if (true != HW_CAN_Control(p_can_regs, CAN_MODE_HALT))
    {
        err = SSP_ERR_CAN_MODE_SWITCH_FAILED;       // Report error if mode failed to switch.
    }
    else
    {
        *p_mode = CAN_MODE_HALT;                      // Save mode for diagnostic purposes.
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief      Go to CAN reset mode.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_reset_mode(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode)
{
    ssp_err_t err = SSP_SUCCESS;

    if (true != HW_CAN_Control(p_can_regs, CAN_MODE_RESET))
    {
        err = SSP_ERR_CAN_MODE_SWITCH_FAILED;       // Report error if mode failed to switch.
    }
    else
    {
        *p_mode = CAN_MODE_RESET;                      // Save mode for diagnostic purposes.
        err = can_hardware_set(p_cfg, p_can_regs);
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Go to CAN operate mode.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_operate_mode(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode)
{
    SSP_PARAMETER_NOT_USED(p_cfg);
    ssp_err_t err = SSP_SUCCESS;

    /** Halt -> Normal OPERATION mode *********************************************************/
    if (true != HW_CAN_Control(p_can_regs, CAN_MODE_NORMAL))
    {
        err = SSP_ERR_CAN_MODE_SWITCH_FAILED;       // Report error if mode failed to switch.
    }
    else
    {
        *p_mode = CAN_MODE_NORMAL;                      // Save mode for diagnostic purposes.
    }
    if (SSP_SUCCESS == err)
    {
        /** Time Stamp Counter reset. Set the TSRC bit to 1 in CAN Operation mode. */
        if (false == HW_CAN_TimeStampReset(p_can_regs))
        {
            err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
        }
    }
    if (SSP_SUCCESS == err)
    {
        /** Check for errors so far, report, and clear. */
        if (true == HW_CAN_ErrorsGet(p_can_regs))
        {
            err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
        }
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Switch CAN modes. *
 *
 **********************************************************************************************************************/

static ssp_err_t  can_modes_switch(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, ssp_feature_t * p_feature, can_mode_t * p_mode)
{
    ssp_err_t err = SSP_SUCCESS;
    R_BSP_ModuleStart(p_feature);
    /** Exit Sleep mode. This will also take us from HALT mode to OPERATE_CANMODE. */
    err = can_sleep_exit(p_can_regs, p_feature, p_mode);

    if (SSP_SUCCESS == err)
    {
        /** Go to RESET mode. **********************************************************/
        err = can_reset_mode(p_can_regs, p_cfg, p_mode);
    }

    if (SSP_SUCCESS == err)
    {
        /** Mask invalid for all mailboxes by default, for a starting point. This will change after testing normal
         * operation.*/
        HW_CAN_MaskEnableDefautSet(p_can_regs);
        /** Reset -> HALT mode ************************************************************/
        err = can_halt_mode(p_can_regs, p_mode);
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Configure CAN mailboxes.
 *
 *
 **********************************************************************************************************************/

static ssp_err_t can_mailbox_configure(R_CAN0_Type * p_can_regs, can_cfg_t const * const p_cfg, can_mode_t * p_mode)
{
    SSP_PARAMETER_NOT_USED(p_mode);
    ssp_err_t err = SSP_SUCCESS;
    can_extended_cfg_t  * extended_cfg = NULL;
    extended_cfg = (can_extended_cfg_t *) p_cfg->p_extend;

    /** Clear mailboxes in Halt mode. */
    HW_CAN_MailboxesClear(p_can_regs);

    /** Set Error Display mode in Halt mode. */
    HW_CAN_ErrorModeSet(p_can_regs);

    if (true != HW_CAN_MailboxIdSet(p_can_regs, p_cfg->mailbox_count, p_cfg->p_mailbox, p_cfg->id_mode))
    {
        err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
    }
    if (SSP_SUCCESS == err)
    {
        if (true !=
            HW_CAN_MailboxMaskSet(p_can_regs, p_cfg->mailbox_count, extended_cfg->p_mailbox_mask, p_cfg->id_mode))
        {
            err = SSP_ERR_CAN_INIT_FAILED;              // Report error if channel failed to initialize.
        }
    }
    return err;
}
