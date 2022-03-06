/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_communication.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Core Driver Message Processing routines
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

#include "sync_timing_core_communication.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver.h"
#include "sync_timing_core_mem_access.h"
#include "sync_timing_core_clockadj.h"
#include "sync_timing_core_log.h"

#include "sync_timing_api_internal.h"


/*****************************************************************************************
* Static global variables
****************************************************************************************/

static SYNC_TIMING_CORE_DYN_CFG_T gSyncTimingCoreDynCfg                         = {0};


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Send_Events
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/19/2018
 *
 * DESCRIPTION   : This function is used to send events to the clients
 *
 * IN PARAMS     : uTimingDevId - Timing Device ID on which event occured
 *               : timingEvent  - The Timing Event desired to send
 *               : pEventData   - Event Data - 4 words at most - data varies based on the event
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Send_Events(uint8_t  uTimingDevId, uint32_t event,
                                              SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T *pEventData)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_DRIVER_EVENT_T coreMsgDriverEvent      = {0};
    uint32_t                            uAppIndex               = 0;
    SYNC_TIMING_CORE_APPLN_DYN_CFG_T    *pApplnDynCfg           = NULL;                                   
    //SYNC_TIMING_BOOL_E                  bApplnInstanceRunning   = SYNC_TIMING_RUNNING;
    SYNC_TIMING_BOOL_E                  bSendEvent              = SYNC_TIMING_TRUE;

    // Send an event to the module for which this log level mod is desired
    for (uAppIndex = 0; uAppIndex < SYNC_TIMING_MAX_APPLICATION_INSTANCES; uAppIndex++)
    {
        pApplnDynCfg = &(gSyncTimingCoreDynCfg.ApplnDynCfg[uAppIndex]);

        if (pApplnDynCfg->bClientValid == SYNC_TIMING_TRUE)
        {

            if (event & pApplnDynCfg->regDriverEvent)
            {
                bSendEvent = SYNC_TIMING_TRUE;
            }                        
        
            if (event == SYNC_TIMING_DEVICE_CHIP_EVENT)
            {
                bSendEvent = SYNC_TIMING_FALSE;
#if (SYNC_TIMING_CHIP_TYPE == ARUBA)
                if (pApplnDynCfg->regDeviceChipsetEvent.deviceInputEvents & 
                     pEventData->deviceEventInfo.deviceInputEvents)    
                {
                    bSendEvent = SYNC_TIMING_TRUE;
                }

                if (pApplnDynCfg->regDeviceChipsetEvent.devicePllEvents & 
                     pEventData->deviceEventInfo.devicePllEvents) 
                {
                    bSendEvent = SYNC_TIMING_TRUE;
                }

                if (pApplnDynCfg->regDeviceChipsetEvent.deviceGenEvents & 
                     pEventData->deviceEventInfo.deviceGenEvents)  
                {
                    bSendEvent = SYNC_TIMING_TRUE;
                }
#else
                if (pApplnDynCfg->regDeviceChipsetEvent.deviceEvents & 
                     pEventData->deviceEventInfo.deviceEvents)    
                {
                    bSendEvent = SYNC_TIMING_TRUE;
                }
#endif                        
            }

            if (bSendEvent)
            {
                // Send the event 
                coreMsgDriverEvent.msgHdr.uTimingDevId = uTimingDevId;
                coreMsgDriverEvent.msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                coreMsgDriverEvent.msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_EVENT;
                coreMsgDriverEvent.msgHdr.coreMsgCmd = SYNC_TIMING_CORE_MSG_DRIVER_EVENT;
                coreMsgDriverEvent.msgHdr.uClientAppId = pApplnDynCfg->uClientAppId;
                coreMsgDriverEvent.event = event;
                Sync_Timing_OSAL_Wrapper_Memcpy(&(coreMsgDriverEvent.eventData), pEventData, 
                                          sizeof(SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T));
                
                syncStatus = Sync_Timing_Internal_API_Comm_ProcessIncomingMsg((void *)&coreMsgDriverEvent, 
                                                               sizeof(coreMsgDriverEvent));
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_SendMsg failed: "
                                      "syncStatus = %d\n",syncStatus);
                }
                else
                {
                    SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, 
                                    "SENT EVENT to CLIENT = 0x%04x\n", 
                                    coreMsgDriverEvent.msgHdr.coreMsgCmd);
                }
            }
            else
            {
                SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, 
                                  "APPLICATION INFO : %u: not registered for events; "
                                  "Skip sending event.\n", 
                                  uAppIndex);
            }
        }

    }
    return syncStatus;

}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_SendMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to send responses or events to client
 *
 * IN PARAMS     : uClientID    -- Client ID
 *                 pMsg         -- Response or Event Message buffer
 *                 uMsgLen      -- Length of message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_SendMsg(uint32_t uAppIndex,
                                                char *pMsg, uint32_t uMsgLen
                                               )
{
    SYNC_STATUS_E             syncStatus        = SYNC_STATUS_SUCCESS;
    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessIncomingMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/03/2018
 *
 * DESCRIPTION   : This function is used to process the incoming messages from the clients
 *
 * IN PARAMS     : pRecvBuff    -- Received Message buffer
 *                 uMsgLen      -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessIncomingMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T          *pCoreMsgHdr        = NULL;
    SYNC_TIMING_DRIVER_CORE_CFG_T       *pCoreDriverCfg     = NULL;
    SYNC_TIMING_CORE_MSG_INIT_T         *pCoreInitMsg       = NULL;
    uint32_t                            uIdx                = 0;
    SYNC_TIMING_CORE_APPLN_DYN_CFG_T    *pAppDynCfg           = NULL;    

    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    //printf("%s:%d (%u)\n", __FUNCTION__, __LINE__, pCoreMsgHdr->coreMsgCmd);

    //SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE,"Received MSG 0x%04x From Client %u for TimingDevId %u\n", 
    //                  pCoreMsgHdr->coreMsgCmd, 
    //                  pCoreMsgHdr->uClientAppId,
    //                  pCoreMsgHdr->uTimingDevId);

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_INIT:
            {
                syncStatus = Sync_Timing_CORE_Driver_MainInit();
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_CORE_Driver_MainInit failed: "
                                      "syncStatus = %d\n",syncStatus);
                }

                /* Get Core Config */
                syncStatus = Sync_Timing_CFG_GetCoreCfg((void **)&pCoreDriverCfg);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_CFG_GetCoreCfg failed: %d \n", syncStatus);
                    SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                }

                pCoreInitMsg = (SYNC_TIMING_CORE_MSG_INIT_T *)pRecvBuff;
                // Populate the active timing devices list for the application
                pCoreInitMsg->activeDeviceInfo.uNumActiveTimingDevices = 
                                                                   pCoreDriverCfg->uNumTimingDevices;
                for (uIdx = 0; uIdx < pCoreDriverCfg->uNumTimingDevices; uIdx++)
                {
                     Sync_Timing_OSAL_Wrapper_Memcpy(
                         &(pCoreInitMsg->activeDeviceInfo.timingDeviceName[uIdx][0]),
                         &(pCoreDriverCfg->TimingDeviceCfg[uIdx].deviceName[0]),
                         Sync_Timing_OSAL_Wrapper_Strlen(
                                             &(pCoreDriverCfg->TimingDeviceCfg[uIdx].deviceName[0]))
                        );
                     pCoreInitMsg->activeDeviceInfo.timingDeviceId[uIdx] = 
                                                     pCoreDriverCfg->TimingDeviceCfg[uIdx].uDeviceId;
                     syncStatus = Sync_Timing_CORE_Device_GetInitStatusInfo(uIdx, &(pCoreInitMsg->activeDeviceInfo));
                }
                
                pAppDynCfg = &(gSyncTimingCoreDynCfg.ApplnDynCfg[uCurrentAppIndex]);
                pAppDynCfg->bClientValid = SYNC_TIMING_TRUE;
                
                break;
            }
            break;
        case SYNC_TIMING_CORE_MSG_TERM:
            {
                syncStatus = Sync_Timing_CORE_Driver_MainTerm();
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_CORE_Driver_MainTerm failed: "
                                      "syncStatus = %d\n",syncStatus);
                }

                break;
            }
            break;
        case SYNC_TIMING_CORE_MSG_REG_EVENTS:
        case SYNC_TIMING_CORE_MSG_DEVICE_RESET:
        case SYNC_TIMING_CORE_MSG_DEVICE_VERSION:
        case SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD:    
            syncStatus = Sync_Timing_Internal_CORE_ProcessDeviceCtrlMsg(uCurrentAppIndex, 
                                                                        pRecvBuff, uMsgLen);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Internal_CORE_ProcessDeviceCtrlMsg failed: "
                                  "syncStatus = %d\n",syncStatus);
            }
            break;

        case SYNC_TIMING_CORE_MSG_READ_DIRECT:
        case SYNC_TIMING_CORE_MSG_READ_INDIRECT:
        case SYNC_TIMING_CORE_MSG_WRITE_DIRECT:
        case SYNC_TIMING_CORE_MSG_WRITE_INDIRECT:
        case SYNC_TIMING_CORE_MSG_API_COMMAND:    
            syncStatus = Sync_Timing_Internal_CORE_ProcessMemAccessMsg(uCurrentAppIndex, 
                                                                     pRecvBuff, uMsgLen);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                     "Sync_Timing_Internal_CORE_ProcessMemAccessMsg failed: "
                                     "syncStatus = %d\n",syncStatus);
            }
            break;
