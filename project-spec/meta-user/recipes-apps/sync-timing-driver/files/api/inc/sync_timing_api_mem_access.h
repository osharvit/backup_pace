/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *  \addtogroup mem_access
 *  @{
 *  \addtogroup mem_access_ds
 *  @{
 *
 *  \file          sync_timing_api_mem_access.h
 *
 *  \details       Header file for Sync Timing Driver Memory Access API Definitions
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

#ifndef _SYNC_TIMING_API_MEM_ACCESS_H_
#define _SYNC_TIMING_API_MEM_ACCESS_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

/** @} mem_access_ds */
/** @} mem_access */
/** @} api */

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/


SYNC_STATUS_E Sync_Timing_API_Mem_WriteDirect(uint8_t timingDevId, uint16_t memAddr, 
                                              uint32_t len, uint8_t* pData,
                                              SYNC_TIMING_BOOL_E bNonBlocking,
                                              uint32_t uTimeoutMS);

SYNC_STATUS_E Sync_Timing_API_Mem_ReadDirect(uint8_t timingDevId, uint16_t memAddr, 
                                             uint32_t len, uint8_t *pData);

SYNC_STATUS_E Sync_Timing_API_Mem_WriteInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                uint32_t len, uint8_t *pData,
                                                SYNC_TIMING_BOOL_E bNonBlocking,
                                                uint32_t uTimeoutMS);

SYNC_STATUS_E Sync_Timing_API_Mem_ReadInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                               uint32_t len, uint8_t *pData);

SYNC_STATUS_E Sync_Timing_API_Mem_SendCommand(uint8_t timingDevId, uint8_t* pCommand, 
                                              uint32_t len, 
                                              uint32_t uExpRespLength, 
                                              uint8_t* pCmdStatus,
                                              uint8_t* pCmdResponse,
                                              SYNC_TIMING_BOOL_E bNonBlocking,
                                              uint32_t uTimeoutMS);

/** \endcond */

#endif // SYNC_TIMING_API_MEM_ACCESS_H

