/****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_reset.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Source code for Host/Chipset OEM CHIP RESET interface.
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

#include "sync_timing_oem_linux_gpio.h"

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/
//2-dimensional array of pointers to OEM handlers
static void*  gpOem_ResetCtrl_Handle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES]; 

//2-dimensional array of Reset Ctrl handles
static SYNC_TIMING_OEM_RESETCTRL_HANDLE_T   gOemResetCtrlHandle[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES] ; 

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_ResetCtrl_ValidateOemHandle
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
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
SYNC_STATUS_E Sync_Timing_Internal_OEM_ResetCtrl_ValidateOemHandle( void* pOemHandle )
{
    uint8_t                              timingDevId          = 0, 
                                         resetCtrlDevId          = 0;
    SYNC_STATUS_E                        syncStatus           = SYNC_STATUS_INVALID_PARAMETER;
    SYNC_TIMING_OEM_RESETCTRL_HANDLE_T   *pOemResetCtrlHandle = (SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)pOemHandle;

    if(pOemResetCtrlHandle)
    {
        timingDevId = pOemResetCtrlHandle->timingDevId;
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(resetCtrlDevId < SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES)
            {
                if(&gOemResetCtrlHandle[timingDevId][resetCtrlDevId] == pOemResetCtrlHandle)
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
 * FUNCTION NAME : Sync_Timing_Internal_OEM_RESETCTRL_GetUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
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
void* Sync_Timing_Internal_OEM_RESETCTRL_GetUnitResource( uint8_t timingDevId )
{
    void*   curResetCtrlHdl       = NULL;
    uint8_t resetCtrlDevId        = 0;
    
    if(timingDevId < SYNC_TIMING_MAX_DEVICES)
    {
        for (resetCtrlDevId = 0; resetCtrlDevId < SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES; resetCtrlDevId++)
        {
            if(gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId] != NULL)
                break;
        }
        
        if (resetCtrlDevId >= SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES)
        {
            resetCtrlDevId = 0; // 1 device only, else return error
        }
        
        // valid devId
        if(gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId] == NULL)
        {
            gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId] =  &gOemResetCtrlHandle[timingDevId][resetCtrlDevId];
            if(gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId])
            {
                Sync_Timing_OSAL_Wrapper_Memset(gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId], 
                                                0, sizeof(SYNC_TIMING_OEM_RESETCTRL_HANDLE_T));

                if(((SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId]))
                {         
                    ((SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId])->timingDevId = timingDevId;
                }
                else
                {
                    gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId] = NULL;
                }
            }
        }
        ((SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId])->timingDevId = timingDevId;
        curResetCtrlHdl = gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId];
    }
    
    return curResetCtrlHdl;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OEM_ResetCtrl_FreeUnitResource
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
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
SYNC_STATUS_E Sync_Timing_Internal_OEM_ResetCtrl_FreeUnitResource( void* pOemHandle )
{
    uint8_t                             timingDevId          = 0,
                                        resetCtrlDevId       = 0;
    SYNC_TIMING_OEM_RESETCTRL_HANDLE_T  *pOemResetCtrlHandle = NULL;
    SYNC_STATUS_E                       syncStatus           = SYNC_STATUS_INVALID_PARAMETER;

    if(pOemHandle)
    {
        pOemResetCtrlHandle = (SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)pOemHandle;
        timingDevId   = pOemResetCtrlHandle->timingDevId;
        if(timingDevId < SYNC_TIMING_MAX_DEVICES)
        { 
            // valid timingDevId
            while(resetCtrlDevId < SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES)
            {
                if(&gOemResetCtrlHandle[timingDevId][resetCtrlDevId] == pOemHandle)
                {
                    //printf("%s:%d\n", __FUNCTION__, __LINE__);
                    pOemResetCtrlHandle = NULL;
                    gpOem_ResetCtrl_Handle[timingDevId][resetCtrlDevId] = NULL;
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
 * FUNCTION NAME : Sync_Timing_OEM_ResetCtrl_Init
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to initialize the reset ctrl device for the timing
 *                 chipset
 *
 * IN PARAMS     : timingDevId   - The Timing device Id
 *               : pOemData      - The OEM Data required for reset control device
 *
 * OUT PARAMS    : ppOemHandle   - The OEM handle for future API calls
 *               : pOemResetCtrlStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Init(uint8_t                          timingDevId, 
                                           void                               *pOemData, 
                                           void                               **ppOemHandle,
                                           SYNC_TIMING_OEM_RESETCTRLSTATUS_E  *pOemResetCtrlStatus
                                          )
{
    SYNC_STATUS_E                           syncStatus         = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_CFG_DATA_T              *pOemCfg           = NULL;
    SYNC_TIMING_OEM_RESETCTRL_CFG_T         *pOemResetCtrlCfg  = NULL;
    SYNC_TIMING_OEM_RESETCTRL_HANDLE_T      *pResetCtrlHandle  = NULL;

    do
    {
        if (!pOemData || !ppOemHandle || !pOemResetCtrlStatus)
        {
            if (pOemResetCtrlStatus)
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }
        
        *ppOemHandle = NULL;
        pOemCfg = (SYNC_TIMING_OEM_CFG_DATA_T *)pOemData;

        pOemResetCtrlCfg = (SYNC_TIMING_OEM_RESETCTRL_CFG_T *)pOemCfg->pResetCtrlCfg;
        if (NULL == pOemResetCtrlCfg)
        {
            *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
            syncStatus =  SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pResetCtrlHandle = (SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)Sync_Timing_Internal_OEM_RESETCTRL_GetUnitResource(timingDevId);
        if (pResetCtrlHandle)
        {
        
            pResetCtrlHandle->oemDeviceType = pOemCfg->oemDeviceType;

            syncStatus = Sync_Timing_OemLinuxGpiodev_Open(pOemResetCtrlCfg->oemResetCtrlRSTNum);
            if (pOemResetCtrlCfg->bUseBLMDNum)
            {
                syncStatus |= Sync_Timing_OemLinuxGpiodev_Open(pOemResetCtrlCfg->oemResetCtrlBLMDNum);
            }

            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                Sync_Timing_OSAL_Wrapper_Memcpy(&(pResetCtrlHandle->resetCtrlCfg), pOemResetCtrlCfg, 
                                                sizeof(SYNC_TIMING_OEM_RESETCTRL_CFG_T)); 
                
                // update connections status
                pResetCtrlHandle->devCfgState = SYNC_TIMING_VALID;

                pResetCtrlHandle->timingDevId = timingDevId;
                *ppOemHandle = (void *)pResetCtrlHandle;
                if (pOemResetCtrlStatus)
                {
                    *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_SUCCESS;
                }
            }
            else
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_ERROR;
                syncStatus = Sync_Timing_OemLinuxGpiodev_Close(pOemResetCtrlCfg->oemResetCtrlRSTNum);
                if (pOemResetCtrlCfg->bUseBLMDNum)
                {
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_Close(pOemResetCtrlCfg->oemResetCtrlBLMDNum);
                }
                Sync_Timing_Internal_OEM_ResetCtrl_FreeUnitResource(pResetCtrlHandle);
            }
        }
        else
        {
            // TBD: should be resource exceed!!
            if (pOemResetCtrlStatus)
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_NO_RESOURCE;
            }
            syncStatus = SYNC_STATUS_FAILURE;
        }
    } while(0);
    
    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_ResetCtrl_Activate
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to activate the reset ctrl device for the timing
 *                 chipset
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the reset ctrl device
 *               : resetType      - The type of reset desired
 *
 * OUT PARAMS    : pOemResetCtrlStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Activate(void             *pOemHdl,
                               SYNC_TIMING_OEM_RESETCTRL_RESETTYPE_E  resetType,   
                               SYNC_TIMING_OEM_RESETCTRLSTATUS_E      *pOemResetCtrlStatus
                                                    )
{
    SYNC_STATUS_E syncStatus = SYNC_STATUS_FAILURE;
    SYNC_TIMING_OEM_RESETCTRL_HANDLE_T *pResetCtrlHandle = NULL;

    do
    {
        if (!pOemHdl || !pOemResetCtrlStatus )
        {
            if (pOemResetCtrlStatus)
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pResetCtrlHandle = (SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)pOemHdl;

        syncStatus = Sync_Timing_Internal_OEM_ResetCtrl_ValidateOemHandle(pResetCtrlHandle);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_ERROR;

            syncStatus |= Sync_Timing_OemLinuxGpiodev_Config(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 
                                                             SYNC_TIMING_GPIO_DIR_OUT, 
                                                             SYNC_TIMING_OEM_TRIG_DISABLE,
                                                             NULL);
            if (pResetCtrlHandle->resetCtrlCfg.bUseBLMDNum)
            {
                syncStatus |= Sync_Timing_OemLinuxGpiodev_Config(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlBLMDNum, 
                                                             SYNC_TIMING_GPIO_DIR_OUT, 
                                                             SYNC_TIMING_OEM_TRIG_DISABLE,
                                                             NULL);
            }

            switch(resetType)
            {
                case SYNC_TIMING_OEM_RESETCTRL_RESET_TOGGLE:
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 0);
                    Sync_Timing_OSAL_Wrapper_SleepMS(1000);
                    if (pResetCtrlHandle->resetCtrlCfg.bUseBLMDNum)
                    {
                        syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlBLMDNum, 1);
                    }
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 1);
                    Sync_Timing_OSAL_Wrapper_SleepMS(1000);
                    break;

                case SYNC_TIMING_OEM_RESETCTRL_RESET_HOLD:
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 0);
                    break;

                case SYNC_TIMING_OEM_RESETCTRL_RESET_RELEASE:
                    if (pResetCtrlHandle->resetCtrlCfg.bUseBLMDNum)
                    {
                        syncStatus = Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlBLMDNum, 1);
                    }
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 1);
                    break;

                case SYNC_TIMING_OEM_RESETCTRL_RESET_BOOTLOADER_MODE:
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 0);
                    Sync_Timing_OSAL_Wrapper_SleepMS(1000);
                    if (pResetCtrlHandle->resetCtrlCfg.bUseBLMDNum)
                    {
                        syncStatus = Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlBLMDNum, 0);
                    }
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_SetVal(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum, 1);
                    Sync_Timing_OSAL_Wrapper_SleepMS(1000);
                    break;

                default:
                    syncStatus = SYNC_STATUS_FAILURE;
                    break;
            }
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_SUCCESS;
            }
        }
        else
        {
            *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
        }
    } while(0);

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OEM_ResetCtrl_Term
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to terminate the reset ctrl device for the timing
 *                 chipset
 *
 * IN PARAMS     : pOemHdl        - The Oem Handle of the reset ctrl device
 *
 * OUT PARAMS    : pOemResetCtrlStatus - More detailed status of the API
 *
 * RETURN VALUE  :  SYNC_STATUS_SUCCESS or SYNC_STATUS_INVALID_PARAMETER
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Term(void               *pOemHdl,
                                 SYNC_TIMING_OEM_RESETCTRLSTATUS_E  *pOemResetCtrlStatus
                                                )
{
    SYNC_STATUS_E syncStatus = SYNC_STATUS_FAILURE;

    SYNC_TIMING_OEM_RESETCTRL_HANDLE_T *pResetCtrlHandle = NULL;

    do
    {
        if (!pOemHdl || !pOemResetCtrlStatus)
        {
            //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
            if (pOemResetCtrlStatus)
            {
                //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
            }
            syncStatus = SYNC_STATUS_INVALID_PARAMETER;
            break;
        }

        pResetCtrlHandle = (SYNC_TIMING_OEM_RESETCTRL_HANDLE_T*)pOemHdl;

        syncStatus = Sync_Timing_Internal_OEM_ResetCtrl_ValidateOemHandle(pResetCtrlHandle);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_ERROR;

            if(pResetCtrlHandle->devCfgState == SYNC_TIMING_VALID)
            {
                syncStatus = Sync_Timing_OemLinuxGpiodev_Close(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlRSTNum);
                if (pResetCtrlHandle->resetCtrlCfg.bUseBLMDNum)
                {
                    syncStatus |= Sync_Timing_OemLinuxGpiodev_Close(pResetCtrlHandle->resetCtrlCfg.oemResetCtrlBLMDNum);
                }
                //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);

                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    syncStatus = Sync_Timing_Internal_OEM_ResetCtrl_FreeUnitResource(pResetCtrlHandle);
                    //printf("%s:%d:%u\n", __FUNCTION__, __LINE__, syncStatus);
                }
            }

            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_SUCCESS;
            }         
        }
        else
        {
            *pOemResetCtrlStatus = SYNC_TIMING_RESETCTRL_INVALID_PARAMETER;
        }
    } while(0);

    return syncStatus;
}


