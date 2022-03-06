/****************************************************************************************/
/**
 *  \defgroup api SYNC TIMING DRIVER API
 *  @brief     This section defines the various Driver APIs for the Timing Chipset.
 *  @{
 *  \defgroup common Initialization and Control
 *  @brief     This section defines the Initialization and Control APIs for the Timing Chipset.
 *  @{
 *  \defgroup common_ds  Initialization and Control Data Structures (Common)
 *   @brief    Initialization and Control Data Structures available for the Timing Chipset.
 *  \defgroup common_api Initialization and Control APIs
 *   @brief    Initialization and Control Functions available for the Timing Chipset.
 *  @{
 *
 *  \file          sync_timing_api_common.c
 *
 *  \details       Implementation file for Timing Driver Common Control APIs that will 
 *                 be used by the application
 *
 *  \date          Created: 06/29/2018
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

#include "sync_timing_api_common.h"
#include "sync_timing_api_internal.h"
#include "sync_timing_core_interface.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Driver_Init
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      06/27/2018 
 * 
 *//**
 *  
 *  \brief           This API initializes the Timing API Driver and establishes 
 *                   communication with the core Timing driver for accessing the chipset
 * 
 * \param[in]        pAppInfo      Application Information 
 *                                 (Optional field for Linux OS where it can be set to NULL,
 *                                  since it can be determined runtime.)
 * \param[in]        pNotifyFunc   Callback function pointer for asynchronous
 *                                 notifications; To be implemented by OEM;
 *                                 The callback function is of the following prototype
 *                                 typedef uint32_t (*SYNC_TIMING_CALLBACK_FN_T)
 *                                                 (uint8_t timingDevId, 
 *                                                  uint32_t callbackEvent, 
 *                                                  void * callbackPayload);
 * \param[in]        pCfgFileName  Config File Name from where the API layer should pick
 *                                 up the configuration information. Future use only;
 * \param[out]       pClientAppId  The Client Application ID returned by the driver
 * \param[out]       pActiveDeviceInfo Active Timing Device Information list
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 * \note             This API must be called prior to any other API function. Cannot be
 *                   called more than once. In such case it will return
 *                   SYNC_STATUS_ALREADY_INITIALIZED.
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Driver_Init(void *pAppInfo,
                                          SYNC_TIMING_CALLBACK_FN_T pNotifyFunc,
                                          char *pCfgFileName,
                                          void **pClientAppId,
                                          SYNC_TIMING_DEVICE_INFO_T *pActiveDeviceInfo
                                         )
{
    SYNC_STATUS_E                         syncStatus           = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T  *pClientAppContext   = NULL;
    uint32_t                              syncOsalStatus       = SYNC_TIMING_OSAL_SUCCESS;
    char                                  appMutexName[SYNC_TIMING_CFG_MAX_NAME_SZ] = {0};
    SYNC_TIMING_BOOL_E                    bCreatedMutex        = SYNC_TIMING_FALSE;
    SYNC_TIMING_BOOL_E                    bHoldingMutex        = SYNC_TIMING_FALSE;
    SYNC_TIMING_APPLN_CFG_T               *pApplnCfg           = NULL;
    uint8_t                               uIdx                 = 0;

    pClientAppContext = Sync_Timing_Internal_API_Ctrl_GetDeviceContext();

    //printf("%s:%d\n", __FUNCTION__, __LINE__);  

    do
    {
        if (NULL == pClientAppContext)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, 
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        if (SYNC_TIMING_TRUE == pClientAppContext->bApiDriverInitialized)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, 
                                      SYNC_STATUS_ALREADY_INITIALIZED);
        }

        pClientAppContext->pClientLogModuleId = SYNC_TIMING_LOG_DEFAULT_HANDLE;
        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        sprintf(appMutexName, "/clientAppMutex_%u", 0);
        syncOsalStatus = Sync_Timing_OSAL_Wrapper_Mutex_Create(&appMutexName[0], 
                                                    &pClientAppContext->pClientAppCtxMutex);
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, 
                            SYNC_STATUS_FAILURE);
        }
        else
        {
            bCreatedMutex = SYNC_TIMING_TRUE;
        }

        syncOsalStatus = Sync_Timing_OSAL_Wrapper_Mutex_Get(pClientAppContext->pClientAppCtxMutex, 
                                                            SYNC_TIMING_OSAL_WAIT_FOREVER);
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, 
                            SYNC_STATUS_NOT_READY);
        }
        else
        {
            bHoldingMutex = SYNC_TIMING_TRUE;
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        pClientAppContext->bApiDriverInitialized = SYNC_TIMING_TRUE;
        pClientAppContext->uClientAppId = pClientAppContext->ApplnInfo.AppInstanceId;
        pClientAppContext->pNotifyFunc = pNotifyFunc;
        for (uIdx = 0; uIdx < SYNC_TIMING_MAX_DEVICES; uIdx++)
        {
            pClientAppContext->uRegDriverEvents[uIdx] = 0;
            Sync_Timing_OSAL_Wrapper_Memset(&pClientAppContext->uRegDeviceChipsetEvents[0],
                                                        0, 4 * sizeof(uint32_t));
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        /* Initialize client side communication with core driver */
        syncStatus = Sync_Timing_Internal_API_Comm_Init(&(pClientAppContext->ApplnInfo), pApplnCfg, 
                                                        pActiveDeviceInfo);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                      "Client %u: Sync_Timing_Internal_API_Comm_Init failed: syncStatus = %u\n",
                      pClientAppContext->uClientAppId, syncStatus);
            pClientAppContext->bApiDriverInitialized = SYNC_TIMING_FALSE;
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, 
                            SYNC_STATUS_FAILURE);
        }

        //Now with driver initialized switch to use the driver core log handle for light weight driver
        pClientAppContext->pClientLogModuleId = pSyncTimingCoreLogHandle;

        *pClientAppId = (void *)pClientAppContext->pClientLogModuleId;
        //printf("%s:%d\n", __FUNCTION__, __LINE__);  
        
    } while(0);

    if (SYNC_TIMING_TRUE == bHoldingMutex)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    /* If the driver ends up not being initialized, back out Mutex creation/allocation. */
    if ((pClientAppContext != NULL) && 
        (SYNC_TIMING_FALSE == pClientAppContext->bApiDriverInitialized))
    {
        if (SYNC_TIMING_TRUE == bCreatedMutex)
        {
            (void)Sync_Timing_OSAL_Wrapper_Mutex_Delete(pClientAppContext->pClientAppCtxMutex);
        }
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Driver_Term
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/06/2018 
 * 
 *//**
 *  
 *  \brief           This API terminates the Timing API Driver and disconnects 
 *                   communication with the core Timing driver for accessing the chipset
 * 
 * \param[in]        pClientAppId  The Client Application ID
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_NOT_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 * \note             This API must be called when the client application is terminating. 
 *                   Cannot be called more than once.
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Driver_Term(void *pClientAppId)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    uint8_t                                uIdx                   = 0;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, 
                                         SYNC_STATUS_SUCCESS);


        /* Terminate client side communication with core driver */
        syncStatus = Sync_Timing_Internal_API_Comm_Term();
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Client 0x%p: Sync_Timing_Internal_API_Comm_Term failed: "
                              "syncStatus = %u ",
                              pClientAppId, syncStatus);
        }

        pClientAppContext->bApiDriverInitialized = SYNC_TIMING_FALSE;
        pClientAppContext->pClientLogModuleId = 0;
        pClientAppContext->pNotifyFunc = NULL;
        for (uIdx = 0; uIdx < SYNC_TIMING_MAX_DEVICES; uIdx++)
        {
            pClientAppContext->uRegDriverEvents[uIdx] = 0;
            Sync_Timing_OSAL_Wrapper_Memset(&pClientAppContext->uRegDeviceChipsetEvents[0],
                                                        0, 4 * sizeof(uint32_t));
        }

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    if (NULL != pClientAppContext->pClientAppCtxMutex)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Delete(pClientAppContext->pClientAppCtxMutex);
    }

    
    return syncStatus;

}


