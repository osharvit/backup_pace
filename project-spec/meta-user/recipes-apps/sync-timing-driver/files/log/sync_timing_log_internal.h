/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_log_internal.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 08/31/2018
 *
 * DESCRIPTION        : Timing Driver Log Module internal definitions
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
 
#ifndef _SYNC_TIMING_LOG_INTERNAL_H_
#define _SYNC_TIMING_LOG_INTERNAL_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/* Log Module Local Context - stores run-time information and access protection etc */
typedef struct
{
    char                              logThreadMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_OSAL_MSG_QUEUE_T      logThreadMsgQueue;
    /* Message queue on which the modules will send register, de-register commands 
       and file log messages to the log thread */

    SYNC_TIMING_LOG_CFG_T             SyncTimingLogCfg;

    SYNC_TIMING_BOOL_E                bLogCommnInitialized;

    SYNC_TIMING_BOOL_E                bNotFree;

} SYNC_TIMING_LOG_LOCAL_CONTEXT_T;

typedef enum
{
    SYNC_TIMING_LOG_MSG_REGISTER = 200,
    SYNC_TIMING_LOG_MSG_DEREGISTER,
    SYNC_TIMING_LOG_MSG_TRACE,
    SYNC_TIMING_LOG_MSG_CFG_TRACE,
    SYNC_TIMING_LOG_MSG_INVALID
    
} SYNC_TIMING_LOG_MSG_TYPE_E;

typedef struct
{
    SYNC_TIMING_LOG_MSG_TYPE_E logMsgType;
    uint32_t                   uLogModuleInstId;
    char                       logModuleName[SYNC_TIMING_CFG_MAX_NAME_SZ];  
    void                       *pHandle;

} SYNC_TIMING_LOG_MSG_HDR_T;

/* Message header */
typedef struct
{
    SYNC_TIMING_LOG_MSG_HDR_T  logMsgHdr;
    SYNC_TIMING_BOOL_E         bStdOutLogEnabled;
    SYNC_TIMING_BOOL_E         bFileOutLogEnabled;
    uint8_t                    stdOutLogTraceLevel;
    uint8_t                    fileOutLogTraceLevel;
    char                       LogFileName[SYNC_TIMING_MAX_FILE_NAME_SZ];
    uint32_t                   uNumRotateLogFiles;
    uint32_t                   uMaxLogFileSize;    
    SYNC_TIMING_BOOL_E         bClearLogFileOnStartup;    
} SYNC_TIMING_LOG_MSG_CFG_T;

/* Message header */
typedef struct
{
    SYNC_TIMING_LOG_MSG_HDR_T  logMsgHdr;
    char                       LogFileName[SYNC_TIMING_MAX_FILE_NAME_SZ];
    char                       logMsg[SYNC_TIMING_LOG_THREAD_MAX_LOG_MSG_SIZE];
    uint32_t                   uNumRotateLogFiles;
    uint32_t                   uMaxLogFileSize;    
} SYNC_TIMING_LOG_MSG_TRACE_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/


#endif //_SYNC_TIMING_LOG_INTERNAL_H_


