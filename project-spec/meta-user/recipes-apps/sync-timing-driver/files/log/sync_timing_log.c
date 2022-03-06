/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_log.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Source code for Host OEM Logging interface.
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
#include "sync_timing_log_internal.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_MAX_LOG_LEVEL_STRING_SZ 24

/*****************************************************************************************
  Global Variable Declarations
 ****************************************************************************************/

//static SYNC_TIMING_LOG_CFG_T  SyncTimingLogCfg[SYNC_TIMING_MAX_LOG_MODULES] = {0};

/*static SYNC_TIMING_LOG_CFG_FILTER_T SyncTimingDefaultStdoutFilter = 
                                                                    {SYNC_TIMING_TRUE, 
                                                                     "",
                                                                     SYNC_TIMING_LOG_LEVEL_INFO
                                                                    };

static SYNC_TIMING_LOG_CFG_FILTER_T SyncTimingDefaultFilterFilter = 
                                                                    {SYNC_TIMING_TRUE, 
                                                                     "/var/log/synctimingdriver.log",
                                                                     SYNC_TIMING_LOG_LEVEL_INFO
                                                                    };*/

static char logDefaultTemplate[SYNC_TIMING_LOG_LEVEL_MAX][SYNC_TIMING_MAX_LOG_FORMAT_SZ] = 
                                    {
                                     {"[CRITICAL] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[ERROR   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[WARNING ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[INFO1   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[INFO2   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[INFO3   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {"[DEBUG   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                     {" "},
                                     {"[ALWAYS  ] [%-24s] [%s] [%s]:[%d]: %s\0"}
                                    };

static SYNC_TIMING_LOG_CFG_T  SyncTimingDefaultLogCfg = {"Unknown\0",
                                                         0xFFFFFFFF,
                                                         {SYNC_TIMING_TRUE, 
                                                          "\0",
                                                          SYNC_TIMING_LOG_LEVEL_INFO
                                                         },
                                                         {SYNC_TIMING_TRUE, 
                                                          "/var/log/synctimingdriver.log\0",
                                                          SYNC_TIMING_LOG_LEVEL_INFO
                                                         },
                                                         SYNC_TIMING_FALSE,
                                                         SYNC_TIMING_TRUE,
                                                         SYNC_TIMING_TRUE,
                                                         SYNC_TIMING_FALSE,
                                                         SYNC_TIMING_TRUE,
                                                         SYNC_TIMING_TRUE,
                                                         SYNC_TIMING_FALSE,
                                                         {
                                                          {"[CRITICAL] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[ERROR   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[WARNING ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[INFO1   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[INFO2   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[INFO3   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {"[DEBUG   ] [%-24s] [%s] [%s]:[%d]: %s\0"},
                                                          {" "},
                                                          {"[ALWAYS  ] [%-24s] [%s] [%s]:[%d]: %s\0"}
                                                         }
                                                        };

#if 1
static char logLevelString[SYNC_TIMING_LOG_LEVEL_MAX][SYNC_TIMING_MAX_LOG_LEVEL_STRING_SZ] =
                                    {
                                     {"[CRITICAL] "},
                                     {"[ERROR   ] "},
                                     {"[WARNING ] "},
                                     {"[INFO1   ] "},
                                     {"[INFO2   ] "},
                                     {"[INFO3   ] "},
                                     {"[DEBUG   ] "},
                                     {" "},
                                     {"[ALWAYS  ] "},
                                    };
static char logTemplate[SYNC_TIMING_MAX_LOG_LEVEL_STRING_SZ] = {"%s%s \0"};

#else
const char critical_fmt_template[] = "[CRITICAL] [devId:%u] [%s] [%s]:[%d]: %s";
const char error_fmt_template[]    = "[ERROR] [devId:%u] [%s] [%s]:[%d]: %s";
const char warning_fmt_template[]  = "[WARNING] [devId:%u] [%s] [%s]:[%d]: %s";
const char info_fmt_template[]     = "[INFO] [devId:%u] [%s] [%s]:[%d]: %s";
const char debug_fmt_template[]    = "[DEBUG] [devId:%u] [%s] [%s]:[%d]: %s";  
#endif

static SYNC_TIMING_LOG_LOCAL_CONTEXT_T  gSyncTimingLogLocalContext[SYNC_TIMING_MAX_LOCAL_LOG_INSTANCES] = {0};

/*****************************************************************************************
    Functions Declarations
 ****************************************************************************************/

void* Sync_Timing_Internal_LOG_GetLocalUnitResource(void* pLogCfgData);

SYNC_STATUS_E Sync_Timing_Internal_LOG_FreeLocalUnitResource(void* pLogHandle);

void Sync_Timing_Internal_Log_FileTrace (const char *pFname, const char *pMsg);


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_LOG_GetTimestamp
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/17/2018
 *
 * DESCRIPTION   : Internal function to return the current timestamp
 *
 * IN PARAMS     : pTimeStamp     - Pointer to memory to store the timestamp
 *               : sz             - Size of the input pTimeStamp buffer
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : Actual size of the timestamp
 *
 ****************************************************************************************/
ssize_t Sync_Timing_Internal_LOG_GetTimestamp(char *pTimeStamp, uint8_t sz)
{
  struct timeval tv;
  time_t rawtime;
  struct tm timeinfo;
  ssize_t written = -1;
  int32_t w;

  gettimeofday(&tv, NULL);
  rawtime = tv.tv_sec;
  localtime_r(&rawtime, &timeinfo);

  /*printf("%02d:%02d:%02d:%03ld\n", timeinfo.tm_hour, timeinfo.tm_min,
                                     timeinfo.tm_sec, tv.tv_usec/1000);*/
  
  //printf ( "Current local time and date: %s.%03ld", asctime(&timeinfo), tv.tv_usec );

  written = (ssize_t)strftime(pTimeStamp, sz, "[%Y-%m-%d_%H:%M:%S", &timeinfo);
  //if ((written > 0) && ((size_t)written < sz))
  //{
     w = snprintf(pTimeStamp+written, sz-(size_t)written, ".%06ld] ", tv.tv_usec);
    //written = (w > 0) ? written + w : -1;
  //}

  return (written +w);

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_LOG_Register
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/17/2018
 *
 * DESCRIPTION   : This function is used to register a module with the Logging device
 *
 * IN PARAMS     : pLogCfg        - Pointer to SYNC_TIMING_LOG_CFG_T data; 
 *
 * OUT PARAMS    : pLogStatus     - More detailed status of the API
 *               : pLogHandle     - Log handle
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_Register (void                        *pLogCfgData, 
                                        SYNC_TIMING_LOG_STATUS_E    *pLogStatus,
                                        void                        **ppLogHandle)
{
    SYNC_STATUS_E                    syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_LOG_CFG_T            *pInputLogCfg      = NULL;
    SYNC_TIMING_LOG_CFG_T            *pLocalLogCfg      = NULL;
    uint32_t                         uLogLevelIdx       = 0;
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T  *pLogLocalCtx      = NULL;


    //Sync_Timing_OSAL_Wrapper_SleepMS(2000);
    //printf("%s:%d\n", __FUNCTION__, __LINE__);

    do
    {
        if (!pLogStatus)
        {
            syncStatus = SYNC_STATUS_INVALID_PARAMETER; 
            //return invalid Param as no config and handle pointer passed
            break;
        }
        else
        {
            *pLogStatus = SYNC_TIMING_LOG_FAILURE;
        }

        // Check to make sure not a duplicate register
        
        // If new register, then allocate an entry in the local context and set it up
        pLogLocalCtx = Sync_Timing_Internal_LOG_GetLocalUnitResource(pLogCfgData);

        if (pLogLocalCtx == NULL)
        {
            syncStatus = SYNC_STATUS_NO_RESOURCES; 
            //return invalid Param as no config and handle pointer passed
            break;
        }
        
        pLocalLogCfg = &(pLogLocalCtx->SyncTimingLogCfg);

        if (NULL == pLogCfgData)  // Use Default
        {
            //printf("%s:%d\n", __FUNCTION__, __LINE__);

            if (pLogStatus)
            {
                *pLogStatus = SYNC_TIMING_LOG_NOT_CONFIGURED; 
                // Log Config params not supplied; use default
                Sync_Timing_OSAL_Wrapper_Memset(&(pLocalLogCfg->logModuleName[0]),
                                                0, SYNC_TIMING_CFG_MAX_NAME_SZ);
                sprintf(pLocalLogCfg->logModuleName, "%s", 
                                                    Sync_Timing_OSAL_Wrapper_GetShortProgramName());
                
                pLocalLogCfg->uLogModuleInstId = Sync_Timing_OSAL_Wrapper_GetProgramId();

                Sync_Timing_OSAL_Wrapper_Memset(&(pLocalLogCfg->logToStdoutFilter.LogFileName),
                                                0, SYNC_TIMING_MAX_FILE_NAME_SZ);
                pLocalLogCfg->logToStdoutFilter.bLogEnabled = SYNC_TIMING_TRUE;
                pLocalLogCfg->logToStdoutFilter.logTraceLevel = SYNC_TIMING_LOG_LEVEL_ERROR;

                Sync_Timing_OSAL_Wrapper_Memset(&(pLocalLogCfg->logToFileFilter.LogFileName),
                                                0, SYNC_TIMING_MAX_FILE_NAME_SZ);
                Sync_Timing_OSAL_Wrapper_Memcpy(&(pLocalLogCfg->logToFileFilter.LogFileName), 
                                                "/var/log/synctimingdriver.log",
                                                sizeof("/var/log/synctimingdriver.log"));
                pLocalLogCfg->logToFileFilter.bLogEnabled = SYNC_TIMING_TRUE;
                pLocalLogCfg->logToFileFilter.logTraceLevel = SYNC_TIMING_LOG_LEVEL_ERROR;
                pLocalLogCfg->logToFileFilter.uNumRotateLogFiles = 0;
                pLocalLogCfg->logToFileFilter.uMaxLogFileSize = 200*1024*1024;
                

                pLocalLogCfg->bTraceLevel = SYNC_TIMING_TRUE;
                pLocalLogCfg->bTraceModuleName = SYNC_TIMING_TRUE;
                pLocalLogCfg->bTraceTimeStamp = SYNC_TIMING_TRUE;
                pLocalLogCfg->bTraceMiniTimestamp = SYNC_TIMING_FALSE;
                pLocalLogCfg->bTraceFuncInfo = SYNC_TIMING_TRUE;
                pLocalLogCfg->bClearLogFileOnStartup = SYNC_TIMING_FALSE;

                for (uLogLevelIdx = 0; uLogLevelIdx < SYNC_TIMING_LOG_LEVEL_MAX; uLogLevelIdx++)
                {
                    Sync_Timing_OSAL_Wrapper_Memset(&(pLocalLogCfg->logTemplate[uLogLevelIdx]),
                                                    0, SYNC_TIMING_MAX_LOG_FORMAT_SZ);
                    Sync_Timing_OSAL_Wrapper_Strcpy(pLocalLogCfg->logTemplate[uLogLevelIdx], 
                                                    logDefaultTemplate[uLogLevelIdx]);
                }
            }
            syncStatus = SYNC_STATUS_SUCCESS; 
            // Return success as it may be intentional
        }
        else
        {
            //printf("%s:%d\n", __FUNCTION__, __LINE__);

            pInputLogCfg = (SYNC_TIMING_LOG_CFG_T *)pLogCfgData;

            if (pInputLogCfg->logToFileFilter.bLogEnabled == SYNC_TIMING_TRUE)
            {
                if (Sync_Timing_OSAL_Wrapper_Strlen(pInputLogCfg->logToFileFilter.LogFileName) == 0)
                {
                    *pLogStatus = SYNC_TIMING_LOG_INVALID_PARAMETER;
                    syncStatus = SYNC_STATUS_INVALID_PARAMETER;
                    printf("Invalid logfilename for file output %s:%d\n", __FUNCTION__, __LINE__);
                    break;
                }
            }

            Sync_Timing_OSAL_Wrapper_Memset(pLocalLogCfg, 
                                            0, 
                                            sizeof(SYNC_TIMING_LOG_CFG_T));

            Sync_Timing_OSAL_Wrapper_Strcpy(&(pLocalLogCfg->logModuleName[0]),
                                            &(pInputLogCfg->logModuleName[0]));
            pLocalLogCfg->uLogModuleInstId = pInputLogCfg->uLogModuleInstId;
            
            
            pLocalLogCfg->bTraceLevel = pInputLogCfg->bTraceLevel;
            pLocalLogCfg->bTraceModuleName = pInputLogCfg->bTraceModuleName;
            pLocalLogCfg->bTraceTimeStamp = pInputLogCfg->bTraceTimeStamp;
            pLocalLogCfg->bTraceMiniTimestamp = pInputLogCfg->bTraceMiniTimestamp;
            pLocalLogCfg->bTraceFuncInfo = pInputLogCfg->bTraceFuncInfo;
            
            pLocalLogCfg->bCustomLogTemplate = pInputLogCfg->bCustomLogTemplate;

            pLocalLogCfg->bClearLogFileOnStartup = pInputLogCfg->bClearLogFileOnStartup;

            pLocalLogCfg->logToFileFilter.bLogEnabled = pInputLogCfg->logToFileFilter.bLogEnabled;
            pLocalLogCfg->logToFileFilter.logTraceLevel = pInputLogCfg->logToFileFilter.logTraceLevel;
            pLocalLogCfg->logToFileFilter.uMaxLogFileSize = pInputLogCfg->logToFileFilter.uMaxLogFileSize;
            pLocalLogCfg->logToFileFilter.uNumRotateLogFiles = pInputLogCfg->logToFileFilter.uNumRotateLogFiles;

            Sync_Timing_OSAL_Wrapper_Strcpy(&(pLocalLogCfg->logToFileFilter.LogFileName[0]),
                                            &(pInputLogCfg->logToFileFilter.LogFileName[0]));
            
            pLocalLogCfg->logToStdoutFilter.bLogEnabled = pInputLogCfg->logToStdoutFilter.bLogEnabled;
            pLocalLogCfg->logToStdoutFilter.logTraceLevel = pInputLogCfg->logToStdoutFilter.logTraceLevel;

            //printf("%s:%d\n", __FUNCTION__, __LINE__);
            //fflush(stdout);

            if (!pInputLogCfg->bCustomLogTemplate)
            {
                for (uLogLevelIdx = 0; uLogLevelIdx < SYNC_TIMING_LOG_LEVEL_MAX; uLogLevelIdx++)
                {
                    Sync_Timing_OSAL_Wrapper_Memset(
                                            &(pLocalLogCfg->logTemplate[uLogLevelIdx]), 
                                            0, SYNC_TIMING_MAX_LOG_FORMAT_SZ);

                    Sync_Timing_OSAL_Wrapper_Strcpy(pLocalLogCfg->logTemplate[uLogLevelIdx], 
                                                    &logTemplate[0]);
                    /*if (pInputLogCfg->bTraceTimeStamp)
                    {
                        Sync_Timing_OSAL_Wrapper_Strcat(pLocalLogCfg->logTemplate[uLogLevelIdx], 
                                                        "[%s] ");
                    }
                    if (pInputLogCfg->bTraceFuncInfo)
                    {
                        Sync_Timing_OSAL_Wrapper_Strcat(pLocalLogCfg->logTemplate[uLogLevelIdx], 
                                                        "[%s]:[%d]: %s");
                    }
                    else
                    {
                        Sync_Timing_OSAL_Wrapper_Strcat(pLocalLogCfg->logTemplate[uLogLevelIdx], 
                                                        ": %s");
                    }*/
                }
                *pLogStatus = SYNC_TIMING_LOG_SUCCESS;
                syncStatus = SYNC_STATUS_SUCCESS;
                
                //printf("%s:%d\n", __FUNCTION__, __LINE__);
                //printf("%s:%s\n", pLocalLogCfg->logModuleName,
                //                  pLocalLogCfg->logTemplate[0]);

            }
            else
            {
                // TBD - return NOT_SUPPORTED for now
                printf("Custom template not yet supported - %s:%d\n", __FUNCTION__, __LINE__);
                *pLogStatus = SYNC_TIMING_LOG_NOT_SUPPORTED;
                syncStatus = SYNC_STATUS_NOT_SUPPORTED;
            }
        }
        
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            // Return handle to the calling function 
            *ppLogHandle = (void *)pLogLocalCtx;
        }
    }
    while(0);

    return syncStatus;
  
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_LOG_SetTraceLevel
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/17/2018
 *
 * DESCRIPTION   : This function is used to set the logging trace level
 *
 * IN PARAMS     : uLogHandle     - The log handle
 *               : logFilterId    - Log Filter ID
 *               : logTraceLevel  - Logging trace level - one of SYNC_TIMING_LOG_LEVEL_E
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_SetTraceLevel (void *pLogHandle, 
                                             SYNC_TIMING_LOG_CFG_FILTER_ID_E logFilterId, 
                                             uint32_t logTraceLevel)
{
    SYNC_STATUS_E                    syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_LOG_CFG_T            *pLocalLogCfg      = NULL;
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T  *pLogLocalCtx      = NULL;

    pLogLocalCtx = (SYNC_TIMING_LOG_LOCAL_CONTEXT_T *)pLogHandle;
    pLocalLogCfg = &(pLogLocalCtx->SyncTimingLogCfg);

    if (logFilterId == SYNC_TIMING_LOG_CFG_FILTER_STDOUT)
    {
        pLocalLogCfg->logToStdoutFilter.logTraceLevel = logTraceLevel;
        if (logTraceLevel == SYNC_TIMING_LOG_LEVEL_NONE)
        {
            pLocalLogCfg->logToStdoutFilter.bLogEnabled = SYNC_TIMING_FALSE;
        }
        else
        {
            pLocalLogCfg->logToFileFilter.bLogEnabled = SYNC_TIMING_TRUE;
        }
    }
    else if (logFilterId == SYNC_TIMING_LOG_CFG_FILTER_FILE)
    {
        pLocalLogCfg->logToFileFilter.logTraceLevel = logTraceLevel;
        if (logTraceLevel == SYNC_TIMING_LOG_LEVEL_NONE)
        {
            pLocalLogCfg->logToFileFilter.bLogEnabled = SYNC_TIMING_FALSE;
        }
        else
        {
            pLocalLogCfg->logToFileFilter.bLogEnabled = SYNC_TIMING_TRUE;
        }
    }
    else
    {
        syncStatus = SYNC_STATUS_INVALID_PARAMETER;
    }


    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_LOG_GetTraceLevel
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 01/16/2021
 *
 * DESCRIPTION   : This function is used to get the current logging trace level for a module
 *
 * IN PARAMS     : uLogHandle     - The log handle
 *               : logFilterId    - Log Filter ID
 *               : logTraceLevel  - Logging trace level - one of SYNC_TIMING_LOG_LEVEL_E
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_GetTraceLevel (void *pLogHandle, 
                                             SYNC_TIMING_LOG_CFG_FILTER_ID_E logFilterId, 
                                             uint32_t *pLogTraceLevel)
{
    SYNC_STATUS_E                    syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_LOG_CFG_T            *pLocalLogCfg      = NULL;
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T  *pLogLocalCtx      = NULL;

    pLogLocalCtx = (SYNC_TIMING_LOG_LOCAL_CONTEXT_T *)pLogHandle;
    pLocalLogCfg = &(pLogLocalCtx->SyncTimingLogCfg);

    if (logFilterId == SYNC_TIMING_LOG_CFG_FILTER_STDOUT)
    {
        *pLogTraceLevel = pLocalLogCfg->logToStdoutFilter.logTraceLevel;
    }
    else if (logFilterId == SYNC_TIMING_LOG_CFG_FILTER_FILE)
    {
        *pLogTraceLevel = pLocalLogCfg->logToFileFilter.logTraceLevel;
    }
    else
    {
        syncStatus = SYNC_STATUS_INVALID_PARAMETER;
    }

    return syncStatus;
}
/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_LOG_Trace
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/17/2018
 *
 * DESCRIPTION   : This function is used to log traces to desired output
 *
 * IN PARAMS     : uLogHandle     - The log handle
 *               : logTraceLevel  - Trace level of this particular log
 *               : pFuncName      - Function Name from where the log is originating
 *               : LineNum        - Line number from where the log is originating
 *               : fmt            - Log Formatting
 *
 * OUT PARAMS    : pOemSpiStatus - More detailed status of the API
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
void Sync_Timing_LOG_Trace (void *pLogHandle, uint32_t logTraceLevel, 
                            const char * pFuncName, const uint32_t uLineNum, 
                            const char *fmt, ...)
{
    SYNC_TIMING_LOG_CFG_T           *pLocalLogCfg       = NULL;
    int                             len                 = 0;
    char                            msg_decors[512]     = {0};
    char                            pre_string[512]     = {0};
    char                            new_fmt[1024]       = {0};
    char                            timestamp[64]       = {0};
    SYNC_TIMING_LOG_MSG_TRACE_T     logMsgTrace         = {0};
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T *pLogLocalCtx       = NULL;

    //printf("%s:%d\n", __FUNCTION__, __LINE__);
    //fflush(stdout);

    if (pLogHandle != SYNC_TIMING_LOG_DEFAULT_HANDLE)
    {
        pLogLocalCtx = (SYNC_TIMING_LOG_LOCAL_CONTEXT_T *)pLogHandle;
        pLocalLogCfg = &(pLogLocalCtx->SyncTimingLogCfg);
    }
    else
    {
        pLocalLogCfg = &SyncTimingDefaultLogCfg;
    }

    Sync_Timing_OSAL_Wrapper_Memset(&logMsgTrace, 0, sizeof(logMsgTrace));

    if (1)
    {
        va_list args;

        Sync_Timing_OSAL_Wrapper_Memset(&pre_string[0], 0, 512);
        
        if (pLocalLogCfg->bTraceLevel)
        {
            Sync_Timing_OSAL_Wrapper_Strcat(pre_string, logLevelString[logTraceLevel]);
        }

        if (pLocalLogCfg->bTraceModuleName)
        {
            Sync_Timing_OSAL_Wrapper_Memset(&msg_decors[0], 0, 512);
            sprintf(&msg_decors[0], "[%-24s] ", pLocalLogCfg->logModuleName);
            Sync_Timing_OSAL_Wrapper_Strcat(pre_string, &msg_decors[0]);
        }

        if (pLocalLogCfg->bTraceTimeStamp)
        {
            /* Grab the timestamp now */
            Sync_Timing_Internal_LOG_GetTimestamp(timestamp, sizeof(timestamp));
            Sync_Timing_OSAL_Wrapper_Strcat(pre_string, timestamp);
        }

        if (pLocalLogCfg->bTraceFuncInfo)
        {
            Sync_Timing_OSAL_Wrapper_Memset(&msg_decors[0], 0, 512);
            sprintf(&msg_decors[0], "[%s]:[%d]: ", pFuncName, uLineNum);
            Sync_Timing_OSAL_Wrapper_Strcat(pre_string, &msg_decors[0]);
        }

        //printf("%s:%d\n", __FUNCTION__, __LINE__);
        //fflush(stdout);

        /* Calculate length for the augmented format string and allocate. */
        len = snprintf(NULL, 0, "%s%s", pre_string, fmt);

        /* Construct the new format string */
        snprintf(new_fmt, len + 1, "%s%s", pre_string, fmt);

        //printf("strlen(new_fmt) = %ld\n", strlen(new_fmt));
        //printf("new_fmt = %s\n", new_fmt);
        //fflush(stdout);

        va_start(args, fmt);
        vsprintf(logMsgTrace.logMsg, new_fmt, args);
        va_end(args);

        /* Print as before, using new format string */
        if ((pLocalLogCfg->logToFileFilter.bLogEnabled) && 
            (pLocalLogCfg->logToFileFilter.logTraceLevel >= logTraceLevel || 
             logTraceLevel == SYNC_TIMING_LOG_LEVEL_ALWAYS))
        {
            {
                Sync_Timing_Internal_Log_FileTrace(
                            (const char *)pLocalLogCfg->logToFileFilter.LogFileName,
                            &logMsgTrace.logMsg[0]);
                //printf("%s:%u\n", __FUNCTION__, __LINE__);
            }
        }

        if ((pLocalLogCfg->logToStdoutFilter.bLogEnabled) &&
            (pLocalLogCfg->logToStdoutFilter.logTraceLevel >= logTraceLevel || 
             logTraceLevel == SYNC_TIMING_LOG_LEVEL_ALWAYS))
        {
            printf("%s", logMsgTrace.logMsg);
        }

    }
        
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_LOG_DeRegister
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/17/2018
 *
 * DESCRIPTION   : This function is used to initialize the Logging device
 *
 * IN PARAMS     : uLogHandle     - The log handle
 *               : pOemData       - Pointer to OEM data; Only Logging specific data will 
 *                                  be used
 *
 * OUT PARAMS    : pOemSpiStatus - More detailed status of the API
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_DeRegister (void *pLogHandle,
                                    SYNC_TIMING_LOG_STATUS_E *pLogStatus)
{
    SYNC_STATUS_E                    syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_LOG_CFG_T            *pLocalLogCfg      = NULL;
    SYNC_TIMING_LOG_MSG_CFG_T        logMsgCfg          = {0};
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T  *pLogLocalCtx      = NULL;
    uint32_t                         syncOsalStatus     = SYNC_TIMING_OSAL_SUCCESS;

    pLogLocalCtx = (SYNC_TIMING_LOG_LOCAL_CONTEXT_T *)pLogHandle;
    pLocalLogCfg = &(pLogLocalCtx->SyncTimingLogCfg);

    // send that information to the master log thread

    *pLogStatus = SYNC_TIMING_LOG_SUCCESS;

    pLocalLogCfg->logToStdoutFilter.bLogEnabled = SYNC_TIMING_FALSE;
    pLocalLogCfg->logToFileFilter.bLogEnabled = SYNC_TIMING_FALSE;

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        // Send msg to the log thread
        Sync_Timing_OSAL_Wrapper_Memset(&logMsgCfg, 0, sizeof(logMsgCfg));
        logMsgCfg.logMsgHdr.logMsgType       = SYNC_TIMING_LOG_MSG_DEREGISTER;
        logMsgCfg.logMsgHdr.uLogModuleInstId = pLocalLogCfg->uLogModuleInstId;
        logMsgCfg.logMsgHdr.pHandle          = (void *)pLogHandle;

        Sync_Timing_OSAL_Wrapper_Strcpy(&(logMsgCfg.logMsgHdr.logModuleName[0]),
                                        &(pLocalLogCfg->logModuleName[0]));

        logMsgCfg.bFileOutLogEnabled         = pLocalLogCfg->logToFileFilter.bLogEnabled;
        logMsgCfg.bStdOutLogEnabled          = pLocalLogCfg->logToStdoutFilter.bLogEnabled;
        logMsgCfg.fileOutLogTraceLevel       = pLocalLogCfg->logToFileFilter.logTraceLevel;
        logMsgCfg.stdOutLogTraceLevel        = pLocalLogCfg->logToStdoutFilter.logTraceLevel;
        
        syncOsalStatus = Sync_Timing_OSAL_Wrapper_MsgQ_Send(
                                  &(pLogLocalCtx->logThreadMsgQueue), 
                                  (void *)&(logMsgCfg), 
                                  sizeof(logMsgCfg), 
                                  0, 0);
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            syncStatus = SYNC_STATUS_FAILURE;
        }
    }

    if (pLogLocalCtx->bLogCommnInitialized == SYNC_TIMING_TRUE)
    {
        syncOsalStatus = Sync_Timing_OSAL_Wrapper_MsgQ_Close(
                                           &(pLogLocalCtx->logThreadMsgQueue)
                                           );
        
        if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
        {
            *pLogStatus = SYNC_TIMING_LOG_FAILURE;
            syncStatus = SYNC_STATUS_FAILURE;
        }
        
        pLogLocalCtx->bLogCommnInitialized = SYNC_TIMING_FALSE;
    }

    // De-allocate the entry in the local context
    Sync_Timing_Internal_LOG_FreeLocalUnitResource((void *)pLogHandle);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_LOG_GetLocalUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 09/04/2018
 *
 * DESCRIPTION   : Internal function to get a local unit resource
 *
 * IN PARAMS     : pLogCfgData   - Log Cfg Data
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : Pointer to the free resource
 *
 ****************************************************************************************/
void* Sync_Timing_Internal_LOG_GetLocalUnitResource(void* pLogCfgData)
{
    uint8_t uLogHndleIdx   = 0;

    // Check to ensure this is not a duplicate request for the same log module name and instance id

    for (uLogHndleIdx = 0; uLogHndleIdx < SYNC_TIMING_MAX_LOCAL_LOG_INSTANCES; uLogHndleIdx++)
    {
        if (gSyncTimingLogLocalContext[uLogHndleIdx].bNotFree == SYNC_TIMING_FALSE)
            break;
    }

    if (uLogHndleIdx != SYNC_TIMING_MAX_LOCAL_LOG_INSTANCES)
    {
        SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE, "LOG_GetLocalUnitResource "
                                                          "uLogHndleIdx = %u "
                                                          "pLogHandle = 0x%08x\n",
                                                          uLogHndleIdx,
                                                          &(gSyncTimingLogLocalContext[uLogHndleIdx]));        

        gSyncTimingLogLocalContext[uLogHndleIdx].bNotFree = SYNC_TIMING_TRUE;
        return &(gSyncTimingLogLocalContext[uLogHndleIdx]);
    }
    else
    {
        SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE, 
                            "LOG_GetLocalUnitResource - no resources free\n"); 
        return NULL;
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_LOG_FreeLocalUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 09/04/2018
 *
 * DESCRIPTION   : Internal function to free up resource
 *
 * IN PARAMS     : pLogHandle        - The Log Handle
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_LOG_FreeLocalUnitResource(void* pLogHandle)
{
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_INVALID_PARAMETER;
    SYNC_TIMING_LOG_LOCAL_CONTEXT_T     *pLogLocalCtx       = NULL;

    if (pLogHandle)
    {
        pLogLocalCtx = (SYNC_TIMING_LOG_LOCAL_CONTEXT_T *)pLogHandle;
        if (pLogLocalCtx->bNotFree == SYNC_TIMING_FALSE)
        {
            SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE, "LOG_FreeLocalUnitResource "
                                                             "pLogHandle = 0x%08x\n",
                                                             pLogHandle);  

            pLogLocalCtx->bNotFree = SYNC_TIMING_TRUE;
            syncStatus = SYNC_STATUS_SUCCESS;
        }
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_Log_FileTrace
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/30/2018
 *
 * DESCRIPTION   : This function is used to log traces to the file
 *
 * IN PARAMS     : pFname         - File name to write to
 *               : pMsg           - Message to print
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
void Sync_Timing_Internal_Log_FileTrace (const char *pFname, const char *pMsg)
{
    FILE    *fp = NULL;

    fp = fopen((const char *)pFname, "a+");
    if (fp)
    {
         fprintf(fp, "%s", pMsg);
         fclose(fp);
    }
}