//#ifdef SYNC_TIMING_ACCUTIME_SUPPORTED 
            syncStatus = Sync_Timing_Internal_CORE_ProcessClkCtrlMsg(uCurrentAppIndex, 
                                                                     pRecvBuff, uMsgLen);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Internal_CORE_ProcessClkCtrlMsg failed: "
                                  "syncStatus = %d\n",syncStatus);
            }
            break;
//#endif
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT:    
        case SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT:
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS:
            syncStatus = Sync_Timing_Internal_CORE_ProcessClkAdjMsg(uCurrentAppIndex, 
                                                                     pRecvBuff, uMsgLen);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Internal_CORE_ProcessClkAdjMsg failed: "
                                  "syncStatus = %d\n",syncStatus);
            }
            break;            
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE:
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE:
            syncStatus = Sync_Timing_Internal_CORE_ProcessDebugMsg(uCurrentAppIndex, 
                                                                        pRecvBuff, uMsgLen);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Internal_CORE_ProcessDebugMsg failed: "
                                  "syncStatus = %d\n",syncStatus);
            }

            break;
        default:
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                              "Unknown message command (0x%0x) received.\n ",
                              pCoreMsgHdr->coreMsgCmd);            
            break;
    }

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessDeviceCtrlMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/02/2018
 *
 * DESCRIPTION   : This function is used to process the incoming Device Ctrl API messages from 
 *                 the clients
 *
 * IN PARAMS     : uCurrentAppIndex -- client app index
 *                 pRecvBuff        -- Received Message buffer
 *                 uMsgLen          -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessDeviceCtrlMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{
    SYNC_TIMING_CORE_MSG_REG_EVENT_T          *pRegEventsMsg        = NULL;
    SYNC_TIMING_CORE_MSG_DEVICE_RESET_T       *pResetMsg            = NULL;
    SYNC_TIMING_CORE_MSG_DEVICE_VERSION_T     *pDeviceVersionMsg    = NULL;
    SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T    *pDeviceDownloadMsg   = NULL;

    SYNC_STATUS_E                             syncStatus            = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T                *pCoreMsgHdr          = NULL;
    uint32_t                                  uAppIdx               = 0;
    SYNC_TIMING_CORE_APPLN_DYN_CFG_T          *pAppDynCfg           = NULL;
    uint32_t                                  ui                    = 0;

    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_REG_EVENTS:
            pRegEventsMsg = (SYNC_TIMING_CORE_MSG_REG_EVENT_T *)pRecvBuff;

            for (uAppIdx = 0; uAppIdx < SYNC_TIMING_MAX_APPLICATION_INSTANCES; uAppIdx++)
            {
                if (uAppIdx == uCurrentAppIndex)
                {
                    pAppDynCfg = &(gSyncTimingCoreDynCfg.ApplnDynCfg[uAppIdx]);

                    if (pAppDynCfg->bClientValid == SYNC_TIMING_TRUE)
                    {

                        pAppDynCfg->regDriverEvent |= pRegEventsMsg->driverEvent;
                        
                        if (pRegEventsMsg->driverEvent == SYNC_TIMING_DEVICE_CHIP_EVENT)
                        {
                            Sync_Timing_OSAL_Wrapper_Memcpy(
                                        &(pAppDynCfg->regDeviceChipsetEvent),
                                        &(pRegEventsMsg->driverEventFilter.deviceEventFilter),
                                        sizeof(SYNC_TIMING_DEVICE_EVENT_INFO_T));
                        }

                    }
                }
            }

            pRegEventsMsg->msgHdr.reqStatus = syncStatus;
            pRegEventsMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;

            /* Send response only if client requested it */
            if (pRegEventsMsg->msgHdr.bRespReqd)
            {
                pRegEventsMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pRegEventsMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_SendMsg failed: "
                                      "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, 
                                  "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                  pRegEventsMsg->msgHdr.coreMsgCmd);
            }
            break;
        case SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD:
            pDeviceDownloadMsg = (SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T *)pRecvBuff;
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "uNumBootFiles = %u\n",
                                                         pDeviceDownloadMsg->uNumBootFiles);
            for (ui = 0; ui < pDeviceDownloadMsg->uNumBootFiles; ui++)
            {
                if (pDeviceDownloadMsg->pBootFileList[ui])
                {
                    SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Boot File[%u] = %s\n", ui,
                                                                 pDeviceDownloadMsg->pBootFileList[ui]);
                }
            }

