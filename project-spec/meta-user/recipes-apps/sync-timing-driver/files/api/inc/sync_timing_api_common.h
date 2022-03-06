/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *  \addtogroup common
 *  @{
 *  \addtogroup common_ds
 *  @{
 *
 *  \file          sync_timing_api_common.h
 *
 *  \details       Header file for Sync Timing Driver Common API Definitions
 *
 *  \date          Created: 06/29/2018
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
 
#ifndef _SYNC_TIMING_API_COMMON_H_
#define _SYNC_TIMING_API_COMMON_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_log.h"


/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

/** @} common_ds */
/** @} common */
/** @} api */

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/

SYNC_STATUS_E Sync_Timing_API_Driver_Init(void *pAppInfo,
                                          SYNC_TIMING_CALLBACK_FN_T pNotifyFunc,
                                          char *pCfgFileName,
                                          void **pClientAppId,
                                          SYNC_TIMING_DEVICE_INFO_T *pActiveDeviceInfo
                                         );

SYNC_STATUS_E Sync_Timing_API_Driver_Term(void *pClientAppId);


SYNC_STATUS_E Sync_Timing_API_Driver_RegisterEvents_Ex(uint8_t timingDevId, 
                                    SYNC_TIMING_DEVICE_DRIVER_EVENT_E driverEvent,
                                    SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T *pDriverEventFilter);


SYNC_STATUS_E Sync_Timing_API_Device_Reset(uint8_t timingDevId, 
                                           SYNC_TIMING_DEVICE_RESET_TYPE_E chipResetType);

SYNC_STATUS_E Sync_Timing_API_Device_Download(uint8_t timingDevId, uint32_t uNumBootFiles,
                                              char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ]);


SYNC_STATUS_E Sync_Timing_API_Device_GetVersionInfo(uint8_t timingDevId, 
                                                  SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo);

/** \endcond */

#endif // SYNC_TIMING_API_COMMON_H

