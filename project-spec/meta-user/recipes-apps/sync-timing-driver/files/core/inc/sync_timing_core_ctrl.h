/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_ctrl.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Core Timing Driver definitions for the device context ctrl mechanism
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
 
#ifndef _SYNC_TIMING_CORE_CTRL_H_
#define _SYNC_TIMING_CORE_CTRL_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_cfg_parser.h"


/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

#if 0
/* Driver Callback Notify Function prototype */
typedef SYNC_STATUS_E (*SYNC_TIMING_CORE_DEVICE_MEM_WRITEDIRECT_FN_T)(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

typedef SYNC_STATUS_E (*SYNC_TIMING_CORE_DEVICE_MEM_READDIRECT_FN_T)(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

typedef SYNC_STATUS_E (*SYNC_TIMING_CORE_DEVICE_MEM_WRITEINDIRECT_FN_T)(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

typedef SYNC_STATUS_E (*SYNC_TIMING_CORE_DEVICE_MEM_READINDIRECT_FN_T)(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

typedef SYNC_STATUS_E (*SYNC_TIMING_CORE_DEVICE_MEM_XFERTOBL_FN_T)(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t len, 
                                    uint8_t* pData);

typedef struct
{
    SYNC_TIMING_CORE_DEVICE_MEM_WRITEDIRECT_FN_T    pMemWriteDirectFn;
    SYNC_TIMING_CORE_DEVICE_MEM_READDIRECT_FN_T     pMemReadDirectFn;
    SYNC_TIMING_CORE_DEVICE_MEM_WRITEINDIRECT_FN_T  pMemWriteInDirectFn;
    SYNC_TIMING_CORE_DEVICE_MEM_READINDIRECT_FN_T   pMemReadInDirectFn;
    SYNC_TIMING_CORE_DEVICE_MEM_XFERTOBL_FN_T       pMemXferToBlFn;

} SYNC_TIMING_CORE_DEVICE_API_TABLE_T;
#endif


/* Device context that is maintained by the core to ensure serialization of access 
   to the timing device 
 */
typedef struct
{
    uint8_t                             timingDevId;          
    // Timing Device ID
    SYNC_TIMING_BOOL_E                  bDriverInitialized;   
    SYNC_TIMING_BOOL_E                  bDeviceInitialized;   
    // Is the Driver initialized for this device
    SYNC_TIMING_BOOL_E                  bfwVersionInfoAvailable;  
    uint32_t                            fwVersionMajor;
    uint32_t                            fwVersionMinor;
    uint32_t                            fwVersionBranch;
    uint32_t                            fwVersionBuildNum;

    uint8_t                             deviceRevision;
    uint8_t                             deviceSubRevision;

    SYNC_TIMING_DEVICE_MODE_E           deviceMode;

    void                                *pMutex;              
    // Mutex object to protect access to this context

    void                                *pOemData;

    SYNC_TIMING_DEVICE_CFG_T            *pDeviceCfg;

    SYNC_TIMING_CHIP_INTERFACE_E        ChipIf;            
    // Chip interface for this device
    
    void                                *pOemDataPathHandle; 
    // OEM Data Path handle (from OEM Data Path layer)
    void                                *pOemResetCtrlHandle; 
    // Chip reset control OEM handle
    void                                *pOemIRQCtrlHandle;  
    // IRQ Control handle

    uint32_t                            syncTimingOemDataPathStatus;
    uint32_t                            syncTimingOemResetCtrlStatus;
    uint32_t                            syncTimingOemIRQCtrlStatus;

    SYNC_TIMING_BOOL_E                  bPtpSteeredRf;
    SYNC_TIMING_BOOL_E                  bSysclkPLLHO;


    SYNC_TIMING_BOOL_E                  bHostClearStatus;

    char                                intrPollThreadName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_OSAL_THREAD_T           intrPollThread;
    SYNC_TIMING_BOOL_E                  bIntrPollThreadInitialized;

    SYNC_TIMING_BOOL_E                  bROSInputPollList[8];

    SYNC_TIMING_BOOL_E                  bPendingDriverEvent;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_E   pendingDriverEvent;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T pendingDriverEventData;  

} SYNC_TIMING_CORE_DEVICE_CONTEXT_T;


SYNC_TIMING_CORE_DEVICE_CONTEXT_T *Sync_Timing_CORE_Ctrl_GetDeviceContext(uint8_t timingDevId);

SYNC_STATUS_E Sync_Timing_CORE_Ctrl_AcqDevice(uint8_t               timingDevId,
                                 SYNC_TIMING_CORE_DEVICE_CONTEXT_T  **ppTimingDevContext,
                                 SYNC_TIMING_BOOL_E                 *pHeldMutex
                                 );


#endif //_SYNC_TIMING_CORE_CTRL_H_

