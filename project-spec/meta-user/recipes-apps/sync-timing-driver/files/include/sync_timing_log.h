/****************************************************************************************/
/**
 *  \defgroup porting SYNC TIMING DRIVER PORTING GUIDE
 *  @brief     This section defines the various Porting APIs for the Timing Chipset.
 *  @{
 *  \defgroup log_if Logging Interface Porting
 *  @brief    This section defines the Logging Interface APIs for SYNC Timing Chipset
 *            It is expected that the OEM will implement these functions for their 
 *            development platform. Sync will provide a generic linux based reference 
 *            implementation.
 *  @{
 *
 *  \file          sync_timing_log.h
 *
 *  \details       This file contains LOG associated functions and data structures.
 *
 *  \date          Created: 07/19/2018
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

#ifndef _SYNC_TIMING_LOG_H_
#define _SYNC_TIMING_LOG_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/* Core module ID for logging, etc is fixed to 1000 */
//#define SYNC_TIMING_CORE_MODULE_ID              1000

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

#ifdef _DOXYGEN_
#define SYNC_TIMING_LOG_SUPPORTED
#endif

#ifdef SYNC_TIMING_LOG_SUPPORTED
/**
 *  \defgroup log_if_ds LOG Interface Data Structures 
 *  @brief LOG Controller Interface Data Structures for Timing driver
 *  @{
 */

/*! Enumeration for Log layer detailed status - OEM can enhance this if necessary */
typedef enum 
{
    SYNC_TIMING_LOG_SUCCESS = 0,         //!< LOG Driver returned SUCCESS
    SYNC_TIMING_LOG_INVALID_PARAMETER,   //!< LOG Driver returned INVALID_PARAMETER
    SYNC_TIMING_LOG_NOT_CONFIGURED,      //!< LOG Driver returned NOT CONFIGURED
    SYNC_TIMING_LOG_NOT_SUPPORTED,       //!< LOG Driver returned NOT_SUPPORTED  
    SYNC_TIMING_LOG_FAILURE ,            //!< LOG Driver returned FAILURE
} SYNC_TIMING_LOG_STATUS_E;

/*! Enumeration for the supported log levels - OEM can enhance this if necessary */
typedef enum 
{
    SYNC_TIMING_LOG_LEVEL_CRITICAL = 0,  //!< LOG Level Critical
    SYNC_TIMING_LOG_LEVEL_ERROR    = 1,  //!< LOG Level Error   
    SYNC_TIMING_LOG_LEVEL_WARNING  = 2,  //!< LOG Level Warning
    SYNC_TIMING_LOG_LEVEL_INFO1    = 3,  //!< LOG Level Info1
    SYNC_TIMING_LOG_LEVEL_INFO2    = 4,  //!< LOG Level Info2
    SYNC_TIMING_LOG_LEVEL_INFO     = SYNC_TIMING_LOG_LEVEL_INFO2,  //!< Default Info LOG Level
    SYNC_TIMING_LOG_LEVEL_INFO3    = 5,  //!< LOG Level Info3
    SYNC_TIMING_LOG_LEVEL_DEBUG    = 6,  //!< LOG Level Debug
    SYNC_TIMING_LOG_LEVEL_NONE     = 7,  //!< LOG Level None ~ Trace Disabled
    SYNC_TIMING_LOG_LEVEL_ALWAYS   = 8,  //!< LOG Level Always - Internal use only
    SYNC_TIMING_LOG_LEVEL_MAX      = 9   //!< Invalid LOG Level - Used for range check only
   
} SYNC_TIMING_LOG_LEVEL_E;

/*! Log Cfg filter identification */
typedef enum
{
    SYNC_TIMING_LOG_CFG_FILTER_STDOUT = 0,  
    //!< Filter ID for Stdout
    SYNC_TIMING_LOG_CFG_FILTER_FILE,        
    //!< Filter ID for File output
    SYNC_TIMING_LOG_CFG_FILTER_INVALID      
    //!< Invalid filter ID - used only for range check; DONOT use in application

} SYNC_TIMING_LOG_CFG_FILTER_ID_E;

/*! LOG Configuration Filter */
typedef struct
{
    SYNC_TIMING_BOOL_E          bLogEnabled;
    //!< Log Enabled or not

    char                        LogFileName[SYNC_TIMING_MAX_FILE_NAME_SZ];
    //!< Log File Name; Applicable for only logging to file;

    uint32_t                    logTraceLevel;
    //!< Log Trace Level for file logging - CRITICAL or WARNING or ERROR or DEBUG or INFO; 
    //!< Only logs above this trace level will be output
    
    uint32_t                    uNumRotateLogFiles;
    //!< A  non-zero value indicates that logs will be rotated across the specified number of files; 
    //!< This excludes the default log file specified by LogFileName. Other files will be named as
    //!< LogFileName.01, LogFileName.02, and so on. Default value is 0 which means logs will be 
    //!< rotated within the same file; Applicable for only logging to file;

    uint32_t                    uMaxLogFileSize;
    //!< Maximum size of each log file created; After size is reached logs are automatically rotated
    //!< If numRotateLogFiles is non-zero then the logs will go into the next numbered files.
    //!< Default set to 200 MBytes; Applicable for only logging to file;

} SYNC_TIMING_LOG_CFG_FILTER_T;

