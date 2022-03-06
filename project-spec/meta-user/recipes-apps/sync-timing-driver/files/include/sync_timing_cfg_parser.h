/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_cfg_parser.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/02/2018
 *
 * DESCRIPTION        : Timing Driver APIs that defines the cfg parser interface
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

#ifndef _SYNC_TIMING_CFG_PARSER_H_
#define _SYNC_TIMING_CFG_PARSER_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_oem_common.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

typedef struct
{
    char                             clientSendMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    uint32_t                         uMaxClientSendMsgs;
    uint32_t                         uMaxClientSendMsgSize;
    char                             clientRecvMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    uint32_t                         uMaxClientRecvMsgs;
    uint32_t                         uMaxClientRecvMsgSize;
} SYNC_TIMING_APPLN_CONN_CFG_T;

typedef struct
{
    uint32_t                         uClientAppID;
    char                             ClientAppName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_APPLN_CONN_CFG_T     ApplnConnCfg;
    SYNC_TIMING_LOG_CFG_T            LogCfg;
} SYNC_TIMING_APPLN_CFG_T;

typedef struct
{
    uint8_t                          uDeviceId;
    char                             deviceName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    
    SYNC_TIMING_OEM_CFG_DATA_T       OemCfg;

    union 
    {
        SYNC_TIMING_OEM_SPI_CFG_T    OemSpiCfg;
        SYNC_TIMING_OEM_I2C_CFG_T    OemI2cCfg;
    } OemDataPathCfg[SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES];

    SYNC_TIMING_OEM_RESETCTRL_CFG_T  OemResetCtrlCfg[SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES];


    SYNC_TIMING_OEM_IRQCTRL_CFG_T    OemIrqCtrlCfg[SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES];



    SYNC_TIMING_CHIP_INTERFACE_E     ChipIf;

} SYNC_TIMING_DEVICE_CFG_T;

typedef struct
{
    SYNC_TIMING_LOG_CFG_T            LogCfg;
    uint32_t                         uNumTimingDevices;
    SYNC_TIMING_DEVICE_CFG_T         TimingDeviceCfg[SYNC_TIMING_MAX_DEVICES];
} SYNC_TIMING_DRIVER_CORE_CFG_T;

typedef struct
{
    uint32_t                         uNumApplns;
    uint32_t                         uTotalNumApplnInstances;
    SYNC_TIMING_DRIVER_CORE_CFG_T    DriverCoreCfg;
} SYNC_TIMING_GLOBAL_CFG_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

SYNC_STATUS_E Sync_Timing_CFG_LoadDefaultCfg();

SYNC_STATUS_E Sync_Timing_CFG_ParseCfg(char *pCfgComponent);

SYNC_STATUS_E Sync_Timing_CFG_GetGlobalCfg(void **gSyncTimingCoreGlobalCfg);


SYNC_STATUS_E Sync_Timing_CFG_GetOemCfg(uint8_t timingDevId, void **pOemCfg, 
                                        SYNC_TIMING_CHIP_INTERFACE_E *pOemIf);

SYNC_STATUS_E Sync_Timing_CFG_GetCoreCfg(void **pCoreCfg);

#endif //_SYNC_TIMING_CFG_PARSER_H_

