/****************************************************************************************/
/**
 *  \defgroup api SYNC TIMING DRIVER API
 *  @{
 *  \defgroup mem_access Memory Access Read/Write
 *  @brief     This section defines the Memory Access Read/Write APIs for the Timing Chipset.
 *  @{
 *  \defgroup mem_access_ds  Memory Access data structures
 *   @brief    Memory Access Data Structures available for the Timing Chipset.
 *  \defgroup mem_access_api Memory Access Read/Write APIs
 *   @brief    Memory Access Functions available for the Timing Chipset.
 *  @{
 *
 *  \file          sync_timing_api_mem_access.c
 *
 *  \details       Implementation file for Timing Driver Memory access APIs that will be 
 *                 used by the application
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

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_api_mem_access.h"
#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_cfg_parser.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_api_internal.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    Static global variables
****************************************************************************************/

/*****************************************************************************************
    Functions
 ****************************************************************************************/


/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Mem_WriteDirect
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/03/2018 
 * 
 *//**
 *  
 *  \brief           This API is used to perform direct memory write into the timing 
 *                   chipset
 * 
 * \param[in]        timingDevId   The Timing Device Id
 * \param[in]        memAddr       Direct memory address to write to
 * \param[in]        len           Length of data to write
 * \param[in]        pData         Pointer to memory containing data to be written
 * \param[in]        bNonBlocking  Indicates if the API needs to block or not for return status 
 *                                 from the HW; Default is blocking;
 * \param[in]        uTimeoutMS    Timeout if blocking is enabled; 
 *                                 If set to 0 and blocking then default value chosen is 
 *                                 SYNC_TIMING_OSAL_WAIT_FOREVER
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             Please use the Non-Blocking option with caution.  
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Mem_WriteDirect(uint8_t timingDevId, uint16_t memAddr, 
                                               uint32_t len, uint8_t* pData,
                                               SYNC_TIMING_BOOL_E bNonBlocking,
                                               uint32_t uTimeoutMS)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      memAccessMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      *pRecvBuff             = NULL;
    uint32_t                               uRespTimeoutMS         = uTimeoutMS;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData) || (len > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE))
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send MEM WRITE DIRECT command to core */
        Sync_Timing_OSAL_Wrapper_Memset(&(memAccessMsg), 0, sizeof(memAccessMsg));

        memAccessMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        memAccessMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_WRITE_DIRECT;

        /* Copy data to write into the message buffer */
        Sync_Timing_OSAL_Wrapper_Memcpy(&(memAccessMsg.data[0]),
                                        (const void *)pData, 
                                        len);

        memAccessMsg.memAddr = memAddr;
        memAccessMsg.len     = len;

        memAccessMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        memAccessMsg.msgHdr.uTimingDevId            = timingDevId;
        memAccessMsg.msgHdr.bRespReqd               = !(bNonBlocking);
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_WRITE_DIRECT CMD "
                          "(memAddr = 0x%04x, len = %u)to CORE\n", memAddr, len);

        if (uRespTimeoutMS == 0)
        {
            uRespTimeoutMS = SYNC_TIMING_OSAL_WAIT_FOREVER;
        }
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&memAccessMsg, sizeof(memAccessMsg), 
                                                      !(bNonBlocking), 
                                                      uRespTimeoutMS,
                                                      (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_FAILURE);
        }

        if (memAccessMsg.msgHdr.bRespReqd == SYNC_TIMING_TRUE)
        {
            if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                                  "Sync_Timing_API_Mem_WriteDirect failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                                syncStatus, pRecvBuff->msgHdr.reqStatus);
            }
        }

    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Mem_ReadDirect
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/03/2018 
 * 
 *//**
 *  
 *  \brief           This API is used to perform direct memory read from the timing 
 *                   chipset
 * 
 * \param[in]        timingDevId   The Timing Device Id
 * \param[in]        memAddr       Direct memory address to read from
 * \param[in]        len           Length of data to write
 * \param[in]        pData         Pointer to memory containing space for data read back
 *                                 to be written
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Mem_ReadDirect(uint8_t timingDevId, uint16_t memAddr, 
                                              uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      memAccessMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      *pRecvBuff             = NULL;

    do
    {
      syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                           &bHoldingMutex);
      SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_SUCCESS);

      if ((0 == len) || (!pData) || (len > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE))
      {
          SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, 
                                    SYNC_STATUS_INVALID_PARAMETER);
      }

      /* Send MEM WRITE DIRECT command to core */
      Sync_Timing_OSAL_Wrapper_Memset(&(memAccessMsg), 0, sizeof(memAccessMsg));

      memAccessMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
      memAccessMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_READ_DIRECT;
      memAccessMsg.memAddr                        = memAddr;
      memAccessMsg.len                            = len;

      memAccessMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
      memAccessMsg.msgHdr.uTimingDevId            = timingDevId;
      memAccessMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
      
      SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                        "Sending SYNC_TIMING_CORE_MSG_READ_DIRECT CMD "
                        "(memAddr = 0x%04x, len = %u)to CORE\n", memAddr, len);
      
      syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&memAccessMsg, sizeof(memAccessMsg), 
                                                    SYNC_TIMING_TRUE, 
                                                    SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                    (char **)&pRecvBuff);
      if (syncStatus != SYNC_STATUS_SUCCESS)
      {
          SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                            "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
          SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                          syncStatus, SYNC_STATUS_FAILURE);
      }

      if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
      {
          SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                           "Sync_Timing_API_Mem_ReadDirect failed: %d \n", syncStatus);
          SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                          syncStatus, pRecvBuff->msgHdr.reqStatus);
      }
      else
      {
          /* Copy data read back into the return buffer */
          Sync_Timing_OSAL_Wrapper_Memcpy(pData, (const void *)&(pRecvBuff->data[0]), len);
      }
      
    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
      (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Mem_WriteInDirect
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/03/2018 
 * 
 *//**
 *  
 *  \brief           This API is used to perform indirect memory write into the timing 
 *                   chipset
 * 
 * \param[in]        timingDevId   The Timing Device Id
 * \param[in]        memAddr       Direct memory address to write to
 * \param[in]        len           Length of data to write
 * \param[in]        pData         Pointer to memory containing data to be written
 * \param[in]        bNonBlocking  Indicates if the API needs to block or not for return status 
 *                                 from the HW; Default is blocking when 0 is sent as input;
 * \param[in]        uTimeoutMS    Timeout if blocking is enabled; 
 *                                 If set to 0 and blocking then default value chosen is 
 *                                 SYNC_TIMING_OSAL_WAIT_FOREVER
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             Please use the Non-Blocking option with caution.  
 *                   This API is not supported for Aruba. Only available in Si5389 chipsets. 
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Mem_WriteInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                uint32_t len, uint8_t* pData,
                                                SYNC_TIMING_BOOL_E bNonBlocking,
                                                uint32_t uTimeoutMS)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      memAccessMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      *pRecvBuff             = NULL;
    uint32_t                               uRespTimeoutMS         = uTimeoutMS;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                              syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData) || (len > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE))
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send MEM WRITE DIRECT command to core */
        Sync_Timing_OSAL_Wrapper_Memset(&(memAccessMsg), 0, sizeof(memAccessMsg));

        memAccessMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        memAccessMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_WRITE_INDIRECT;

        /* Copy data to write into the message buffer */
        Sync_Timing_OSAL_Wrapper_Memcpy(&(memAccessMsg.data[0]),
                                        (const void *)pData, 
                                        len);

        memAccessMsg.memAddr = memAddr;
        memAccessMsg.len     = len;

        memAccessMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        memAccessMsg.msgHdr.uTimingDevId            = timingDevId;
        memAccessMsg.msgHdr.bRespReqd               = !(bNonBlocking);
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_WRITE_INDIRECT CMD "
                          "(memAddr = 0x%04x, len = %u)to CORE\n", memAddr, len);

        if (uRespTimeoutMS == 0)
        {
            uRespTimeoutMS = SYNC_TIMING_OSAL_WAIT_FOREVER;
        }
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&memAccessMsg, sizeof(memAccessMsg), 
                                                      !(bNonBlocking), 
                                                      uRespTimeoutMS,
                                                      (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                             "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_FAILURE);
        }

        if (memAccessMsg.msgHdr.bRespReqd == SYNC_TIMING_TRUE)
        {
            if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                                 "Sync_Timing_API_Mem_WriteDirect failed: %d \n", syncStatus);
                SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                                syncStatus, pRecvBuff->msgHdr.reqStatus);
            }
        }

    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Mem_ReadInDirect
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      07/03/2018 
 * 
 *//**
 *  
 *  \brief           This API is used to perform indirect memory read from the timing 
 *                   chipset
 * 
 * \param[in]        timingDevId   The Timing Device Id
 * \param[in]        memAddr       Direct memory address to read from
 * \param[in]        len           Length of data to write
 * \param[in]        pData         Pointer to memory containing space for data read back
 *                                 to be written
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             This API is not supported for Aruba. Only available in Si5389 chipsets.
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Mem_ReadInDirect(uint8_t timingDevId, uint16_t memAddr, 
                                                uint32_t len, uint8_t* pData)
{
    SYNC_STATUS_E                          syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                          retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      memAccessMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T   *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                     bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_MEM_ACCESS_T      *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                             &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                             syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == len) || (!pData) || (len > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE))
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send MEM WRITE DIRECT command to core */
        Sync_Timing_OSAL_Wrapper_Memset(&(memAccessMsg), 0, sizeof(memAccessMsg));

        memAccessMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        memAccessMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_READ_INDIRECT;
        memAccessMsg.memAddr                        = memAddr;
        memAccessMsg.len                            = len;

        memAccessMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        memAccessMsg.msgHdr.uTimingDevId            = timingDevId;
        memAccessMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;
        
        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                          "Sending SYNC_TIMING_CORE_MSG_READ_INDIRECT CMD "
                          "(memAddr = 0x%04x, len = %u)to CORE\n", memAddr, len);
        
        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&memAccessMsg, sizeof(memAccessMsg), 
                                                      SYNC_TIMING_TRUE, 
                                                      SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                      (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                             "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                              "Sync_Timing_API_Mem_ReadDirect failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            /* Copy data read back into the message buffer */
            Sync_Timing_OSAL_Wrapper_Memcpy(pData, (const void *)&(pRecvBuff->data[0]), len);
        }
      
    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;
}

