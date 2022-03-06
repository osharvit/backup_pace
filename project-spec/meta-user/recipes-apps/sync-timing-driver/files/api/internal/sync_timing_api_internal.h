/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_api_internal.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/03/2018
 *
 * DESCRIPTION        : Timing Driver API Layer internal definitions
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

#ifndef _SYNC_TIMING_API_INTERNAL_H_
#define _SYNC_TIMING_API_INTERNAL_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_cfg_parser.h"

#include "sync_timing_core_communication.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_CLIENT_RECV_MAX_BUFFERS 2

#define SYNC_TIMING_MAX_DEVICE_CHIPSET_EVENT_BLOCKS 4

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/* Client Application context - stores run-time information and access protection etc */
typedef struct
{
    uint32_t                          uClientAppId;
    /* Client Application ID as provided by the client */

    void                             *pClientLogModuleId;
    /* Client module logging ID determined from the client app Id */

    uint32_t                          uRegDriverEvents[SYNC_TIMING_MAX_DEVICES];
    /* Events from SYNC_TIMING_DRIVER_EVENT_E that the application is interested in receiving
       for every timing device on the system */

    uint32_t                          uRegDeviceChipsetEvents[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_DEVICE_CHIPSET_EVENT_BLOCKS];   
    /* Events from SYNC_TIMING_DEVICE_EVENT_INFO_T that is the application is interested in receiving
       for every timing device on the system */
       
   
    SYNC_TIMING_APPLN_INFO_T          ApplnInfo;
    /* Information about the application */


    void                              *pClientAppCtxMutex;
    /* Client mutex for serializing API calls */  

    SYNC_TIMING_BOOL_E                bApiDriverInitialized;
    /* Stores information if the API Driver is already initialized or not */

    SYNC_TIMING_CALLBACK_FN_T         pNotifyFunc;
    /* Notification function registered by the application code for receiving events 
       from the driver */

} SYNC_TIMING_API_CLIENT_APP_CONTEXT_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

SYNC_TIMING_API_CLIENT_APP_CONTEXT_T *Sync_Timing_Internal_API_Ctrl_GetDeviceContext();

SYNC_STATUS_E Sync_Timing_Internal_API_Ctrl_AcqDevice(
                               SYNC_TIMING_API_CLIENT_APP_CONTEXT_T  **ppClientAppContext,
                               SYNC_TIMING_BOOL_E                    *pHeldMutex
                                                     );
SYNC_STATUS_E Sync_Timing_Internal_API_Comm_Init(SYNC_TIMING_APPLN_INFO_T  *pApplnInfo, 
                                                 SYNC_TIMING_APPLN_CFG_T   *pApplnCfg,
                                                 SYNC_TIMING_DEVICE_INFO_T *pActiveDeviceInfo);

SYNC_STATUS_E Sync_Timing_Internal_API_Comm_Term();

SYNC_STATUS_E Sync_Timing_Internal_API_Comm_SendMsg(char *pMsg, uint32_t uMsgLen, 
                                               uint32_t bBlockForResponse, 
                                               uint32_t uRespTimeoutMs,
                                               char **pRecvBuff);

SYNC_STATUS_E Sync_Timing_Internal_API_Comm_SendRegnMsg(char *pMsg, uint32_t uMsgLen, 
                                               uint32_t bBlockForResponse, 
                                               uint32_t uRespTimeoutMs,
                                               char **ppRecvBuff);

SYNC_STATUS_E Sync_Timing_Internal_API_Comm_FreeRecvBuff(char *pRecvBuff);

SYNC_STATUS_E Sync_Timing_Internal_API_Comm_ProcessIncomingMsg(void *pRecvBuff, uint32_t uMsgLen);

#endif //_SYNC_TIMING_API_INTERNAL_H_


