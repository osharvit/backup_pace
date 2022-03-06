/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_communication.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Core Timing Driver APIs that defines the communication interface
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
 
#ifndef _SYNC_TIMING_CORE_COMMUNICATION_H_
#define _SYNC_TIMING_CORE_COMMUNICATION_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_cfg_parser.h"
#include "sync_timing_core_interface.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/
/* A Dynamic runtime configuration per application that is stored by the Core */
typedef struct
{
    SYNC_TIMING_BOOL_E              bClientValid;
    /* Is this index valid or not */
    SYNC_TIMING_BOOL_E              bClientBlocked;
    /* Is this client blocked or not */
    SYNC_TIMING_APPLN_INFO_T        ApplnInfo;
    /* Application information stored for future purpose */
    uint32_t                        uClientAppId;
    /* Client Application ID - informative */
    SYNC_TIMING_DEVICE_DRIVER_EVENT_E   regDriverEvent;
    SYNC_TIMING_DEVICE_EVENT_INFO_T     regDeviceChipsetEvent;
    /* Application registered callback device chipset events */


} SYNC_TIMING_CORE_APPLN_DYN_CFG_T;

typedef struct
{
    char                              coreName[SYNC_TIMING_CFG_MAX_APP_NAME_SZ];   
    // Core Name
    uint32_t                          coreInstanceId;                              
    // Core instance ID. E.g. PID
    uint32_t                          uNumActiveApplns;
    /* Number of active applications; Core will iterate through this list looking for
       the messages from various clients */
    SYNC_TIMING_OSAL_MSG_QUEUE_T      coreRegnRecvMsgQueue;
    SYNC_TIMING_CORE_APPLN_DYN_CFG_T  ApplnDynCfg[SYNC_TIMING_MAX_APPLICATION_INSTANCES];
    /* Run-time application information */
    uint32_t                          fileLogTraceLevel;
    uint32_t                          stdoutLogTraceLevel;
    

} SYNC_TIMING_CORE_DYN_CFG_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Driver_MainInit();

SYNC_STATUS_E Sync_Timing_CORE_Driver_MainTerm();

SYNC_STATUS_E Sync_Timing_Internal_CORE_Send_Events(uint8_t uTimingDevId, uint32_t event,
                                           SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T *pEventData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Appln_Register(SYNC_TIMING_CORE_MSG_INIT_T *pCoreInitMsg, 
                                              uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Appln_DeRegister(SYNC_TIMING_CORE_MSG_TERM_T *pCoreTermMsg,
                                                uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessIncomingMsg(uint32_t uAppIndex, 
                                                           void *pRecvBuff, 
                                                           uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_SendMsg(uint32_t uAppIndex,
                                                char *pMsg, 
                                                uint32_t uMsgLen
                                               );


SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessDeviceCtrlMsg(uint32_t uCurrentAppIndex, 
                                                          void *pRecvBuff, 
                                                          uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessMemAccessMsg(uint32_t uCurrentAppIndex, 
                                                            void *pRecvBuff, 
                                                            uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessDebugMsg(uint32_t uCurrentAppIndex, 
                                                        void *pRecvBuff, 
                                                        uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessClkAdjMsg(uint32_t uCurrentAppIndex, 
                                                         void *pRecvBuff, 
                                                         uint32_t uMsgLen);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessClkCtrlMsg(uint32_t uCurrentAppIndex, 
                                                         void *pRecvBuff, 
                                                         uint32_t uMsgLen);


SYNC_STATUS_E Sync_Timing_Internal_CORE_ProcessPendingDriverEvent();

#endif //_SYNC_TIMING_CORE_COMMUNICATION_H_

