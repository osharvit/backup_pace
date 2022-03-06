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
#include "sync_timing_core_log.h"
#include "sync_timing_log_internal.h"


/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_MAX_LOG_LEVEL_STRING_SZ 24

/*****************************************************************************************
  Global Variable Declarations
 ****************************************************************************************/

static SYNC_TIMING_LOG_GLOBAL_CONTEXT_T gSyncTimingLogGlobalContext = {0};


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Log_GlobalInit
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 08/30/2018
 *
 * DESCRIPTION   : This function is used to initialize the Logging device
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Log_GlobalInit()
{

    SYNC_STATUS_E syncStatus      = SYNC_STATUS_SUCCESS;
    gSyncTimingLogGlobalContext.bLogThreadInitialized = SYNC_TIMING_FALSE;

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Log_GetGlobalLogInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 09/04/2018
 *
 * DESCRIPTION   : Function used to get the global log information
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : ppLogGlobalInfo
 *               : pNumRegLogModules
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Log_GetGlobalLogInfo(SYNC_TIMING_LOG_GLOBAL_INFO_T *pLogGlobalInfo,
                                               uint32_t                      *pNumRegLogModules)
{
    SYNC_STATUS_E               syncStatus      = SYNC_STATUS_INVALID_PARAMETER;

    if (pLogGlobalInfo && pNumRegLogModules)
    {
        //printf("gSyncTimingLogGlobalContext.SyncTimingLogGlobalInfo = %p\n", 
        //        gSyncTimingLogGlobalContext.SyncTimingLogGlobalInfo);
        *pNumRegLogModules = gSyncTimingLogGlobalContext.uNumRegLogModules;

        Sync_Timing_OSAL_Wrapper_Memcpy(pLogGlobalInfo, 
                         gSyncTimingLogGlobalContext.SyncTimingLogGlobalInfo, 
                         gSyncTimingLogGlobalContext.uNumRegLogModules * sizeof(SYNC_TIMING_LOG_GLOBAL_INFO_T));

        syncStatus = SYNC_STATUS_SUCCESS;
    }
    return syncStatus;
}



