/****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_irq.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/12/2018
 *
 * DESCRIPTION        : Source code for Host/Chipset OEM IRQ CTRL interface.
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

#include "sync_timing_oem_linux_irq.h"

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/
//2-dimensional array of pointers to OEM handlers
static void*  gpOem_IRQCtrl_Handle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES]; 

//2-dimensional array of IRQ Ctrl handles
static SYNC_TIMING_OEM_IRQCTRL_HANDLE_T   gOemIRQCtrlHandle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES] ; 

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_IrqCtrl_ValidateOemHandle
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This internal function is used to validate a given Oem Handle
 *
 * IN PARAMS     : pOemHandle   - OEM Handle
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_OEM_IRQCtrl_ValidateOemHandle( void* pOemHandle )
{
    uint8_t                              timingDevId        = 0, 
                                         resetCtrlDevId     = 0;
    SYNC_STATUS_E                        syncStatus         = SYNC_STATUS_INVALID_PARAMETER;
    SYNC_TIMING_OEM_IRQCTRL_HANDLE_T     *pOemIRQCtrlHandle = (SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)pOemHandle;

    if(pOemIRQCtrlHandle)
    {
        timingDevId = pOemIRQCtrlHandle->timingDevId;
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(resetCtrlDevId < SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES)
            {
                if(&gOemIRQCtrlHandle[timingDevId][resetCtrlDevId] == pOemIRQCtrlHandle)
                {
                    syncStatus = SYNC_STATUS_SUCCESS; // valid OEM handler
                    break;
                }
                resetCtrlDevId++;
            }
        }
    }
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_IRQCTRL_GetUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This internal function is used to allocate an Oem Handle for reset ctrl device
 *
 * IN PARAMS     : timingDevId   - Timing device Id
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
void* Sync_Timing_Internal_OEM_IRQCTRL_GetUnitResource( uint8_t timingDevId )
{
    void*   curIRQCtrlHdl       = NULL;
    uint8_t resetCtrlDevId        = 0;
    
    if(timingDevId < SYNC_TIMING_MAX_DEVICES)
    {
        for (resetCtrlDevId = 0; resetCtrlDevId < SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES; resetCtrlDevId++)
        {
            if(gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] != NULL)
                break;
        }
        
        if (resetCtrlDevId >= SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES)
        {
            resetCtrlDevId = 0; // 1 device only, else return error
        }
        
        // valid devId
        if(gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] == NULL)
        {
            gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] =  &gOemIRQCtrlHandle[timingDevId][resetCtrlDevId];
            if(gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId])
            {
                Sync_Timing_OSAL_Wrapper_Memset(gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId], 0, sizeof(SYNC_TIMING_OEM_IRQCTRL_HANDLE_T));

                if(((SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId]))
                {         
                    ((SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId])->timingDevId = timingDevId;
                }
                else
                {
                    gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] = NULL;
                }
            }
        }
        ((SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId])->timingDevId = timingDevId;
        curIRQCtrlHdl = gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId];
    }
    
    return curIRQCtrlHdl;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_IRQCtrl_FreeUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This internal function is used to free the Oem Handle for reset ctrl device
 *
 * IN PARAMS     : pOemHandle   - OEM Handle
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_Internal_OEM_IRQCtrl_FreeUnitResource( void* pOemHandle )
{
    uint8_t                             timingDevId          = 0,
                                        resetCtrlDevId       = 0;
    SYNC_TIMING_OEM_IRQCTRL_HANDLE_T    *pOemIRQCtrlHandle   = NULL;
    SYNC_STATUS_E                       syncStatus           = SYNC_STATUS_INVALID_PARAMETER;

    if(pOemHandle)
    {
        pOemIRQCtrlHandle = (SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)pOemHandle;
        timingDevId   = pOemIRQCtrlHandle->timingDevId;
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(resetCtrlDevId < SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES)
            {
                if(gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] == pOemHandle)
                {
                    pOemIRQCtrlHandle = NULL;
                    gpOem_IRQCtrl_Handle[timingDevId][resetCtrlDevId] = NULL;
                    syncStatus = SYNC_STATUS_SUCCESS;
                    break;
                }
                resetCtrlDevId++;
            }   
        }
    }
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_IRQCtrl_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used to initialize the reset ctrl device for the timing
 *                 chipset
 *
 * IN PARAMS     : timingDevId   - The Timing device Id
 *               : pOemData      - The OEM Data required for reset control device
 *
 * OUT PARAMS    : ppOemHandle   - The OEM handle for future API calls
 *               : pOemIRQCtrlStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_IRQCtrl_Init(uint8_t                            timingDevId, 
                                           void                               *pOemData, 
                                           SYNC_TIMING_OEM_IRQ_CALLBACK       pOemIrqCallbackFn,
                                           void                               **ppOemHandle,
                                           SYNC_TIMING_OEM_IRQCTRLSTATUS_E    *pOemIRQCtrlStatus
                                          )
{
    SYNC_STATUS_E                         syncStatus         = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_CFG_DATA_T            *pOemCfg           = NULL;
    SYNC_TIMING_OEM_IRQCTRL_CFG_T         *pOemIRQCtrlCfg    = NULL;
    SYNC_TIMING_OEM_IRQCTRL_HANDLE_T      *pIRQCtrlHandle    = NULL;
    uint16_t                              uIrqIdx               = 0;

    do
    {
        if (!pOemData || !ppOemHandle || !pOemIRQCtrlStatus)
        {
            if (pOemIRQCtrlStatus)
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }
        
        *ppOemHandle = NULL;
        pOemCfg = (SYNC_TIMING_OEM_CFG_DATA_T *)pOemData;

        pOemIRQCtrlCfg = (SYNC_TIMING_OEM_IRQCTRL_CFG_T *)pOemCfg->pIRQCtrlCfg;
        if (NULL == pOemIRQCtrlCfg)
        {
            *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_INVALID_PARAMETER;
            syncStatus =  SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        if (pOemIRQCtrlCfg->uNumIrqs == 0)
        {
            *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_NOT_CONFIGURED;
            syncStatus =  SYNC_STATUS_NOT_READY;
            break;
        }

        pIRQCtrlHandle = 
            (SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)Sync_Timing_Internal_OEM_IRQCTRL_GetUnitResource(
                                                                                       timingDevId);

        if (pIRQCtrlHandle)
        {
        
            pIRQCtrlHandle->oemDeviceType = pOemCfg->oemDeviceType;

            for (uIrqIdx = 0; uIrqIdx < pOemIRQCtrlCfg->uNumIrqs; uIrqIdx++)
            {
                // Call IRQ Open call for all requested IRQ pins
                syncStatus = Sync_Timing_OemLinuxIrqdev_Open(pOemIRQCtrlCfg->irqInfo[uIrqIdx].oemIrqNum);
                if (syncStatus == SYNC_STATUS_SUCCESS)
                { 
                    // Initialize IRQ
                    syncStatus |= Sync_Timing_OemLinuxIrqdev_GlobalInit();

                    if (syncStatus == SYNC_STATUS_SUCCESS)
                    {
                        printf("irqTag = 0x%x, uIrqIdx = %u\n", pOemIRQCtrlCfg->irqInfo[uIrqIdx].irqTag, uIrqIdx);
                        
                        syncStatus |= Sync_Timing_OemLinuxIrqdev_Config(pOemIRQCtrlCfg->irqInfo[uIrqIdx].irqTag, 
                                                            pOemIRQCtrlCfg->irqInfo[uIrqIdx].oemIrqNum,
                                                            pOemIRQCtrlCfg->irqInfo[uIrqIdx].irqTriggerType,                                                        
                                                            pOemIrqCallbackFn);
                    }
                    else
                    {
                        syncStatus = SYNC_STATUS_FAILURE;
                        *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_ERROR;
                    }
                }
                else
                {
                    // Break out of the loop and return an error
                    syncStatus = SYNC_STATUS_FAILURE;
                    *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_ERROR;
                    break;
                }
            }

            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                Sync_Timing_OSAL_Wrapper_Memcpy(&(pIRQCtrlHandle->oemIRQCtrlCfg), pOemIRQCtrlCfg, 
                                                sizeof(SYNC_TIMING_OEM_IRQCTRL_CFG_T)); 
                
                // update connections status
                pIRQCtrlHandle->devCfgState = SYNC_TIMING_VALID;

                pIRQCtrlHandle->timingDevId = timingDevId;
                *ppOemHandle = (void *)pIRQCtrlHandle;
                if (pOemIRQCtrlStatus)
                {
                    *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_SUCCESS;
                }
            }
            else
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_ERROR;
                syncStatus = SYNC_STATUS_FAILURE;
                // Terminate IRQ
                // TODO
                
                // Call IRQ Close call for all requested IRQ pins
                // TODO
                Sync_Timing_Internal_OEM_IRQCtrl_FreeUnitResource(pIRQCtrlHandle);
            }
        }
        else
        {
            if (pOemIRQCtrlStatus)
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_NO_RESOURCE;
            }
            syncStatus = SYNC_STATUS_FAILURE;
        }
    } while(0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_IRQCtrl_Term
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used to terminate the reset ctrl device for the timing
 *                 chipset
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the reset ctrl device
 *
 * OUT PARAMS    : pOemIRQCtrlStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_IRQCtrl_Term(void                             *pOemHdl,
                                           SYNC_TIMING_OEM_IRQCTRLSTATUS_E  *pOemIRQCtrlStatus
                                          )
{
    SYNC_STATUS_E                       syncStatus      = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_IRQCTRL_HANDLE_T    *pIRQCtrlHandle = NULL;
    uint16_t                            uIrqIdx         = 0;

    do
    {
        if (!pOemHdl || !pOemIRQCtrlStatus)
        {
            if (pOemIRQCtrlStatus)
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pIRQCtrlHandle = (SYNC_TIMING_OEM_IRQCTRL_HANDLE_T*)pOemHdl;

        syncStatus = Sync_Timing_Internal_OEM_IRQCtrl_ValidateOemHandle(pIRQCtrlHandle);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_ERROR;

            if(pIRQCtrlHandle->devCfgState == SYNC_TIMING_VALID)
            {
                for (uIrqIdx = 0; uIrqIdx < pIRQCtrlHandle->oemIRQCtrlCfg.uNumIrqs; uIrqIdx++)
                {
                    // Disable IRQ
                    syncStatus = Sync_Timing_OemLinuxIrqdev_Config(
                                        pIRQCtrlHandle->oemIRQCtrlCfg.irqInfo[uIrqIdx].irqTag,
                                        pIRQCtrlHandle->oemIRQCtrlCfg.irqInfo[uIrqIdx].oemIrqNum,
                                        SYNC_TIMING_OEM_TRIG_DISABLE,
                                        NULL);
                    
                    // Call IRQ Close call for all requested IRQ pins
                    syncStatus |= Sync_Timing_OemLinuxIrqdev_Close(
                                        pIRQCtrlHandle->oemIRQCtrlCfg.irqInfo[uIrqIdx].oemIrqNum);
                    
                }
                syncStatus |= Sync_Timing_OemLinuxIrqdev_GlobalTerm();              
                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    syncStatus = Sync_Timing_Internal_OEM_IRQCtrl_FreeUnitResource(pIRQCtrlHandle);
                }
            }

            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_SUCCESS;
            }
            else
            {
                *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_ERROR;
            }
        }
        else
        {
            *pOemIRQCtrlStatus = SYNC_TIMING_IRQCTRL_INVALID_PARAMETER;
        }
    } while(0);

    return syncStatus;
}

