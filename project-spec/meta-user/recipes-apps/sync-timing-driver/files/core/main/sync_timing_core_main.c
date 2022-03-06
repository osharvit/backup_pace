/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_main.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Core Driver Main Entry Process
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
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_cfg_parser.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_oem_common.h"
#include "sync_timing_core_communication.h"
#include "sync_timing_core_driver.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver_version.h"
#include "sync_timing_core_log.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/
//#define SYNC_TIMING_CORE_DRIVER_RESETS_TIMING_DEVICE

/*****************************************************************************************
* Static global variables
*****************************************************************************************/
void *pSyncTimingCoreLogHandle = 0;

/*****************************************************************************************
* Functions
*****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Driver_MainInit()
{
    SYNC_STATUS_E                 syncStatus        = SYNC_STATUS_SUCCESS;
    uint32_t                      uTimingDevId      = 0;
    uint32_t                      uIdx              = 0;
    SYNC_TIMING_GLOBAL_CFG_T      *pGlobalCfg       = NULL;
    SYNC_TIMING_OEM_CFG_DATA_T    *pOemCfg          = NULL;
    SYNC_TIMING_DRIVER_CORE_CFG_T *pCoreCfg         = NULL;
    SYNC_TIMING_LOG_STATUS_E      LogStatus         = SYNC_TIMING_LOG_SUCCESS;
    SYNC_TIMING_CHIP_INTERFACE_E  ChipIf; 
    SYNC_TIMING_DEVICE_VERSION_T  deviceVersionInfo;
    char                          fwBuildInfo[SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE] = {0};
    char                          driverBuildInfo[SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE] = {0};
    SYNC_TIMING_LOG_GLOBAL_INFO_T LogGlobalInfo[SYNC_TIMING_MAX_GLOBAL_LOG_MODULES];
    uint32_t                      NumRegLogModules  = 0;
    SYNC_TIMING_DEVICE_MODE_E     currDeviceMode    = SYNC_TIMING_DEVICE_MODE_INVALID;

    do
    {
        //printf("%s:%d\n", __FUNCTION__, __LINE__);

#ifdef SYNC_TIMING_LOG_SUPPORTED
        syncStatus = Sync_Timing_CORE_Log_GlobalInit();
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                              "Sync_Timing_CORE_Log_GlobalInit failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }
#endif
        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        /* Load Default Cfg data */
        syncStatus = Sync_Timing_CFG_LoadDefaultCfg();
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                              "Sync_Timing_Core_LoadDefaultCfg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        /* Process CFG File for core and application */
        syncStatus = Sync_Timing_CFG_ParseCfg("core");
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                "Sync_Timing_Core_ParseConfig (core) failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        /* Get Global Config */
        syncStatus = Sync_Timing_CFG_GetGlobalCfg((void **)&pGlobalCfg);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE,   
                                "Sync_Timing_Core_GetGlobalCfg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);

        /* Get Core Config */
        syncStatus = Sync_Timing_CFG_GetCoreCfg((void **)&pCoreCfg);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE,   
                                "Sync_Timing_CFG_GetCoreCfg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);
        //fflush(stdout);

        SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE, "Sync Timing CFG Parser Initialized.\n");

