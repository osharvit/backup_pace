/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *  \addtogroup debug
 *  @{
 *  \addtogroup debug_ds
 *  @{
 *
 *  \file          sync_timing_api_debug.h
 *
 *  \details       Header file for Sync Timing Driver Debug API Definitions
 *
 *  \date          Created: 07/30/2018
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

#ifndef _SYNC_TIMING_API_DEBUG_H_
#define _SYNC_TIMING_API_DEBUG_H_

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

/** @} debug_ds */
/** @} debug */
/** @} api */

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/

SYNC_STATUS_E Sync_Timing_API_Debug_SetCurrentMode(uint8_t timingDevId, 
                                                SYNC_TIMING_DEVICE_MODE_E deviceMode);

SYNC_STATUS_E Sync_Timing_API_Debug_GetCurrentMode(uint8_t timingDevId, 
                                                SYNC_TIMING_DEVICE_MODE_E *pCurrDeviceMode);


/** \endcond */

#endif // _SYNC_TIMING_API_DEBUG_H_

