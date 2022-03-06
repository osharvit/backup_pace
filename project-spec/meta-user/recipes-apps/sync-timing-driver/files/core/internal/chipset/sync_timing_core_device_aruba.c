/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_device_aruba.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/18/2019
 *
 * DESCRIPTION        : Core Timing Driver Aruba Device Access Functions 
 *
 ****************************************************************************************/

/****************************************************************************************/
/**                  Copyright (c) 2019, 2021 Skyworks Solution Inc.                   **/
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
#include "sync_timing_oem_driver.h"
#include "sync_timing_core_mem_access.h"
#include "sync_timing_core_interface.h"
#include "sync_timing_core_driver.h"
#include "sync_timing_core_aruba_interface.h"
#include "sync_timing_core_clockadj.h"


#include "sync_timing_core_communication.h"
#include "sync_timing_core_driver_version.h"
#include "sync_timing_aruba_cmd.h"
#include "sync_timing_aruba_cmd_map.h"
#include "iniparser.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_ADDL_TRACE 1
#define SYNC_TIMING_USE_DRIVER_1PPS 0
#define SYNC_TIMING_DCO_MB 0

#define SYNC_TIMING_MIN_FW_MAJOR_VERSION_AUTO_MRV_DCO 0
#define SYNC_TIMING_MIN_FW_MINOR_VERSION_AUTO_MRV_DCO 9

#define SYNC_TIMING_FW_MAJOR_VERSION_CLEAR_STATUS 0
#define SYNC_TIMING_FW_MINOR_VERSION_CLEAR_STATUS 10
#define SYNC_TIMING_FW_BRANCH_CLEAR_STATUS 0


#define SYNC_TIMING_MEASURE_FW_DLOAD_TIME 0

/*****************************************************************************************
* Static global variables
*****************************************************************************************/

