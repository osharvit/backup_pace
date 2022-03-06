/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_ctrl.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Core Driver Device Context Ctrl Routines
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
#include "sync_timing_core_ctrl.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver.h"


/*****************************************************************************************
* Static global variables
*****************************************************************************************/

// Ensure Sync_Timing_Core_Devices[] is zero initialized
static SYNC_TIMING_CORE_DEVICE_CONTEXT_T gSyncTimingCoreDevices[SYNC_TIMING_MAX_DEVICES] = {{0}};

/*****************************************************************************************
* Functions
*****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Ctrl_GetDeviceContext
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/27/2018
 *
 * DESCRIPTION   : This function is used to get the device context for a given timing device
 *
 * IN PARAMS     : timingDevId - Timing device Id
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_TIMING_CORE_DEVICE_CONTEXT_T *Sync_Timing_CORE_Ctrl_GetDeviceContext(uint8_t timingDevId)
{
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext = gSyncTimingCoreDevices;

    return ((timingDevId < SYNC_TIMING_MAX_DEVICES) && (pTimingDevContext != NULL)) ?
      (&pTimingDevContext[timingDevId]) : NULL;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Ctrl_AcqDevice
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/27/2018
 *
 * DESCRIPTION   : This function is used to acquire and lock the device for use
 *
 * IN PARAMS     : timingDevId  -  Timing device Id
 *
 * OUT PARAMS    : ppTimingDevContext - The device context structure
 *               : pHeldMutex         - An indicate signifying if the mutex has been 
 *                                      acquired or not.
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Ctrl_AcqDevice(uint8_t                timingDevId,
                                  SYNC_TIMING_CORE_DEVICE_CONTEXT_T  **ppTimingDevContext,
                                  SYNC_TIMING_BOOL_E                 *pHeldMutex
                                             )
{
    SYNC_STATUS_E                     syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext = NULL;

    if (pHeldMutex != NULL)
    {
        *pHeldMutex = SYNC_TIMING_FALSE;
    }

    do
    {
        if (timingDevId >= SYNC_TIMING_MAX_DEVICES)
        {
            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, "Requested device ID %d exceeds devices available\n", timingDevId);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        pTimingDevContext = Sync_Timing_CORE_Ctrl_GetDeviceContext(timingDevId);
        if (NULL == pTimingDevContext)
        {
            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, "Unable to find device context for device ID %d\n", timingDevId);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_INITIALIZED);
        }

        if (SYNC_TIMING_FALSE == pTimingDevContext->bDriverInitialized)
        {
            SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, "Device ID %d is not initialized\n", timingDevId);
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_INITIALIZED);
        }

        *ppTimingDevContext = pTimingDevContext;

        if (pHeldMutex != NULL)
        {
            if (Sync_Timing_OSAL_Wrapper_Mutex_Get(pTimingDevContext->pMutex, SYNC_TIMING_OSAL_WAIT_FOREVER)
                  != SYNC_TIMING_OSAL_SUCCESS)
            {
                SYNC_TIMING_INFO(pSyncTimingCoreLogHandle, "Unable to take mutex for device ID %d\n", timingDevId);
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }
            *pHeldMutex = SYNC_TIMING_TRUE;
        }
    } while(0);

    return syncStatus;
}


