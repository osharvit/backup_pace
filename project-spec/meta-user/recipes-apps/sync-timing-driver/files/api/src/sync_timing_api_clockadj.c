/****************************************************************************************/
/**
 *  \defgroup api SYNC TIMING DRIVER API
 *  @{
 *  \defgroup clockadj Clock Adjustment
 *  @brief     This section defines the Clock Adjustment APIs for the Timing Chipset.
 *  @{
 *  \defgroup clockadj_ds  Clock Adjustment data structures
 *   @brief    Clock Adjustment Data Structures available for the Timing Chipset.
 *  \defgroup clockadj_api Clock Adjustment APIs
 *   @brief    Clock Adjustment Functions available for the Timing Chipset.
 *  @{
 *
 *  \file          sync_timing_api_clockadj.c
 *
 *  \details       Implementation file for Timing Driver Clock Adjustment APIs that
 *                 will be used by the application
 *
 *  \date          Created: 07/13/2018
 *
 *****************************************************************************************/
 
 /****************************************************************************************/
 /**                  Copyright (c) 2018, 2021 Skyworks Solution Inc.                   **/
/****************************************************************************************/
/** This software is provided 'as-is', without any express or implied warranty.        **/
/** In no event will the authors be held liable for any damages arising from the use   **/
/** of this software.                                                                  **/
/** Permission is granted to anyone to use this software for any purpose, including    **/
/** commercial applications, and to alter it and redistribute it freely, subject to    **/
/** the following restrictions:                                                        **/
/** 1. The origin of this software must not be misrepresented; you must not claim that **/
/**    you wrote the original software. If you use this software in a product,         **/
/**    an acknowledgment in the product documentation would be appreciated but is not  **/
/**    required.                                                                       **/
/** 2. Altered source versions must be plainly marked as such, and must not be         **/
/**    misrepresented as being the original software.                                  **/
/** 3. This notice may not be removed or altered from any source distribution.         **/
 /****************************************************************************************/

#include "sync_timing_api_mem_access.h"
#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_cfg_parser.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_api_internal.h"
#include "sync_timing_api_clockadj.h"


/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_ClockAdj_GetPLLInput
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      06/01/2020
 * 
 *//**
 *  
 * \brief            This API is used to get the current active Input used for a given PLL
 * 
 * \param[in]        timingDevId        The Timing Device Id
 * \param[in]        uPLLId             The PLL ID for which the current input is to be obtained
 *                                      0 - PLL A, 1 = PLL B, 2 - PLL C, 3 - PLL D
 * \param[in]        pllInputSelect     The input port being used by the PLL
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             PLL Id availability varies from chipset to chipset. 
 *                   SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE is returned when PLL is in Holdover
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_ClockAdj_GetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect)
{
    SYNC_STATUS_E                               syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                               retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      clkAdjPllInputMsg      = {0};
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T        *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                          bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                                   syncStatus, SYNC_STATUS_SUCCESS);

        /* Send get pll input command to the core */
        Sync_Timing_OSAL_Wrapper_Memset(&(clkAdjPllInputMsg), 0, sizeof(clkAdjPllInputMsg));

        clkAdjPllInputMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjPllInputMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        clkAdjPllInputMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT;
        clkAdjPllInputMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        clkAdjPllInputMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjPllInputMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        clkAdjPllInputMsg.uPLLId                         = uPLLId;
        clkAdjPllInputMsg.pllInputSelect                 = 0xFF;
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                            "Sending SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&clkAdjPllInputMsg, 
                                                           sizeof(clkAdjPllInputMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_API_ClockAdj_GetPLLInput failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                                      syncStatus, pRecvBuff->msgHdr.reqStatus);
        }  
        else
        {
            /* Copy data read back into the message buffer */
            *pllInputSelect = pRecvBuff->pllInputSelect;
        }         

    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}


/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_ClockAdj_SetPLLInput
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      02/07/2019
 * 
 *//**
 *  
 * \brief            This API is used to set the SyncE Input to use for a given PLL
 * 
 * \param[in]        timingDevId        The Timing Device Id
 * \param[in]        uPLLId             The PLL ID for which SyncE input is to be set
 *                                      0 - PLL A, 1 = PLL B, 2 - PLL C, 3 - PLL D
 * \param[in]        pllInputSelect     The input port to use for SyncE
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             PLL Id availability varies from chipset to chipset.
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_ClockAdj_SetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect)
{
    SYNC_STATUS_E                               syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                               retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      clkAdjPllInputMsg      = {0};
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T        *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                          bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                                   syncStatus, SYNC_STATUS_SUCCESS);

        /* Send set pll input command to the core */
        Sync_Timing_OSAL_Wrapper_Memset(&(clkAdjPllInputMsg), 0, sizeof(clkAdjPllInputMsg));

        clkAdjPllInputMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjPllInputMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        clkAdjPllInputMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT;
        clkAdjPllInputMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        clkAdjPllInputMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjPllInputMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        clkAdjPllInputMsg.uPLLId                         = uPLLId;
        clkAdjPllInputMsg.pllInputSelect                 = pllInputSelect;
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                            "Sending SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&clkAdjPllInputMsg, 
                                                           sizeof(clkAdjPllInputMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_API_ClockAdj_SetPLLInput failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                                      syncStatus, pRecvBuff->msgHdr.reqStatus);
        }        

    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_ClockAdj_GetCurrentStatus
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      02/11/2019
 * 
 *//**
 *  
 * \brief            This API is used to get the status of the PLL or Input
 * 
 * \param[in]        timingDevId        The Timing Device Id
 * \param[in]        statusType         The object for which status is being requested
 * \param[out]       pStatus            Pointer to memory to return the status value
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_ClockAdj_GetCurrentStatus(uint8_t timingDevId, 
                                                        SYNC_TIMING_CLOCKADJ_STATUS_ID_E statusType, 
                                                        SYNC_TIMING_CLOCKADJ_STATUS_E    *pStatus)
{
    SYNC_STATUS_E                               syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                               retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_CLKADJ_STATUS_T        clkAdjStatusMsg        = {0};
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T        *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                          bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_CLKADJ_STATUS_T        *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                                   syncStatus, SYNC_STATUS_SUCCESS);

        /* Send get current status command to the core */
        Sync_Timing_OSAL_Wrapper_Memset(&(clkAdjStatusMsg), 0, sizeof(clkAdjStatusMsg));

        clkAdjStatusMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjStatusMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        clkAdjStatusMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS;
        clkAdjStatusMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        clkAdjStatusMsg.msgHdr.uTimingDevId            = timingDevId;
        clkAdjStatusMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        clkAdjStatusMsg.statusType                     = statusType;
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                            "Sending SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&clkAdjStatusMsg, 
                                                           sizeof(clkAdjStatusMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_API_ClockAdj_GetCurrentStatus failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                                      syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            /* Copy data read back into the message buffer */
            *pStatus = pRecvBuff->status;
        }

    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;

}


/** @} clockadj_api */
/** @} clockadj */
/** @} api    */

