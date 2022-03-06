/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_driver.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/27/2018
 *
 * DESCRIPTION        : Core Timing Driver Init/Reset/Term Processing routines
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

#include "sync_timing_config.h"
#include "sync_timing_core_driver.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_core_ctrl.h"
#include "sync_timing_core_communication.h"
#include "sync_timing_core_mem_access.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver_version.h"



/*****************************************************************************************
* Static global variables
*****************************************************************************************/

char gOemStatusMapStr[17][32] = { "OEM_SUCCESS",             //0x00
                                  "OEM_INVALID_PARAMETER",
                                  "OEM_NOT_CONFIGURED",
                                  "OEM_NOT_SUPPORTED",
                                  "OEM_NO_RESOURCE",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "",
                                  "OEM_ERROR"                //0x10
                                };

/*****************************************************************************************
* Functions
*****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Driver_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/27/2018
 *
 * DESCRIPTION   : This function is used to initialize the Core timing driver for a 
 *                 given timing device id
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *                 oemChipIf     - Oem Chip interface to use for this timing device
 *                 pOemData      - The Oem Data
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Driver_Init(uint8_t timingDevId, 
                                           SYNC_TIMING_CHIP_INTERFACE_E ChipIf, 
                                           void *pOemData, SYNC_TIMING_DEVICE_CFG_T *pDeviceCfg)
{
    SYNC_STATUS_E                     syncStatus                    = SYNC_STATUS_SUCCESS;
    uint32_t                          syncOsalStatus                = SYNC_TIMING_OSAL_SUCCESS;

    SYNC_TIMING_OEM_DATAPATH_STATUS_E syncTimingOemDataPathStatus   = SYNC_TIMING_DATAPATH_NOT_CONFIGURED;

    SYNC_TIMING_OEM_RESETCTRLSTATUS_E syncTimingOemResetCtrlStatus  = SYNC_TIMING_RESETCTRL_NOT_CONFIGURED;


    SYNC_TIMING_OEM_IRQCTRLSTATUS_E   syncTimingOemIRQCtrlStatus    = SYNC_TIMING_IRQCTRL_NOT_CONFIGURED;




    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext            = NULL;
    char                              mutexName                     = '0' + timingDevId;
    SYNC_TIMING_BOOL_E                bCreatedMutex                 = SYNC_TIMING_FALSE;
    SYNC_TIMING_BOOL_E                bHoldingMutex                 = SYNC_TIMING_FALSE;
    void                              *pOemHandle                   = NULL;

    pTimingDevContext = Sync_Timing_CORE_Ctrl_GetDeviceContext(timingDevId);

    do
    {
        if (NULL == pTimingDevContext)
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        if (SYNC_TIMING_TRUE == pTimingDevContext->bDriverInitialized)
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_ALREADY_INITIALIZED);
        }

        syncOsalStatus = Sync_Timing_OSAL_Wrapper_Mutex_Create(&mutexName, 
                                                              &pTimingDevContext->pMutex);
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }
        else
        {
            bCreatedMutex = SYNC_TIMING_TRUE;
        }

        syncOsalStatus = Sync_Timing_OSAL_Wrapper_Mutex_Get(pTimingDevContext->pMutex, 
                                                           SYNC_TIMING_OSAL_WAIT_FOREVER);
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_READY);
        }
        else
        {
            bHoldingMutex = SYNC_TIMING_TRUE;
        }

        pTimingDevContext->bDriverInitialized = SYNC_TIMING_TRUE;
        pTimingDevContext->bDeviceInitialized = SYNC_TIMING_FALSE;
        pTimingDevContext->timingDevId = timingDevId;
        pTimingDevContext->ChipIf = ChipIf;

        pTimingDevContext->bPendingDriverEvent = SYNC_TIMING_FALSE;
        pTimingDevContext->pendingDriverEvent = SYNC_TIMING_DEVICE_EVENT_MAX;
        Sync_Timing_OSAL_Wrapper_Memset(&(pTimingDevContext->pendingDriverEventData), 0, 
                                        sizeof(SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T));

        pTimingDevContext->pDeviceCfg = pDeviceCfg;
        pTimingDevContext->pOemData   = pOemData;
                
        pTimingDevContext->syncTimingOemDataPathStatus = 2;  // NOT CONFIGURED

        syncStatus = Sync_Timing_OEM_DATAPATH_Init(timingDevId, pOemData, &pOemHandle, 
                                               &syncTimingOemDataPathStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                             "Sync_Timing_OEM_DATAPATH_Init failed: syncStatus = %d, "
                             "OemDataPathStatus = %s \n", 
                             syncStatus, gOemStatusMapStr[syncTimingOemDataPathStatus]);
            /* Not really initialized after all. */
            pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }
        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                         "Sync_Timing_OEM_DATAPATH_Init succeeded: syncStatus = %d, "
                         "OemDataPathStatus = %s \n", 
                         syncStatus, gOemStatusMapStr[syncTimingOemDataPathStatus]);        
        pTimingDevContext->pOemDataPathHandle = pOemHandle;
        pTimingDevContext->syncTimingOemDataPathStatus = syncTimingOemDataPathStatus;

        syncStatus = Sync_Timing_Internal_CORE_Device_Init(pTimingDevContext);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                             "Sync_Timing_Internal_CORE_Device_Init failed: syncStatus = %d\n",
                             syncStatus);
          
            /* Not really initialized after all. */
            //pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
            pTimingDevContext->bDeviceInitialized = SYNC_TIMING_FALSE;
            syncStatus = SYNC_STATUS_SUCCESS;
            //SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }
        else
        {
            pTimingDevContext->bDeviceInitialized = SYNC_TIMING_TRUE;
        }

        pTimingDevContext->syncTimingOemResetCtrlStatus = 2;

        syncStatus = Sync_Timing_OEM_ResetCtrl_Init(timingDevId, pOemData, &pOemHandle, 
                                                        &syncTimingOemResetCtrlStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
          SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                    "Sync_Timing_OEM_ResetCtrl_Init failed: syncStatus = %d, "
                                        "OemResetCtrlStatus = %s \n", 
                                        syncStatus, gOemStatusMapStr[syncTimingOemResetCtrlStatus]);

          /* Not really initialized after all but we will let it go through since some customers 
             may not have TOD output hardware  */
          //pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
          //SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
          syncStatus = SYNC_STATUS_SUCCESS; 
        }
        else
        {
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                               "Sync_Timing_OEM_ResetCtrl_Init succeeded: syncStatus = %d, "
                               "OemResetCtrlStatus = %s \n", 
                               syncStatus, gOemStatusMapStr[syncTimingOemResetCtrlStatus]);        
        }
        pTimingDevContext->pOemResetCtrlHandle = pOemHandle;
        pTimingDevContext->syncTimingOemResetCtrlStatus = syncTimingOemResetCtrlStatus;





        pTimingDevContext->syncTimingOemIRQCtrlStatus = 2;

        // Clear pending interrupts
        syncStatus = Sync_Timing_Internal_CORE_Device_ClearInterrupts(pTimingDevContext);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                             "Sync_Timing_Internal_CORE_Device_ClearInterrupts failed; "
                             "Check if DEVICE is accessible and is in APPLN mode.\n", 
                             syncStatus);
                      
            /* Not really initialized after all. */
            //pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
            //SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }

        syncStatus = Sync_Timing_OEM_IRQCtrl_Init(timingDevId, pOemData, 
                                                  Sync_Timing_Internal_CORE_Driver_IrqCallback, 
                                                  &pOemHandle, 
                                                  &syncTimingOemIRQCtrlStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_OEM_IRQCtrl_Init failed: syncStatus = %d, "
                              "OemIRQCtrlStatus = %s \n", 
                              syncStatus, gOemStatusMapStr[syncTimingOemIRQCtrlStatus]);
          
            /* Not really initialized after all. */
            //pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
            //SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            syncStatus = SYNC_STATUS_SUCCESS;
            pTimingDevContext->pOemIRQCtrlHandle = NULL;
        }
        else
        {
            pTimingDevContext->pOemIRQCtrlHandle = pOemHandle;

            syncStatus = Sync_Timing_Internal_CORE_Device_SetupIRQ(pTimingDevContext);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                    "Sync_Timing_Internal_CORE_Device_SetupIRQ failed; "
                                    "Check if DEVICE is accessible and is in APPLN mode.\n", 
                                    syncStatus);
                          
                /* Not really initialized after all. */
                //pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
                //SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                syncStatus = SYNC_STATUS_SUCCESS;
                pTimingDevContext->pOemIRQCtrlHandle = NULL;
            }
            else
            {
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_OEM_IRQCtrl_Init and SetupIRQ succeeded: syncStatus = %d, "
                                  "OemIRQCtrlStatus = %s \n", 
                                  syncStatus, gOemStatusMapStr[syncTimingOemIRQCtrlStatus]);
            }
        }
        pTimingDevContext->syncTimingOemIRQCtrlStatus = syncTimingOemIRQCtrlStatus;        


        pTimingDevContext->bfwVersionInfoAvailable = SYNC_TIMING_FALSE;

    } while(0);

    if (SYNC_TIMING_TRUE == bHoldingMutex)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    /* If the driver ends up not being initialized, back out Mutex creation/allocation. */
    if ((pTimingDevContext != NULL) && 
        (SYNC_TIMING_FALSE == pTimingDevContext->bDriverInitialized))
    {
        if (SYNC_TIMING_TRUE == bCreatedMutex)
        {
            (void)Sync_Timing_OSAL_Wrapper_Mutex_Delete(pTimingDevContext->pMutex);
        }
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Driver_Term
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to Terminate the Core timing driver for a 
 *                 given timing device id
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER or 
 *                 SYNC_STATUS_NOT_INITIALIZED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Driver_Term(uint8_t timingDevId)
{
    SYNC_STATUS_E                       syncStatus                   = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext           = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex                = SYNC_TIMING_FALSE;
    SYNC_TIMING_OEM_DATAPATH_STATUS_E   syncTimingOemDataPathStatus  = SYNC_TIMING_DATAPATH_SUCCESS;
    SYNC_TIMING_OEM_RESETCTRLSTATUS_E   syncTimingOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_SUCCESS;
    SYNC_TIMING_OEM_IRQCTRLSTATUS_E     syncTimingOemIRQCtrlStatus   = SYNC_TIMING_IRQCTRL_SUCCESS;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        syncStatus = Sync_Timing_OEM_DATAPATH_Term(pTimingDevContext->pOemDataPathHandle, 
                                                  &syncTimingOemDataPathStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                "Sync_Timing_OEM_DATAPATH_Term failed: syncStatus = %d, "
                                "syncTimingOemDataPathStatus = %d \n", 
                                syncStatus, syncTimingOemDataPathStatus);
        }
        pTimingDevContext->pOemDataPathHandle = NULL;

        syncStatus = Sync_Timing_OEM_ResetCtrl_Term(pTimingDevContext->pOemResetCtrlHandle, 
                                                        &syncTimingOemResetCtrlStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                "Sync_Timing_API_OEM_ResetCtrl_Term failed: syncStatus = %d, "
                                "syncTimingOemResetCtrlStatus = %d \n", 
                                syncStatus, syncTimingOemResetCtrlStatus);
        }
        pTimingDevContext->pOemResetCtrlHandle = NULL;





        syncStatus = Sync_Timing_OEM_IRQCtrl_Term(pTimingDevContext->pOemIRQCtrlHandle, 
                                                        &syncTimingOemIRQCtrlStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                "Sync_Timing_OEM_IRQCtrl_Term failed: syncStatus = %d, "
                                "syncTimingOemIRQCtrlStatus = %d \n", 
                                syncStatus, syncTimingOemIRQCtrlStatus);
        }
        pTimingDevContext->pOemIRQCtrlHandle = NULL;

    } while(0);

    pTimingDevContext->bDriverInitialized = SYNC_TIMING_FALSE;
    pTimingDevContext->bfwVersionInfoAvailable = SYNC_TIMING_FALSE;
    pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_INVALID;

    if (SYNC_TIMING_TRUE == bHoldingMutex)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    if (NULL != pTimingDevContext->pMutex)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Delete(pTimingDevContext->pMutex);
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_Reset
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to reset the timing chipset
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : resetType     - Reset Type desired
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_Reset(uint8_t                          timingDevId, 
                                            SYNC_TIMING_DEVICE_RESET_TYPE_E  resetType)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "DevReset -- resetType = %u \n", resetType);

        syncStatus = Sync_Timing_Internal_CORE_Device_Reset(pTimingDevContext, resetType);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_SetMode
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/30/2018
 *
 * DESCRIPTION   : This function is used to set the current mode of the timing device
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : deviceMode    - Device Mode desired
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_SetMode(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_MODE_E deviceMode)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                          "SetMode -- deviceMode = %u \n", deviceMode);

        syncStatus = Sync_Timing_Internal_CORE_Device_SetMode(pTimingDevContext, deviceMode);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_GetMode
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/30/2018
 *
 * DESCRIPTION   : This function is used to get the current mode of the timing device
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *
 * OUT PARAMS    : pCurrDeviceMode    - Device Mode 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_GetMode(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_MODE_E *pCurrDeviceMode)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        syncStatus = Sync_Timing_Internal_CORE_Device_GetMode(pTimingDevContext, pCurrDeviceMode);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_GetVersionInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/06/2018
 *
 * DESCRIPTION   : This function is used to get the version information
 *
 * IN PARAMS     : timingDevId              - Timing Device Id
 *
 * OUT PARAMS    : pDeviceVersionInfo       - Device Version Information 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_GetVersionInfo(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        syncStatus = Sync_Timing_Internal_CORE_Device_GetVersionInfo(pTimingDevContext, 
                                                                     pDeviceVersionInfo);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_GetBuildInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/06/2018
 *
 * DESCRIPTION   : This function is used to get the build info from the device
 *
 * IN PARAMS     : timingDevId              - Timing Device Id
 *
 * OUT PARAMS    : pVersionBuildInfo        - Version Build Information 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_GetBuildInfo(uint8_t timingDevId, 
                                                     char *pVersionBuildInfo)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if (pVersionBuildInfo)
        {
            Sync_Timing_OSAL_Wrapper_Memset(pVersionBuildInfo, 0, 
                                            SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);
        }
        else
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                       SYNC_STATUS_INVALID_PARAMETER);
        }

        syncStatus =  Sync_Timing_Internal_CORE_Device_GetBuildInfo(pTimingDevContext, 
                                                                    pVersionBuildInfo);

        SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "%s\n", pVersionBuildInfo);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    return syncStatus;

}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_GetInitStatusInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/06/2018
 *
 * DESCRIPTION   : This function is used to get the build info from the device
 *
 * IN PARAMS     : timingDevId              - Timing Device Id
 *
 * OUT PARAMS    : pVersionBuildInfo        - Version Build Information 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_GetInitStatusInfo(uint8_t timingDevId, 
                                                     SYNC_TIMING_DEVICE_INFO_T *pDeviceInfo)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


        pDeviceInfo->bTimingDeviceAccessible[timingDevId] = SYNC_TIMING_FALSE;
        if (pTimingDevContext->syncTimingOemDataPathStatus == SYNC_TIMING_DATAPATH_SUCCESS)
        {
            pDeviceInfo->bTimingDeviceAccessible[timingDevId] = SYNC_TIMING_TRUE;
        }

        pDeviceInfo->bTimingDeviceResetAvailable[timingDevId] = SYNC_TIMING_FALSE;
        if (pTimingDevContext->syncTimingOemResetCtrlStatus == SYNC_TIMING_RESETCTRL_SUCCESS)
        {
            pDeviceInfo->bTimingDeviceResetAvailable[timingDevId] = SYNC_TIMING_TRUE;
        }
        
        pDeviceInfo->bTimingDeviceInterruptsAvailable[timingDevId] = SYNC_TIMING_FALSE;
        if (pTimingDevContext->syncTimingOemIRQCtrlStatus == SYNC_TIMING_IRQCTRL_SUCCESS)
        {
            pDeviceInfo->bTimingDeviceInterruptsAvailable[timingDevId] = SYNC_TIMING_TRUE;
        }



    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    return syncStatus;

}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Device_Download
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/04/2020
 *
 * DESCRIPTION   : This function is used to download patch, firmware and fplan into the timing chipset
 *
 * IN PARAMS     : timingDevId          - Timing Device Id
 *               : uNumBootfiles        - Number of bootfiles to download
 *               : pBootFileList        - Boot File List
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Device_Download(uint8_t timingDevId, uint32_t uNumBootfiles, 
                                               char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ])
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        syncStatus = Sync_Timing_Internal_CORE_Device_Download(pTimingDevContext, uNumBootfiles, 
                                                               pBootFileList);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    
    return syncStatus;
}
                                                
