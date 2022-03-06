/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_mem_access.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/28/2018
 *
 * DESCRIPTION        : Core Timing Driver APIs that provides chipset memory read/write  
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

#ifndef _SYNC_TIMING_CORE_MEM_ACCESS_H_
#define _SYNC_TIMING_CORE_MEM_ACCESS_H_

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

SYNC_STATUS_E Sync_Timing_CORE_Mem_WriteDirect(uint8_t timingDevId, uint16_t memAddr, 
                                               uint32_t len, uint8_t* data);

SYNC_STATUS_E Sync_Timing_CORE_Mem_ReadDirect(uint8_t timingDevId, uint16_t memAddr, 
                                              uint32_t len, uint8_t* data);

SYNC_STATUS_E Sync_Timing_CORE_Mem_WriteInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                 uint32_t len, uint8_t* data);

SYNC_STATUS_E Sync_Timing_CORE_Mem_ReadInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                uint32_t len, uint8_t* data);

SYNC_STATUS_E Sync_Timing_CORE_Mem_TransferToBootloader(uint8_t timingDevId, 
                                                      uint32_t len, uint8_t* pData);

SYNC_STATUS_E Sync_Timing_CORE_Mem_APICommand(uint8_t timingDevId, uint8_t* pCommand, 
                                              uint32_t uCmdlen, uint32_t uExpRespLength, 
                                              uint8_t* pCmdResponse, uint8_t *pCmdRespStatus);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_ReadDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteInDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_ReadInDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_TransferToBootLoader(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t len, 
                                    uint8_t* pData);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_RawAPICommand(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t cmdDataLen, 
                                    uint8_t* pCommandData,
                                    SYNC_TIMING_BOOL_E  bWaitForReply,
                                    uint32_t cmdReplyLen,
                                    uint8_t* pReplyData,
                                    uint8_t *pReplyStatus);

SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteCommand(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t cmdDataLen, 
                                    uint8_t* pCommandData,
                                    SYNC_TIMING_BOOL_E  bWaitForReply,
                                    uint32_t cmdReplyLen,
                                    uint8_t* pReplyData, 
                                    uint8_t* pReplyStatus);

#endif //_SYNC_TIMING_CORE_MEM_ACCESS_H_