#ifdef SYNC_TIMING_LOG_SUPPORTED
        /* Initialize Logging for Core Module */
        syncStatus = Sync_Timing_LOG_Register(&(pCoreCfg->LogCfg), &LogStatus, 
                                              &pSyncTimingCoreLogHandle);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                "Sync_Timing_LOG_Register failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }
        else
        {
            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, "Sync Timing Log Initialized.\n");
        }

        syncStatus = Sync_Timing_LOG_SetTraceLevel(pSyncTimingCoreLogHandle, 
                                                   SYNC_TIMING_LOG_CFG_FILTER_STDOUT, 
                                                   SYNC_TIMING_LOG_LEVEL_WARNING);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_LOG_SetTraceLevel STDOUT failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }

        syncStatus = Sync_Timing_LOG_SetTraceLevel(pSyncTimingCoreLogHandle, 
                                                   SYNC_TIMING_LOG_CFG_FILTER_FILE, 
                                                   SYNC_TIMING_LOG_LEVEL_DEBUG);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_LOG_SetTraceLevel FILE output failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                         "Sync Timing Driver Build Time = %lu\n", 
                         (unsigned long) SYNC_TIMING_DRIVER_BUILD_TIME);

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Sync Timing CHIP TYPE = %lu\n", 
                                                   SYNC_TIMING_CHIP_TYPE);

        SYNC_TIMING_CRITICAL(pSyncTimingCoreLogHandle, "Log Test Critical\n");
        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle,    "Log Test Error\n");
        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle,  "Log Test Warning\n");
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle,    "Log Test Info1\n");
        SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle,    "Log Test Info2\n");
        SYNC_TIMING_INFO3(pSyncTimingCoreLogHandle,    "Log Test Info3\n");
        SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle,    "Log Test Debug\n");
        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,   "Log Test Always\n");

#ifdef SYNC_TIMING_LOG_SUPPORTED

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Sync Timing Core Setting Log levels "
                                                     "StdOut = %u: FileOut = %u\n", 
                                                   pCoreCfg->LogCfg.logToStdoutFilter.logTraceLevel,
                                                   pCoreCfg->LogCfg.logToFileFilter.logTraceLevel);

        syncStatus = Sync_Timing_LOG_SetTraceLevel(pSyncTimingCoreLogHandle, 
                                                   SYNC_TIMING_LOG_CFG_FILTER_STDOUT,
                                                   pCoreCfg->LogCfg.logToStdoutFilter.logTraceLevel);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_LOG_SetTraceLevel STDOUT failed: %d \n", syncStatus);
        }
    
        syncStatus = Sync_Timing_LOG_SetTraceLevel(pSyncTimingCoreLogHandle, 
                                                   SYNC_TIMING_LOG_CFG_FILTER_FILE,
                                                   pCoreCfg->LogCfg.logToFileFilter.logTraceLevel);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_LOG_SetTraceLevel FILE output failed: %d \n", syncStatus);
        }
#endif

        Sync_Timing_OSAL_Wrapper_SleepMS(50);
#endif

        /* Get OEM Data and initialize core driver for each timing chipset device */
        for (uTimingDevId = 0; uTimingDevId < pCoreCfg->uNumTimingDevices; uTimingDevId++)
        {
            syncStatus = Sync_Timing_CFG_GetOemCfg(uTimingDevId, 
                                                    (void **)&pOemCfg, &ChipIf);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Core_GetOemCfgData failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }
            
#if SYNC_TIMING_HARDWARE_PRESENT
            syncStatus = Sync_Timing_CORE_Driver_Init(uTimingDevId, ChipIf, pOemCfg, 
                                                      &(pCoreCfg->TimingDeviceCfg[uTimingDevId]));
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_CORE_Driver_Init failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }
            else
            {
                SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, 
                                  "Sync Timing CORE Driver Initialized. Communicating with firmware ... \n");
            }

#ifdef SYNC_TIMING_CORE_DRIVER_RESETS_TIMING_DEVICE
            syncStatus = Sync_Timing_CORE_Device_Reset(uTimingDevId, SYNC_TIMING_DEVICE_RESET_TOGGLE);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                 "Sync_Timing_CORE_Driver_ChipReset failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }
