/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_clockadj.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/13/2018
 *
 * DESCRIPTION        : Core Timing Driver Clock Adjust Functions 
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
#include "sync_timing_core_clockadj.h"
#include "sync_timing_core_communication.h"

#include "sync_timing_oem_driver.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_mem_access.h"
#include "sync_timing_core_driver.h"

/*****************************************************************************************
* Static global variables
****************************************************************************************/

/*****************************************************************************************
    Functions
 ****************************************************************************************/


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_ClockAdj_GetPLLInput
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/01/2020
 *
 * DESCRIPTION   : This function is used to get the current PLL Input Clock
 *
 * IN PARAMS     : timingDevId    - The Timing device ID
 *               : uPLLId         - The PLL Id          
 *
 * OUT PARAMS    : pllInputSelect - The current input for the PLL
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_GetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        syncStatus = Sync_Timing_Internal_CORE_ClockAdj_GetPLLInput(pTimingDevContext, uPLLId, 
                                                                    pllInputSelect);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "ClkAdjGetPLLInput -- uPLLId = %u; "
                                                    "pllInputSelect = %u.\n", 
                                                    uPLLId, pllInputSelect);


    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_ClockAdj_SetPLLInput
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 02/07/2019
 *
 * DESCRIPTION   : This function is used to set the PLL Input Clock
 *
 * IN PARAMS     : timingDevId    - The Timing device ID
 *               : uPLLId         - The PLL Id
 *               : pllInputSelect - The selected input for the PLL
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_SetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "ClkAdjSetPLLInput -- uPLLId = %u; "
                                                    "pllInputSelect = %u.\n", 
                                                    uPLLId, pllInputSelect);

        syncStatus = Sync_Timing_Internal_CORE_ClockAdj_SetPLLInput(pTimingDevContext, uPLLId, 
                                                                    pllInputSelect);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_ClockAdj_GetCurrentStatus
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 02/07/2019
 *
 * DESCRIPTION   : This function is used to get the current status of the PLL or input
 *
 * IN PARAMS     : timingDevId    - The Timing device ID
 *               : statusType     - The object for which status is desired
 *
 * OUT PARAMS    : pStatus        - Pointer to memory to return the status
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_GetCurrentStatus(uint8_t timingDevId, 
                                                        SYNC_TIMING_CLOCKADJ_STATUS_ID_E statusType, 
                                                        SYNC_TIMING_CLOCKADJ_STATUS_E    *pStatus)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "statusType = %u; ", statusType);

        syncStatus = Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus(pTimingDevContext, 
                                                                         statusType, 
                                                                         pStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    
    return syncStatus;
}