#if SYNC_TIMING_HARDWARE_PRESENT
            syncStatus = Sync_Timing_CORE_Device_Download(pDeviceDownloadMsg->msgHdr.uTimingDevId, 
                                                          pDeviceDownloadMsg->uNumBootFiles,
                                                          pDeviceDownloadMsg->pBootFileList);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_CORE_Device_Download failed: "
                                  "syncStatus = %d\n",syncStatus);
            }
#endif

            pDeviceDownloadMsg->msgHdr.reqStatus = syncStatus;
            pDeviceDownloadMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;

            /* Send response only if client requested it */
            if (pDeviceDownloadMsg->msgHdr.bRespReqd)
            {
                pDeviceDownloadMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pDeviceDownloadMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_SendMsg failed: "
                                      "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, 
                                  "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                  pDeviceDownloadMsg->msgHdr.coreMsgCmd);
            }
            break;

        case SYNC_TIMING_CORE_MSG_DEVICE_RESET:
            pResetMsg = (SYNC_TIMING_CORE_MSG_DEVICE_RESET_T *)pRecvBuff;
            SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "pMsgData->ResetType = %u\n", 
                                                          pResetMsg->ResetType);

#if SYNC_TIMING_HARDWARE_PRESENT
            syncStatus = Sync_Timing_CORE_Device_Reset(pResetMsg->msgHdr.uTimingDevId, 
                                                           pResetMsg->ResetType);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                       "Sync_Timing_CORE_Device_Reset failed: "
                                       "syncStatus = %d\n",syncStatus);
            }