static const char gVerBldType[SYNC_TIMING_MAX_BUILD_TYPES][SYNC_TIMING_MAX_BUILD_TYPE_STRING_SIZE] = 
                                    {
                                        {""}, // Production - leave empty
                                        {"-Hotfix"},
                                        {"-CustomerSpecial"},
                                        {"-Pre-Release"},
                                        {"-Experimental"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Unused/Invalid"},
                                        {"-Dev"}
                                    };

static SYNC_TIMING_BOOL_E           gbTerminateIntrPollThread   = SYNC_TIMING_FALSE;


static const char                   gChipRevToStrMap[2][2] = {"A", "B"};

static const uint32_t               gFWToDriverPllMap[3] = {SYNC_TIMING_CLOCKADJ_STATUS_PLL_R,
                                                            SYNC_TIMING_CLOCKADJ_STATUS_PLL_A,
                                                            SYNC_TIMING_CLOCKADJ_STATUS_PLL_B
                                                           };

/*****************************************************************************************
* Functions
*****************************************************************************************/

                                    
/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_WriteDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : Internal function is used to perform direct write on Internal chipset
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : memAddr            - Indirect memory address to write to
 *               : len                - Length of data to be read
 *               : pData              - Pointer to memory containing data to be written
 *
 * OUT PARAMS    : None 
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext,
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData)
{
    SYNC_STATUS_E                      syncStatus                  = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_OEM_DATAPATH_STATUS_E  syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
    uint8_t  headerLen  = 0;
    uint8_t txBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C
    uint8_t rxBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C

    Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);

    //if (pTimingDevContext->deviceMode != SYNC_TIMING_DEVICE_MODE_APPLN)
    //{
    //   SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Device not in Application Mode\n");
    //    return SYNC_STATUS_NOT_READY;
    //}

    SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, "Internal Core write %u bytes to direct addr 0x%02x ARUBA \n",
                                                 len, memAddr);

    if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
    {

        if(len == 1)     // single register, single byte writes
        {
            SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, "Internal Core write 1 byte \n");

            txBuff[0] = SYNC_TIMING_OP_SET_ADDR;            // Set Address operation
            txBuff[1] = memAddr & 0xff;                     // addrlo
            txBuff[2] = (memAddr >> 8) & 0xff;              // addrhi
            txBuff[3] = SYNC_TIMING_OP_WR_WDATA;            // Write Data
            Sync_Timing_OSAL_Wrapper_Memcpy(&txBuff[4], pData, len);
            headerLen = 4;
        }
        else                                               // subsequent register, multi-byte writes
        {
            SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, "Internal Core write >1 byte \n");

            txBuff[0] = SYNC_TIMING_OP_SET_ADDR;            // Set Address operation
            txBuff[1] = memAddr & 0xff;                     // addrlo
            txBuff[2] = (memAddr >> 8) & 0xff;              // addrhi
            txBuff[3] = SYNC_TIMING_OP_WR_WBURSTU;          // Write Burst, Update Address
            Sync_Timing_OSAL_Wrapper_Memcpy(&txBuff[4], pData, len);
            headerLen = 4;
        }
    }
    else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
    {

        txBuff[0] = memAddr & 0xff;                  //addrlo
        txBuff[1] = (memAddr >> 8) & 0xff;           //addrhi
        Sync_Timing_OSAL_Wrapper_Memcpy(&txBuff[2], pData, len);
        headerLen = 2;
    }
    else
    {
        syncStatus = SYNC_STATUS_NOT_SUPPORTED;
        syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
    }
    syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(pTimingDevContext->pOemDataPathHandle,
                                                      &txBuff[0], headerLen + len, &rxBuff[0], len,
                                                      &syncTimingOemDataPathStatus);

    if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
    {
        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                          "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                          syncTimingOemDataPathStatus);
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_ReadDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : Internal function is used to perform direct reads
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : memAddr            - Direct memory address to read from
 *               : len                - Length of data to be read
 *
 * OUT PARAMS    : pData         - Pointer to memory to return read data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_ReadDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData)
{
    SYNC_STATUS_E                    syncStatus             = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_OEM_DATAPATH_STATUS_E  syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;

    uint32_t   read_idx    = 0;
    uint32_t   readOffset  = 0;
    uint8_t    headerLen   = 0;
    uint8_t    txBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C
    uint8_t    rxBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C

    Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);
    Sync_Timing_OSAL_Wrapper_Memset(&rxBuff[0], 0, SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);

    //if (pTimingDevContext->deviceMode != SYNC_TIMING_DEVICE_MODE_APPLN)
    //{
    //    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Device not in Application Mode\n");
    //    return SYNC_STATUS_NOT_READY;
    //}

    //TODO - Handle address greater than 64K (sync only access)
    SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, "Internal Core read %u bytes from direct addr 0x%02x ARUBA \n",
                                                 len, memAddr);

    if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
    {
        txBuff[0]  = SYNC_TIMING_OP_SET_ADDR;            // Set Address operation
        txBuff[1]  = memAddr & 0xff;                     // addrlo
        txBuff[2]  = (memAddr >> 8) & 0xff;              // addrhi
        txBuff[3]  = SYNC_TIMING_OP_RD_RBURSTU;          // Read Burst, Update Address
        headerLen  = 4;                                  // Header length for SPI transfer buffer
        readOffset = 4;                                  // Read offset for reading from SPI device
    }
    else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
    {
        txBuff[0]  = memAddr & 0xff;                  //addrlo
        txBuff[1]  = (memAddr >> 8) & 0xff;           //addrhi
        headerLen  = 2;                               // Header length for SPI transfer buffer
        readOffset = 0;                               // Read offset for reading from SPI device
    }
    else
    {
        syncStatus = SYNC_STATUS_NOT_SUPPORTED;
        syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
    }
    syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(pTimingDevContext->pOemDataPathHandle,
                                        &txBuff[0], headerLen + len, &rxBuff[0], len,
                                        &syncTimingOemDataPathStatus);

    if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
    {
        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                          "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                          syncTimingOemDataPathStatus);
    }

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        // Copy the data payload to the receive buffer.
        for (read_idx = 0; read_idx < len; read_idx++) 
        {
            pData[read_idx] = rxBuff[readOffset + read_idx];
            SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle,"RdByte %d: \n", pData[read_idx]);
        }
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_WriteInDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : Internal function is used to perform indirect write
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : memAddr            - Indirect memory address to write to
 *               : len                - Length of data to be read
 *               : pData              - Pointer to memory containing data to be written
 *
 * OUT PARAMS    : None 
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteInDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData)
{
    SYNC_STATUS_E    syncStatus             = SYNC_STATUS_NOT_SUPPORTED;

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_ReadInDirect
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : Internal function is used to perform indirect reads
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : memAddr            - Direct memory address to read from
 *               : len                - Length of data to be read
 *
 * OUT PARAMS    : pData         - Pointer to memory to return read data
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_ReadInDirect(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint16_t memAddr, 
                                    uint32_t len, 
                                    uint8_t* pData)
{
    SYNC_STATUS_E    syncStatus             = SYNC_STATUS_NOT_SUPPORTED;

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_WaitForCTS
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/05/2020
 *
 * DESCRIPTION   : Internal function is used to wait for CTS ready before sending CMD
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : len                - Length of data to be read
 *               : pData              - Pointer to memory containing data to be written
 *
 * OUT PARAMS    : pData[0]           - Contains the BL Status byte 
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_WaitForCTS(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext)
{
    SYNC_STATUS_E       syncStatus              = SYNC_STATUS_NOT_SUPPORTED;
    SYNC_TIMING_OEM_DATAPATH_STATUS_E  syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
    uint32_t            uCount                  = 0,
                        readOffset              = 0;
    uint8_t             headerLen               = 0;
    uint8_t txBuff[SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C
    uint8_t rxBuff[SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C

    Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);
    Sync_Timing_OSAL_Wrapper_Memset(&rxBuff[0], 0, SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);

    do
    {
        // Check (and wait if necessary) for CTS bit to be set to 1
        Sync_Timing_OSAL_Wrapper_Memset(&rxBuff[0], 0, SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                                 SYNC_TIMING_MAX_SPI_HDR_SIZE);

        while(1)
        {
            if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
            {
                readOffset = 1;
                txBuff[0] = SYNC_TIMING_OP_API_REPLY;
                headerLen = 1;

            }
            else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
            {
                readOffset = 0;
                txBuff[0] = SYNC_TIMING_OP_API_CMD_REPLY & 0xff;
                txBuff[1] = (SYNC_TIMING_OP_API_CMD_REPLY >> 8) & 0xff;
                headerLen = 2;

            }
            else
            {
                syncStatus = SYNC_STATUS_NOT_SUPPORTED;
                syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
            }
            syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(
                                                  pTimingDevContext->pOemDataPathHandle,
                                                  &txBuff[0], headerLen + 32, &rxBuff[0],
                                                  32, &syncTimingOemDataPathStatus);


            if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                                  syncTimingOemDataPathStatus);
            }
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle,"API Reply: 0x%02X 0x%02X\n", 
                                                          rxBuff[0], rxBuff[1]);

            if ((rxBuff[readOffset] & 0x80) == 0x80)
                break;

            uCount++;
            if ((uCount % 1000) == 0)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_Internal_CORE_Mem_WriteCommand - uCount %u\n", 
                                  uCount);
            }            
            if (uCount > 20000) 
            {
                syncStatus = SYNC_STATUS_TIMEOUT;
                break;
            }        
        }

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   
    }
    while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_RawAPICommand
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/11/2020
 *
 * DESCRIPTION   : Internal wrapper function is used to send an API command to the firmware
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : cmdDataLen         - Length of command data
 *               : pCommandData       - Pointer to memory containing command and command data
 *               : bWaitForReply      - Wait for command response
 *               : cmdReplyLen        - length of command response
 *
 * OUT PARAMS    : pReplyData         - Memory for returning command response
 *               : pReplyStatus       - Command response status
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_RawAPICommand(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t cmdDataLen, 
                                    uint8_t* pCommandData,
                                    SYNC_TIMING_BOOL_E  bWaitForReply,
                                    uint32_t cmdReplyLen,
                                    uint8_t* pReplyData,
                                    uint8_t *pReplyStatus)
{
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CLOCKADJ_STATUS_E       clkStatus           = 0;
    uint8_t                             uPllId              = 0;
    uint8_t                             uReplyStatus        = 0;

    do
    {
        if (pCommandData[0] == cmd_ID_PLL_ACTIVE_REFCLOCK)
        {
            uPllId = pCommandData[1];
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                              "Sync_Timing_Internal_CORE_Mem_RawAPICommand -- uPllId = %u.\n", 
                               uPllId);

            syncStatus = Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus(pTimingDevContext, 
                                                                             gFWToDriverPllMap[uPllId],
                                                                             &clkStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "ClkAdjGetPLLInput -- clkStatus = 0x%x.\n", 
                                                                  clkStatus);

            /*if ((clkStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED) 
                                                      != SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED)
            {
                SYNC_TIMING_SET_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_READY);
            } */         
                                                      
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    cmdDataLen, pCommandData, 
                                                                    SYNC_TIMING_TRUE,
                                                                    cmdReplyLen, pReplyData, 
                                                                    &uReplyStatus);
            *pReplyStatus = uReplyStatus;
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        }
        else
        {
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    cmdDataLen, pCommandData, 
                                                                    SYNC_TIMING_TRUE,
                                                                    cmdReplyLen, pReplyData, 
                                                                    &uReplyStatus);
            *pReplyStatus = uReplyStatus;
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        }
        
    } while(0);
        
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Mem_WriteCommand
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 01/15/2020
 *
 * DESCRIPTION   : Internal function is used to send an API command to the firmware
 *
 * IN PARAMS     : pTimingDevContext  - Timing device context
 *               : cmdDataLen         - Length of command data
 *               : pCommandData       - Pointer to memory containing command and command data
 *               : bWaitForReply      - Wait for command response
 *               : cmdReplyLen        - length of command response
 *
 * OUT PARAMS    : pReplyData         - Memory for returning command response
 *               : pReplyStatus       - Command response status
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE or
 *                  SYNC_STATUS_NOT_SUPPORTED
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Mem_WriteCommand(
                                    SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                    uint32_t cmdDataLen, 
                                    uint8_t* pCommandData,
                                    SYNC_TIMING_BOOL_E  bWaitForReply,
                                    uint32_t cmdReplyLen,
                                    uint8_t* pReplyData,
                                    uint8_t *pReplyStatus)
{

    SYNC_STATUS_E       syncStatus              = SYNC_STATUS_NOT_SUPPORTED;
    SYNC_TIMING_OEM_DATAPATH_STATUS_E  syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
    uint32_t            uCount                  = 0,
                        ui                      = 0,
                        readOffset              = 0;
    uint8_t             headerLen               = 0;
    uint8_t             txBuff[SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C
    uint8_t             rxBuff[SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    // maximum size for SPI and I2C
    uint32_t            index                   = 0;
    uint32_t            count                   = 0;
    uint32_t            uCmdPartitionSize       = SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE;
    uint32_t            uDataLen                = 0;
    uint32_t            uActualCmdDataLen       = cmdDataLen - 1;
    uint8_t             uCmd                    = 0;
    uint8_t             *pActualCmdData         = NULL;

    *pReplyStatus = 0;
    Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);
    Sync_Timing_OSAL_Wrapper_Memset(&rxBuff[0], 0, SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE +
                                                             SYNC_TIMING_MAX_SPI_HDR_SIZE);

    do
    {
        SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle, "cmd (0x%x) DataLen (incl. cmd byte) BEING SENT: %u; "
                                                    "Expected cmdReplyLen = %u \n", 
                                                    pCommandData[0], cmdDataLen, cmdReplyLen);        
        if (pCommandData)
        {
            uCmd = pCommandData[0];

            if (uActualCmdDataLen == 0)
            {
                pActualCmdData = NULL;
            }
            else
            {
                pActualCmdData = &pCommandData[1];
            }
        }

        if (uActualCmdDataLen == 0 || pActualCmdData == NULL)
        {
            syncStatus = Sync_Timing_Internal_CORE_WaitForCTS(pTimingDevContext);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, uCmdPartitionSize);

            if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
            {
                // Send Command SYNC_TIMING_OP_API_CMD
                txBuff[0] = SYNC_TIMING_OP_API_CMD;
                txBuff[1] = uCmd;
                uDataLen = 2;
            }
            else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
            {
                // Send Command SYNC_TIMING_OP_API_CMD
                txBuff[0] = SYNC_TIMING_OP_API_CMD_REPLY & 0xff;
                txBuff[1] = (SYNC_TIMING_OP_API_CMD_REPLY >> 8) & 0xff;
                txBuff[2] = uCmd;
                uDataLen = 3;
            }
            else
            {
                syncStatus = SYNC_STATUS_NOT_SUPPORTED;
                syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
            }

            SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "CMD BEING SENT: 0x%02X \n", 
                                                         uCmd);        
            /*
            for (ui = 0; ui < uActualCmdDataLen; ui++)
            {
                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle,"uActualCmdDataLen[0x%02X] = 0x%02X\n", 
                                                              ui, pActualCmdData[ui]);
            }
            */


            syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(pTimingDevContext->pOemDataPathHandle, 
                                                  &txBuff[0], uDataLen, &rxBuff[0], 0,
                                                  &syncTimingOemDataPathStatus);

            if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                                  syncTimingOemDataPathStatus);
            }
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);            
        }
        else
        {
            while (index < uActualCmdDataLen)
            {
                syncStatus = Sync_Timing_Internal_CORE_WaitForCTS(pTimingDevContext);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                
                count = uCmdPartitionSize - 2;
                if ((index + count) > uActualCmdDataLen)
                {
                    count = uActualCmdDataLen - index;
                }          

                Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0, uCmdPartitionSize);

                if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
                {
                    // Send Command SYNC_TIMING_OP_API_CMD
                    txBuff[0] = SYNC_TIMING_OP_API_CMD;
                    txBuff[1] = uCmd;
                    headerLen = 2;
                    Sync_Timing_OSAL_Wrapper_Memcpy(&txBuff[2], &pActualCmdData[index], count);
                    if (count < uCmdPartitionSize)
                    {
                        uDataLen = (count + headerLen);
                    }
                    else
                    {
                        uDataLen = uCmdPartitionSize;
                    }
                }
                else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
                {
                    // Send Command SYNC_TIMING_OP_API_CMD
                    txBuff[0] = SYNC_TIMING_OP_API_CMD_REPLY & 0xff;
                    txBuff[1] = (SYNC_TIMING_OP_API_CMD_REPLY >> 8) & 0xff;
                    txBuff[2] = uCmd;
                    headerLen = 3;
                    Sync_Timing_OSAL_Wrapper_Memcpy(&txBuff[3], &pActualCmdData[index], count);
                    if (count < uCmdPartitionSize)
                    {
                        uDataLen = (count + headerLen);
                    }
                    else
                    {
                        uDataLen = uCmdPartitionSize;
                    }
                }
                else
                {
                    syncStatus = SYNC_STATUS_NOT_SUPPORTED;
                    syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                }

                SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "uDataLen = %u; index = %u; count = %u \n", 
                                                             uDataLen, index, count);        

                SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "CMD BEING SENT: 0x%02X \n", 
                                                             uCmd);        
                /*
                for (ui = 0; ui < uDataLen; ui++)
                {
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle,"txBuff[0x%02X] = 0x%02X\n", 
                                                                  ui, txBuff[ui]);
                }
                */


                syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(pTimingDevContext->pOemDataPathHandle, 
                                                      &txBuff[0], uDataLen, &rxBuff[0], 0,
                                                      &syncTimingOemDataPathStatus);

                if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                                      syncTimingOemDataPathStatus);
                }
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                
                index += count;
            }
        }

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        
        // If bWaitForReply == TRUE, then loop until CTS bit gets set to 1 to read Reply 
        // SYNC_TIMING_OP_API_REPLY
        if (bWaitForReply)
        {
            if (pReplyData)
            {
                Sync_Timing_OSAL_Wrapper_Memset(pReplyData, 0, cmdReplyLen);
            }
            while(1)
            {
                Sync_Timing_OSAL_Wrapper_Memset(&txBuff[0], 0,
                    SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE);
                Sync_Timing_OSAL_Wrapper_Memset(&rxBuff[0], 0,
                    SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE);
                if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_SPI)
                {
                    readOffset = 1;
                    txBuff[0] = SYNC_TIMING_OP_API_REPLY;
                    headerLen = 2;
                }
                else if (pTimingDevContext->ChipIf == SYNC_TIMING_CHIP_INTERFACE_I2C)
                {
                    readOffset = 0;
                    txBuff[0] = SYNC_TIMING_OP_API_CMD_REPLY & 0xff;
                    txBuff[1] = (SYNC_TIMING_OP_API_CMD_REPLY >> 8) & 0xff;
                    headerLen = 2;
                }
                else
                {
                    syncStatus = SYNC_STATUS_NOT_SUPPORTED;
                    syncTimingOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                }
                syncStatus = Sync_Timing_OEM_DATAPATH_Transfer(
                                               pTimingDevContext->pOemDataPathHandle,
                                               &txBuff[0], headerLen + cmdReplyLen, &rxBuff[0],
                                               cmdReplyLen + headerLen,
                                               &syncTimingOemDataPathStatus);

                if (syncTimingOemDataPathStatus != SYNC_TIMING_DATAPATH_SUCCESS)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_OEM_DATAPATH_Transfer set status %d\n", 
                                      syncTimingOemDataPathStatus);
                }

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);    

                SYNC_TIMING_INFO2(pSyncTimingCoreLogHandle,"CMD API_REPLY STATUS: 0x%02X \n", 
                                                              rxBuff[readOffset]);
                *pReplyStatus = rxBuff[readOffset];

                if ((rxBuff[readOffset] & 0x80) == 0x80)
                {
                    SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle,
                                                "CMD API_REPLY STATUS: 0x%02X; cmdReplyLen = %u \n",
                                                rxBuff[readOffset], cmdReplyLen);
                    if (pReplyData)
                    {
                        Sync_Timing_OSAL_Wrapper_Memcpy(pReplyData, &rxBuff[readOffset + 1], cmdReplyLen);
                        for (ui = 0; ui < cmdReplyLen; ui++)
                        {
                            SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle,"pReplyData[0x%02X] = 0x%02X\n", 
                                                                          ui, pReplyData[ui]);
                        }
                    }
                    if (rxBuff[readOffset] != 0x80)
                    {
                        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                          "FW API CMD %x returned error status = 0x%x\n", 
                                          uCmd, rxBuff[readOffset]);
                        syncStatus = SYNC_STATUS_FAILURE;
                    }
                    break;
                }

                uCount++;
                if ((uCount % 1000) == 0)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                      "Sync_Timing_Internal_CORE_Mem_WriteCommand - uCount %u\n", 
                                      uCount);
                }            
                if (uCount > 20000) 
                {
                    syncStatus = SYNC_STATUS_TIMEOUT;
                    break;
                }        
            }
        }
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);     
    }
    while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_Reset
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This function is used to reset the timing chipset
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : resetType     - Reset Type desired
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Reset(
                                               SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                               SYNC_TIMING_DEVICE_RESET_TYPE_E   resetType)
{
        SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
        cmd_RESTART_map_t                   cmdRestart              = {0};
        uint8_t                             uMcuBootstate           = 0;
        uint32_t                            uCount                  = 0;
        SYNC_TIMING_BOOL_E                  bA1RestartCmdMap        = SYNC_TIMING_FALSE;
        uint8_t                             uReplyStatus            = 0;
        cmdRestart.CMD = cmd_ID_RESTART;

        do
        {
            SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                             "DevReset -- resetType = %u \n", resetType);
            
            if ((pTimingDevContext->deviceRevision == 0x0) && 
                (pTimingDevContext->deviceSubRevision == 0x1))
            {
                bA1RestartCmdMap = SYNC_TIMING_TRUE;
            }

            if (pTimingDevContext->deviceMode == SYNC_TIMING_DEVICE_MODE_APPLN  && 
                bA1RestartCmdMap == SYNC_TIMING_TRUE)
            {
                cmdRestart.CMD = 0xDF;
            }

            // If it is soft reset we will initiate it from here. Hard resets will go to the OEM Layer
            if (resetType == SYNC_TIMING_DEVICE_RESET_SOFT)
            {
                cmdRestart.OPTIONS = 0;             
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdRestart), 
                                                                        (uint8_t *)&cmdRestart, 
                                                                        SYNC_TIMING_FALSE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                Sync_Timing_OSAL_Wrapper_SleepMS(1000); 
            }
            else  if (resetType == SYNC_TIMING_DEVICE_RESET_TOGGLE)// Hard Reset
            {
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdRestart), 
                                                                        (uint8_t *)&cmdRestart, 
                                                                        SYNC_TIMING_FALSE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                
                Sync_Timing_OSAL_Wrapper_SleepMS(1000);                 
            }
            else if (resetType == SYNC_TIMING_DEVICE_RESET_BOOTLOADER_MODE)
            {
                cmdRestart.OPTIONS |= CMD_RESTART_ARG_OPTIONS_WAIT_TRUE_BIT; 
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdRestart), 
                                                                        (uint8_t *)&cmdRestart, 
                                                                        SYNC_TIMING_FALSE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                
                Sync_Timing_OSAL_Wrapper_SleepMS(1000);                 
            }
            else
            {
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_SUPPORTED);
            }
            
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                         SYNC_TIMING_MCU_BOOTSTATE, 
                                         1, 
                                         (uint8_t*)&(uMcuBootstate));    
            
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                       SYNC_STATUS_SUCCESS);
    
            if ((resetType == SYNC_TIMING_DEVICE_RESET_SOFT) || 
                (resetType == SYNC_TIMING_DEVICE_RESET_TOGGLE))
            {
                // check if chipset has booted to application mode before setting up IRQ
                // Boot state >= 0xB8

                while(uMcuBootstate < 0xB8)
                {
                    syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                                 SYNC_TIMING_MCU_BOOTSTATE, 
                                                 1, 
                                                 (uint8_t*)&(uMcuBootstate));    

                    SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                               SYNC_STATUS_SUCCESS);
                    
                    uCount++;
                    if ((uCount % 10) == 0)
                    {
                        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                          "Sync_Timing_CORE_Device_Reset - uCount %u\n", 
                                          uCount);
                    }
                    
                    Sync_Timing_OSAL_Wrapper_SleepMS(50);
                    if (uCount > 100) 
                    {
                        break;
                    }                     
                }

                if (uCount > 100)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                     "Chipset not in a state where interrupts can be setup ... "
                                     "Please check if app image in NVM or not. \n");
                }
                else
                {
                    syncStatus = Sync_Timing_Internal_CORE_Device_SetupIRQ(pTimingDevContext);
                    if (syncStatus != SYNC_STATUS_SUCCESS)
                    {
                        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                         "Sync_Timing_Internal_CORE_Device_SetupIRQ failed; "
                                         "Check if DEVICE is accessible and is in APPLN mode.\n", 
                                         syncStatus);
                    }     
                }
            }

            if (uMcuBootstate <= BOOTLOADER_READY_STATE)
            {
                pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_BOOTLOADER;
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                                   "Device in bootloader mode : uMcuBootstate = %u \n", uMcuBootstate);                 
            }
            else if (uMcuBootstate >= APPLICATION_READY_STATE)
            {
                pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_APPLN;
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                                   "Device in application mode : uMcuBootstate = %u \n", uMcuBootstate);                  
            }
            else
            {
                pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_INVALID;
                SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                   "Device in INVALID APPLN mode : uMcuBootstate = %u \n", uMcuBootstate);  
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
            }

        } while (0);

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_GetBuildInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This internal function is used to get the build information
 *
 * IN PARAMS     : pTimingDevContext        - Timing Device Context
 *
 * OUT PARAMS    : pDeviceBuildInfo         - Device Build Information 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetBuildInfo(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext, 
                                            char *pVersionBuildInfo)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    cmd_APP_INFO_map_t                  cmdAppInfo              = {0};
    reply_APP_INFO_map_t                replyAppInfo            = {0};
    uint8_t                             uReplyStatus            = 0;


    do
    {

        Sync_Timing_OSAL_Wrapper_Memset(pVersionBuildInfo, 0, 
                                        SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);

        cmdAppInfo.CMD = cmd_ID_APP_INFO; // Get App Info Command
        Sync_Timing_OSAL_Wrapper_Memset(&replyAppInfo, 0, sizeof(reply_APP_INFO_map_t));
        
        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdAppInfo), 
                                                                (uint8_t *)&cmdAppInfo, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyAppInfo), 
                                                                (uint8_t *)&replyAppInfo, 
                                                                &uReplyStatus);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            sprintf(pVersionBuildInfo,"Error-Reading");
            syncStatus = SYNC_STATUS_SUCCESS;
        }
        else
        {
            //SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
            
            sprintf(pVersionBuildInfo,"svn_%u_%u", (uint32_t)replyAppInfo.A_BRANCH,
                                                   (uint32_t)replyAppInfo.A_BUILD);
        }

    } while (0);

    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_GetVersionInfo
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This internal function is used to get the version information
 *
 * IN PARAMS     : pTimingDevContext        - Timing Device Context
 *
 * OUT PARAMS    : pDeviceVersionInfo       - Device Version Information 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetVersionInfo(
                                               SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                               SYNC_TIMING_DEVICE_VERSION_T *pDeviceVersionInfo)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_SUCCESS;
    uint8_t                             ui                      = 0;
    cmd_DEVICE_INFO_map_t               cmdDevInfo              = {0};
    reply_DEVICE_INFO_map_t             replyDevInfo            = {0};
    cmd_APP_INFO_map_t                  cmdAppInfo              = {0};
    reply_APP_INFO_map_t                replyAppInfo            = {0};
    uint8_t                             uReplyStatus            = 0;
    SYNC_TIMING_DEVICE_MODE_E           currDeviceMode;

    do
    {
        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->driverVersion, 0, 
                                        SYNC_TIMING_MAX_DRIVER_VERSION_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->driverBuildInfo, 0, 
                                        SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->fwVersion, 0, 
                                        SYNC_TIMING_MAX_FW_VERSION_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->blVersion, 0, 
                                        SYNC_TIMING_MAX_BL_VERSION_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->chipsetRevision, 0, 
                                        SYNC_TIMING_MAX_CHIPSET_VERSION_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->fplanVersion, 0, 
                                        SYNC_TIMING_MAX_FPLAN_VERSION_STRING_SIZE);
        
        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->fplanDesignId, 0, 
                                        SYNC_TIMING_MAX_FPLAN_DESIGN_ID_STRING_SIZE);

        Sync_Timing_OSAL_Wrapper_Memset(pDeviceVersionInfo->cbproVersion, 0,
                                        SYNC_TIMING_MAX_CBPRO_VERSION_STRING_SIZE);

        /* Generate the Driver version = ChipType.Major.Minor_BuildNum-BuildType*/
        sprintf(pDeviceVersionInfo->driverVersion,"%u.%u.%u_%u%s",
                                              (uint32_t)SYNC_TIMING_DRIVER_VERSION_CHIP_TYPE,
                                              (uint32_t)SYNC_TIMING_DRIVER_VERSION_MAJOR,
                                              (uint32_t)SYNC_TIMING_DRIVER_VERSION_MINOR,
                                              (uint32_t)SYNC_TIMING_DRIVER_VERSION_BUILD_NUM,
                                              gVerBldType[SYNC_TIMING_DRIVER_VERSION_BUILD_TYPE]);

        SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "%s\n", pDeviceVersionInfo->driverVersion);
      
        Sync_Timing_OSAL_Wrapper_Memcpy(&(pDeviceVersionInfo->driverBuildInfo[0]), 
                            (const char *)(SYNC_TIMING_DRIVER_VERSION_BUILD_INFO), 
                            SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE);

        cmdDevInfo.CMD = cmd_ID_DEVICE_INFO; // Get Device Info Command
        Sync_Timing_OSAL_Wrapper_Memset(&replyDevInfo, 0, sizeof(replyDevInfo));

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdDevInfo), 
                                                                (uint8_t *)&cmdDevInfo, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyDevInfo), 
                                                                (uint8_t *)&replyDevInfo, 
                                                                &uReplyStatus);

        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            sprintf(pDeviceVersionInfo->blVersion,"Error-Reading");
            sprintf(pDeviceVersionInfo->chipsetRevision,"Error-Reading");
            syncStatus = SYNC_STATUS_SUCCESS;
        }        
        else
        {
            //SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            for (ui = 0; ui < 10; ui++)
            {
                ;//SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle,"pReplyData[0x%02X] = 0x%02X\n", 
                 //                                             ui, rxBuff[ui]);
            }

            sprintf(pDeviceVersionInfo->blVersion,"%u.%u.%u_%u",
                                                  (uint32_t)replyDevInfo.ROM,
                                                  (uint32_t)replyDevInfo.MROM,
                                                  (uint32_t)replyDevInfo.BROM,
                                                  (uint32_t)(replyDevInfo.SVN));

            pTimingDevContext->deviceRevision = (replyDevInfo.M0 >> 0x4) & 0x0F;
            pTimingDevContext->deviceSubRevision = (replyDevInfo.M0) & 0x0F;
            
            sprintf(pDeviceVersionInfo->chipsetRevision, "%s%u",
                                              gChipRevToStrMap[pTimingDevContext->deviceRevision],
                                              pTimingDevContext->deviceSubRevision);
        }

        syncStatus = Sync_Timing_Internal_CORE_Device_GetMode(pTimingDevContext, &currDeviceMode);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        
        if (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)
        {
        
            cmdAppInfo.CMD = cmd_ID_APP_INFO; // Get App Info Command
            Sync_Timing_OSAL_Wrapper_Memset(&replyAppInfo, 0, sizeof(replyAppInfo));

            
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdAppInfo), 
                                                                    (uint8_t *)&cmdAppInfo, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyAppInfo), 
                                                                    (uint8_t *)&replyAppInfo, 
                                                                    &uReplyStatus);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                sprintf(pDeviceVersionInfo->fwVersion,"Error-Reading");
                sprintf(pDeviceVersionInfo->fplanVersion,"Error-Reading");
                syncStatus = SYNC_STATUS_SUCCESS;
                pTimingDevContext->bfwVersionInfoAvailable = SYNC_TIMING_FALSE;
            }          
            else
            {
                //SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                
                sprintf(pDeviceVersionInfo->fwVersion,"%u.%u.%u_svn_%u",
                                                      (uint32_t)replyAppInfo.A_MAJOR,
                                                      (uint32_t)replyAppInfo.A_MINOR,
                                                      (uint32_t)replyAppInfo.A_BRANCH,
                                                      (uint32_t)replyAppInfo.A_BUILD);
                sprintf(pDeviceVersionInfo->fplanVersion,"%u.%u.%u_svn_%u",
                                                         (uint32_t)replyAppInfo.P_MAJOR,
                                                         (uint32_t)replyAppInfo.P_MINOR,
                                                         (uint32_t)replyAppInfo.P_BRANCH,
                                                         (uint32_t)replyAppInfo.P_BUILD); 

                sprintf(pDeviceVersionInfo->fplanDesignId,"%c%c%c%c%c%c%c%c",
                                                          replyAppInfo.DESIGN_ID[0],
                                                          replyAppInfo.DESIGN_ID[1],
                                                          replyAppInfo.DESIGN_ID[2],
                                                          replyAppInfo.DESIGN_ID[3],
                                                          replyAppInfo.DESIGN_ID[4],
                                                          replyAppInfo.DESIGN_ID[5],
                                                          replyAppInfo.DESIGN_ID[6],
                                                          replyAppInfo.DESIGN_ID[7]); 
                sprintf(pDeviceVersionInfo->cbproVersion, "%u.%u.%u", 
                                                          replyAppInfo.sCBPRO.cbpro_rev_major,
                                                          replyAppInfo.sCBPRO.cbpro_rev_minor,
                                                          replyAppInfo.sCBPRO.cbpro_rev_revision);
                

                pTimingDevContext->fwVersionMajor = (uint32_t)replyAppInfo.A_MAJOR;
                pTimingDevContext->fwVersionMinor = (uint32_t)replyAppInfo.A_MINOR;
                pTimingDevContext->fwVersionBranch = (uint32_t)replyAppInfo.A_BRANCH;
                pTimingDevContext->fwVersionBuildNum = (uint32_t)replyAppInfo.A_BUILD;
                pTimingDevContext->bfwVersionInfoAvailable = SYNC_TIMING_TRUE;

                if ((pTimingDevContext->fwVersionMajor < SYNC_TIMING_MIN_FW_MAJOR_VERSION) 
                    ||
                    (
                        (pTimingDevContext->fwVersionMajor == SYNC_TIMING_MIN_FW_MAJOR_VERSION) && 
                        (pTimingDevContext->fwVersionMinor < SYNC_TIMING_MIN_FW_MINOR_VERSION)
                    ) 
                    ||
                    (
                        (pTimingDevContext->fwVersionMajor == SYNC_TIMING_MIN_FW_MAJOR_VERSION) && 
                        (pTimingDevContext->fwVersionMinor == SYNC_TIMING_MIN_FW_MINOR_VERSION) &&
                        (pTimingDevContext->fwVersionBuildNum < SYNC_TIMING_MIN_FW_BUILD_NUM)
                    )
                   ) 
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                        "FW VER (%u.%u_%u) LOADED IS "
                                        "LESS THAN MIN REC. (%u.%u_%u)\n",
                                        pTimingDevContext->fwVersionMajor, 
                                        pTimingDevContext->fwVersionMinor, 
                                        pTimingDevContext->fwVersionBuildNum,
                                        SYNC_TIMING_MIN_FW_MAJOR_VERSION,
                                        SYNC_TIMING_MIN_FW_MINOR_VERSION,
                                        SYNC_TIMING_MIN_FW_BUILD_NUM);
                }            


                pTimingDevContext->bHostClearStatus = SYNC_TIMING_FALSE;
                if ((pTimingDevContext->fwVersionMajor == SYNC_TIMING_FW_MAJOR_VERSION_CLEAR_STATUS) && 
                    (pTimingDevContext->fwVersionMinor == SYNC_TIMING_FW_MINOR_VERSION_CLEAR_STATUS) &&
                    (pTimingDevContext->fwVersionBranch == SYNC_TIMING_FW_BRANCH_CLEAR_STATUS))
                {
                    pTimingDevContext->bHostClearStatus = SYNC_TIMING_TRUE;
                }
                if (pTimingDevContext->bHostClearStatus)
                {
                    SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "HOST SENDS CLEAR STATUS\n");
                }
                else
                {
                    SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "HOST WILL NOT SEND CLEAR STATUS\n");
                }
            }
        }
        else
        {
            sprintf(pDeviceVersionInfo->fwVersion,"Error-Reading");
            sprintf(pDeviceVersionInfo->fplanVersion,"Error-Reading");
            sprintf(pDeviceVersionInfo->fplanDesignId,"Error-Reading");
            syncStatus = SYNC_STATUS_SUCCESS;
            pTimingDevContext->bfwVersionInfoAvailable = SYNC_TIMING_FALSE;
         }

    } while (0);

    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_GetMode
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This internal function is used to get the current mode of the timing device
 *
 * IN PARAMS     : pTimingDevContext   - Timing Device context
 *
 * OUT PARAMS    : pCurrDeviceMode    - Device Mode 
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_GetMode(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            SYNC_TIMING_DEVICE_MODE_E *pCurrDeviceMode)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    uint8_t                             uMcuBootstate           = 0;

    do
    {
        // read boot state register to determine the current mode
        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                     SYNC_TIMING_MCU_BOOTSTATE, 
                                     1, 
                                     (uint8_t*)&(uMcuBootstate));    

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                         "uMcuBootstate = %u \n", uMcuBootstate);      
        
        if (uMcuBootstate > 0  && uMcuBootstate <= BOOTLOADER_READY_STATE)
        {
            *pCurrDeviceMode = SYNC_TIMING_DEVICE_MODE_BOOTLOADER;
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                               "Device in Bootloader mode : uMcuBootstate = %u \n", uMcuBootstate);             
        }
        else if (uMcuBootstate >= APPLICATION_READY_STATE)
        {
            *pCurrDeviceMode = SYNC_TIMING_DEVICE_MODE_APPLN;
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                               "Device in application mode : uMcuBootstate = %u \n", uMcuBootstate);                  
        }
        else
        {
            *pCurrDeviceMode = SYNC_TIMING_DEVICE_MODE_INVALID;
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                               "Device in INVALID APPLN mode : uMcuBootstate = %u \n", uMcuBootstate);  
        }
        
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, 
                         "GetMode -- currDeviceMode = %u \n", *pCurrDeviceMode);

        pTimingDevContext->deviceMode = *pCurrDeviceMode;

    } while (0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_SetMode
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This internal function is used to set the current mode of the timing device
 *
 * IN PARAMS     : pTimingDevContext   - Timing Device Context
 *               : deviceMode          - Device Mode desired
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_SetMode(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext,
                                            SYNC_TIMING_DEVICE_MODE_E deviceMode)
{
    SYNC_STATUS_E                       syncStatus             = SYNC_STATUS_NOT_SUPPORTED;

    // if currently in application mode and move to bootloader mode then we want to send
    // the restart command to put it into bootloader mode

    // if currently in bootloader mode, then issue BOOT command.
    // if app in NVM then it will boot from it. 
    // if app was downloaded using the update command, then it will be used
    // if neither then boot will fail and the API will return failure

    return syncStatus;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 03/03/2020
 *
 * DESCRIPTION   : This internal function is used to do any initialization with the chipset if needed
 *
 * IN PARAMS     : pTimingDevContext        - Timing Device Context
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Init(
                                              SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext)
{
    SYNC_STATUS_E                   syncStatus                  = SYNC_STATUS_SUCCESS;
    cmd_METADATA_map_t              cmdMETADATA                 = {0};
    reply_METADATA_map_t            replyMETADATA               = {0};    
    SYNC_TIMING_DEVICE_MODE_E       CurrDeviceMode              = SYNC_TIMING_DEVICE_MODE_INVALID;
    uint8_t                         uReplyStatus                = 0;    
    SYNC_TIMING_DEVICE_VERSION_T    deviceVersion               = {0};

    Sync_Timing_OSAL_Wrapper_SleepMS(50);

    do
    {
        Sync_Timing_Internal_CORE_Device_GetMode(pTimingDevContext, &CurrDeviceMode);

        if (CurrDeviceMode != SYNC_TIMING_DEVICE_MODE_INVALID)
        {
            syncStatus = Sync_Timing_Internal_CORE_Device_GetVersionInfo(pTimingDevContext, 
                                                                         &deviceVersion);
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,"Chipset Revision = %s\n", 
                               deviceVersion.chipsetRevision);
            
            SYNC_TIMING_ERRCHECK_NO_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        }


        if (CurrDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)
        {
            cmdMETADATA.CMD = cmd_ID_METADATA;
            
            Sync_Timing_OSAL_Wrapper_Memset(&replyMETADATA, 0, sizeof(replyMETADATA));
            
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdMETADATA), 
                                                                    (uint8_t *)&cmdMETADATA, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyMETADATA), 
                                                                    (uint8_t *)&replyMETADATA, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);    

            pTimingDevContext->bPtpSteeredRf = replyMETADATA.PLAN_OPTIONS & 
                                           CMD_METADATA_REPLY_PLAN_OPTIONS_PTP_STEERED_RF_TRUE_BIT;

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, ">>> DEVICE INIT : DCO_NA_STEP_SIZE = %x; "
                               "DCO_MR_STEP_SIZE = %x; "
                               "DCO_MA_STEP_SIZE = %x; "
                               "DCO_NB_STEP_SIZE = %x; "
                               "DCO_MB_STEP_SIZE = %x; "
                               "PHASE_JAM_PPS_OUT_RANGE_HIGH = %x; "
                               "PHASE_JAM_PPS_OUT_RANGE_LOW = %x; "
                               "PHASE_JAM_PPS_OUT_STEP_SIZE = %lu; "
                               "PTP_STEERED_RF_FLAG = %x \n",
                               replyMETADATA.DCO_NA_STEP_SIZE,
                               replyMETADATA.DCO_MR_STEP_SIZE,
                               replyMETADATA.DCO_MA_STEP_SIZE,
                               replyMETADATA.DCO_NB_STEP_SIZE,
                               replyMETADATA.DCO_MB_STEP_SIZE,
                               replyMETADATA.PHASE_JAM_PPS_OUT_RANGE_HIGH,
                               replyMETADATA.PHASE_JAM_PPS_OUT_RANGE_LOW,
                               replyMETADATA.PHASE_JAM_PPS_OUT_STEP_SIZE,
                               pTimingDevContext->bPtpSteeredRf);    
        }
        else
        {
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_NOT_READY); 
        }


    } while (0);

    return syncStatus;   
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_ClearInterrupts
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 05/05/2020
 *
 * DESCRIPTION   : This internal function is used to clear the interrupts from firmware
 *
 * IN PARAMS     : pTimingDevContext        - Timing Device Context
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_ClearInterrupts(
                                              SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    //uint64_t                            uClrIntr                = 0;
    cmd_CLEAR_STATUS_FLAGS_map_t        cmdClearStatus          = {0};
    uint8_t                             uReplyStatus            = 0;    

    do
    {
        if (pTimingDevContext->deviceRevision == 0 && pTimingDevContext->deviceSubRevision == 1)
        {
            // Clear all INTR flags in the chipset registers (0x11 through 0x7)
            //syncStatus = Sync_Timing_Internal_CORE_Mem_WriteDirect(pTimingDevContext, 
            //                                                       SYNC_TIMING_INTC_IRQx_START, 
            //                                                       7, (unsigned char *) &uClrIntr);
            //SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        }
        else
        {
            cmdClearStatus.CMD = cmd_ID_CLEAR_STATUS_FLAGS; // Clear all status flags
            
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdClearStatus), 
                                                                    (uint8_t *)&cmdClearStatus, 
                                                                    SYNC_TIMING_FALSE, 
                                                                    0, 
                                                                    NULL, 
                                                                    &uReplyStatus);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle,
                                   "Command to clear status flags failed; uReplyStatus = %u\n", 
                                   uReplyStatus);
            }
            else
            {
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,
                                   "Command to clear status flags succeeded; uReplyStatus = %u\n", 
                                   uReplyStatus);
            }
        }

    } while (0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_PollIrqThread
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 05/19/2020
 *
 * DESCRIPTION   : This internal function implements the polling logic for ROS for input signals
 *
 * IN PARAMS     : arg        - Timing Device ID
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
void * Sync_Timing_Internal_CORE_Device_PollIrqThread(uint32_t arg)
{
    uint32_t                            uIdlePollDelay      = 5000;
    uint32_t                            uActivePollDelay    = 5000;
    SYNC_TIMING_BOOL_E                  bActivePolling      = SYNC_TIMING_FALSE;
    
    uint8_t                             uTimingDevId        = (intptr_t) arg;;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext  = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex       = SYNC_TIMING_FALSE;
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;

    cmd_INPUT_STATUS_map_t              cmdInputStatus      = {0};
    reply_INPUT_STATUS_map_t            replyInputStatus    = {0};
    uint8_t                             ui                  = 0;

    SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T    eventData     = {0};    
    uint8_t                             uReplyStatus        = 0;

    
    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, 
                      "Sync_Timing_Internal_CORE_Device_PollIrqThread now running ...\n");

    gbTerminateIntrPollThread = SYNC_TIMING_FALSE;
    cmdInputStatus.CMD = cmd_ID_INPUT_STATUS;

    while(SYNC_TIMING_TRUE)
    {
        // Acquire device context
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(uTimingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        eventData.deviceEventInfo.deviceInputEvents = 0;

        // Check for Input needed ROS polling and read those INPUT flags
        // Updated device event info for input events if ROS seen 
        for (ui = 0; ui < 8; ui++)
        {
            if (pTimingDevContext->bROSInputPollList[ui] == SYNC_TIMING_TRUE)
            {
                cmdInputStatus.INPUT_SELECT = ui;
                Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdInputStatus), 
                                                                        (uint8_t *)&cmdInputStatus, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        sizeof(replyInputStatus), 
                                                                        (uint8_t *)&replyInputStatus, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   
                
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "fin(%u): LOSS_OF_SIGNAL = %x, " 
                                                              "OUT_OF_FREQUENCY = %x, "
                                                              "INPUT_CLOCK_VALIDATION = %x, "
                                                              "PHASE_MONITOR = %x \n",
                                                              ui,
                                                              replyInputStatus.LOSS_OF_SIGNAL,
                                                              replyInputStatus.OUT_OF_FREQUENCY,
                                                              replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                              replyInputStatus.PHASE_MONITOR);


                if (((replyInputStatus.LOSS_OF_SIGNAL | 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_FALSE_BIT) == 0) &&
                    ((replyInputStatus.OUT_OF_FREQUENCY | 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_FALSE_BIT) == 0) &&
                    ((replyInputStatus.PHASE_MONITOR & 
                    CMD_INPUT_STATUS_REPLY_PHASE_MONITOR_PHASE_MONITOR_PHASE_ERROR_FALSE_BIT) == 0) &&
                    (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                    CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID))
                {
                    eventData.deviceEventInfo.deviceInputEvents |= (SYNC_TIMING_DEVICE_EVENT_ROS_IN0 << ui);
                    pTimingDevContext->bROSInputPollList[ui] = SYNC_TIMING_FALSE;    
                }
            }
        }

        // Send event to applications registered for ROS
        if (eventData.deviceEventInfo.deviceInputEvents != 0)
        {
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Driver Events: deviceInputEvents = 0x%x "
                                                          "devicePllEvents = 0x%x "
                                                          "deviceGenEvents = 0x%x \n",
                                                           eventData.deviceEventInfo.deviceInputEvents,
                                                           eventData.deviceEventInfo.devicePllEvents,
                                                           eventData.deviceEventInfo.deviceGenEvents);   
            
            syncStatus = Sync_Timing_Internal_CORE_Send_Events(uTimingDevId, SYNC_TIMING_DEVICE_CHIP_EVENT, 
                                                               &eventData);
            
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        }

        // Determine if we need active polling or Idle polling
        bActivePolling = SYNC_TIMING_FALSE;
        for (ui = 0; ui < 8; ui++)
        {
            bActivePolling |= pTimingDevContext->bROSInputPollList[ui];
        }

        if (bHoldingMutex != SYNC_TIMING_FALSE)
        {
            (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
        }
        
        if (gbTerminateIntrPollThread)
        {
            break;
        }

        if (bActivePolling)
        {
            Sync_Timing_OSAL_Wrapper_SleepMS(uActivePollDelay);  
        }
        else
        {
            Sync_Timing_OSAL_Wrapper_SleepMS(uIdlePollDelay);  
        }
    }
    return 0;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_SetupIRQ
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This internal function is used to setup the driver event interrupts from firmware
 *
 * IN PARAMS     : pTimingDevContext        - Timing Device Context
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_SetupIRQ(
                                              SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext)
{
    SYNC_STATUS_E               syncStatus              = SYNC_STATUS_SUCCESS;
    uint8_t                     uIntEnValA1[8]          = {0xF5, 0xB, 0xB, 0xB, 0x1F, 0x0, 0x0, 0x1};
    uint8_t                     uIntEnVal[9]            = {0xF5, 0xF5, 0xB, 0xB, 0xB, 0x1F, 0x0, 0x0, 0x1};
    uint64_t                    uClrIntr                = 0;
    uint8_t                     *pIntrEnVal             = &(uIntEnVal[0]);
    uint8_t                     ui                      = 0;
    intptr_t                    i                       = pTimingDevContext->timingDevId;
    uint32_t                    syncOsalStatus;  
    uint16_t                    intrIrqStart            = SYNC_TIMING_INTC_IRQx_START;
    uint16_t                    intrEnStart             = SYNC_TIMING_INTC_IENx_START;
    uint32_t                    uIntIrqDataLen          = 8;
    uint32_t                    uIntEnDataLen           = 9;
    cmd_CLEAR_STATUS_FLAGS_map_t        cmdClearStatus          = {0};
    uint8_t                             uReplyStatus            = 0;    
        
    do
    {
        // Check if it is A1 part
        if (pTimingDevContext->deviceRevision == 0 && pTimingDevContext->deviceSubRevision == 1)
        {
            intrIrqStart = SYNC_TIMING_INTC_IRQx_START_A1;
            intrEnStart = SYNC_TIMING_INTC_IENx_START_A1;
            uIntIrqDataLen = 7;
            uIntEnDataLen = 8;
            pIntrEnVal = &(uIntEnValA1[0]);

            // Clear all INTR flags in the chipset registers (0x11 through 0x7) - REMOVE THIS for B0
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,"Clearing INTC_FLAGS.\n");
                    
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteDirect(pTimingDevContext, 
                                                                   intrIrqStart, 
                                                                   uIntIrqDataLen, 
                                                                   (unsigned char *) &uClrIntr);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            // Enable all required INTC EN in the chipsets registers 
            // (0x1B through 0x22 for A1)
            // 0x1000 through 0x1008 for >= B0 - Not required; CbPro supports this;
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,"Setting up INTC_Enable for A1\n");

            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteDirect(pTimingDevContext, 
                                                                   intrEnStart, 
                                                                   uIntEnDataLen, 
                                                                   (unsigned char *) pIntrEnVal);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            if (pTimingDevContext->bIntrPollThreadInitialized == SYNC_TIMING_FALSE)
            {
                for (ui = 0; ui < 8; ui++)
                {
                    pTimingDevContext->bROSInputPollList[ui] = SYNC_TIMING_FALSE;
                }

                /* Create a low priority thread that will handle log messages */
                Sync_Timing_OSAL_Wrapper_Memset(&(pTimingDevContext->intrPollThreadName[0]), 
                                                0, SYNC_TIMING_CFG_MAX_NAME_SZ);
                sprintf(pTimingDevContext->intrPollThreadName, "/sync_timing_intr_poll_thread");

                syncOsalStatus = Sync_Timing_OSAL_Wrapper_Thread_Create(
                                                    &(pTimingDevContext->intrPollThread),
                                                    &pTimingDevContext->intrPollThreadName[0], 
                                                    (void *)&Sync_Timing_Internal_CORE_Device_PollIrqThread,
                                                    (void *)i, 0, 0, 0);
                if (syncOsalStatus != SYNC_TIMING_OSAL_SUCCESS)
                {
                    SYNC_TIMING_SET_ERR_BREAK(SYNC_TIMING_LOG_DEFAULT_HANDLE, syncStatus, SYNC_STATUS_FAILURE);
                }
                
                /* Give some time for the thread to start running */
                Sync_Timing_OSAL_Wrapper_SleepMS(100);

                pTimingDevContext->bIntrPollThreadInitialized = SYNC_TIMING_TRUE;        
            }
            else
            {
                for (ui = 0; ui < 8; ui++)
                {
                    pTimingDevContext->bROSInputPollList[ui] = SYNC_TIMING_FALSE;
                }
            }            
        }
        else  //B0 or later
        {
            cmdClearStatus.CMD = cmd_ID_CLEAR_STATUS_FLAGS; // Clear all status flags
            
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdClearStatus), 
                                                                    (uint8_t *)&cmdClearStatus, 
                                                                    SYNC_TIMING_FALSE, 
                                                                    0, 
                                                                    NULL, 
                                                                    &uReplyStatus);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle,
                                   "Command to clear status flags failed; uReplyStatus = %u\n", 
                                   uReplyStatus);
            }
            else
            {
                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,
                                   "Command to clear status flags succeeded; uReplyStatus = %u\n", 
                                   uReplyStatus);
            }
        }
        
    } while (0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Device_Download
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/04/2020
 *
 * DESCRIPTION   : This function is used to download the freq plan in the timing chipset
 *
 * IN PARAMS     : timingDevId   - Timing Device Id
 *               : uNumBootfiles - number of bootfiles to download
 *               : pBootFileList - Bootfile list to download
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/  
SYNC_STATUS_E Sync_Timing_Internal_CORE_Device_Download(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T *pTimingDevContext, 
                                            uint32_t uNumBootFiles, 
                                            char pBootFileList[][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ])
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_NOT_SUPPORTED;
    uint32_t                            ui                      = 0,
                                        uj                      = 0,
                                        pIdx                    = 0;
    uint8_t                             uMcuBootstate           = 0;
    uint32_t                            uCount                  = 0;
    SYNC_TIMING_BOOL_E                  bA1RestartCmdMap        = SYNC_TIMING_FALSE;
    uint32_t                            index                   = 0;
    uint32_t                            count                   = 0;
    uint32_t                            uFileSize               = 0;
    FILE                                *fpBootFD               = NULL;
    uint32_t                            uFilePartitionSize      = SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE;
    uint32_t                            uBytesRead              = 0;
    
    cmd_RESTART_map_t                   cmdRestart              = {0};
    cmd_HOST_LOAD_map_t                 cmdHostLoad             = {0};
    cmd_BOOT_map_t                      cmdBoot                 = {0};
    cmd_NVM_STATUS_map_t                cmdNvmStatus            = {0};
    reply_NVM_STATUS_map_t              replyNvmStatus          = {0};

    cmd_DEVICE_INFO_map_t               cmdDevInfo              = {0};
    reply_DEVICE_INFO_map_t             replyDevInfo            = {0};
    uint8_t                             uReplyStatus            = 0;    
    int32_t           dloadOrder[SYNC_TIMING_MAX_DEVICE_DOWNLOAD_BOOTFILES] = {-1, -1, -1, -1};
    uint8_t           patchFilePattern[7] = {0x1B, 0x01, 0x1B, 0x1A, 0x1A, 0x1A, 0x1A};
    uint8_t                             uPatchPatternSz         = 7;
    //SYNC_TIMING_BOOL_E                  bPatchFileFound         = SYNC_TIMING_FALSE;
    SYNC_TIMING_DEVICE_MODE_E       CurrDeviceMode              = SYNC_TIMING_DEVICE_MODE_INVALID;
    SYNC_TIMING_DEVICE_VERSION_T    deviceVersion               = {0};

    cmdRestart.CMD = cmd_ID_RESTART;
    cmdRestart.OPTIONS |= CMD_RESTART_ARG_OPTIONS_WAIT_TRUE_BIT; 

    cmdHostLoad.CMD = cmd_ID_HOST_LOAD;

    cmdBoot.CMD = cmd_ID_BOOT;

    cmdNvmStatus.CMD = cmd_ID_NVM_STATUS;

    Sync_Timing_Internal_CORE_Device_GetMode(pTimingDevContext, &CurrDeviceMode);
    
    if (CurrDeviceMode != SYNC_TIMING_DEVICE_MODE_INVALID)
    {
        syncStatus = Sync_Timing_Internal_CORE_Device_GetVersionInfo(pTimingDevContext, &deviceVersion);
        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle,"Chipset Revision = %s\n", 
                           deviceVersion.chipsetRevision);
        
        SYNC_TIMING_ERRCHECK_NO_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
    }
    
    do 
    {
       
        if ((pTimingDevContext->deviceRevision == 0x0) && 
            (pTimingDevContext->deviceSubRevision == 0x1))
        {
            bA1RestartCmdMap = SYNC_TIMING_TRUE;
        }    
        
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "uNumBootFiles = %u\n",
                                                     uNumBootFiles);
        
        for (ui = 0; ui < uNumBootFiles; ui++)
        {
            if (pBootFileList[ui])
            {
                dloadOrder[ui] = ui;
                SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "Boot File[%u] = %s\n", ui,
                                                         pBootFileList[ui]);
                //if (bPatchFileFound == SYNC_TIMING_FALSE)
                {
                    fpBootFD = fopen(pBootFileList[dloadOrder[ui]], "rb");
                    if (fpBootFD == NULL) 
                    {
                        SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Error: unable to open %s\n", 
                                                                    pBootFileList[dloadOrder[ui]]);
                        SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                    }

                    Sync_Timing_OSAL_Wrapper_Memset(&(cmdHostLoad.DATA[0]), 0, 255);
                    uBytesRead = fread(&(cmdHostLoad.DATA[0]), 1, 128, fpBootFD);

                    for (uj = 0; uj < uBytesRead; uj++)
                    {
                        if (Sync_Timing_OSAL_Wrapper_Memcmp(&patchFilePattern[0], 
                                                    &(cmdHostLoad.DATA[uj]), uPatchPatternSz) == 0)
                        {
                            // Found patch file in the list of files provided
                            // set dloadOrder[0] to the current index
                            if (ui != pIdx)
                            {
                                dloadOrder[ui] = dloadOrder[pIdx];
                                dloadOrder[pIdx] = ui;
                                pIdx++;
                            }
                            else
                            {
                                dloadOrder[pIdx] = ui;
                                pIdx++;
                            }
                                
                            //bPatchFileFound = SYNC_TIMING_TRUE;
                            break;
                        }
                    }

                    fclose(fpBootFD);
                }
            }
        }    

        // If in application mode, put part in bootloader mode (modified RESTART_A1 command)
        // If in some bootloader mode (0x16), send RESTART command
        if (pTimingDevContext->deviceMode == SYNC_TIMING_DEVICE_MODE_APPLN  && 
            bA1RestartCmdMap == SYNC_TIMING_TRUE)
        {
            cmdRestart.CMD = 0xDF;
        }

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdRestart), 
                                                                (uint8_t *)&cmdRestart, 
                                                                SYNC_TIMING_FALSE, 
                                                                0, 
                                                                NULL, 
                                                                &uReplyStatus);
        
        Sync_Timing_OSAL_Wrapper_SleepMS(50);   //TODO - Move to WaitForCTS in next release
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        // Check bootstate is bootloader ready
        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                     SYNC_TIMING_MCU_BOOTSTATE, 
                                     1, 
                                     (uint8_t*)&(uMcuBootstate));    
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                   SYNC_STATUS_SUCCESS);
        uCount = 0;
        while(uMcuBootstate != BOOTLOADER_READY_STATE)
        {
            syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                         SYNC_TIMING_MCU_BOOTSTATE, 
                                         1, 
                                         (uint8_t*)&(uMcuBootstate));    

            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                       SYNC_STATUS_SUCCESS);
            
            uCount++;
            if ((uCount % 10) == 0)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_CORE_Device_Reset - uCount = %u; uMcuBootstate = %u\n", 
                                  uCount, uMcuBootstate);
            }
            
            Sync_Timing_OSAL_Wrapper_SleepMS(50);
            if (uCount > 100) 
            {
                break;
            }                     
        }

        if (uCount > 100)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                             "Chipset not in a state where interrupts can be setup ... "
                             "Please check if app image in NVM or not. \n");
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                           SYNC_STATUS_FAILURE);
        }
        else
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, "Ready to download FW: uMcuBootstate = %u\n");
        }

        cmdDevInfo.CMD = cmd_ID_DEVICE_INFO; // Get Device Info Command 
        Sync_Timing_OSAL_Wrapper_Memset(&replyDevInfo, 0, sizeof(replyDevInfo));

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdDevInfo), 
                                                                (uint8_t *)&cmdDevInfo, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyDevInfo), 
                                                                (uint8_t *)&replyDevInfo, 
                                                                &uReplyStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


        //TODO - USE SIO_INFO to get the command buffer size - use that once C header is fixed


        // Download the boot files
        for (ui = 0; ui < uNumBootFiles; ui++)
        {
            if (pBootFileList[dloadOrder[ui]])
            {
                index = 0;
                
                fpBootFD = fopen(pBootFileList[dloadOrder[ui]], "rb");
                if (fpBootFD == NULL) 
                {
                    SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, "Error: unable to open %s\n", 
                                                                pBootFileList[dloadOrder[ui]]);
                    SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
                }

                fseek(fpBootFD, 0, SEEK_END); // seek to end of file
                uFileSize = ftell(fpBootFD); // get current file pointer
                fseek(fpBootFD, 0, SEEK_SET); // seek back to beginning of file

                SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Boot File[%u] = %s; uFileSize = %u\n", 
                                                             dloadOrder[ui],
                                                             pBootFileList[dloadOrder[ui]], 
                                                             uFileSize);                
                while(index < uFileSize)
                {
                    count = uFilePartitionSize;
                    if ((index + count) > uFileSize)
                    {
                        count = uFileSize - index;
                    }          
                    
                    Sync_Timing_OSAL_Wrapper_Memset(&(cmdHostLoad.DATA[0]), 0, 255);
                    
                    uBytesRead = fread(&(cmdHostLoad.DATA[0]), 1, count, fpBootFD);
                    
                    SYNC_TIMING_DEBUG(pSyncTimingCoreLogHandle, "Boot File[%u] = %s; uBytesRead = %u\n", 
                                                                 dloadOrder[ui],
                                                                 pBootFileList[dloadOrder[ui]], 
                                                                 uBytesRead);   
                    
                    syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                            (1+count), 
                                                                            (uint8_t *)&cmdHostLoad, 
                                                                            SYNC_TIMING_FALSE, 
                                                                            0, 
                                                                            NULL, 
                                                                            &uReplyStatus);
                    
                    SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                    index = index + count;
                }

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 
                fclose(fpBootFD);
            }   
        }  

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


        // Retrieve and print NVM Status
        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdNvmStatus), 
                                                                (uint8_t *)&cmdNvmStatus, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyNvmStatus), 
                                                                (uint8_t *)&replyNvmStatus, 
                                                                &uReplyStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "NVM STATUS: ERROR2CNT = %u, ERROR1CNT = %u, "
                                                     "MISC = %u, DESCRIPTORS = %u, "
                                                     "INVALIDATED = %u\n",
                                                      replyNvmStatus.ERROR2CNT,
                                                      replyNvmStatus.ERROR1CNT,
                                                      replyNvmStatus.MISC,
                                                      replyNvmStatus.DESCRIPTORS,
                                                      replyNvmStatus.INVALIDATED);

        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                     SYNC_TIMING_MCU_BOOTSTATE, 
                                     1, 
                                     (uint8_t*)&(uMcuBootstate));    
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                   SYNC_STATUS_SUCCESS);

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                            "Downloaded images to RAM; Now Sending BOOT Command - uMcuBootstate = %u\n", 
                             uMcuBootstate);                
                                                      
        // Send Boot command
        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdBoot), 
                                                                (uint8_t *)&cmdBoot, 
                                                                SYNC_TIMING_FALSE, 
                                                                0, 
                                                                NULL, 
                                                                &uReplyStatus);
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        // Wait until boot state is CHIP_OUTPUT_READY_STATE and above;
        // Setup irq if boot state is CHIP_OUTPUT_READY_STATE and above;
        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                     SYNC_TIMING_MCU_BOOTSTATE, 
                                     1, 
                                     (uint8_t*)&(uMcuBootstate));    
        
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                   SYNC_STATUS_SUCCESS);

        // check if chipset has booted to application mode before setting up IRQ or call API command
        // Boot state >= CHIP_OUTPUT_READY_STATE - EVENTUALLY we will move to an API command
        uCount = 0;
        while(uMcuBootstate < CHIP_OUTPUT_READY_STATE)
        {
            syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                                         SYNC_TIMING_MCU_BOOTSTATE, 
                                         1, 
                                         (uint8_t*)&(uMcuBootstate));    

            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                       SYNC_STATUS_SUCCESS);
            
            uCount++;
            if ((uCount % 10) == 0)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                  "Sync_Timing_CORE_Device_Reset - uCount = %u; uMcuBootstate = %u\n", 
                                  uCount, uMcuBootstate);
            }
            
            Sync_Timing_OSAL_Wrapper_SleepMS(50);
            if (uCount > 100) 
            {
                break;
            }                     
        }

        if (uCount > 100)
        {
            SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                             "WARNING !!!!!!!!!!!!! "
                             "Chipset not in a state where device and interrupts can be setup ... "
                             "Please check if app image/plan loaded. \n");
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                             SYNC_STATUS_FAILURE);
        }
        else
        {
            //Sync_Timing_OSAL_Wrapper_SleepMS(1000);
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Device Booted - Initialize and setup IRQ "
                                                         "uMcuBootstate = %u\n", 
                                                          uMcuBootstate);
            syncStatus = Sync_Timing_Internal_CORE_Device_Init(pTimingDevContext);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                 "Sync_Timing_Internal_CORE_Device_Init failed; "
                                 "Check if DEVICE is accessible and is in APPLN mode.\n", 
                                 syncStatus);
            }     

            syncStatus = Sync_Timing_Internal_CORE_Device_SetupIRQ(pTimingDevContext);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                 "Sync_Timing_Internal_CORE_Device_SetupIRQ failed; "
                                 "Check if DEVICE is accessible and is in APPLN mode.\n", 
                                 syncStatus);
            }     
        }
  
        // Save appropriate mode
        if (uMcuBootstate <= BOOTLOADER_READY_STATE)
        {
            pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_BOOTLOADER;
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                               "Device in bootloader mode : uMcuBootstate = %u \n", uMcuBootstate);                 
        }
        else if (uMcuBootstate >= APPLICATION_READY_STATE)
        {
            pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_APPLN;
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, 
                               "Device in application mode : uMcuBootstate = %u \n", uMcuBootstate);                  
        }
        else
        {
            pTimingDevContext->deviceMode = SYNC_TIMING_DEVICE_MODE_INVALID;
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                               "Device in INVALID APPLN mode : uMcuBootstate = %u \n", uMcuBootstate);  
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_FAILURE);
        }
    }
    while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_Driver_IrqCallback
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This CORE Driver function is used to handle the interrupts from the IRQ layer
 *
 * IN PARAMS     : irqTag        - IRQ Tag identifying the interrupt
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
#if 0
SYNC_STATUS_E Sync_Timing_Internal_CORE_Driver_IrqCallback(uint32_t irqTag)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    uint8_t                             uTimingDevId            = 0;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    uint16_t                            irqPinNum               = 0;
    //uint32_t                            eventData[4]            = {0};

    uint8_t                             uDrvrEventFlags[7];
    uint64_t                            uClrIntr                = 0;
    uint8_t                             uIdx                    = 0;

    uTimingDevId = irqTag & 0xFF;
    irqPinNum = (irqTag & 0xFFFFFF00) >> 8;
    
    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, "uTimingDevId = 0x%x, irqTag = 0x%x, irqPinNum = %u \n", 
                                                   uTimingDevId, irqTag, irqPinNum);
    
    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(uTimingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        // Read the interrupt status register to determine the type of interrupt
        syncStatus = Sync_Timing_Internal_CORE_Mem_ReadDirect(pTimingDevContext, 
                         SYNC_TIMING_INTC_IRQx_START, 
                         7, 
                         (uint8_t*)&(uDrvrEventFlags));
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, "Driver Event Flags Value 0x%x : 0x%x : 0x%x : "
                                                    "0x%x : 0x%x : 0x%x : 0x%x\n",
                                                    uDrvrEventFlags[0], uDrvrEventFlags[1], 
                                                    uDrvrEventFlags[2], uDrvrEventFlags[3], 
                                                    uDrvrEventFlags[4], uDrvrEventFlags[5],
                                                    uDrvrEventFlags[6]);

        // Clear all pending INTR flags in the chipset registers (0x11 through 0x17)
        for (uIdx = 0; uIdx < 7; uIdx++)
        {
            if (uDrvrEventFlags[uIdx])
            {
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteDirect(pTimingDevContext, 
                                                                       SYNC_TIMING_INTC_IRQx_START + uIdx, 
                                                                       1, (unsigned char *) &uClrIntr);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
            }
        }

        
        // Now process the events received from firmware;
        
        //eventData[0] = 0;

        //syncStatus = Sync_Timing_Internal_CORE_Send_Events(uTimingDevId, SYNC_TIMING_DEVICE_CHIP_EVENT, 
        //                                          eventData);

        //SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        Sync_Timing_OSAL_Wrapper_SleepMS(100);            

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    return syncStatus;

}
#else
SYNC_STATUS_E Sync_Timing_Internal_CORE_Driver_IrqCallback(uint32_t irqTag)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    uint8_t                             uTimingDevId            = 0;
    SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext      = NULL;
    SYNC_TIMING_BOOL_E                  bHoldingMutex           = SYNC_TIMING_FALSE;

    uint16_t                            irqPinNum               = 0;

    cmd_INTERRUPT_STATUS_map_t          cmdIntrStatus           = {0};
    reply_INTERRUPT_STATUS_map_t        replyIntrStatus         = {0};
    cmd_INPUT_STATUS_map_t              cmdInputStatus          = {0};
    reply_INPUT_STATUS_map_t            replyInputStatus        = {0};
    cmd_PLL_STATUS_map_t                cmdPllStatus            = {0};
    reply_PLL_STATUS_map_t              replyPllStatus          = {0};
    
    SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T    eventData         = {0};
    uint8_t                             uReplyStatus            = 0;


    uTimingDevId = irqTag & 0xFF;
    irqPinNum = (irqTag & 0xFFFFFF00) >> 8;
    
    SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "New Interrupt Received from Chipset: "
                                                 "uTimingDevId = 0x%x, irqTag = 0x%x, irqPinNum = %u \n", 
                                                  uTimingDevId, irqTag, irqPinNum);
    
    cmdIntrStatus.CMD = cmd_ID_INTERRUPT_STATUS;

    
    cmdInputStatus.CMD = cmd_ID_INPUT_STATUS;
    
    cmdPllStatus.CMD = cmd_ID_PLL_STATUS;      

    //Sync_Timing_OSAL_Wrapper_SleepMS(250);

    do
    {
        syncStatus = Sync_Timing_CORE_Ctrl_AcqDevice(uTimingDevId, &pTimingDevContext, 
                                                     &bHoldingMutex);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        // Read the interrupt status register to determine the type of interrupt
        Sync_Timing_OSAL_Wrapper_Memset(&replyIntrStatus, 0, sizeof(replyIntrStatus));
        
        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdIntrStatus), 
                                                                (uint8_t *)&cmdIntrStatus, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyIntrStatus), 
                                                                (uint8_t *)&replyIntrStatus, 
                                                                &uReplyStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);        
        
        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "INPUT_CLOCK_INVALID = 0x%x, "
                                                      "INPUT_CLOCK_VALID = 0x%x, "
                                                      "PLLR = 0x%x, "
                                                      "PLLA = 0x%x, "
                                                      "PLLB = 0x%x, "
                                                      "GENERAL = 0x%x, "
                                                      "SOFTWARE = 0x%x, "
                                                      "PHASE_CONTROL = %u \n", 
                                                       replyIntrStatus.INPUT_CLOCK_INVALID,
                                                       replyIntrStatus.INPUT_CLOCK_VALID,
                                                       replyIntrStatus.PLLR,
                                                       replyIntrStatus.PLLA,
                                                       replyIntrStatus.PLLB,
                                                       replyIntrStatus.GENERAL,
                                                       replyIntrStatus.SOFTWARE,
                                                       replyIntrStatus.PHASE_CONTROL);

        // Clear all status flags now that we have processed the interrupt.
        if (pTimingDevContext->bHostClearStatus)
        {
            syncStatus = Sync_Timing_Internal_CORE_Device_ClearInterrupts(pTimingDevContext);
            if (syncStatus != SYNC_STATUS_SUCCESS)
            {
                SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                    "Sync_Timing_Internal_CORE_Device_ClearInterrupts failed; \n", 
                                    syncStatus);
            }
        }
        
        // Now process the events received from firmware;
        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN0_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN0_VALID_TRUE_BIT))
        {
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN0;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN0: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);
            
            pTimingDevContext->bROSInputPollList[0] = SYNC_TIMING_TRUE;

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN0;
            }
            else
            {            
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN0;
                
                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN0;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN0;
                }     
            }
        }
            
        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN1_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN1_VALID_TRUE_BIT))
        {
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN1;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   

            pTimingDevContext->bROSInputPollList[2] = SYNC_TIMING_TRUE;                

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN1: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN1;
            }
            else
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN1; 

                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN1;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN1;
                }     
            }
        }
            
        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN2_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN2_VALID_TRUE_BIT))
        {
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN2;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   

            pTimingDevContext->bROSInputPollList[4] = SYNC_TIMING_TRUE;

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN2: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);
            
            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN2;
            }
            else
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN2;

                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN2;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN2;
                }     
            }
        }

        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN2B_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN2B_VALID_TRUE_BIT))
        {
            
            
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN2B;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN2B: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);
            
            pTimingDevContext->bROSInputPollList[5] = SYNC_TIMING_TRUE;                

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN2B;
            }
            else
            {            
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN2B;

                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN2B;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN2B;
                }   
            }
        }         
            
        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN3_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN3_VALID_TRUE_BIT))
        {
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN3;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN3: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);
            
            pTimingDevContext->bROSInputPollList[6] = SYNC_TIMING_TRUE;                

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN3;
            }
            else
            {                  
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN3;

                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN3;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN3;
                }     
            }
        }

        if ((replyIntrStatus.INPUT_CLOCK_INVALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_INVALID_IN3B_INVALID_TRUE_BIT) ||
            (replyIntrStatus.INPUT_CLOCK_VALID & 
            CMD_INTERRUPT_STATUS_REPLY_INPUT_CLOCK_VALID_IN3B_VALID_TRUE_BIT))
        {
            cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN3B;
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);   
            
            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "IN3B: LOSS_OF_SIGNAL = %x, " 
                                                          "OUT_OF_FREQUENCY = %x, "
                                                          "INPUT_CLOCK_VALIDATION = %x, "
                                                          "PHASE_MONITOR = %x \n",
                                                          replyInputStatus.LOSS_OF_SIGNAL,
                                                          replyInputStatus.OUT_OF_FREQUENCY,
                                                          replyInputStatus.INPUT_CLOCK_VALIDATION,
                                                          replyInputStatus.PHASE_MONITOR);

            pTimingDevContext->bROSInputPollList[7] = SYNC_TIMING_TRUE;    
            
            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID)
            {
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_ROS_IN3B;
            }
            else
            {              
                eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_INVALID_IN3B;
                if (replyInputStatus.LOSS_OF_SIGNAL & 
                    CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_LOS_IN3B;
                }

                if (replyInputStatus.OUT_OF_FREQUENCY & 
                    CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
                {
                    eventData.deviceEventInfo.deviceInputEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_IN3B;
                }     
            }
        }

        if (replyIntrStatus.PLLR)
        {
            eventData.deviceEventInfo.devicePllEvents |= replyIntrStatus.PLLR;
            cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLR;
            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);  

            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);  

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "PLL-R: replyPllStatus.PLL_LOSS_OF_LOCK_MISC = 0x%x "
                                                          "replyPllStatus.PLL_HOLDOVER = 0x%x "
                                                          "replyPllStatus.PLL_SHORT_TERM_HOLDOVER = 0x%x \n",
                                                           replyPllStatus.PLL_LOSS_OF_LOCK_MISC,
                                                           replyPllStatus.PLL_HOLDOVER,
                                                           replyPllStatus.PLL_SHORT_TERM_HOLDOVER);            
            if (replyPllStatus.PLL_HOLDOVER & 
                CMD_PLL_STATUS_REPLY_PLL_HOLDOVER_VALID_HOLDOVER_VALID_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HO_PLL_R;
            }

            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_FREQUENCY_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_PLL_R;
            }
                
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_PHASE_TRUE_BIT)                
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_R;
            }       

            if (replyIntrStatus.PLLR & CMD_INTERRUPT_STATUS_REPLY_PLLR_PLLR_HITLESS_SWITCH_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_R;
            }       
            
            if (replyIntrStatus.PLLR & CMD_INTERRUPT_STATUS_REPLY_PLLR_PLLR_CYCLE_SLIP_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_R;
            }       

        }

        if (replyIntrStatus.PLLA)
        {
            eventData.deviceEventInfo.devicePllEvents |= replyIntrStatus.PLLA << 8;
            cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLA;  
            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);    
            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);    

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "PLL-A: replyPllStatus.PLL_LOSS_OF_LOCK_MISC = 0x%x "
                                                          "replyPllStatus.PLL_HOLDOVER = 0x%x "
                                                          "replyPllStatus.PLL_SHORT_TERM_HOLDOVER = 0x%x \n",
                                                           replyPllStatus.PLL_LOSS_OF_LOCK_MISC,
                                                           replyPllStatus.PLL_HOLDOVER,
                                                           replyPllStatus.PLL_SHORT_TERM_HOLDOVER);           
            if (replyPllStatus.PLL_HOLDOVER & 
                CMD_PLL_STATUS_REPLY_PLL_HOLDOVER_VALID_HOLDOVER_VALID_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HO_PLL_A;
            }

            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_FREQUENCY_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_PLL_A;
            }
                
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_PHASE_TRUE_BIT)                
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_A;
            }              

            if (replyIntrStatus.PLLA & CMD_INTERRUPT_STATUS_REPLY_PLLA_PLLA_HITLESS_SWITCH_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_A;
            }       
            
            if (replyIntrStatus.PLLA & CMD_INTERRUPT_STATUS_REPLY_PLLA_PLLA_CYCLE_SLIP_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_A;
            }       

        }

        if (replyIntrStatus.PLLB)
        {
            eventData.deviceEventInfo.devicePllEvents |= replyIntrStatus.PLLB << 16;
            cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLB;
            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 
            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS); 

            SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "PLL-B: replyPllStatus.PLL_LOSS_OF_LOCK_MISC = 0x%x "
                                                          "replyPllStatus.PLL_HOLDOVER = 0x%x "
                                                          "replyPllStatus.PLL_SHORT_TERM_HOLDOVER = 0x%x \n",
                                                           replyPllStatus.PLL_LOSS_OF_LOCK_MISC,
                                                           replyPllStatus.PLL_HOLDOVER,
                                                           replyPllStatus.PLL_SHORT_TERM_HOLDOVER);             
            if (replyPllStatus.PLL_HOLDOVER & 
                CMD_PLL_STATUS_REPLY_PLL_HOLDOVER_VALID_HOLDOVER_VALID_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HO_PLL_B;
            }

            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_FREQUENCY_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOF_PLL_B;
            }
                
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_PHASE_TRUE_BIT)                
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_B;
            }            
                
            if (replyIntrStatus.PLLB & CMD_INTERRUPT_STATUS_REPLY_PLLB_PLLB_HITLESS_SWITCH_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_B;
            }       
            
            if (replyIntrStatus.PLLB & CMD_INTERRUPT_STATUS_REPLY_PLLB_PLLB_CYCLE_SLIP_TRUE_BIT)
            {
                eventData.deviceEventInfo.devicePllEvents |= SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_B;
            }       
                
        }

        if (replyIntrStatus.GENERAL)
        {
            eventData.deviceEventInfo.deviceGenEvents = replyIntrStatus.GENERAL;
        }

        SYNC_TIMING_ALWAYS(pSyncTimingCoreLogHandle, "Driver Events: deviceInputEvents = 0x%x "
                                                      "devicePllEvents = 0x%x "
                                                      "deviceGenEvents = 0x%x \n",
                                                       eventData.deviceEventInfo.deviceInputEvents,
                                                       eventData.deviceEventInfo.devicePllEvents,
                                                       eventData.deviceEventInfo.deviceGenEvents);   
        
        syncStatus = Sync_Timing_Internal_CORE_Send_Events(uTimingDevId, SYNC_TIMING_DEVICE_CHIP_EVENT, 
                                                           &eventData);

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);                  

    } while (0);

    if (bHoldingMutex != SYNC_TIMING_FALSE)
    {
        (void)Sync_Timing_OSAL_Wrapper_Mutex_Put(pTimingDevContext->pMutex);
    }

    Sync_Timing_OSAL_Wrapper_SleepMS(100);  

    return syncStatus;

}
#endif



