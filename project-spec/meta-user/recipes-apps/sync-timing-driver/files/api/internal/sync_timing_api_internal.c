/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_api_internal.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/03/2018
 *
 * DESCRIPTION        : Timing Driver API Internal Routines
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

#include "sync_timing_api_internal.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_cfg_parser.h"

/*****************************************************************************************
    Static global variables
****************************************************************************************/

static SYNC_TIMING_API_CLIENT_APP_CONTEXT_T gSyncTimingApiClientAppContext = {0};


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Ctrl_GetDeviceContext
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/27/2018
 *
 * DESCRIPTION   : This function is used to get the client application context
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_TIMING_API_CLIENT_APP_CONTEXT_T *Sync_Timing_Internal_API_Ctrl_GetDeviceContext()
{
    return &gSyncTimingApiClientAppContext;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Ctrl_AcqDevice
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/27/2018
 *
 * DESCRIPTION   : This function is used to acquire and lock the device for use
 *
 * IN PARAMS     : timingDevId  -  Timing device Id
 *
 * OUT PARAMS    : ppTimingDevContext - The device context structure
 *               : pHeldMutex         - An indicate signifying if the mutex has been 
 *                                      acquired or not.
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Ctrl_AcqDevice(
                               SYNC_TIMING_API_CLIENT_APP_CONTEXT_T  **ppClientAppContext,
                               SYNC_TIMING_BOOL_E                    *pHeldMutex
                                                     )
{
    SYNC_STATUS_E                         syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T  *pClientAppContext = NULL;

    if (pHeldMutex != NULL)
    {
        *pHeldMutex = SYNC_TIMING_FALSE;
    }

    do
    {
        pClientAppContext = Sync_Timing_Internal_API_Ctrl_GetDeviceContext();
        if (NULL == pClientAppContext)
        {
            SYNC_TIMING_INFO(SYNC_TIMING_LOG_DEFAULT_HANDLE, "Unable to find device context for client\n");
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_NOT_INITIALIZED);
        }

        if (SYNC_TIMING_FALSE == pClientAppContext->bApiDriverInitialized)
        {
            SYNC_TIMING_INFO(SYNC_TIMING_LOG_DEFAULT_HANDLE, "Client is not initialized\n");
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_NOT_INITIALIZED);
        }

        *ppClientAppContext = pClientAppContext;

        if (pHeldMutex != NULL)
        {
            if (Sync_Timing_OSAL_Wrapper_Mutex_Get(pClientAppContext->pClientAppCtxMutex, 
                                                  SYNC_TIMING_OSAL_WAIT_FOREVER)
                  != SYNC_TIMING_OSAL_SUCCESS)
            {
                SYNC_TIMING_INFO(SYNC_TIMING_LOG_DEFAULT_HANDLE, "Unable to take mutex for client\n");
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
            }
            *pHeldMutex = SYNC_TIMING_TRUE;
        }
        
    } while(0);

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to initialize the client process for communicating
 *                 with the core
 *
 * IN PARAMS     : uClientAppId    -- Client Application ID
 *                 pApplnCfg       -- Client Application Cfg
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_Init(SYNC_TIMING_APPLN_INFO_T  *pApplnInfo, 
                                                 SYNC_TIMING_APPLN_CFG_T   *pApplnCfg,
                                                 SYNC_TIMING_DEVICE_INFO_T *pActiveDeviceInfo)
{
    SYNC_STATUS_E                     syncStatus        = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_INIT_T       coreInitMsg;

    do 
    {
        /* Give some time for the thread to start running */
        Sync_Timing_OSAL_Wrapper_SleepMS(200);

        /* Send Init command to core to register the client */
        Sync_Timing_OSAL_Wrapper_Memset(&(coreInitMsg), 0, sizeof(coreInitMsg));
        coreInitMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        coreInitMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_INIT;
        coreInitMsg.msgHdr.uClientAppId            = pApplnInfo->AppInstanceId;
        coreInitMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;

        Sync_Timing_OSAL_Wrapper_Memcpy(&(coreInitMsg.clientInfo), pApplnInfo, 
                                        sizeof(SYNC_TIMING_APPLN_INFO_T));

        SYNC_TIMING_INFO(gSyncTimingApiClientAppContext.pClientLogModuleId,
                         "Sending SYNC_TIMING_CORE_MSG_INIT CMD to CORE;"
                         "sizeof(coreInitMsg) = %lu\n",
                         sizeof(coreInitMsg));

        syncStatus = Sync_Timing_Internal_API_Comm_SendRegnMsg((char *)&coreInitMsg, 
                                                           sizeof(coreInitMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           NULL);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                                      syncStatus, SYNC_STATUS_FAILURE);
        }
        else
        {
            if (coreInitMsg.msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                                  "SYNC_TIMING_CORE_MSG_INIT Command processing failed: %d \n", 
                                  syncStatus);
            }
            else
            {
                /* Copy the active device info to send back to the main API */
                Sync_Timing_OSAL_Wrapper_Memcpy(pActiveDeviceInfo, &(coreInitMsg.activeDeviceInfo), 
                                                sizeof(coreInitMsg.activeDeviceInfo));
            }
        }

    } while(0);
    
    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_Term
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to terminate the client communication with the 
 *                 core
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_Term()
{
    SYNC_STATUS_E                     syncStatus        = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_TERM_T       coreTermMsg;

    do 
    {

        /* Send Term command to core to terminate the connection  */
        Sync_Timing_OSAL_Wrapper_Memset(&(coreTermMsg), 0, sizeof(coreTermMsg));

        coreTermMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        coreTermMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_TERM;
        coreTermMsg.msgHdr.uClientAppId            = gSyncTimingApiClientAppContext.ApplnInfo.AppInstanceId;
        coreTermMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;

        Sync_Timing_OSAL_Wrapper_Memcpy(&(coreTermMsg.clientInfo), 
                                        &(gSyncTimingApiClientAppContext.ApplnInfo), 
                                        sizeof(SYNC_TIMING_APPLN_INFO_T));
        
        SYNC_TIMING_INFO(gSyncTimingApiClientAppContext.pClientLogModuleId,
                         "Sending SYNC_TIMING_CORE_MSG_TERM CMD to CORE\n");

        /* Send SYNC_TIMING_CORE_MSG_TERM CMD to CORE */
        syncStatus = Sync_Timing_Internal_API_Comm_SendRegnMsg((char *)&coreTermMsg, sizeof(coreTermMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           NULL);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
        }
        else
        {
            if (coreTermMsg.msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                                  "SYNC_TIMING_CORE_MSG_TERM command processing failed: %d \n", 
                                  syncStatus);
            }
        }

    } while(0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_SendRegnMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/28/2018
 *
 * DESCRIPTION   : This function is used to send register/de-register message to the core
 *
 * IN PARAMS     : pMsg              -- Message to send
 *                 uMsgLen           -- Length of the message
 *                 bBlockForResponse -- Should we wait for response
 *                 uRespTimeoutMs    -- Response timeout
 *
 * OUT PARAMS    : ppRecvBuff        -- Received Response Message buffer
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_SendRegnMsg(char *pMsg, uint32_t uMsgLen, 
                                               uint32_t bBlockForResponse, 
                                               uint32_t uRespTimeoutMs,
                                               char **ppRecvBuff)
{
    SYNC_STATUS_E             syncStatus        = SYNC_STATUS_SUCCESS;

    do
    {
        SYNC_TIMING_DEBUG(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                          "Sync_Timing_Internal_API_Comm_SendMsg: "
                          "Sending Msg CMD to CORE; sizeof(coreMsg) = %u\n", uMsgLen);

        // Call Core Driver function
        //printf("%s:%d\n", __FUNCTION__, __LINE__);
        
        syncStatus = Sync_Timing_Internal_CORE_ProcessIncomingMsg(0, (void *)pMsg, uMsgLen);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                             "Sync_Timing_Internal_CORE_ProcessIncomingMsg failed: %d. \n", 
                             syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                                      syncStatus, SYNC_STATUS_FAILURE);
        }

        // Process Incoming Response Msg
        //*ppRecvBuff = (char *)(pMsg);

    } while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_SendMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to send commands/messages to the core
 *
 * IN PARAMS     : pMsg              -- Message to send
 *                 uMsgLen           -- Length of the message
 *                 bBlockForResponse -- Should we wait for response
 *                 uRespTimeoutMs    -- Response timeout
 *
 * OUT PARAMS    : ppRecvBuff        -- Received Response Message buffer
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_SendMsg(char *pMsg, uint32_t uMsgLen, 
                                               uint32_t bBlockForResponse, 
                                               uint32_t uRespTimeoutMs,
                                               char **ppRecvBuff)
{
    SYNC_STATUS_E             syncStatus        = SYNC_STATUS_SUCCESS;

    do
    {
        SYNC_TIMING_DEBUG(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                          "Sync_Timing_Internal_API_Comm_SendMsg: "
                          "Sending Msg CMD to CORE; sizeof(coreMsg) = %u\n", uMsgLen);

        // Call Core Driver function
        syncStatus = Sync_Timing_Internal_CORE_ProcessIncomingMsg(0, (void *)pMsg, uMsgLen);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                             "Sync_Timing_Internal_CORE_ProcessIncomingMsg failed: %d. \n", 
                             syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                                      syncStatus, SYNC_STATUS_FAILURE);
        }

        // Process Incoming Response Msg
        *ppRecvBuff = (char *)(pMsg);

    } while(0);

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_FreeRecvBuff
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/07/2019
 *
 * DESCRIPTION   : This function is used to free a receive buffer 
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_FreeRecvBuff(char *pRecvBuff)
{
    SYNC_STATUS_E               syncStatus        = SYNC_STATUS_SUCCESS;

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_API_Comm_ProcessIncomingMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to process the responses or events from the core
 *
 * IN PARAMS     : pRecvBuff    -- Received Message buffer
 *                 uMsgLen      -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_ProcessIncomingMsg(void *pRecvBuff, uint32_t uMsgLen)
{
    SYNC_STATUS_E                           syncStatus      = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T              *pCoreMsgHdr    = NULL;
    SYNC_TIMING_CORE_MSG_DRIVER_EVENT_T     *pDriverEventMsg= NULL;
    uint8_t                                 uTimingDevId    = 0;
    SYNC_TIMING_BOOL_E                      bInvokeCallback = SYNC_TIMING_FALSE;

    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    if (pCoreMsgHdr->coreMsgType == SYNC_TIMING_CORE_MSG_TYPE_EVENT)
    {
        SYNC_TIMING_INFO(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                        "Received MSG TYPE 0x%x from CORE \n", pCoreMsgHdr->coreMsgType);
        SYNC_TIMING_INFO(gSyncTimingApiClientAppContext.pClientLogModuleId, 
                       "Received RESP/EVENT from CORE = 0x%x\n", pCoreMsgHdr->coreMsgCmd);
        switch(pCoreMsgHdr->coreMsgCmd)
        {
            case SYNC_TIMING_CORE_MSG_DRIVER_EVENT:
                uTimingDevId    = pCoreMsgHdr->uTimingDevId;
                pDriverEventMsg = (SYNC_TIMING_CORE_MSG_DRIVER_EVENT_T *)pRecvBuff;

                if (gSyncTimingApiClientAppContext.pNotifyFunc)
                {
                    if ((gSyncTimingApiClientAppContext.uRegDriverEvents[uTimingDevId] & \
                         pDriverEventMsg->event))
                    {
                            switch(pDriverEventMsg->event)
                            {
                                case SYNC_TIMING_DEVICE_CHIP_EVENT:
#if (SYNC_TIMING_CHIP_TYPE == ARUBA)
                                    if ((gSyncTimingApiClientAppContext.uRegDeviceChipsetEvents[uTimingDevId][0] & \
                                         pDriverEventMsg->eventData.deviceEventInfo.deviceInputEvents))    
                                    {
                                        bInvokeCallback = SYNC_TIMING_TRUE;
                                    }
                                    if ((gSyncTimingApiClientAppContext.uRegDeviceChipsetEvents[uTimingDevId][1] & \
                                         pDriverEventMsg->eventData.deviceEventInfo.devicePllEvents))    
                                    {
                                        bInvokeCallback = SYNC_TIMING_TRUE;
                                    }
                                    if ((gSyncTimingApiClientAppContext.uRegDeviceChipsetEvents[uTimingDevId][2] & \
                                         pDriverEventMsg->eventData.deviceEventInfo.deviceGenEvents))    
                                    {
                                        bInvokeCallback = SYNC_TIMING_TRUE;
                                    }
#else
                                    if ((gSyncTimingApiClientAppContext.uRegDeviceChipsetEvents[uTimingDevId][0] & \
                                         pDriverEventMsg->eventData.deviceEventInfo.deviceEvents))    
                                    {
                                        bInvokeCallback = SYNC_TIMING_TRUE;
                                    }
#endif
                                    break;
                                default:
                                    break;
                            }

                            if (bInvokeCallback)
                            {
                                gSyncTimingApiClientAppContext.pNotifyFunc(uTimingDevId, 
                                                                       pDriverEventMsg->event, 
                                                                       &(pDriverEventMsg->eventData));
                            }
                    }
                }
                break;
            default:
                break;
        }
    }

    return syncStatus;
}