#endif
            
            pResetMsg->msgHdr.reqStatus = syncStatus;
            pResetMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;

            /* Send response only if client requested it */
            if (pResetMsg->msgHdr.bRespReqd)
            {
                pResetMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pResetMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                         "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pResetMsg->msgHdr.coreMsgCmd);
            }
            break;
        case SYNC_TIMING_CORE_MSG_DEVICE_VERSION:
            pDeviceVersionMsg = (SYNC_TIMING_CORE_MSG_DEVICE_VERSION_T *)pRecvBuff;
            
#if SYNC_TIMING_HARDWARE_PRESENT
            syncStatus = Sync_Timing_CORE_Device_GetVersionInfo(pDeviceVersionMsg->msgHdr.uTimingDevId, 
                                                           &(pDeviceVersionMsg->deviceVersionInfo));
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                       "Sync_Timing_CORE_Device_GetVersionInfo failed: "
                                       "syncStatus = %d\n",syncStatus);
            }
#endif
            
            pDeviceVersionMsg->msgHdr.reqStatus = syncStatus;
            pDeviceVersionMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;

            /* Send response only if client requested it */
            if (pDeviceVersionMsg->msgHdr.bRespReqd)
            {
                pDeviceVersionMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pDeviceVersionMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                         "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pDeviceVersionMsg->msgHdr.coreMsgCmd);
            }
            break;
        default:
            break;
    }
    
    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessMemAccessMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/02/2018
 *
 * DESCRIPTION   : This function is used to process the incoming Mem Access API messages from 
 *                 the clients
 *
 * IN PARAMS     : uCurrentAppIndex -- client app index
 *                 pRecvBuff        -- Received Message buffer
 *                 uMsgLen          -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessMemAccessMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T *pMemAccessMsg = (SYNC_TIMING_CORE_MSG_MEM_ACCESS_T *)pRecvBuff;
    SYNC_TIMING_CORE_MSG_API_CMD_T    *pApiCmdMsg    = (SYNC_TIMING_CORE_MSG_API_CMD_T *)pRecvBuff;
    SYNC_STATUS_E                     syncStatus     = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T        *pCoreMsgHdr   = NULL;
    
    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;
    