/*! LOG Configuration structure */
typedef struct
{
    char                            logModuleName[SYNC_TIMING_CFG_MAX_NAME_SZ];  
    //!< Log Module Name
    uint32_t                        uLogModuleInstId;
    //!< Log Module Instance Id - unique for every register called by the application
    SYNC_TIMING_LOG_CFG_FILTER_T    logToStdoutFilter;
    //!< Log Filter when logging to STDOUT
    SYNC_TIMING_LOG_CFG_FILTER_T    logToFileFilter;
    //!< Log Filter when logging to File
    SYNC_TIMING_BOOL_E              bClearLogFileOnStartup;
    //!< Clear log file on startup - applicable only for File output
    
    SYNC_TIMING_BOOL_E              bTraceLevel;
    //!< Include log trace level string in the trace message
    SYNC_TIMING_BOOL_E              bTraceModuleName;
    //!< Include module name string in the trace message
    SYNC_TIMING_BOOL_E              bTraceMiniTimestamp;
    //!< Include a mini timestamp (TRUE) or full timestamp (FALSE) string in trace message
    SYNC_TIMING_BOOL_E              bTraceTimeStamp;
    //!< Include Timestamp string in trace; If FALSE then no timestamp in trace message
    SYNC_TIMING_BOOL_E              bTraceFuncInfo;
    //!< Include function name and line number strings in trace message

    SYNC_TIMING_BOOL_E              bCustomLogTemplate;
    //!< Use Custom Log Template provided
    char                      logTemplate[SYNC_TIMING_LOG_LEVEL_MAX][SYNC_TIMING_MAX_LOG_FORMAT_SZ];
    //!< Log Output Custom Template for each level

} SYNC_TIMING_LOG_CFG_T;

/*! Log Global Registration Information - maintained in the core log thread for book keeping */
typedef struct
{
    SYNC_TIMING_BOOL_E         bNotFree;
    uint32_t                   uLogModuleInstId;
    char                       logModuleName[SYNC_TIMING_CFG_MAX_NAME_SZ];  
    SYNC_TIMING_BOOL_E         bStdOutLogEnabled;
    SYNC_TIMING_BOOL_E         bFileOutLogEnabled;
    uint32_t                   stdOutLogTraceLevel;
    uint32_t                   fileOutLogTraceLevel;
    char                       LogFileName[SYNC_TIMING_MAX_FILE_NAME_SZ];
    uint32_t                   uNumRotateLogFiles;
    uint32_t                   uMaxLogFileSize;
    void                       *pHandle;
} SYNC_TIMING_LOG_GLOBAL_INFO_T;

/*! Log Global Cfg - Used by sync_timing_util application to change the log level of another application */
typedef struct
{
    uint32_t                        uLogModuleInstId;
    char                            logModuleName[SYNC_TIMING_CFG_MAX_NAME_SZ];  
    void                            *pHandle;
    SYNC_TIMING_LOG_CFG_FILTER_ID_E filterId;
    uint32_t                        logTraceLevel;

} SYNC_TIMING_LOG_MODULE_CFG_T;

/** @} log_if_ds */
#endif  //SYNC_TIMING_LOG_SUPPORTED


/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
*****************************************************************************************/

#ifdef SYNC_TIMING_LOG_SUPPORTED
/**
 *  \defgroup log_if_api LOG controller Functions
 *  @brief LOG Controller Interface Functions for the Timing driver
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_LOG_Register
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/17/2018
 *//**
 * 
 * \brief           This function registers a module with the low level driver log module for the 
 *                  specified module Id
 *
 * \param[in]       pLogCfgData  Pointer to SYNC_TIMING_LOG_CFG_T data.
 * \param[out]      pLogStatus   Pointer to storage to return the operation status
 * \param[out]      ppLogHandle  Log handle that will be used for future log APIs
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_Register (void                        *pLogCfgData, 
                                        SYNC_TIMING_LOG_STATUS_E    *pLogStatus,
                                        void                        **ppLogHandle);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_LOG_SetTraceLevel
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/17/2018
 *//**
 * 
 * \brief           This function set the desired log trace level for a particular module
 *
 * \param[in]       pLogHandle   The log handle
 * \param[in]       logFilterId  Log Filter ID
 * \param[in]       logLevel     Log Level for this moduleId
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_SetTraceLevel (void                               *pLogHandle, 
                                             SYNC_TIMING_LOG_CFG_FILTER_ID_E    logFilterId, 
                                             uint32_t                           logLevel);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_LOG_GetTraceLevel
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     01/16/2021
 *//**
 * 
 * \brief           This function get the current log trace level for a particular module
 *                  Available for Light weight driver only.
 *
 * \param[in]       pLogHandle   The log handle
 * \param[in]       logFilterId  Log Filter ID
 * \param[out]      pLogLevel    Log Level for requested moduleId
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_GetTraceLevel (void                               *pLogHandle, 
                                             SYNC_TIMING_LOG_CFG_FILTER_ID_E    logFilterId, 
                                             uint32_t                           *pLogLevel);                                             

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_LOG_Trace
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/17/2018
 *//**
 * 
 * \brief           This function implements the desired log trace function
 *
 * \param[in]       pLogHandle   The log handle
 * \param[in]       logLevel     Log Level for this message
 * \param[in]       pFuncName    Function Name from where the log is originating
 * \param[in]       LineNum      Line number from where the log is originating
 * \param[in]       fmt          Log Formatting
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
void Sync_Timing_LOG_Trace (void  *pLogHandle, uint32_t logLevel, const char * pFuncName, 
                            const uint32_t LineNum, const char *fmt, ...);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_LOG_DeRegister
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/17/2018
 *//**
 * 
 * \brief           This function de-registers a module with the low level driver log module
 *
 * \param[in]       pLogHandle     The log handle
 * \param[out]      pLogStatus     Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_LOG_DeRegister (void  *pLogHandle, SYNC_TIMING_LOG_STATUS_E *pLogStatus);

/** @} log_if_api */
#endif  //SYNC_TIMING_LOG_SUPPORTED


