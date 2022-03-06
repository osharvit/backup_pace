/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_log.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 08/31/2018
 *
 * DESCRIPTION        : Timing Driver Core Log Module internal definitions
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

 
#ifndef _SYNC_TIMING_CORE_LOG_H_
#define _SYNC_TIMING_CORE_LOG_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_log_internal.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/* Log Module Global Context - stores run-time information and access protection etc */
typedef struct
{
    uint32_t                          pClientLogModuleId;
    /* Client module logging ID determined from the client app Id */

    char                              logMsgRecvMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_OSAL_MSG_QUEUE_T      logMsgRecvMsgQueue;
    /* Message queue on which the modules will send register, de-register commands 
       and file log messages to the log thread */

    char                              logThreadName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_OSAL_THREAD_T         logMsgRecvThread;
    /* Log thread receiving and processing messages (events and responses) from the 
       modules */

    SYNC_TIMING_BOOL_E                bLogThreadInitialized;

    SYNC_TIMING_LOG_GLOBAL_INFO_T     SyncTimingLogGlobalInfo[SYNC_TIMING_MAX_GLOBAL_LOG_MODULES];

    uint32_t                          uNumRegLogModules;

    uint32_t                          uLogFileContextIdx[SYNC_TIMING_MAX_GLOBAL_LOG_MODULES];

} SYNC_TIMING_LOG_GLOBAL_CONTEXT_T;

/* Log file context - used to store dynamic information about every log file configured */
typedef struct
{
    char                        LogFileName[SYNC_TIMING_MAX_FILE_NAME_SZ];
    uint32_t                    uCurrentRotateIndex;
    uint32_t                    uNumLogModules;
    uint32_t                    uNumRotateLogFiles;
    uint32_t                    uMaxLogFileSize;

} SYNC_TIMING_LOG_FILE_CONTEXT_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

SYNC_STATUS_E Sync_Timing_CORE_Log_GlobalInit();


SYNC_STATUS_E Sync_Timing_Internal_CORE_Log_ProcessIncomingMsg(void *pLogMsg);

void Sync_Timing_Internal_CORE_Log_FileTrace (SYNC_TIMING_LOG_GLOBAL_INFO_T *pGlobalInfo, 
                                              uint32_t uLogFileCtxIdx,
                                              const char *pMsg);

void* Sync_Timing_Internal_CORE_Log_GetNewGlobalUnitResource(char *pLogModuleName,
                                                             uint32_t uLogModuleInstId, 
                                                             void *pHandle,
                                                             char *pFileName,
                                                             uint32_t uNumRotateLogFiles,
                                                             uint32_t uMaxLogFileSize);

void* Sync_Timing_Internal_CORE_Log_FindGlobalUnitResource(char     *pLogModuleName, 
                                                           uint32_t uLogModuleInstId,
                                                           void     *pHandle,
                                                           uint32_t *pGlobalLogInfoIdx);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Log_FreeGlobalUnitResource(char *pLogModuleName, 
                                                                   uint32_t uLogModuleInstId,
                                                                   void *pHandle);

SYNC_STATUS_E Sync_Timing_CORE_Log_GetGlobalLogInfo(SYNC_TIMING_LOG_GLOBAL_INFO_T *pLogGlobalInfo,
                                                    uint32_t *pNumRegLogModules);

SYNC_STATUS_E Sync_Timing_CORE_Log_CleanupGlobalUnitResource(char *pLogModuleName, 
                                                             uint32_t uLogModuleInstId);


#endif //_SYNC_TIMING_LOG_INTERNAL_H_