#if SYNC_TIMING_HARDWARE_PRESENT
    if (pCoreMsgHdr->coreMsgCmd == SYNC_TIMING_CORE_MSG_READ_DIRECT)
    {
        syncStatus = Sync_Timing_CORE_Mem_ReadDirect(pMemAccessMsg->msgHdr.uTimingDevId, 
                                                     pMemAccessMsg->memAddr, 
                                                     pMemAccessMsg->len,
                                                     &(pMemAccessMsg->data[0])
                                                    );
    }
    else if (pCoreMsgHdr->coreMsgCmd == SYNC_TIMING_CORE_MSG_READ_INDIRECT)
    {
        syncStatus = Sync_Timing_CORE_Mem_ReadInDirect(pMemAccessMsg->msgHdr.uTimingDevId, 
                                                       pMemAccessMsg->memAddr, 
                                                       pMemAccessMsg->len,
                                                       &(pMemAccessMsg->data[0])
                                                      );
    }
    else if (pCoreMsgHdr->coreMsgCmd == SYNC_TIMING_CORE_MSG_WRITE_DIRECT)
    {
        syncStatus = Sync_Timing_CORE_Mem_WriteDirect(pMemAccessMsg->msgHdr.uTimingDevId, 
                                                      pMemAccessMsg->memAddr, 
                                                      pMemAccessMsg->len,
                                                      &(pMemAccessMsg->data[0])
                                                     );
    }
    else if (pCoreMsgHdr->coreMsgCmd == SYNC_TIMING_CORE_MSG_WRITE_INDIRECT)
    {
        syncStatus = Sync_Timing_CORE_Mem_WriteInDirect(pMemAccessMsg->msgHdr.uTimingDevId, 
                                                        pMemAccessMsg->memAddr, 
                                                        pMemAccessMsg->len,
                                                        &(pMemAccessMsg->data[0])
                                                       );
    }
    else
    {
        syncStatus = Sync_Timing_CORE_Mem_APICommand(pApiCmdMsg->msgHdr.uTimingDevId, 
                                                     &pApiCmdMsg->command[0], 
                                                     pApiCmdMsg->uCmdLen,
                                                     pApiCmdMsg->uExpCmdRespLen,
                                                     &(pApiCmdMsg->cmdResponse[0]),
                                                     &pApiCmdMsg->uCmdStatus);
    }

    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                          "Sync_Timing_CORE_Mem_Call failed: "
                          "syncStatus = %d\n",syncStatus);
    }
