/****************************************************************************************/
/**
 *  \defgroup api SYNC TIMING DRIVER API
 *  @{
 *  \defgroup debug Debug API
 *  @brief     This section defines the Debug APIs for the Timing Chipset.
 *  @{
 *  \defgroup debug_ds  Debug data structures
 *   @brief    Debug Data Structures available for the Timing Chipset.
 *  \defgroup debug_api Debug APIs
 *   @brief    Debug Functions available for the Timing Chipset.
 *  @{
 *
 *  \file          sync_timing_api_debug.c
 *
 *  \details       Implementation file for Timing Driver Debug APIs that can be 
 *                 used by the application
 *
 *  \date          Created: 07/30/2018
 *
 ****************************************************************************************/
  
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

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_api_mem_access.h"
#include "sync_timing_api_debug.h"
#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_cfg_parser.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_api_internal.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    Static global variables
****************************************************************************************/

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Debug_SetCurrentMode
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/30/2018 
 * 
 *//**
 *  
 *  \brief           This DEBUG API is used to set the current device mode
 * 
 * \param[in]        timingDevId   The Timing Device Id
 * \param[in]        deviceMode    Device Mode - APPLICATION or BOOTLOADER
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             This API is not supported for Aruba. Only available in Si5389 chipsets. 
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Debug_SetCurrentMode(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_MODE_E deviceMode)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_DEBUG_MODE_T      deviceModeMsg;
    SYNC_TIMING_CORE_MSG_DEBUG_MODE_T      *pRecvBuff             = NULL;
    
    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        if (deviceMode >= SYNC_TIMING_DEVICE_MODE_INVALID)
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send Set Device Mode command to the core */
        Sync_Timing_OSAL_Wrapper_Memset(&(deviceModeMsg), 0, sizeof(deviceModeMsg));

        deviceModeMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        deviceModeMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE;
        deviceModeMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        deviceModeMsg.msgHdr.uTimingDevId            = timingDevId;
        deviceModeMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        deviceModeMsg.deviceMode                     = deviceMode;
        
        SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                                    "Sending SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&deviceModeMsg, sizeof(deviceModeMsg), 
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
                              "Sync_Timing_API_Debug_SetCurrentMode failed: %d \n", syncStatus);
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
 * FUNCTION NAME     Sync_Timing_API_Debug_GetCurrentMode
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/30/2018 
 * 
 *//**
 *  
 *  \brief           This DEBUG API is used to get the current device mode
 * 
 * \param[in]        timingDevId        The Timing Device Id
 * \param[out]       pCurrDeviceMode    Pointer to storage for returning the current 
 *                                      device mode
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/

SYNC_STATUS_E Sync_Timing_API_Debug_GetCurrentMode(uint8_t                    timingDevId, 
                                                   SYNC_TIMING_DEVICE_MODE_E  *pCurrDeviceMode)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_DEBUG_MODE_T      deviceModeMsg;
    SYNC_TIMING_CORE_MSG_DEBUG_MODE_T      *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        if (!pCurrDeviceMode)
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send Get Device Mode command to the core */
        Sync_Timing_OSAL_Wrapper_Memset(&(deviceModeMsg), 0, sizeof(deviceModeMsg));

        deviceModeMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        deviceModeMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE;
        deviceModeMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        deviceModeMsg.msgHdr.uTimingDevId            = timingDevId;
        deviceModeMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        
        SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                                    "Sending SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&deviceModeMsg, sizeof(deviceModeMsg), 
                                                            SYNC_TIMING_TRUE, 
                                                            SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                            (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                             "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_API_Debug_GetCurrentMode failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            *pCurrDeviceMode = pRecvBuff->deviceMode;
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


/** @} debug_api */
/** @} debug */
/** @} api    */

