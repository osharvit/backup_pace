/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *  \addtogroup clockadj
 *  @{
 *  \addtogroup clockadj_ds
 *  @{
 *
 *  \file          sync_timing_api_clockadj.h
 *
 *  \details       Header file for Sync Timing Driver CLOCK ADJUSTMENT API definitions
 *
 *  \date          Created: 07/13/2018
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

#ifndef _SYNC_TIMING_API_CLOCKADJ_H_
#define _SYNC_TIMING_API_CLOCKADJ_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

/** @} clockadj_ds */
/** @} clockadj */
/** @} api */

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/


SYNC_STATUS_E Sync_Timing_API_ClockAdj_GetPLLInput(uint8_t timingDevId, 
                                                SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                                SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect);

SYNC_STATUS_E Sync_Timing_API_ClockAdj_SetPLLInput(uint8_t timingDevId, uint32_t uPLLId, 
                                                SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect);

SYNC_STATUS_E Sync_Timing_API_ClockAdj_GetCurrentStatus(uint8_t timingDevId, 
                                                SYNC_TIMING_CLOCKADJ_STATUS_ID_E statusType, 
                                                SYNC_TIMING_CLOCKADJ_STATUS_E    *pStatus);
/** \endcond */

#endif // _SYNC_TIMING_API_CLOCKADJ_H_

