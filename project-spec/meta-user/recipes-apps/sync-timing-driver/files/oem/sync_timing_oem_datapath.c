/****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_spi.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/27/2018
 *
 * DESCRIPTION        : Source code for Host/Chipset OEM SPI communication interface.
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
#include "sync_timing_common.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_oem_common.h"
#include "sync_timing_osal.h"

#include "sync_timing_oem_linux_spidev.h"
#include "sync_timing_oem_linux_i2cdev.h"

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/
//2-dimensional array of pointers to OEM handlers
static void*  gpOem_DataPath_Handle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES]; 

//2-dimensional array of datapath handles
static SYNC_TIMING_OEM_DATAPATH_HANDLE_T   gOemDataPathHandle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES] ; 

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_DATAPATH_ValidateOemHandle
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : This function is used to validate a given Oem Handle
 *
 * IN PARAMS     : pOemHandle   - OEM Handle
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_OEM_DATAPATH_ValidateOemHandle( void* pOemHandle )
{
    uint8_t                       timingDevId = 0, 
                                  OemHandleId = 0;
    SYNC_STATUS_E                 syncStatus = SYNC_STATUS_INVALID_PARAMETER;
    SYNC_TIMING_OEM_DATAPATH_HANDLE_T* pOemDataPathHandle = (SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pOemHandle;
    
    if(pOemDataPathHandle)
    {
        timingDevId = pOemDataPathHandle->timingDevId;
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(OemHandleId < SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES)
            {
                if(&gOemDataPathHandle[timingDevId][OemHandleId] == pOemDataPathHandle)
                {
                    //printf("%s:%d:%u:%u\n", __FUNCTION__, __LINE__, timingDevId, OemHandleId);
                    syncStatus = SYNC_STATUS_SUCCESS; // valid OEM handler
                    break;
                }
                OemHandleId++;
            }
        }
    }
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_DATAPATH_GetUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : Internal function to get a unit resource
 *
 * IN PARAMS     : timingDevId        - Timing Device ID
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
void* Sync_Timing_Internal_OEM_DATAPATH_GetUnitResource( uint8_t timingDevId )
{
    void  *pCurDataPathHdl  = NULL;
    void  *pHandle          = NULL;
    uint8_t dataPathDevId   = 0;
    
    if(timingDevId < SYNC_TIMING_MAX_DEVICES)
    {
        for (dataPathDevId = 0; dataPathDevId < SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES; dataPathDevId++)
        {
          if(gpOem_DataPath_Handle[timingDevId][dataPathDevId] != NULL)
            break;
        }
        
        if (dataPathDevId >= SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES)
        {
          dataPathDevId = 0; // 1 device only, else return error
        }
        
        // Check handle before providing for use
        pHandle = gpOem_DataPath_Handle[timingDevId][dataPathDevId];
        if(pHandle == NULL)
        {
            pHandle = &gOemDataPathHandle[timingDevId][dataPathDevId];
            gpOem_DataPath_Handle[timingDevId][dataPathDevId] = pHandle;
            Sync_Timing_OSAL_Wrapper_Memset(pHandle, 0, sizeof(SYNC_TIMING_OEM_DATAPATH_HANDLE_T));
            ((SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pHandle)->timingDevId = timingDevId;
        }
        
        ((SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pHandle)->timingDevId = timingDevId;
        pCurDataPathHdl = pHandle;
    }
    
    return pCurDataPathHdl;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : Internal function to free up resource
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the ptp device
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource( void* pOemHandle )
{
    uint8_t                             timingDevId           = 0,
                                        dataPathDevId           = 0;
    SYNC_TIMING_OEM_DATAPATH_HANDLE_T   *pOemDataPathHandle   = NULL;
    SYNC_STATUS_E                       syncStatus            = SYNC_STATUS_INVALID_PARAMETER;

    if(pOemHandle)
    {
        pOemDataPathHandle = (SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pOemHandle;
        
        timingDevId   = pOemDataPathHandle->timingDevId;
        
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(dataPathDevId < SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES)
            {
                //printf("%s:%d:%u:%u\n", __FUNCTION__, __LINE__, timingDevId, dataPathDevId);
                if(&(gOemDataPathHandle[timingDevId][dataPathDevId]) == pOemHandle)
                {
                    //pOemDataPathHandle = NULL;
                    //printf("%s:%d:%u:%u\n", __FUNCTION__, __LINE__, timingDevId, dataPathDevId);
                    gpOem_DataPath_Handle[timingDevId][dataPathDevId] = NULL;
                    syncStatus = SYNC_STATUS_SUCCESS;
                    break;
                }
                dataPathDevId++;
            }   
        }
    }
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_DATAPATH_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : This function is used to initialize the Data Path device
 *
 * IN PARAMS     : timingDevId    - Timing Device ID
 *               : pOemData       - Pointer to OEM data; Only the specified datapath type 
 *                                  data will be used
 *
 * OUT PARAMS    : ppOemHandle   - Pointer to storage for returning the Oem Handle
 *                 pOemDataPathStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Init(uint8_t                    timingDevId, 
                                    void                               *pOemData, 
                                    void                               **ppOemHandle,
                                    SYNC_TIMING_OEM_DATAPATH_STATUS_E  *pOemDataPathStatus
                                     )
{
    SYNC_STATUS_E                      syncStatus        = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_CFG_DATA_T         *pOemCfg          = NULL;
    SYNC_TIMING_OEM_SPI_CFG_T          *pOemSpiCfg       = NULL;
    SYNC_TIMING_OEM_I2C_CFG_T          *pOemI2cCfg       = NULL;
    SYNC_TIMING_OEM_DATAPATH_HANDLE_T  *pDataPathHandle  = NULL;

    do
    {
        if (!pOemData || !ppOemHandle || !pOemDataPathStatus)
        {
            if (pOemDataPathStatus)
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }
        
        // Init oemHdlptr
        *ppOemHandle = NULL;
        pOemCfg = (SYNC_TIMING_OEM_CFG_DATA_T *)pOemData;

        if (pOemCfg->OemDataPath == SYNC_TIMING_OEM_DATAPATH_SPI)
        {
            pOemSpiCfg = (SYNC_TIMING_OEM_SPI_CFG_T *)pOemCfg->dataPathCfg.pSpiCfg;
            if (NULL == pOemSpiCfg)
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
                syncStatus =  SYNC_STATUS_INVALID_PARAMETER;
                break;
            }
        }
        else if (pOemCfg->OemDataPath == SYNC_TIMING_OEM_DATAPATH_I2C)
        {
            pOemI2cCfg = (SYNC_TIMING_OEM_I2C_CFG_T *)pOemCfg->dataPathCfg.pI2cCfg;
            if (NULL == pOemI2cCfg)
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
                syncStatus = SYNC_STATUS_INVALID_PARAMETER;
                break;
            }
        }
        else
        {
            *pOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
            syncStatus =  SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
            break;
        }

        pDataPathHandle = (SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)
                                Sync_Timing_Internal_OEM_DATAPATH_GetUnitResource(timingDevId);
        if (pDataPathHandle)
        {
            if (pOemCfg->OemDataPath == SYNC_TIMING_OEM_DATAPATH_SPI)
            {
                pDataPathHandle->timingDataPath = SYNC_TIMING_OEM_DATAPATH_SPI;
                pDataPathHandle->oemDeviceType = pOemCfg->oemDeviceType;

                syncStatus = Sync_Timing_OemLinuxSpidev_Open(pOemSpiCfg->spiDevName,
                                                             pOemSpiCfg->spiDevId, 
                                                             pOemSpiCfg->spiSpeed,
                                                             pOemSpiCfg->spiBitsPerWord, 
                                                             pOemSpiCfg->spiMode,
                                                             0);
                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    Sync_Timing_OSAL_Wrapper_Memcpy(&(pDataPathHandle->dataPathCfg.spiCfg), 
                                                    pOemSpiCfg, sizeof(SYNC_TIMING_OEM_SPI_CFG_T)); 
                    
                    // update connections status
                    pDataPathHandle->devCfgState = SYNC_TIMING_TRUE; // indicate connection

                    pDataPathHandle->timingDevId = timingDevId;
                    *ppOemHandle = (void *)pDataPathHandle;
                    if (pOemDataPathStatus)
                    {
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
                    }
                }
                else
                {
                    syncStatus = SYNC_STATUS_FAILURE;
                    if (pOemDataPathStatus)
                    {
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                    }
                    Sync_Timing_OemLinuxSpidev_Close(pOemSpiCfg->spiDevId);
                    Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource(pDataPathHandle);
                }
            }
            else if (pOemCfg->OemDataPath == SYNC_TIMING_OEM_DATAPATH_I2C)
                {
                pDataPathHandle->timingDataPath = SYNC_TIMING_OEM_DATAPATH_I2C;
                pDataPathHandle->oemDeviceType = pOemCfg->oemDeviceType;

                syncStatus = Sync_Timing_OemLinuxI2cdev_Open(pOemI2cCfg->i2cDevName,
                                                             pOemI2cCfg->i2cDevId,
                                                             pOemI2cCfg->i2cDevAddr);
                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    Sync_Timing_OSAL_Wrapper_Memcpy(&(pDataPathHandle->dataPathCfg.i2cCfg),
                                                    pOemI2cCfg, sizeof(SYNC_TIMING_OEM_I2C_CFG_T));

                    // update connections status
                    pDataPathHandle->devCfgState = SYNC_TIMING_TRUE; // indicate connection

                    pDataPathHandle->timingDevId = timingDevId;
                    *ppOemHandle = (void *)pDataPathHandle;
                    if (pOemDataPathStatus)
                    {
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
                    }
                }
                else
                {
                    syncStatus = SYNC_STATUS_FAILURE;
                    if (pOemDataPathStatus)
                    {
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                    }
                    Sync_Timing_OemLinuxI2cdev_Close(pOemI2cCfg->i2cDevId);
                    Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource(pDataPathHandle);
                }

            }
            else
            {
                // Should not come here but anyways take care of cleanup and return
                Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource(pDataPathHandle);
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                syncStatus =  SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                break;
            }
        }
        else
        {
            // TBD: should be resource exceed!!
            if (pOemDataPathStatus)
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_NO_RESOURCE;
            }
            syncStatus = SYNC_STATUS_FAILURE;
        }
    } while(0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_DATAPATH_Transfer
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : This function is used to read/write data from/to the SPI device
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the SPI device
 *               : pTxBuff        - Pointer to buffer containing transmit data
 *               : buffLength     - Transfer Size
 *               : readLength     - Expected length of the reply
 *               : pRxBuff        - Pointer to buffer containing recieved data
 *
 * OUT PARAMS    : pOemDataPathStatus  - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Transfer(void                   *pOemHdl,
                                     uint8_t                           *pTxBuff,
                                     uint16_t                          buffLength,
                                     uint8_t                           *pRxBuff,
                                     uint16_t                          readLength,
                                     SYNC_TIMING_OEM_DATAPATH_STATUS_E *pOemDataPathStatus
                                          )
{
    SYNC_STATUS_E                       syncStatus        = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_DATAPATH_HANDLE_T   *pDataPathHandle  = NULL;

    do
    {
        if (!pOemHdl || !pOemDataPathStatus || !pTxBuff || !pRxBuff)
        {
            if (pOemDataPathStatus)
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pDataPathHandle = (SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pOemHdl;

        syncStatus = Sync_Timing_Internal_OEM_DATAPATH_ValidateOemHandle(pDataPathHandle);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;

            if (pDataPathHandle->timingDataPath == SYNC_TIMING_OEM_DATAPATH_SPI)
            {
                syncStatus = Sync_Timing_OemLinuxSpidev_Transfer(
                                                    pDataPathHandle->dataPathCfg.spiCfg.spiDevId,
                                                    buffLength, pTxBuff, pRxBuff, readLength
                                                    );
                // increase statistic counters
                pDataPathHandle->numDataBytes += readLength;
                pDataPathHandle->numDataAccesses++;

                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    *pOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
                }
                else
                {
                    syncStatus = SYNC_STATUS_FAILURE;
                    *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                }
            }
            else if(pDataPathHandle->timingDataPath == SYNC_TIMING_OEM_DATAPATH_I2C)
            {
                syncStatus = Sync_Timing_OemLinuxI2cdev_Transfer(
                                                    pDataPathHandle->dataPathCfg.i2cCfg.i2cDevId,
                                                    buffLength, pTxBuff, pRxBuff, readLength
                                                    );
                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    *pOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
                }
                else
                {
                    syncStatus = SYNC_STATUS_FAILURE;
                    *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                }
            }
            else
            {
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                syncStatus = SYNC_STATUS_NOT_SUPPORTED;
            }
        }
        else
        {
            *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
        }
    } while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_DATAPATH_Term
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 07/31/2018
 *
 * DESCRIPTION   : This function is used to terminate the SPI device
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the ptp device
 *
 * OUT PARAMS    : pOemDataPathStatus  - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Term(void                       *pOemHdl,
                                    SYNC_TIMING_OEM_DATAPATH_STATUS_E  *pOemDataPathStatus
                                    )
{
    SYNC_STATUS_E                       syncStatus       = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_DATAPATH_HANDLE_T   *pDataPathHandle = NULL;

    do
    {
        if (!pOemHdl || !pOemDataPathStatus)
        {
            //printf("%s:%d\n", __FUNCTION__, __LINE__);
            if (pOemDataPathStatus)
            {
                //printf("%s:%d\n", __FUNCTION__, __LINE__);
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pDataPathHandle = (SYNC_TIMING_OEM_DATAPATH_HANDLE_T*)pOemHdl;

        syncStatus = Sync_Timing_Internal_OEM_DATAPATH_ValidateOemHandle(pDataPathHandle);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;

            if(pDataPathHandle->devCfgState == SYNC_TIMING_TRUE)
            {
                if (pDataPathHandle->timingDataPath == SYNC_TIMING_OEM_DATAPATH_SPI)
                {
                    syncStatus = Sync_Timing_OemLinuxSpidev_Close(
                                                    pDataPathHandle->dataPathCfg.spiCfg.spiDevId);
                    //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);

                    if (syncStatus == SYNC_STATUS_SUCCESS)
                    {
                        syncStatus = Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource(pDataPathHandle);
                        //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
                    }
                    else
                    {
                        syncStatus = SYNC_STATUS_FAILURE;
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                    }
                }
                else if (pDataPathHandle->timingDataPath == SYNC_TIMING_OEM_DATAPATH_I2C)
                {
                    syncStatus = Sync_Timing_OemLinuxI2cdev_Close(
                                                    pDataPathHandle->dataPathCfg.i2cCfg.i2cDevId);
                    //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);

                    if (syncStatus == SYNC_STATUS_SUCCESS)
                    {
                        syncStatus = Sync_Timing_Internal_OEM_DATAPATH_FreeUnitResource(pDataPathHandle);
                        //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
                    }
                    else
                    {
                        syncStatus = SYNC_STATUS_FAILURE;
                        *pOemDataPathStatus = SYNC_TIMING_DATAPATH_ERROR;
                    }                    
                }
                else
                {
                    syncStatus = SYNC_STATUS_NOT_SUPPORTED;
                    *pOemDataPathStatus = SYNC_TIMING_DATAPATH_NOT_SUPPORTED;
                }    
            }

            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
                *pOemDataPathStatus = SYNC_TIMING_DATAPATH_SUCCESS;
            }
        }
        else
        {
            *pOemDataPathStatus = SYNC_TIMING_DATAPATH_INVALID_PARAMETER;
        }
    } while(0);

    return syncStatus;
}