/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ClockAdj_SetPLLInput
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This Internal function is used to set the pll input selection
 *
 * IN PARAMS     : pTimingDevContext    - The Timing device Context
 *               : uPLLId               - PLL ID
 *               : pllInputSelect       - Input Selected
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_SetPLLInput(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect)
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    cmd_FORCE_HOLDOVER_map_t                cmdForceHO          = {0};
    cmd_MANUAL_INPUT_CLOCK_SELECT_map_t     cmdManInClkSel      = {0};
    uint8_t                                 uReplyStatus        = 0;

    cmdForceHO.CMD  = cmd_ID_FORCE_HOLDOVER;
    cmdManInClkSel.CMD = cmd_ID_MANUAL_INPUT_CLOCK_SELECT;
    
    do
    {
        SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, "ClkAdjSetPLLInput -- uPLLId = %u; "
                                                    "pllInputSelect = %u.\n", 
                                                    uPLLId, pllInputSelect);

        // Check and setup the input for the PLL
        if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A) // PLL A
        {
            cmdForceHO.PLLX = CMD_FORCE_HOLDOVER_ARG_PLLX_PLL_ENUM_PLLA;
            cmdManInClkSel.PLLX = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_PLLX_PLL_ENUM_PLLA;
        }
        else if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_B) // PLL B
        {
            if (pTimingDevContext->bPtpSteeredRf)
            {
                SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                          SYNC_STATUS_NOT_SUPPORTED);
            }
            cmdForceHO.PLLX = CMD_FORCE_HOLDOVER_ARG_PLLX_PLL_ENUM_PLLB;
            cmdManInClkSel.PLLX = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_PLLX_PLL_ENUM_PLLB;
        }
        else if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_R)  //PLL R
        {
            cmdForceHO.PLLX = CMD_FORCE_HOLDOVER_ARG_PLLX_PLL_ENUM_PLLR;
            cmdManInClkSel.PLLX = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_PLLX_PLL_ENUM_PLLR;
        }
        else
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                "Invalid PLL ID (%u) specified for changing input\n", uPLLId);        
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        switch(pllInputSelect)
        {
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_FORCE_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                
                // Record in timing dev context if PLL-A is being put into forced holdover
                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_TRUE;
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, ">>> PLL-A forced into HO \n");
                }
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN0:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN0;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN1:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN1;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    // record that PLL-A is out of force holdover
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_FALSE;
                }                
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN2;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    // record that PLL-A is out of force holdover
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_FALSE;
                }
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2B:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN2B;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    // record that PLL-A is out of force holdover
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_FALSE;
                }
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN3;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    // record that PLL-A is out of force holdover
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_FALSE;
                }
                break;
            case SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3B:
                cmdForceHO.HOLDOVER = CMD_FORCE_HOLDOVER_ARG_HOLDOVER_FORCE_HOLDOVER_ENUM_NO_HOLDOVER;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdForceHO), 
                                                                        (uint8_t *)&cmdForceHO, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


                cmdManInClkSel.CLOCK_SELECT = CMD_MANUAL_INPUT_CLOCK_SELECT_ARG_CLOCK_SELECT_INPUT_CLOCK_ENUM_IN3B;
                syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                        sizeof(cmdManInClkSel), 
                                                                        (uint8_t *)&cmdManInClkSel, 
                                                                        SYNC_TIMING_TRUE, 
                                                                        0, 
                                                                        NULL, 
                                                                        &uReplyStatus);
                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

                SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
                if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A)
                {
                    // record that PLL-A is out of force holdover
                    pTimingDevContext->bSysclkPLLHO = SYNC_TIMING_FALSE;
                }
                break;
            default:
                break;
        }

    } while (0);

    return syncStatus;
}                                                   

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/18/2019
 *
 * DESCRIPTION   : This Internal function is used to get the pll or input status from the chipset
 *
 * IN PARAMS     : pTimingDevContext    - The Timing device Context
 *               : statusType           - Status type desired (PLL-A, IN0, etc)
 *
 * OUT PARAMS    : pStatus              - Status requested
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T   *pTimingDevContext,
                                            SYNC_TIMING_CLOCKADJ_STATUS_ID_E    statusType, 
                                            SYNC_TIMING_CLOCKADJ_STATUS_E       *pStatus)
{
    SYNC_STATUS_E                       syncStatus              = SYNC_STATUS_SUCCESS;
    cmd_INPUT_STATUS_map_t              cmdInputStatus          = {0};
    reply_INPUT_STATUS_map_t            replyInputStatus        = {0};
    cmd_PLL_STATUS_map_t                cmdPllStatus            = {0};
    reply_PLL_STATUS_map_t              replyPllStatus          = {0};
    cmd_REFERENCE_STATUS_map_t          cmdRefInputStatus       = {0};
    reply_REFERENCE_STATUS_map_t        replyRefInputStatus     = {0};
    SYNC_TIMING_BOOL_E                  bSendRefStatusCmd       = SYNC_TIMING_FALSE;
    SYNC_TIMING_BOOL_E                  bSendPllStatusCmd       = SYNC_TIMING_TRUE;
    uint8_t                             uReplyStatus            = 0;

    *pStatus = 0;

    cmdInputStatus.CMD = cmd_ID_INPUT_STATUS;
    cmdPllStatus.CMD = cmd_ID_PLL_STATUS;    
    cmdRefInputStatus.CMD = cmd_ID_REFERENCE_STATUS;
    
    Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
    Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
    Sync_Timing_OSAL_Wrapper_Memset(&replyRefInputStatus, 0, sizeof(replyRefInputStatus));

    do
    {
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "statusType = %u; ", statusType);

        switch(statusType)
        {
            case SYNC_TIMING_CLOCKADJ_STATUS_PLL_A:
                cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLA;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_PLL_B:
                cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLB;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_PLL_R:
                cmdPllStatus.PLL_SELECT = CMD_PLL_STATUS_ARG_PLL_SELECT_PLL_ENUM_PLLR;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN0:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN0;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN1:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN1;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN2:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN2;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN2B:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN2B;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN3:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN3;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_IN3B:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                cmdInputStatus.INPUT_SELECT = CMD_INPUT_STATUS_ARG_INPUT_SELECT_INPUT_ENUM_IN3B;
                break;
            case SYNC_TIMING_CLOCKADJ_STATUS_REF_INPUT:
                bSendPllStatusCmd = SYNC_TIMING_FALSE;
                bSendRefStatusCmd = SYNC_TIMING_TRUE;
                break;
            default:
                SYNC_TIMING_SET_ERR(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_INVALID_PARAMETER);
                break;
        }

        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if (bSendPllStatusCmd)       
        {
            // Clear all status flags before getting the pll status 
            if (pTimingDevContext->bHostClearStatus)
            {
                syncStatus = Sync_Timing_Internal_CORE_Device_ClearInterrupts(pTimingDevContext);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                        "Sync_Timing_Internal_CORE_Device_ClearInterrupts failed; \n", 
                                        syncStatus);
                }
            }

            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            Sync_Timing_OSAL_Wrapper_Memset(&replyPllStatus, 0, sizeof(replyPllStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdPllStatus), 
                                                                    (uint8_t *)&cmdPllStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyPllStatus), 
                                                                    (uint8_t *)&replyPllStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);


            SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "replyPllStatus.PLL_LOSS_OF_LOCK_MISC = 0x%x "
                                                          "replyPllStatus.PLL_HOLDOVER = 0x%x\n",
                                                           replyPllStatus.PLL_LOSS_OF_LOCK_MISC,
                                                           replyPllStatus.PLL_HOLDOVER);
            
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_LOSS_OF_LOCK_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_PLL_NOT_LOCKED;
            }
                
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_FREQUENCY_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_PLL_FREQ_ERR;
            }
                
            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_OUT_OF_PHASE_TRUE_BIT)                
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_PLL_PHASE_ERR;
            }

            if (replyPllStatus.PLL_LOSS_OF_LOCK_MISC & 
                CMD_PLL_STATUS_REPLY_PLL_LOSS_OF_LOCK_MISC_PLL_INITIAL_LOCK_TRUE_BIT)                
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_PLL_INIT_LOCK;
            }
                
            if (replyPllStatus.PLL_HOLDOVER & 
                CMD_PLL_STATUS_REPLY_PLL_HOLDOVER_HOLDOVER_TRUE_BIT)                
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_PLL_HOLDOVER;
            }

                
            if (*pStatus == 0)
            {
                *pStatus = SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED;
            }
        }
        else if(bSendRefStatusCmd)
        {
            // Clear all status flags before getting the input status 
            if (pTimingDevContext->bHostClearStatus)
            {
                syncStatus = Sync_Timing_Internal_CORE_Device_ClearInterrupts(pTimingDevContext);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                        "Sync_Timing_Internal_CORE_Device_ClearInterrupts failed; \n", 
                                        syncStatus);
                }
            }
            
            Sync_Timing_OSAL_Wrapper_Memset(&replyRefInputStatus, 0, sizeof(replyRefInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdRefInputStatus), 
                                                                    (uint8_t *)&cmdRefInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyRefInputStatus), 
                                                                    (uint8_t *)&replyRefInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            Sync_Timing_OSAL_Wrapper_Memset(&replyRefInputStatus, 0, sizeof(replyRefInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdRefInputStatus), 
                                                                    (uint8_t *)&cmdRefInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyRefInputStatus), 
                                                                    (uint8_t *)&replyRefInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "LOSS_OF_SIGNAL = %x, " 
                                          "OUT_OF_FREQUENCY = %x, "
                                          "INPUT_REFERENCE_VALIDATION = %x \n",
                                          replyRefInputStatus.LOSS_OF_SIGNAL,
                                          replyRefInputStatus.OUT_OF_FREQUENCY,
                                          replyRefInputStatus.REFERENCE_CLOCK_VALIDATION);

            if (replyRefInputStatus.REFERENCE_CLOCK_VALIDATION == 
                CMD_REFERENCE_STATUS_REPLY_REFERENCE_CLOCK_VALIDATION_REFERENCE_CLOCK_STATUS_ENUM_INVALID)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_INVALID;
            }

            if (replyRefInputStatus.REFERENCE_CLOCK_VALIDATION == 
                CMD_REFERENCE_STATUS_REPLY_REFERENCE_CLOCK_VALIDATION_REFERENCE_CLOCK_STATUS_ENUM_PENDING_SHORT_TERM_FAULT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_PENDING_SHORT_TERM_FAULT;
            }

            if (replyRefInputStatus.REFERENCE_CLOCK_VALIDATION == 
                CMD_REFERENCE_STATUS_REPLY_REFERENCE_CLOCK_VALIDATION_REFERENCE_CLOCK_STATUS_ENUM_UNDER_VALIDATION)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_UNDER_VALIDATION;
            }

            if (replyRefInputStatus.LOSS_OF_SIGNAL & 
                CMD_REFERENCE_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_NO_SIGNAL;
            }

            if (replyRefInputStatus.OUT_OF_FREQUENCY & 
                CMD_REFERENCE_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_OOF;
            }

            if (*pStatus == 0 && 
                (replyRefInputStatus.REFERENCE_CLOCK_VALIDATION == 
                 CMD_REFERENCE_STATUS_REPLY_REFERENCE_CLOCK_VALIDATION_REFERENCE_CLOCK_STATUS_ENUM_VALID))
            {
                *pStatus = SYNC_TIMING_CLOCKADJ_STATUS_INPUT_HAS_SIGNAL;
            }
        }
        else
        {
            // Clear all status flags before getting the input status 
            if (pTimingDevContext->bHostClearStatus)
            {
                syncStatus = Sync_Timing_Internal_CORE_Device_ClearInterrupts(pTimingDevContext);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    SYNC_TIMING_WARNING(pSyncTimingCoreLogHandle, 
                                        "Sync_Timing_Internal_CORE_Device_ClearInterrupts failed; \n", 
                                        syncStatus);
                }
            }
            
            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            Sync_Timing_OSAL_Wrapper_Memset(&replyInputStatus, 0, sizeof(replyInputStatus));
            syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                    sizeof(cmdInputStatus), 
                                                                    (uint8_t *)&cmdInputStatus, 
                                                                    SYNC_TIMING_TRUE, 
                                                                    sizeof(replyInputStatus), 
                                                                    (uint8_t *)&replyInputStatus, 
                                                                    &uReplyStatus);
            SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

            SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "LOSS_OF_SIGNAL = %x, " 
                                          "OUT_OF_FREQUENCY = %x, "
                                          "INPUT_CLOCK_VALIDATION = %x \n",
                                          replyInputStatus.LOSS_OF_SIGNAL,
                                          replyInputStatus.OUT_OF_FREQUENCY,
                                          replyInputStatus.INPUT_CLOCK_VALIDATION);

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_INVALID)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_INVALID;
            }

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_PENDING_SHORT_TERM_FAULT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_PENDING_SHORT_TERM_FAULT;
            }

            if (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_UNDER_VALIDATION)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_UNDER_VALIDATION;
            }

            if (replyInputStatus.LOSS_OF_SIGNAL & 
                CMD_INPUT_STATUS_REPLY_LOSS_OF_SIGNAL_LOSS_OF_SIGNAL_FLAG_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_NO_SIGNAL;
            }

            if (replyInputStatus.OUT_OF_FREQUENCY & 
                CMD_INPUT_STATUS_REPLY_OUT_OF_FREQUENCY_OUT_OF_FREQUENCY_FLAG_TRUE_BIT)
            {
                *pStatus |= SYNC_TIMING_CLOCKADJ_STATUS_INPUT_OOF;
            }

            if (*pStatus == 0 && 
                (replyInputStatus.INPUT_CLOCK_VALIDATION == 
                 CMD_INPUT_STATUS_REPLY_INPUT_CLOCK_VALIDATION_INPUT_CLOCK_STATUS_ENUM_VALID))
            {
                *pStatus = SYNC_TIMING_CLOCKADJ_STATUS_INPUT_HAS_SIGNAL;
            }
        }

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "*pClkStatus = 0x%08x; \n", *pStatus);

    } while (0);
    
    return syncStatus;
}     

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_CORE_ClockAdj_GetPLLInput
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/01/2020
 *
 * DESCRIPTION   : This Internal function is used to get the pll input selection
 *
 * IN PARAMS     : pTimingDevContext    - The Timing device Context
 *               : uPLLId               - PLL ID
 *               
 * OUT PARAMS    : pllInputSelect       - Current Input Selected
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or 
 *                  SYNC_STATUS_INVALID_PARAMETER or 
 *                  SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_CORE_ClockAdj_GetPLLInput(
                                            SYNC_TIMING_CORE_DEVICE_CONTEXT_T  *pTimingDevContext, 
                                            SYNC_TIMING_CLOCKADJ_PLL_ID_E uPLLId, 
                                            SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E *pllInputSelect)
{
    SYNC_STATUS_E                                   syncStatus          = SYNC_STATUS_SUCCESS;
    cmd_PLL_ACTIVE_REFCLOCK_map_t                   cmdActRefClk        = {0};
    cmd_PLL_ACTIVE_REFCLOCK_REPLY_REFCLOCK_map_t    replyActRefClk      = {0};
    SYNC_TIMING_CLOCKADJ_STATUS_E                   clkStatus           = 0;
    uint8_t                                         uReplyStatus        = 0;

    cmdActRefClk.CMD  = cmd_ID_PLL_ACTIVE_REFCLOCK;
    cmdActRefClk.PLLX = cmd_ID_MANUAL_INPUT_CLOCK_SELECT;
    
    do
    {
        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "ClkAdjGetPLLInput -- uPLLId = %u.\n", 
                                                       uPLLId);

        syncStatus = Sync_Timing_Internal_CORE_ClockAdj_GetCurrentStatus(pTimingDevContext, 
                                                                         uPLLId,
                                                                         &clkStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "ClkAdjGetPLLInput -- clkStatus = 0x%x.\n", 
                                                              clkStatus);

        if ((clkStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED) != SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED)
        {
            *pllInputSelect = SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE;
            SYNC_TIMING_SET_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);
        }

        // Check and setup the input for the PLL
        if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_A) // PLL A
        {
            cmdActRefClk.PLLX = CMD_PLL_ACTIVE_REFCLOCK_ARG_PLLX_PLL_ENUM_PLLA;
        }
        else if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_B) // PLL B
        {
            cmdActRefClk.PLLX = CMD_PLL_ACTIVE_REFCLOCK_ARG_PLLX_PLL_ENUM_PLLB;
        }
        else if (uPLLId == SYNC_TIMING_CLOCKADJ_PLL_ID_R)  //PLL R
        {
            cmdActRefClk.PLLX = CMD_PLL_ACTIVE_REFCLOCK_ARG_PLLX_PLL_ENUM_PLLR;
        }
        else
        {
            SYNC_TIMING_ERROR(pSyncTimingCoreLogHandle, 
                                "Invalid PLL ID (%u) specified for changing input\n", uPLLId);        
            SYNC_TIMING_SET_ERR_BREAK(pSyncTimingCoreLogHandle, syncStatus, 
                                      SYNC_STATUS_INVALID_PARAMETER);
        }

        syncStatus = Sync_Timing_Internal_CORE_Mem_WriteCommand(pTimingDevContext, 
                                                                sizeof(cmdActRefClk), 
                                                                (uint8_t *)&cmdActRefClk, 
                                                                SYNC_TIMING_TRUE, 
                                                                sizeof(replyActRefClk), 
                                                                (uint8_t *)&replyActRefClk, 
                                                                &uReplyStatus);
        SYNC_TIMING_ERRCHECK_BREAK(pSyncTimingCoreLogHandle, syncStatus, SYNC_STATUS_SUCCESS);

        if (replyActRefClk.active_refclock != CMD_PLL_ACTIVE_REFCLOCK_REPLY_REFCLOCK_ACTIVE_REFCLOCK_MASK)
        {
            *pllInputSelect = replyActRefClk.active_refclock - \
                              CMD_PLL_ACTIVE_REFCLOCK_REPLY_REFCLOCK_ACTIVE_REFCLOCK_ENUM_IN0;       
        }
        else
        {
            *pllInputSelect = SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE;
        }

    } while (0);

    SYNC_TIMING_INFO1(pSyncTimingCoreLogHandle, "Response: *pllInputSelect = 0x%02x; \n", *pllInputSelect);

    return syncStatus;
}    


