/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_cfg.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/02/2018
 *
 * DESCRIPTION        : Core Timing Driver Device and Appln Cfg Routines
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

#include "sync_timing_cfg_parser.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_oem_common.h"
#include "sync_timing_core_interface.h"

#include "iniparser.h"

/*****************************************************************************************
* Static global variables
****************************************************************************************/

static SYNC_TIMING_GLOBAL_CFG_T     gSyncTimingCoreGlobalCfg = {0};


static const char default_timing_device_name[SYNC_TIMING_MAX_DEVICES+1][SYNC_TIMING_CFG_MAX_NAME_SZ] = 
                                    {
                                     {"MainTimingCard\0"},
                                     {"BackupTimingCard\0"},
                                    };

static const char default_driver_core_name[SYNC_TIMING_CFG_MAX_NAME_SZ] = 
                                     {"sync_timing_core_driver\0"};

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CFG_LoadDefaultCfg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/02/2018
 *
 * DESCRIPTION   : This function is used to load the default configuration
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CFG_LoadDefaultCfg()
{
    SYNC_STATUS_E                   syncStatus                  = SYNC_STATUS_SUCCESS;
    uint8_t                         uIdx                        = 0;
    SYNC_TIMING_DEVICE_CFG_T        *pSyncTimingDeviceCfg       = NULL;
    SYNC_TIMING_DRIVER_CORE_CFG_T   *pSyncTimingDriverCoreCfg   = NULL;


    gSyncTimingCoreGlobalCfg.uNumApplns = 1;

    pSyncTimingDriverCoreCfg = &(gSyncTimingCoreGlobalCfg.DriverCoreCfg);

    /* Timing Driver Core Log settings */
    Sync_Timing_OSAL_Wrapper_Memset(&(pSyncTimingDriverCoreCfg->LogCfg.logModuleName[0]), 
                                    0, SYNC_TIMING_CFG_MAX_NAME_SZ);
    sprintf(pSyncTimingDriverCoreCfg->LogCfg.logModuleName, "%s", default_driver_core_name);

    pSyncTimingDriverCoreCfg->LogCfg.uLogModuleInstId = Sync_Timing_OSAL_Wrapper_GetProgramId();

    pSyncTimingDriverCoreCfg->LogCfg.bTraceLevel = SYNC_TIMING_TRUE;
    pSyncTimingDriverCoreCfg->LogCfg.bTraceModuleName = SYNC_TIMING_TRUE;
    pSyncTimingDriverCoreCfg->LogCfg.bTraceMiniTimestamp = SYNC_TIMING_FALSE;
    pSyncTimingDriverCoreCfg->LogCfg.bTraceTimeStamp = SYNC_TIMING_TRUE;
    pSyncTimingDriverCoreCfg->LogCfg.bTraceFuncInfo = SYNC_TIMING_TRUE;
    pSyncTimingDriverCoreCfg->LogCfg.bCustomLogTemplate = SYNC_TIMING_FALSE;
    pSyncTimingDriverCoreCfg->LogCfg.bClearLogFileOnStartup = SYNC_TIMING_TRUE;

    Sync_Timing_OSAL_Wrapper_Memset(
                            &(pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.LogFileName[0]), 
                            0, SYNC_TIMING_MAX_FILE_NAME_SZ
                            );
    pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.logTraceLevel = SYNC_TIMING_LOG_LEVEL_INFO;
    pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.bLogEnabled = SYNC_TIMING_TRUE;

    Sync_Timing_OSAL_Wrapper_Memset(
                            &(pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName[0]), 
                            0, SYNC_TIMING_MAX_FILE_NAME_SZ
                            );
    Sync_Timing_OSAL_Wrapper_Memcpy(
                            &(pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName[0]), 
                            SYNC_TIMING_DEFAULT_DRIVER_LOG,
                            sizeof(SYNC_TIMING_DEFAULT_DRIVER_LOG)
                            );
    pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.bLogEnabled = SYNC_TIMING_TRUE;
    pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.logTraceLevel = SYNC_TIMING_LOG_LEVEL_INFO;
    pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uNumRotateLogFiles = 20;        // 20 Files
    pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uMaxLogFileSize = 20*1024*1024; // 20 MB

    /* Timing Driver Core Timing Device settings */
    pSyncTimingDriverCoreCfg->uNumTimingDevices = 1;
    
    for (uIdx = 0; uIdx < pSyncTimingDriverCoreCfg->uNumTimingDevices; uIdx++)
    {
        pSyncTimingDeviceCfg = &(pSyncTimingDriverCoreCfg->TimingDeviceCfg[uIdx]);
        
        Sync_Timing_OSAL_Wrapper_Memset(&(pSyncTimingDeviceCfg->deviceName[0]), 
                                        0, SYNC_TIMING_CFG_MAX_NAME_SZ);
        Sync_Timing_OSAL_Wrapper_Memcpy(&(pSyncTimingDeviceCfg->deviceName[0]), 
                           &(default_timing_device_name[uIdx][0]),
                           Sync_Timing_OSAL_Wrapper_Strlen(&(default_timing_device_name[uIdx][0]))
                           );

        pSyncTimingDeviceCfg->OemCfg.oemDeviceType  = SYNC_TIMING_OEM_DEVICE_REF_EVB;

        sprintf((char *)pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiDevName, 
                SYNC_TIMING_DEFAULT_SPIDEV_BASE);
        pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiDevId       = 0;
        pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiBitsPerWord = 8;
        pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiMode        = 0;
        pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiSpeed       = SYNC_TIMING_MAX_SPI_SPEED;

        pSyncTimingDeviceCfg->OemResetCtrlCfg[0].oemResetCtrlRSTNum  = 509;
        pSyncTimingDeviceCfg->OemResetCtrlCfg[0].oemResetCtrlBLMDNum = 511;
        pSyncTimingDeviceCfg->OemResetCtrlCfg[0].bUseBLMDNum         = SYNC_TIMING_TRUE;


        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].uNumIrqs = 1;
        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[0].oemIrqNum = 497;
        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[0].irqTag = (497 << 8) | (uIdx & 0xFF);

        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[0].irqTriggerType = 1;





        pSyncTimingDeviceCfg->OemCfg.dataPathCfg.pSpiCfg =
                                            &(pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg);
        pSyncTimingDeviceCfg->OemCfg.pResetCtrlCfg = &(pSyncTimingDeviceCfg->OemResetCtrlCfg[0]);


        pSyncTimingDeviceCfg->OemCfg.pIRQCtrlCfg = &(pSyncTimingDeviceCfg->OemIrqCtrlCfg[0]);




        pSyncTimingDeviceCfg->uDeviceId          = uIdx;
        pSyncTimingDeviceCfg->ChipIf             = SYNC_TIMING_CHIP_INTERFACE_SPI;
        pSyncTimingDeviceCfg->OemCfg.OemDataPath = SYNC_TIMING_OEM_DATAPATH_SPI;
    }
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CFG_ParseCfg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/02/2018
 *
 * DESCRIPTION   : This function is used to parse the config file and extract info
 *
 * IN PARAMS     : pCfgComponent - Component of the Cfg file to be parsed and extracted
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER or
 *                 SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CFG_ParseCfg(char *pCfgComponent)
{
    SYNC_STATUS_E                   syncStatus                  = SYNC_STATUS_SUCCESS;
    dictionary                      *pSyncTimingDriverCfgIni    = NULL;
    uint8_t                         uIdx                        = 0,
                                    uIrqIdx                     = 0;

    char                            uKey[64]                    = {0};
    char                            uDefault[64]                = {0};
    const char                      *pKeyValueStr               = NULL;
    const char                      *pDefaultLogFolder          = NULL;
    SYNC_TIMING_DEVICE_CFG_T        *pSyncTimingDeviceCfg       = NULL;
    SYNC_TIMING_DRIVER_CORE_CFG_T   *pSyncTimingDriverCoreCfg   = NULL;
    do 
    {
        pSyncTimingDriverCfgIni = iniparser_load(SYNC_TIMING_DRIVER_CONF_FILE);

        if (pSyncTimingDriverCfgIni == NULL) 
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                              "Cannot parse Sync Timing Driver Config file\n");
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        //iniparser_dump(pSyncTimingDriverCfgIni, stdout);
        //fflush(stdout);

        sprintf(uKey, "global:num_applications");
        gSyncTimingCoreGlobalCfg.uNumApplns = iniparser_getint(pSyncTimingDriverCfgIni, uKey, 
                                                               SYNC_TIMING_MAX_APPLICATIONS);
        if (gSyncTimingCoreGlobalCfg.uNumApplns > SYNC_TIMING_MAX_APPLICATIONS)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                  syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        sprintf(uKey, "global:num_application_instances");
        gSyncTimingCoreGlobalCfg.uTotalNumApplnInstances = iniparser_getint(pSyncTimingDriverCfgIni, 
                                                             uKey, 
                                                             SYNC_TIMING_MAX_APPLICATION_INSTANCES);

        if (gSyncTimingCoreGlobalCfg.uTotalNumApplnInstances > 
                                                            SYNC_TIMING_MAX_APPLICATION_INSTANCES)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                  syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        pSyncTimingDriverCoreCfg = &(gSyncTimingCoreGlobalCfg.DriverCoreCfg);
        
        sprintf(uKey, "global:log_folder");
        pDefaultLogFolder = iniparser_getstring(pSyncTimingDriverCfgIni, uKey, 
                                           SYNC_TIMING_DEFAULT_LOG_FOLDER);

        if (Sync_Timing_OSAL_Wrapper_Strcmp(pCfgComponent, "core") == 0)
        {

            pSyncTimingDriverCoreCfg = &(gSyncTimingCoreGlobalCfg.DriverCoreCfg);

            Sync_Timing_OSAL_Wrapper_Memset(
                                &(pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName[0]), 
                                0, SYNC_TIMING_MAX_FILE_NAME_SZ
                                );
            sprintf(uKey, "%s:logfile", pCfgComponent);
            pKeyValueStr = iniparser_getstring(pSyncTimingDriverCfgIni, uKey, 
                                               "synctimingdriver.log");
            if (pKeyValueStr[0] != '/')
            {
                Sync_Timing_OSAL_Wrapper_Strcpy(
                                       pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName, 
                                       pDefaultLogFolder
                                       );
                /*Sync_Timing_OSAL_Wrapper_Strcat(
                                       pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName, 
                                       "/"
                                       );*/
                Sync_Timing_OSAL_Wrapper_Strcat(
                                       pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName, 
                                       pKeyValueStr
                                       );
            }
            else
            {
                Sync_Timing_OSAL_Wrapper_Strcpy(
                                       pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.LogFileName, 
                                       pKeyValueStr
                                       );
            }

            sprintf(uKey, "%s:trace_level_in_log_msg", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bTraceLevel = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bTraceLevel > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:module_name_in_log_msg", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bTraceModuleName = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bTraceModuleName > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:timestamp_in_log_msg", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bTraceTimeStamp = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bTraceTimeStamp > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:use_mini_timestamp", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bTraceMiniTimestamp = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bTraceMiniTimestamp > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:func_info_in_log_msg", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bTraceFuncInfo = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bTraceFuncInfo > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:clear_logfile_startup", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.bClearLogFileOnStartup = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.bClearLogFileOnStartup > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:stdout_filter_enabled", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.bLogEnabled = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.bLogEnabled > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:stdout_filter_loglevel", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.logTraceLevel = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 2);

            if (pSyncTimingDriverCoreCfg->LogCfg.logToStdoutFilter.logTraceLevel >= 
                                                                        SYNC_TIMING_LOG_LEVEL_MAX)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:fileout_filter_enabled", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.bLogEnabled = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.bLogEnabled > SYNC_TIMING_TRUE)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:fileout_filter_loglevel", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.logTraceLevel = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 2);
            if (pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.logTraceLevel >= 
                                                                        SYNC_TIMING_LOG_LEVEL_MAX)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:fileout_num_rotate_files", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uNumRotateLogFiles = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if ((pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uNumRotateLogFiles < 
                                                                SYNC_TIMING_MIN_LOG_ROTATE_FILES) || 
                (pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uNumRotateLogFiles > 
                                                                SYNC_TIMING_MAX_LOG_ROTATE_FILES))
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:fileout_max_file_size", pCfgComponent);
            pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uMaxLogFileSize = 
                                                    iniparser_getint(pSyncTimingDriverCfgIni, uKey, 
                                                                     200*1024);
            if ((pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uMaxLogFileSize < 
                                                                   SYNC_TIMING_MIN_LOG_FILE_SIZE) || 
                (pSyncTimingDriverCoreCfg->LogCfg.logToFileFilter.uMaxLogFileSize > 
                                                                   SYNC_TIMING_MAX_LOG_FILE_SIZE))
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            sprintf(uKey, "%s:num_timing_devices", pCfgComponent);
            pSyncTimingDriverCoreCfg->uNumTimingDevices = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);
            if (pSyncTimingDriverCoreCfg->uNumTimingDevices > SYNC_TIMING_MAX_DEVICES)
            {
                SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
            }

            for (uIdx = 1; uIdx <= pSyncTimingDriverCoreCfg->uNumTimingDevices; uIdx++)
            {
                pSyncTimingDeviceCfg = &(pSyncTimingDriverCoreCfg->TimingDeviceCfg[uIdx-1]);
                pSyncTimingDeviceCfg->uDeviceId          = uIdx-1;

                sprintf(uKey, "%s_timing_device_%u:timing_device_name", pCfgComponent, uIdx);
                sprintf(uDefault,"TimingCard_%u", uIdx);
                pKeyValueStr = iniparser_getstring(pSyncTimingDriverCfgIni, uKey, uDefault);
                Sync_Timing_OSAL_Wrapper_Strcpy(pSyncTimingDeviceCfg->deviceName, pKeyValueStr);

                sprintf(uKey, "%s_timing_device_%u:timing_device_interface", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->ChipIf = iniparser_getint(pSyncTimingDriverCfgIni, uKey, 0);        
                if (pSyncTimingDeviceCfg->ChipIf > SYNC_TIMING_CHIP_INTERFACE_I2C)
                {
                    SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                          syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                }

                sprintf(uKey, "%s_timing_device_%u:oem_datapath_interface", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemCfg.OemDataPath = 
                                               iniparser_getint(pSyncTimingDriverCfgIni, uKey, 0);        
                if (pSyncTimingDeviceCfg->OemCfg.OemDataPath > SYNC_TIMING_OEM_DATAPATH_I2C)
                {
                    SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                          syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                }

                sprintf(uKey, "%s_timing_device_%u:oem_device_type", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemCfg.oemDeviceType = 
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 0);        
                if (pSyncTimingDeviceCfg->OemCfg.oemDeviceType >= SYNC_TIMING_OEM_DEVICE_INVALID)
                {
                    SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                          syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                }

                if (pSyncTimingDeviceCfg->OemCfg.OemDataPath == SYNC_TIMING_OEM_DATAPATH_SPI)
                {
                    sprintf(uKey, "%s_timing_device_%u:oem_spi_devname", pCfgComponent, uIdx);
                    sprintf(uDefault,"/dev/spidev1.");
                    pKeyValueStr = iniparser_getstring(pSyncTimingDriverCfgIni, uKey, uDefault);
                    Sync_Timing_OSAL_Wrapper_Strcpy(
                            (char *)pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiDevName, 
                            pKeyValueStr);

                    sprintf(uKey, "%s_timing_device_%u:oem_spi_devid", pCfgComponent, uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiDevId =  
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 0);        

                    sprintf(uKey, "%s_timing_device_%u:oem_spi_speed", pCfgComponent, uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiSpeed =  
                                                    iniparser_getint(pSyncTimingDriverCfgIni, uKey, 
                                                                     SYNC_TIMING_MAX_SPI_SPEED);        
                    if (pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiSpeed 
                                                                    > SYNC_TIMING_MAX_SPI_SPEED)
                    {
                        SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                              syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                    }

                    sprintf(uKey, "%s_timing_device_%u:oem_spi_bpw", pCfgComponent, uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiBitsPerWord =  
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 
                                                                 SYNC_TIMING_MAX_SPI_BPW);        
                    if (pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiBitsPerWord 
                                                                        != SYNC_TIMING_MAX_SPI_BPW)
                    {
                        SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                              syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                    }

                    sprintf(uKey, "%s_timing_device_%u:oem_spi_mode", pCfgComponent, uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiMode =  
                                                iniparser_getint(pSyncTimingDriverCfgIni, uKey, 0);   
                    if (pSyncTimingDeviceCfg->OemDataPathCfg[0].OemSpiCfg.spiMode != 0)
                    {
                        //SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                        //                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                    }                    
                }
                else if (pSyncTimingDeviceCfg->OemCfg.OemDataPath == SYNC_TIMING_OEM_DATAPATH_I2C)
                {
                    sprintf(uKey, "%s_timing_device_%u:oem_i2c_devname",pCfgComponent,uIdx);
                    sprintf(uDefault,"/dev/i2c-");
                    pKeyValueStr = iniparser_getstring(pSyncTimingDriverCfgIni,uKey,uDefault);
                    Sync_Timing_OSAL_Wrapper_Strcpy(
                        (char*)pSyncTimingDeviceCfg->OemDataPathCfg[0].OemI2cCfg.i2cDevName,
                                                                                pKeyValueStr);
                    sprintf(uKey,"%s_timing_device_%u:oem_i2c_devid",pCfgComponent,uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemI2cCfg.i2cDevId =
                                        iniparser_getint(pSyncTimingDriverCfgIni,uKey,0);
                    sprintf(uKey,"%s_timing_device_%u:oem_i2c_addr",pCfgComponent,uIdx);
                    pSyncTimingDeviceCfg->OemDataPathCfg[0].OemI2cCfg.i2cDevAddr =
                                        iniparser_getint(pSyncTimingDriverCfgIni,uKey,0);
                }
                else
                {
                    SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      "Unsupported OEM Data Path set in Config file\n");
                    SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                              syncStatus, SYNC_STATUS_NOT_SUPPORTED);
                }

                sprintf(uKey, "%s_timing_device_%u:oem_main_reset_pin", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemResetCtrlCfg[0].oemResetCtrlRSTNum =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 509);        
                
                sprintf(uKey, "%s_timing_device_%u:oem_blmd_reset_pin", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemResetCtrlCfg[0].oemResetCtrlBLMDNum =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 511);        

                sprintf(uKey, "%s_timing_device_%u:oem_blmd_in_use", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemResetCtrlCfg[0].bUseBLMDNum =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);        

                sprintf(uKey, "%s_timing_device_%u:oem_num_irq_pins", pCfgComponent, uIdx);
                pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].uNumIrqs =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 1);

                if (pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].uNumIrqs > 0 && 
                    pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].uNumIrqs <= SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES)
                {
                    for (uIrqIdx = 0; uIrqIdx < pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].uNumIrqs; uIrqIdx++)
                    {
                        sprintf(uKey, "%s_timing_device_%u:oem_irq_pin_%u_num", pCfgComponent, 
                                                                                    uIdx, uIrqIdx);
                        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[uIrqIdx].oemIrqNum =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 497);

                        sprintf(uKey, "%s_timing_device_%u:oem_irq_pin_%u_trig_type", pCfgComponent, 
                                                                                    uIdx, uIrqIdx);
                        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[uIrqIdx].irqTriggerType =  
                                              iniparser_getint(pSyncTimingDriverCfgIni, uKey, 2);

                        pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[uIrqIdx].irqTag = 
                            (pSyncTimingDeviceCfg->OemIrqCtrlCfg[0].irqInfo[uIrqIdx].oemIrqNum << 8) | ((uIdx-1) & 0xFF);                      
                    }
                                        
                }




            }
        }
        else
        {
            SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                              "Unrecognized config file component\n");
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                                      syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

    } while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CFG_GetGlobalCfg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/02/2018
 *
 * DESCRIPTION   : This function is used to obtain the global cfg data
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : gSyncTimingCoreGlobalCfg  -- Global Configuration Data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER or
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CFG_GetGlobalCfg(void **pSyncTimingCoreGlobalCfg)
{
    SYNC_STATUS_E   syncStatus = SYNC_STATUS_SUCCESS;

    do
    {
        if (NULL == pSyncTimingCoreGlobalCfg )
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, 
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        *pSyncTimingCoreGlobalCfg = &gSyncTimingCoreGlobalCfg;

    } while(0);

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CFG_GetOemCfg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/02/2018
 *
 * DESCRIPTION   : This function is used to get the oem cfg info for a given timing device id
 *
 * IN PARAMS     : timingDevId - Timing device Id
 *
 * OUT PARAMS    : pOemCfg     - OEM CFG data
 *               : pOemIf      - OEM CFG Interface (SPI or I2C)
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER or
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CFG_GetOemCfg(uint8_t uTimingDevId, void **pOemCfg, 
                                         SYNC_TIMING_CHIP_INTERFACE_E *pChipIf)
{
    SYNC_STATUS_E syncStatus = SYNC_STATUS_SUCCESS;

    do
    {
        if (NULL == pOemCfg || NULL == pChipIf)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus,
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        *pOemCfg = &(gSyncTimingCoreGlobalCfg.DriverCoreCfg.TimingDeviceCfg[uTimingDevId].OemCfg);

        *pChipIf = gSyncTimingCoreGlobalCfg.DriverCoreCfg.TimingDeviceCfg[uTimingDevId].ChipIf;

    } while(0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CFG_GetCoreCfg
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/02/2018
 *
 * DESCRIPTION   : This function is used to get the oem cfg info for a given timing device id
 *
 * IN PARAMS     : timingDevId - Timing device Id
 *
 * OUT PARAMS    : pOemCfg     - OEM CFG data
 *               : pOemIf      - OEM CFG Interface (SPI or I2C)
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER or
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CFG_GetCoreCfg(void **pCoreCfg)
{
    SYNC_STATUS_E syncStatus = SYNC_STATUS_SUCCESS;

    do
    {
        if (NULL == pCoreCfg)
        {
            SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus,
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        *pCoreCfg = &(gSyncTimingCoreGlobalCfg.DriverCoreCfg);

    } while(0);
    
    return syncStatus;
}