/***************************************************************************************
 * FUNCTION NAME     Sync_Timing_API_Mem_SendCommand
 *
 * AUTHOR            Srini Venkataraman
 *
 * DATE CREATED      09/23/2020 
 * 
 *//**
 *  
 *  \brief           This API is used to send RAW command to the timing chipset and get
 *                   back a response (Aruba chipsets only)
 * 
 * \param[in]        timingDevId    The Timing Device Id
 * \param[in]        pCommand       Command byte string 
 * \param[in]        uCmdlen        Length of the command string
 * \param[in]        uExpRespLength Expected length of the command response (excluding the cmd resp status byte)
 * \param[in]        bNonBlocking   Indicates if the API needs to block or not for return status 
 *                                  from the HW; Default is blocking; 
 *                                  Non-blocking is not supported as yet.
 * \param[in]        uTimeoutMS     Timeout if blocking is enabled; 
 *                                  If set to 0 and blocking then default value chosen is 
 *                                  SYNC_TIMING_OSAL_WAIT_FOREVER; Timeout is not supported as yet;
 *
 * \param[out]       pCmdStatus     Command response status
 * \param[out]       pCmdResponse   Pointer to memory to store the command response
 *
 * \returns          SYNC_STATUS_SUCCESS, SYNC_STATUS_ALREADY_INITIALIZED, 
 *                   SYNC_STATUS_INVALID_PARAMETER, SYNC_STATUS_NOT_INITIALIZED,
 *                   SYNC_STATUS_FAILURE
 *
 * \note             Please note that support for non-blocking and timeout usage is planned for future. 
 *                   This API is not supported for Si5389. Only available in Aruba based chipsets.
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_API_Mem_SendCommand(uint8_t timingDevId, uint8_t* pCommand, 
                                              uint32_t uCmdlen, uint32_t uExpRespLength,
                                              uint8_t* pCmdStatus,
                                              uint8_t* pCmdResponse,
                                              SYNC_TIMING_BOOL_E bNonBlocking,
                                              uint32_t uTimeoutMS)
{
    SYNC_STATUS_E                           syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_STATUS_E                           retStatus              = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CORE_MSG_API_CMD_T          apiCmdMsg;
    SYNC_TIMING_API_CLIENT_APP_CONTEXT_T    *pClientAppContext     = NULL;
    SYNC_TIMING_BOOL_E                      bHoldingMutex          = SYNC_TIMING_FALSE;
    SYNC_TIMING_CORE_MSG_API_CMD_T          *pRecvBuff             = NULL;

    do
    {
        syncStatus = Sync_Timing_Internal_API_Ctrl_AcqDevice(&pClientAppContext, 
                                                           &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pClientAppContext->pClientLogModuleId, 
                            syncStatus, SYNC_STATUS_SUCCESS);

        if ((0 == uCmdlen) || (!pCommand) || (uCmdlen > SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE+1))
        {
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
        }

        /* Send MEM WRITE DIRECT command to core */
        Sync_Timing_OSAL_Wrapper_Memset(&(apiCmdMsg), 0, sizeof(apiCmdMsg));

        apiCmdMsg.msgHdr.coreMsgType             = SYNC_TIMING_CORE_MSG_TYPE_REQ;
        apiCmdMsg.msgHdr.coreMsgCmd              = SYNC_TIMING_CORE_MSG_API_COMMAND;

        Sync_Timing_OSAL_Wrapper_Memcpy((void *)&apiCmdMsg.command[0], (const void *)pCommand, 
                                        uCmdlen);

        apiCmdMsg.uCmdLen                        = uCmdlen;
        apiCmdMsg.uExpCmdRespLen                 = uExpRespLength;

        apiCmdMsg.msgHdr.uClientAppId            = pClientAppContext->uClientAppId;
        apiCmdMsg.msgHdr.uTimingDevId            = timingDevId;
        apiCmdMsg.msgHdr.bRespReqd               = SYNC_TIMING_TRUE;

        SYNC_TIMING_DEBUG(pClientAppContext->pClientLogModuleId, 
                        "Sending SYNC_TIMING_CORE_MSG_API_COMMAND CMD "
                        "(uCmdlen = %u)to CORE\n", uCmdlen);

        syncStatus = Sync_Timing_Internal_API_Comm_SendMsg((char *)&apiCmdMsg, sizeof(apiCmdMsg), 
                                                    SYNC_TIMING_TRUE, 
                                                    SYNC_TIMING_OSAL_WAIT_FOREVER,
                                                    (char **)&pRecvBuff);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                            "Sync_Timing_Internal_API_Comm_SendMsg failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                          syncStatus, SYNC_STATUS_FAILURE);
        }

        if (pRecvBuff->msgHdr.reqStatus != SYNC_STATUS_SUCCESS)
        {
            *pCmdStatus = pRecvBuff->uCmdStatus;
            SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                           "Sync_Timing_API_Mem_SendCommand failed: %d \n", syncStatus);
            SYNC_TIMING_SET_ERR_BREAK(pClientAppContext->pClientLogModuleId, 
                          syncStatus, pRecvBuff->msgHdr.reqStatus);
        }
        else
        {
            *pCmdStatus = pRecvBuff->uCmdStatus;
            if (pCmdResponse && (uExpRespLength > 0))
            {
                /* Copy data read back into the return buffer */
                Sync_Timing_OSAL_Wrapper_Memcpy((void *)pCmdResponse, 
                                                (const void *)&(pRecvBuff->cmdResponse[0]), 
                                                uExpRespLength);
            }
        }
      
    } while (0);

    retStatus = Sync_Timing_Internal_API_Comm_FreeRecvBuff((char *)pRecvBuff);
    if (retStatus != SYNC_STATUS_SUCCESS)
    {
        SYNC_TIMING_ERROR(pClientAppContext->pClientLogModuleId, 
                         "Sync_Timing_Internal_API_Comm_FreeRecvBuff failed: %d \n", retStatus);
        SYNC_TIMING_SET_ERR(pClientAppContext->pClientLogModuleId, 
                         syncStatus, retStatus);
    }


    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
      (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pClientAppContext->pClientAppCtxMutex);
    }

    return syncStatus;

}


/** @} mem_access_api */
/** @} mem_access */
/** @} api    */

