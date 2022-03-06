/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_driver.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/26/2018
 *
 * DESCRIPTION        : Core Timing Driver APIs that initializes the communication and 
 *                      device contexts with the timing chipset
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

#ifndef _SYNC_TIMING_CORE_DRIVER_H_
#define _SYNC_TIMING_CORE_DRIVER_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_core_ctrl.h"
#include "sync_timing_cfg_parser.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/* Version Info Build Type related macros */
#define SYNC_TIMING_MAX_BUILD_TYPES             16
#define SYNC_TIMING_MAX_BUILD_TYPE_STRING_SIZE  32

#define SYNC_TIMING_MAX_VERIFY_CMDS             10
#define SYNC_TIMING_MAX_VERIFY_CMD_SIZE         9

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

typedef struct
{
    uint32_t    uBank;
    uint8_t     uVerifyCmd[SYNC_TIMING_MAX_VERIFY_CMD_SIZE];
} SYNC_TIMING_BL_VERIFY_CMD_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

SYNC_STATUS_E Sync_Timing_CORE_Driver_Init(uint8_t timingDevId, 
                                            SYNC_TIMING_CHIP_INTERFACE_E OemChipIf, 
                                            void *pOemData, SYNC_TIMING_DEVICE_CFG_T *pDeviceCfg);

SYNC_STATUS_E Sync_Timing_CORE_Driver_Term(uint8_t timingDevId);

SYNC_STATUS_E Sync_Timing_CORE_Device_Reset(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_RESET_TYPE_E DeviceResetType);

SYNC_STATUS_E Sync_Timing_CORE_Device_Update(uint8_t timingDevId, char *pBootFile);

SYNC_STATUS_E Sync_Timing_CORE_Device_SetMode(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_MODE_E deviceMode);

SYNC_STATUS_E Sync_Timing_CORE_Device_GetMode(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_MODE_E *pCurrDeviceMode);

SYNC_STATUS_E Sync_Timing_CORE_Device_GetVersionInfo(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo);

SYNC_STATUS_E Sync_Timing_CORE_Device_GetBuildInfo(uint8_t timingDevId, 
                                            char *pVersionBuildInfo);


SYNC_STATUS_E Sync_Timing_CORE_Device_VerifyFlashSeg(uint8_t timingDevId,
                                            const char *pBootFile,
                                            SYNC_TIMING_BOOL_E *pBootFileMatchesFlash);

SYNC_STATUS_E Sync_Timing_CORE_Device_GetInitStatusInfo(uint8_t timingDevId, 
                                            SYNC_TIMING_DEVICE_INFO_T *pDeviceInfo);

SYNC_STATUS_E Sync_Timing_CORE_Device_Download(uint8_t timingDevId, uint32_t uNumBootfiles, 
                                               char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ]);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Reset(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            SYNC_TIMING_DEVICE_RESET_TYPE_E   resetType);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Update(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            char *pBootFile);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetBuildInfo(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            char *pVersionBuildInfo);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetVersionInfo(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetMode(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            SYNC_TIMING_DEVICE_MODE_E *pCurrDeviceMode);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_SetMode(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext,
                                            SYNC_TIMING_DEVICE_MODE_E deviceMode);
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Init(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_ClearInterrupts(
                                              SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_SetupIRQ(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Driver_IrqCallback(uint32_t irqTag);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Download(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            uint32_t uNumBootfiles, 
                                            char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ]);

#endif //_SYNC_TIMING_CORE_DRIVER_H_