#endif
            syncStatus = Sync_Timing_CORE_Device_GetMode(uTimingDevId, &currDeviceMode);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                 "Sync_Timing_CORE_Device_GetVersionInfo failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }
            else
            {
                if (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)
                {
                    syncStatus = Sync_Timing_CORE_Device_GetVersionInfo(uTimingDevId, &deviceVersionInfo);
                    if (syncStatus != SYNC_STATUS_SUCCESS)
                    {
                        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                         "Sync_Timing_CORE_Device_GetVersionInfo failed: %d \n", syncStatus);
                        SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                    }
                    else
                    {
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Driver Version = %s\n", 
                                                                    deviceVersionInfo.driverVersion);
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Firmware Version = %s\n", 
                                                                    deviceVersionInfo.fwVersion);
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Bootloader Version = %s\n", 
                                                                    deviceVersionInfo.blVersion);
#if (SYNC_TIMING_CHIP_TYPE == ARUBA)                        
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"CBPro Version = %s\n", 
                                                                    deviceVersionInfo.cbproVersion);

                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Frequency Planner Version = %s\n", 
                                                                    deviceVersionInfo.fplanVersion);
                        if (deviceVersionInfo.fplanDesignId)
                        {
                            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Frequency Planner Design Id = %s\n", 
                                                                       deviceVersionInfo.fplanDesignId);
                        }
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Chipset Revision = %s\n", 
                                                                    deviceVersionInfo.chipsetRevision);
#endif
                     }

                    syncStatus = Sync_Timing_CORE_Device_GetBuildInfo(uTimingDevId, &fwBuildInfo[0]);
                    if (syncStatus != SYNC_STATUS_SUCCESS)
                    {
                        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                         "Sync_Timing_CORE_Device_GetBuildInfo failed: %d \n", syncStatus);
                        SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                    }
                    else
                    {
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Firmware Build Info = %s\n", 
                                                                    fwBuildInfo);
                        Sync_Timing_OSAL_Wrapper_Memset(&driverBuildInfo[0], 0, 
                                                        SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);
                        Sync_Timing_OSAL_Wrapper_Memcpy(&driverBuildInfo[0], 
                                            (const char *)(SYNC_TIMING_DRIVER_VERSION_BUILD_INFO), 
                                            SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);
                        SYNC_TIMING_INFO(pSyncTimingCoreLogHandle,"Driver Build Info = %s\n", driverBuildInfo);
                    }
                }
            }
#endif
        }

#ifdef SYNC_TIMING_LOG_SUPPORTED
        syncStatus = Sync_Timing_CORE_Log_GetGlobalLogInfo(LogGlobalInfo, &NumRegLogModules);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                             "Sync_Timing_LOG_GetGlobalLogInfo failed: %d \n", 
                             syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }

        for (uIdx = 0; uIdx < NumRegLogModules; uIdx++)
        {       
            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, 
                         "uIdx:%u   ModName:%s ModInstId:%u  ModLogHndl:%p \n",
                         uIdx,
                         LogGlobalInfo[uIdx].logModuleName,
                         LogGlobalInfo[uIdx].uLogModuleInstId,
                         LogGlobalInfo[uIdx].pHandle);
        }
#endif

    } while(0);
    
    return 0;
}

SYNC_STATUS_E Sync_Timing_CORE_Driver_MainTerm()
{
    SYNC_STATUS_E                 syncStatus        = SYNC_STATUS_SUCCESS;
    uint32_t                      uTimingDevId      = 0;
    SYNC_TIMING_DRIVER_CORE_CFG_T *pCoreCfg         = NULL;

    do 
    {
        /* Get Core Config */
        syncStatus = Sync_Timing_CFG_GetCoreCfg((void **)&pCoreCfg);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE,   
                                "Sync_Timing_CFG_GetCoreCfg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
        }

        for (uTimingDevId = 0; uTimingDevId < pCoreCfg->uNumTimingDevices; uTimingDevId++)
        {
            syncStatus = Sync_Timing_CORE_Driver_Term(uTimingDevId);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_CORE_Driver_Term failed: %d \n", syncStatus);
            }
            else
            {
                SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, 
                                  "Sync Timing CORE Driver Terminated.\n");
            }
        }
    } while(0);
        
    return syncStatus;
}


