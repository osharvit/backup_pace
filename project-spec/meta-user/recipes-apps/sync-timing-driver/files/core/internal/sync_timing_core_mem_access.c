/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_mem_access.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/28/2018
 *
 * DESCRIPTION        : Core Timing Driver Memory Access Functions 
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
#include "sync_timing_core_mem_access.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver.h"

/*****************************************************************************************
* Static global variables
*****************************************************************************************/

/*****************************************************************************************
* Functions
*****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Mem_WriteDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to perform direct memory write
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : memAddr       - Direct memory address to write to
 *               : len           - Length of data to be read
 *               : pData         - Pointer to memory containing data to be written
 *
 * OUT PARAMS    : None 
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Mem_WriteDirect(uint8_t timingDevId, uint16_t memAddr, 
                                               uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                      syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                 bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData))
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "WrDir -- memAddr 0x%x: len %u: \n", memAddr, len);

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteDirect(pTimingDevContext, 
                                                               memAddr, len, pData);

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Mem_ReadDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to perform direct reads
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : memAddr       - Direct memory address to read from
 *               : len           - Length of data to be read
 *
 * OUT PARAMS    : pData         - Pointer to memory to return read data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Mem_ReadDirect(uint8_t timingDevId, uint16_t memAddr, 
                                              uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                      syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                 bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData))
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        if (len > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, "Requested read len > %u; Truncating\n", 
                                SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE);
            
            len = SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE;
        }

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "RdDir -- memAddr 0x%x: len %u: \n", memAddr, len);

        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                                              memAddr, len, pData);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    }  while(0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Mem_WriteInDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to perform indirect write
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : memAddr       - Indirect memory address to write to
 *               : len           - Length of data to be read
 *               : pData         - Pointer to memory containing data to be written
 *
 * OUT PARAMS    : None 
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Mem_WriteInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                 uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                      syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                 bHoldingMutex          = SYNC_TIMING_FALSE;

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData))
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "WrInDir -- memAddr 0x%x: len %u: \n", memAddr, len);

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteInDirect(pTimingDevContext, 
                                                                 memAddr, len, pData);

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    }  while(0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Mem_ReadInDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/28/2018
 *
 * DESCRIPTION   : This function is used to perform indirect reads
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : memAddr       - Indirect memory address to read from
 *               : len           - Length of data to be read
 *
 * OUT PARAMS    : pData         - Pointer to memory to return read data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Mem_ReadInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                      syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                 bHoldingMutex          = SYNC_TIMING_FALSE;


    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData))
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "RdInDir -- memAddr 0x%x: len %u: \n", memAddr, len);
        
        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadInDirect(pTimingDevContext, 
                                                              memAddr, len, pData);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    }  while(0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;

}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_CORE_Mem_APICommand
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 09/23/2020
 *
 * DESCRIPTION   : This function is used to perform direct reads
 *
 * IN PARAMS     : timingDevId    - Timing Device Id
 *               : pCommand       - API command information
 *               : uCmdlen        - Command information length
 *               : uExpRespLength - Expected response length
 *
 * OUT PARAMS    : pCmdResponse   - Pointer to memory to return command response data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_CORE_Mem_APICommand(uint8_t timingDevId, uint8_t* pCommand, 
                                              uint32_t uCmdlen, uint32_t uExpRespLength, 
                                              uint8_t* pCmdResponse, uint8_t *pCmdRespStatus)
{
    SYNC_STATUS_E                      syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext     = NULL;
    SYNC_TIMING_BOOL_E                 bHoldingMutex          = SYNC_TIMING_FALSE;
    uint8_t                            uReplyStatus           = 0;
    
    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(timingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == uCmdlen) || (!pCommand))
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        if (uCmdlen > SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE+1)
        {
            // Should not hit this case here - the API layer shd have taken care.                    
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                "API Command -- Truncating ... pCommand 0x%x: len %u: \n", 
                                 pCommand[0], uCmdlen);
            uCmdlen = SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE+1;
        }
        
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "API Command -- pCommand 0x%x: len %u: \n", pCommand[0], uCmdlen);

        syncStatus = Sync_Timing_Internal_CORE_Mem_RawAPICommand(pTimingDevContext, 
                                                                uCmdlen, pCommand, SYNC_TIMING_TRUE,
                                                                uExpRespLength, pCmdResponse, 
                                                                &uReplyStatus);
        *pCmdRespStatus = uReplyStatus;
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

    }  while(0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;
}