#endif
    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_READ_DIRECT:
        case SYNC_TIMING_CORE_MSG_READ_INDIRECT:
        case SYNC_TIMING_CORE_MSG_WRITE_DIRECT:
        case SYNC_TIMING_CORE_MSG_WRITE_INDIRECT:
            pMemAccessMsg->msgHdr.reqStatus = syncStatus;
            pMemAccessMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pMemAccessMsg->msgHdr.bRespReqd)
            {
                pMemAccessMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pMemAccessMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_SendMsg failed: "
                                      "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, 
                                  "SENT RESP_MSG to CLIENT = 0x%04x; data = %x %x %x %x \n", 
                                  pMemAccessMsg->msgHdr.coreMsgCmd,
                                  pMemAccessMsg->data[0],
                                  pMemAccessMsg->data[1],
                                  pMemAccessMsg->data[2],
                                  pMemAccessMsg->data[3]);
            }
            break;
        case SYNC_TIMING_CORE_MSG_API_COMMAND:
            pApiCmdMsg->msgHdr.reqStatus = syncStatus;
            pApiCmdMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pApiCmdMsg->msgHdr.bRespReqd)
            {
                pApiCmdMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pApiCmdMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_SendMsg failed: "
                                      "syncStatus = %d\n",syncStatus);
                }
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, 
                                  "SENT API_CMD_RESP_MSG to CLIENT = 0x%04x; data = %x %x %x %x \n", 
                                  pApiCmdMsg->msgHdr.coreMsgCmd,
                                  pApiCmdMsg->cmdResponse[0]);
            }            
            break;
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessDebugMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/02/2018
 *
 * DESCRIPTION   : This function is used to process the incoming debug API messages from 
 *                 the clients
 *
 * IN PARAMS     : uCurrentAppIndex -- client app index
 *                 pRecvBuff        -- Received Message buffer
 *                 uMsgLen          -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessDebugMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{
    SYNC_TIMING_CORE_MSG_DEBUG_MODE_T       *pDeviceModeMsg     = NULL;

    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T              *pCoreMsgHdr        = NULL;
    
    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE:
            pDeviceModeMsg = (SYNC_TIMING_CORE_MSG_DEBUG_MODE_T *)pRecvBuff;
            syncStatus = Sync_Timing_CORE_Device_SetMode(pDeviceModeMsg->msgHdr.uTimingDevId, 
                                                     pDeviceModeMsg->deviceMode);
            break;
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE:
            pDeviceModeMsg = (SYNC_TIMING_CORE_MSG_DEBUG_MODE_T *)pRecvBuff;
            syncStatus = Sync_Timing_CORE_Device_GetMode(pDeviceModeMsg->msgHdr.uTimingDevId, 
                                                     &(pDeviceModeMsg->deviceMode));
            break;
        default:
            break;
    }
    
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_CORE_DEBUG Call failed: "
                          "syncStatus = %d\n",syncStatus);
    }

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE:
        case SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE:
            pDeviceModeMsg->msgHdr.reqStatus = syncStatus;
            pDeviceModeMsg->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pDeviceModeMsg->msgHdr.bRespReqd)
            {
                pDeviceModeMsg->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pDeviceModeMsg, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                else
                {
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pDeviceModeMsg->msgHdr.coreMsgCmd);
                }
            }
            break;
        default:
            break;
    }

    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessClkAdjMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/16/2018
 *
 * DESCRIPTION   : This function is used to process the incoming clk adj messages from 
 *                 the clients
 *
 * IN PARAMS     : uCurrentAppIndex -- client app index
 *                 pRecvBuff        -- Received Message buffer
 *                 uMsgLen          -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessClkAdjMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{

    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      *pClkAdjSetPllInput  = NULL;
    SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T      *pClkAdjGetPllInput  = NULL;    
    SYNC_TIMING_CORE_MSG_CLKADJ_STATUS_T        *pClkAdjStatus       = NULL;

    SYNC_STATUS_E                               syncStatus           = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T                  *pCoreMsgHdr         = NULL;
    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT:
            pClkAdjGetPllInput = (SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T *)pRecvBuff;
            syncStatus = Sync_Timing_CORE_ClockAdj_GetPLLInput(
                                                           pClkAdjGetPllInput->msgHdr.uTimingDevId, 
                                                           pClkAdjGetPllInput->uPLLId,
                                                           &(pClkAdjGetPllInput->pllInputSelect));
            break;
        case SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT:
            pClkAdjSetPllInput = (SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T *)pRecvBuff;
            syncStatus = Sync_Timing_CORE_ClockAdj_SetPLLInput(
                                                           pClkAdjSetPllInput->msgHdr.uTimingDevId, 
                                                           pClkAdjSetPllInput->uPLLId,
                                                           pClkAdjSetPllInput->pllInputSelect);
            break;
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS:
            pClkAdjStatus = (SYNC_TIMING_CORE_MSG_CLKADJ_STATUS_T *)pRecvBuff;
            syncStatus = Sync_Timing_CORE_ClockAdj_GetCurrentStatus(
                                                            pClkAdjStatus->msgHdr.uTimingDevId, 
                                                            pClkAdjStatus->statusType,
                                                            &(pClkAdjStatus->status));
            break;
        default: 
            // TBD - Unrecognized command
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "TBD - Unrecognized command received %u\n",
                              pCoreMsgHdr->coreMsgCmd);
            break;
    }

    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_CORE_ClockAdj Call failed: "
                               "syncStatus = %d\n",syncStatus);
    }

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT:
            pClkAdjGetPllInput->msgHdr.reqStatus = syncStatus;
            pClkAdjGetPllInput->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pClkAdjGetPllInput->msgHdr.bRespReqd)
            {
                pClkAdjGetPllInput->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pClkAdjGetPllInput, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                else
                {
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pClkAdjGetPllInput->msgHdr.coreMsgCmd);
                }
            }
            break;            
        case SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT:
            pClkAdjSetPllInput->msgHdr.reqStatus = syncStatus;
            pClkAdjSetPllInput->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pClkAdjSetPllInput->msgHdr.bRespReqd)
            {
                pClkAdjSetPllInput->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pClkAdjSetPllInput, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                else
                {
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pClkAdjSetPllInput->msgHdr.coreMsgCmd);
                }
            }
            break;
        case SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS:
            pClkAdjStatus->msgHdr.reqStatus = syncStatus;
            pClkAdjStatus->msgHdr.coreMsgType = SYNC_TIMING_CORE_MSG_TYPE_RESP;
            
            /* Send response only if client requested it */
            if (pClkAdjStatus->msgHdr.bRespReqd)
            {
                pClkAdjStatus->msgHdr.bRespReqd = SYNC_TIMING_FALSE;
                syncStatus = Sync_Timing_Internal_CORE_SendMsg(uCurrentAppIndex, 
                                                               (char *)pClkAdjStatus, uMsgLen);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_Internal_CORE_SendMsg failed: "
                                         "syncStatus = %d\n",syncStatus);
                }
                else
                {
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "SENT RESP_MSG to CLIENT = 0x%04x\n", 
                                                              pClkAdjStatus->msgHdr.coreMsgCmd);
                }
            }
            break;  
        default:
            // TBD - Unrecognized command
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "TBD - Unrecognized command received %u\n",
                              pCoreMsgHdr->coreMsgCmd);
            break;
    }
    
    return syncStatus;
}
                                                          