/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Driver_RegisterEvents_Ex
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      05/20/2020 
 * 
 *//**
 *  
 *  \brief           This is the new extended API to be used by the application to register for  
 *                   chipset events from the Timing Device. Example of events - Loss of Lock, etc
 * 
 * \param[in]        timingDevId      The Timing Device ID from which events are desired
 * \param[in]        driverEvent      Driver Event that the application is interested in; Register
 *                                    one event at a time;
 * \param[in]        pDriverEventFilter Driver Event Filter
 *                                      THIS STRUCTURE IS CHIPSET SPECIFIC.
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_NOT_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Driver_RegisterEvents_Ex(uint8_t timingDevId, 
                                    SYNC_TIMING_DEVICE_DRIVER_EVENT_E driverEvent,
                                    SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T *pDriverEventFilter)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_NOT_SUPPORTED;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_REG_EVENT_T       regEventsMsg;
    SYNC_TIMING_CORE_MSG_REG_EVENT_T       *pRecvBuff             = NULL;    

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_SUCCESS);

        /* Send Reg Events command to core for the specified timing device */
        Sync_Timing_OSAL_Wrapper_Memset(&(regEventsMsg), 0, sizeof(regEventsMsg));

        regEventsMsg.msgHdr.coreMsgType        = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        regEventsMsg.msgHdr.coreMsgCmd         = SYNC_TIMING_CORE_MSG_REG_EVENTS;
        regEventsMsg.msgHdr.uClientAppId       = pClientAppContext->uClientAppId;
        regEventsMsg.msgHdr.uTimingDevId       = 0;
        regEventsMsg.msgHdr.bRespReqd          = SYNC_TIMING_TRUE;
        regEventsMsg.driverEvent               = driverEvent;

        if (pDriverEventFilter && driverEvent == SYNC_TIMING_DEVICE_CHIP_EVENT)
        {
            Sync_Timing_OSAL_Wrapper_Memcpy(&(regEventsMsg.driverEventFilter), pDriverEventFilter, 
                                            sizeof(SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T));
        }
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_REG_EVENTS CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&regEventsMsg, 
                                                           sizeof(regEventsMsg), 
                                                           SYNC_TIMING_TRUE, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                           (char **)&pRecvBuff
                                                          );
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
                              "Sync_Timing_API_Driver_RegisterEvents_Ex failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            pClientAppContext->uRegDriverEvents[timingDevId] |= driverEvent;
            if (driverEvent == SYNC_TIMING_DEVICE_CHIP_EVENT)
            {
                Sync_Timing_OSAL_Wrapper_Memset(&pClientAppContext->uRegDeviceChipsetEvents[0],
                                                0, 4 * sizeof(uint32_t));
#if (SYNC_TIMING_CHIP_TYPE == ARUBA)
                pClientAppContext->uRegDeviceChipsetEvents[timingDevId][0] = pDriverEventFilter->deviceEventFilter.deviceInputEvents;
                pClientAppContext->uRegDeviceChipsetEvents[timingDevId][1] = pDriverEventFilter->deviceEventFilter.devicePllEvents;
                pClientAppContext->uRegDeviceChipsetEvents[timingDevId][2] = pDriverEventFilter->deviceEventFilter.deviceGenEvents;
                
#else
                pClientAppContext->uRegDeviceChipsetEvents[timingDevId][0] = pDriverEventFilter->deviceEventFilter.deviceEvents;
#endif
            }
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
 * FUNCTION NAME     Sync_Timing_API_Device_Reset
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      06/29/2018 
 * 
 *//**
 *  
 *  \brief           This API resets the Timing Device
 * 
 * \param[in]        timingDevId      The Timing Device ID for which a reset is desired
 * \param[in]        deviceResetType  Device reset type; One of the values in 
 *                                    SYNC_TIMING_DEVICE_RESET_TYPE_E
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_NOT_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 * \note             For Aruba chipset, there is limited support for this API.
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Device_Reset(uint8_t timingDevId, 
                                          SYNC_TIMING_DEVICE_RESET_TYPE_E deviceResetType)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_DEVICE_RESET_T    deviceResetMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_DEVICE_RESET_T    *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        if (deviceResetType >= SYNC_TIMING_DEVICE_RESET_INVALID)
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send Reset command to core to reset the timing device */
        Sync_Timing_OSAL_Wrapper_Memset(&(deviceResetMsg), 0, sizeof(deviceResetMsg));

        deviceResetMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        deviceResetMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_DEVICE_RESET;
        deviceResetMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        deviceResetMsg.msgHdr.uTimingDevId            = timingDevId;
        deviceResetMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        deviceResetMsg.ResetType                      = deviceResetType;
        
        SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                                    "Sending SYNC_TIMING_CORE_MSG_CHIP_RESET CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&deviceResetMsg, sizeof(deviceResetMsg), 
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
                              "Sync_Timing_API_Device_Reset failed: %d \n", syncStatus);
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
 * FUNCTION NAME     Sync_Timing_API_Device_Download
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      06/04/2020 
 * 
 *//**
 *  
 *  \brief           This API is used to download path or firmware or frequency plan bootfile into
 *                   the RAM of the timing device and boot the part
 * 
 * \param[in]        timingDevId    The Timing Device ID for which an unlock is desired
 * \param[in]        uNumBootFiles  Number of bootfiles to download (incl. patch)
 * \param[in]        pBootFileList  The Name of the bootfiles to download; Must be NULL terminated 
 *                                  and file name must be less that size specified by 
 *                                  SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ. 
 *                                  Patch file must be first in the list
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_NOT_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 * \note             This API is not yet ready to use.
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Device_Download(uint8_t timingDevId, uint32_t uNumBootFiles,
                                              char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ])
{
    SYNC_STATUS_E                           syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                           retStatus               = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T  deviceDownloadMsg       = {0};
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T    *pClientAppContext      = NULL;
    SYNC_TIMING_BOOL_E                      bHoldingMutex           = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T  *pRecvBuff              = NULL;
    uint32_t                                ui                      = 0;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        /* Send download command to core to register the timing device */
        Sync_Timing_OSAL_Wrapper_Memset(&(deviceDownloadMsg), 0, sizeof(SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T));

        deviceDownloadMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        deviceDownloadMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD;
        deviceDownloadMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        deviceDownloadMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        deviceDownloadMsg.uNumBootFiles                  = uNumBootFiles;

        SYNC_TIMING_ALWAYS(pClientAppContext->pClientLogModuleId, 
                           "uNumBootFiles = %u\n", deviceDownloadMsg.uNumBootFiles);

        for (ui = 0; ui < uNumBootFiles; ui++)
        {
            SYNC_TIMING_ALWAYS(pClientAppContext->pClientLogModuleId, 
                               "%s\n", pBootFileList[ui]);
            Sync_Timing_OSAL_Wrapper_Memcpy(&(deviceDownloadMsg.pBootFileList[ui][0]), 
                                            &(pBootFileList[ui][0]), 
                                            SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);
        }

        SYNC_TIMING_ALWAYS(pClientAppContext->pClientLogModuleId, 
                           "uNumBootFiles = %u\n", deviceDownloadMsg.uNumBootFiles);
        for (ui = 0; ui < uNumBootFiles; ui++)
        {
            SYNC_TIMING_ALWAYS(pClientAppContext->pClientLogModuleId, 
                               "%s\n", deviceDownloadMsg.pBootFileList[ui]);            
        }        
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&deviceDownloadMsg, 
                                                            sizeof(SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T), 
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
                              "Sync_Timing_API_Device_Download failed: %d \n", syncStatus);
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
 * FUNCTION NAME     Sync_Timing_API_Device_GetVersionInfo
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      08/06/2018 
 * 
 *//**
 *  
 *  \brief           This API is used to get the version information of the device.
 * 
 * \param[in]        timingDevId            The Timing Device ID for which an unlock is desired
 * \param[out]       pDeviceVersionInfo     Pointer to structure that will contain the version
 *                                          information
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_NOT_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER
 *
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Device_GetVersionInfo(uint8_t timingDevId, 
                                                  SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_DEVICE_VERSION_T  deviceVersionMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_DEVICE_VERSION_T  *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        /* Send Reset command to core to register the timing device */
        Sync_Timing_OSAL_Wrapper_Memset(&(deviceVersionMsg), 0, sizeof(deviceVersionMsg));

        deviceVersionMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        deviceVersionMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_DEVICE_VERSION;
        deviceVersionMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        deviceVersionMsg.msgHdr.uTimingDevId            = timingDevId;
        deviceVersionMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        
        SYNC_TIMING_INFO(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_DEVICE_VERSION CMD to CORE\n");
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&deviceVersionMsg, sizeof(deviceVersionMsg), 
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
                              "Sync_Timing_API_Device_GetVersionInfo failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            /* Copy data read back into the return buffer */
            Sync_Timing_OSAL_Wrapper_Memcpy(pDeviceVersionInfo, 
                                            (const void *)&(pRecvBuff->deviceVersionInfo), 
                                            sizeof(SYNC_TIMING_DEVICE_VERSION_T)
                                           );
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


/** @} common_api */
/** @} common */
/** @} api    */