#define SYNC_TIMING_LOG_DEFAULT_HANDLE               (void *)0xFFFF

#ifdef SYNC_TIMING_LOG_SUPPORTED
#define SYNC_TIMING_CRITICAL(moduleId, format, ...)  Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_CRITICAL, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_ERROR(moduleId, format, ...)     Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_ERROR, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_WARNING(moduleId, format, ...)   Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_WARNING, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_INFO(moduleId, format, ...)      Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_INFO, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_INFO1(moduleId, format, ...)     Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_INFO1, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_INFO2(moduleId, format, ...)     Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_INFO2, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_INFO3(moduleId, format, ...)     Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_INFO3, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_DEBUG(moduleId, format, ...)     Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define SYNC_TIMING_ALWAYS(moduleId, format, ...)    Sync_Timing_LOG_Trace(moduleId, SYNC_TIMING_LOG_LEVEL_ALWAYS, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define SYNC_TIMING_CRITICAL(moduleId, format, ...) printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_CRITICAL : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_ERROR(moduleId, format, ...)    printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_ERROR : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_WARNING(moduleId, format, ...)  printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_WARNING : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_INFO(moduleId, format, ...)     printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_INFO : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_INFO1(moduleId, format, ...)     printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_INFO1 : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_INFO2(moduleId, format, ...)     printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_INFO2 : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_INFO3(moduleId, format, ...)     printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_INFO3 : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#define SYNC_TIMING_DEBUG(moduleId, format, ...)    printf("moduleId: %u : SYNC_TIMING_LOG_LEVEL_DEBUG : Fn=%s : Ln=%d \n", moduleId, __FUNCTION__, __LINE__);
#endif


// Check for Error and DONOT break from the while loop.
#define SYNC_TIMING_ERRCHECK_NO_BREAK(moduleId, retVal, compare)  \
    if ((retVal) != (compare))  \
    { \
        SYNC_TIMING_INFO(moduleId, "compare=%d with val=%d\n", (compare),(retVal)); \
        SYNC_TIMING_ERROR(moduleId, " return value %d\n", (retVal)); \
    }

// Check for Error and break from the while loop.
#define SYNC_TIMING_ERRCHECK_BREAK(moduleId, retVal, compare)  \
    if ((retVal) != (compare))  \
    { \
        SYNC_TIMING_INFO(moduleId, "compare=%d with val=%d\n", (compare),(retVal)); \
        SYNC_TIMING_ERROR(moduleId, " return value %d\n", (retVal)); \
        break; \
    }

// Set Error and break from the while loop.
#define SYNC_TIMING_SET_ERR_BREAK(moduleId, var, retVal)  \
    { \
        (var) = (retVal); \
        SYNC_TIMING_INFO(moduleId, "var=%d, retVal=%d\n", (var),(retVal)); \
        SYNC_TIMING_ERROR(moduleId, " return value %d\n", (retVal)); \
        break; \
    }

// Set return value and break from the while loop.
#define SYNC_TIMING_SET_BREAK(moduleId, var, retVal)  \
    { \
        (var) = (retVal); \
        SYNC_TIMING_INFO(moduleId, "var=%d, retVal=%d\n", (var),(retVal)); \
        SYNC_TIMING_INFO(moduleId, " return value %d\n", (retVal)); \
        break; \
    }

// Set return value and do not break from the while loop.
#define SYNC_TIMING_SET_ERR(moduleId, var, retVal)  \
    { \
        (var) = (retVal); \
        SYNC_TIMING_INFO(moduleId, "var=%d, retVal=%d\n", (var),(retVal)); \
        SYNC_TIMING_INFO(moduleId, " return value %d\n", (retVal)); \
    }

// Set return value and break from the while loop.
#define SYNC_TIMING_SET_ERR_BREAK_NO_TRACE(moduleId, var, retVal)  \
    { \
        (var) = (retVal); \
        break; \
    }

/** @} log_if */
/** @} porting */

#endif // _SYNC_TIMING_OEM_DRIVER_H_