//#ifdef SYNC_TIMING_ACCUTIME_SUPPORTED 
/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ProcessClkCtrlMsg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 03/12/2021
 *
 * DESCRIPTION   : This function is used to process the incoming clk control messages from 
 *                 the clients
 *
 * IN PARAMS     : uCurrentAppIndex -- client app index
 *                 pRecvBuff        -- Received Message buffer
 *                 uMsgLen          -- Length of received message
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessClkCtrlMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen)
{

    SYNC_STATUS_E                               syncStatus           = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_HDR_T                  *pCoreMsgHdr         = NULL;

    pCoreMsgHdr = (SYNC_TIMING_CORE_MSG_HDR_T *)pRecvBuff;

    switch(pCoreMsgHdr->coreMsgCmd)
    {
        default: 
            // TBD - Unrecognized command
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "TBD - Unrecognized command received %u\n",
                              pCoreMsgHdr->coreMsgCmd);
            break;
    }

    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Sync_Timing_CORE_ClockAdj Call failed: "
                               "syncStatus = %d\n",syncStatus);
    }

    switch(pCoreMsgHdr->coreMsgCmd)
    {

        default:
            // TBD - Unrecognized command
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "TBD - Unrecognized command received %u\n",
                              pCoreMsgHdr->coreMsgCmd);
            break;
    }
    
    return syncStatus;
}
//#endif



