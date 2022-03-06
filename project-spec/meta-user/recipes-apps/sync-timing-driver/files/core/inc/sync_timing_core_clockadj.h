/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_clockadj.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 07/13/2018
 *
 * DESCRIPTION        : Core Timing Driver APIs that provides chipset clock adjustment 
 *                      access through the OEM Layer
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

#ifndef _SYNC_TIMING_CORE_CLOCKADJ_H_
#define _SYNC_TIMING_CORE_CLOCKADJ_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_osal.h"
#include "sync_timing_core_ctrl.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/


SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_GetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect);

SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_SetPLLInput(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect);

SYNC_STATUS_E Sync_Timing_CORE_ClockAdj_GetCurrentStatus(uint8_t timingDevId, 
                                            SYNC_TIMING_CLOCKADJ_STATUS_ID_E statusType, 
                                            SYNC_TIMING_CLOCKADJ_STATUS_E    *pStatus);


SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_GetPLLInput(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_SetPLLInput(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect);

SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext,
                                            SYNC_TIMING_CLOCKADJ_STATUS_ID_E statusType, 
                                            SYNC_TIMING_CLOCKADJ_STATUS_E    *pStatus);


#endif //_SYNC_TIMING_CORE_CLOCKADJ_H_


