/****************************************************************************************/
/**
 *  \defgroup porting SYNC TIMING DRIVER PORTING GUIDE
 *  @brief     This section defines the various Host Platform Porting APIs for the 
 *             Sync Timing Chipset.
 *  @{
 *  \defgroup chip_if Hardware Interface Porting
 *  @brief    This section defines the various Chip and HW Interface APIs for SYNC Timing Chipset
 *            It is expected that the OEM will implement these functions for their 
 *            development platform. Sync will provide a generic linux based reference 
 *            implementation.
 *  @{
 *  \defgroup chip_if_api Hardware Interface Data Structures and Functions 
 *  @{
 *
 *  \file          sync_timing_oem_driver.h
 *
 *  \details       This file contains OEM associated functions and data structures.
 *
 *  \date          Created: 06/27/2018
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

#ifndef _SYNC_TIMING_OEM_DRIVER_H_
#define _SYNC_TIMING_OEM_DRIVER_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/
#include "sync_timing_common.h"

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

#ifdef _DOXYGEN_
#endif

/**
 *  \defgroup chip_DATAPATH_if_api OEM DATAPATH Interface Controller
 *  @brief DATA PATH Interface Functions - OEM to implement for their hardware
 *  @{
 */

/*! Enumeration for OEM DATA PATH layer detailed status - OEM can enhance this if necessary */
typedef enum
{
    SYNC_TIMING_DATAPATH_SUCCESS = 0x00,      //!< DATAPATH layer returned SUCCESS
    SYNC_TIMING_DATAPATH_INVALID_PARAMETER,   //!< DATAPATH layer returned INVALID_PARAMETER
    SYNC_TIMING_DATAPATH_NOT_CONFIGURED,      //!< DATAPATH layer returned NOT_CONFIGURED
    SYNC_TIMING_DATAPATH_NOT_SUPPORTED,       //!< DATAPATH layer returned NOT_SUPPORTED 
    SYNC_TIMING_DATAPATH_NO_RESOURCE,         //!< DATAPATH layer returned NO_RESOURCE
    SYNC_TIMING_DATAPATH_ERROR   = 0x10,      //!< DATAPATH layer returned ERROR
} SYNC_TIMING_OEM_DATAPATH_STATUS_E;

/** @} chip_DATAPATH_if_api */

/**
 *  \defgroup chip_RESET_if_api Device RESET controller 
 *  @brief RESET Control Interface Functions - OEM to implement for their hardware
 *  @{
 */

/*! Enumeration for OEM RESETCTRL layer detailed status - OEM can enhance this if necessary */
typedef enum
{
    SYNC_TIMING_RESETCTRL_SUCCESS = 0x00,      //!< RESETCTRL layer returned SUCCESS
    SYNC_TIMING_RESETCTRL_INVALID_PARAMETER,   //!< RESETCTRL layer returned INVALID_PARAMETER
    SYNC_TIMING_RESETCTRL_NOT_CONFIGURED,      //!< RESETCTRL layer returned NOT_CONFIGURED
    SYNC_TIMING_RESETCTRL_NOT_SUPPORTED,       //!< RESETCTRL layer returned NOT_SUPPORTED  
    SYNC_TIMING_RESETCTRL_NO_RESOURCE,         //!< RESETCTRL layer returned NO_RESOURCE
    SYNC_TIMING_RESETCTRL_ERROR   = 0x10,      //!< RESETCTRL layer returned ERROR
} SYNC_TIMING_OEM_RESETCTRLSTATUS_E;

/*! Enumeration for OEM RESETCTRL type - OEM can enhance this if necessary */
typedef enum
{
    SYNC_TIMING_OEM_RESETCTRL_RESET_TOGGLE = 0xC0,      //!< RESETCTRL toggle reset pins
    SYNC_TIMING_OEM_RESETCTRL_RESET_HOLD,               //!< RESETCTRL hold reset pins
    SYNC_TIMING_OEM_RESETCTRL_RESET_RELEASE,            //!< RESETCTRL release reset pins
    SYNC_TIMING_OEM_RESETCTRL_RESET_BOOTLOADER_MODE,    //!< RESETCTRL enter boot loader mode
} SYNC_TIMING_OEM_RESETCTRL_RESETTYPE_E;

/** @} chip_RESET_if_api */

/**
 *  \defgroup chip_GPIO_if_api GPIO controller 
 *  @brief GPIO Interface Functions - OEM to implement for their hardware
 *  @{
 */

/*! Enumeration for OEM GPIO layer detailed status - OEM can enhance this if necessary */
typedef enum
{
    SYNC_TIMING_GPIO_SUCCESS = 0x00,      //!< GPIO layer returned SUCCESS
    SYNC_TIMING_GPIO_INVALID_PARAMETER,   //!< GPIO layer returned INVALID_PARAMETER
    SYNC_TIMING_GPIO_NOT_CONFIGURED,      //!< GPIO layer returned NOT_CONFIGURED
    SYNC_TIMING_GPIO_NOT_SUPPORTED,       //!< GPIO layer returned NOT_SUPPORTED  
    SYNC_TIMING_GPIO_NO_RESOURCE,         //!< GPIO layer returned NO_RESOURCE
    SYNC_TIMING_GPIO_ERROR   = 0x10,      //!< GPIO layer returned ERROR
} SYNC_TIMING_OEM_GPIOSTATUS_E;

/*! IRQ Callback function prototype - arguments include callback type and callback payload */
typedef SYNC_STATUS_E (*SYNC_TIMING_OEM_GPIO_CALLBACK)(uint8_t timingDevId, uint32_t gpioId);

/** @} chip_GPIO_if_api */


/**
 *  \defgroup chip_IRQCTRL_if_api IRQ controller
 *  @brief IRQCTRL Interface Functions - OEM to implement for their hardware
 *  @{
 */

/*! Enumeration for OEM GPIO layer detailed status - OEM can enhance this if necessary */
typedef enum
{
    SYNC_TIMING_IRQCTRL_SUCCESS = 0x00,      //!< IRQCTRL layer returned SUCCESS
    SYNC_TIMING_IRQCTRL_INVALID_PARAMETER,   //!< IRQCTRL layer returned INVALID_PARAMETER
    SYNC_TIMING_IRQCTRL_NOT_CONFIGURED,      //!< IRQCTRL layer returned NOT_CONFIGURED
    SYNC_TIMING_IRQCTRL_NOT_SUPPORTED,       //!< IRQCTRL layer returned NOT_SUPPORTED  
    SYNC_TIMING_IRQCTRL_NO_RESOURCE,         //!< IRQCTRL layer returned NO_RESOURCE
    SYNC_TIMING_IRQCTRL_ERROR   = 0x10,      //!< IRQCTRL layer returned ERROR
} SYNC_TIMING_OEM_IRQCTRLSTATUS_E;

/*! IRQ Callback function prototype - arguments include callback type and callback payload */
typedef SYNC_STATUS_E (*SYNC_TIMING_OEM_IRQ_CALLBACK)(uint32_t irqTag);

/** @} chip_IRQCTRL_if_api */





/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
*****************************************************************************************/


/**
 *  \defgroup chip_DATAPATH_if_api OEM DATAPATH Interface Controller
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_DATAPATH_Init
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/31/2018
 *//**
 * 
 * \brief           This function will initialize the DataPath link to SoC. Depending on the 
 *                  hardware being used, this function will result in the DataPath device
 *                  being opened on the sending side (assuming Linux OS). There is no 
 *                  communication with the Chipset as yet.
 *
 * \param[in]       timingDevId     The Timing Device ID 
 *                                  (assuming there can be multiple timing devices on board)
 * \param[in]       pOemData        OEM specific data, passed transparently through the Driver 
 * \param[out]      ppOemHdl        Used to return the OEM context - passed for subsequent calls
 * \param[out]      pStatus         Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Init(uint8_t                            timingDevId, 
                                            void                               *pOemData, 
                                            void                               **ppOemHdl,
                                            SYNC_TIMING_OEM_DATAPATH_STATUS_E  *pStatus
                                           );

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_DATAPATH_Transfer
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/31/2018
 *//**
 * 
 * \brief           This function reads and writes a byte stream of length bufSize from 
 *                  and to the device. From pSendMsg, this function writes a byte stream 
 *                  of length bufSize to the device. From the device, this function reads 
 *                  a byte stream of length bufSize into pRecvMsg
 *
 * \param[in]       pOemHdl      OEM context - as returned in Init
 * \param[in]       pSendMsg     Pointer to memory containing the message to be sent
 * \param[in]       bufSize      Transfer size
 * \param[in]       readLength   Rx buffer size; 0 for write operation
 * \param[out]      pRecvMsg     Buffer (of size bufSize) to store the received data
 * \param[out]      pStatus      Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Transfer(void                              *pOemHdl,
                                                uint8_t                           *pSendMsg,
                                                uint16_t                          bufSize,
                                                uint8_t                           *pRecvMsg,
                                                uint16_t                          readLength,
                                                SYNC_TIMING_OEM_DATAPATH_STATUS_E *pStatus
                                               );

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_DATAPATH_Term
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/31/2018
 *//**
 * 
 * \brief           This function will terminate the DATAPATH link to SoC. Depending on the 
 *                  system this could be closing the DATAPATH device handle on the sending
 *                  side.
 *
 * \param[in]       pOemHdl   OEM context - as returned in Init
 * \param[out]      pStatus   Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_DATAPATH_Term(void                               *pOemHdl,
                                            SYNC_TIMING_OEM_DATAPATH_STATUS_E  *pStatus
                                           );

/** @} chip_DATAPATH_if_api */



/**
 *  \defgroup chip_RESET_if_api Device RESET controller 
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_ResetCtrl_Init
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function will initialize the reset Control link to chip. 
 *
 * \param[in]       devId     The SyncTiming Device ID
 * \param[in]       pOemData  OEM specific data, passed by the core
 * \param[out]      ppOemHdl  Used to return the OEM context - passed for subsequent calls
 * \param[out]      pStatus   Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Init(uint8_t                            devId,
                                             void                               *pOemData,
                                             void                               **ppOemHdl,
                                             SYNC_TIMING_OEM_RESETCTRLSTATUS_E  *pStatus
                                            );

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_ResetCtrl_Term
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function will terminate the resetController link to chip. 
 *
 * \param[in]       pOemHdl   OEM Handle - as returned in Init
 * \param[out]      pStatus   Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Term(void                               *pOemHdl, 
                                             SYNC_TIMING_OEM_RESETCTRLSTATUS_E  *pStatus
                                            );

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_ResetCtrl_Activate
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function will activate the resetController link to timing chipset 
 *                  depending on the resetType.
 *
 * \param[in]       pOemHdl       OEM Reset Controller driver handle.
 * \param[in]       resetType     reset operation of type SYNC_TIMING_OEM_RESETCTRL_RESETTYPE_E
 *                                
 * \param[out]      pStatus       Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_ResetCtrl_Activate(void                                   *pOemHdl,
                                                 SYNC_TIMING_OEM_RESETCTRL_RESETTYPE_E  resetType,
                                                 SYNC_TIMING_OEM_RESETCTRLSTATUS_E      *pStatus
                                                );

/** @} chip_RESET_if_api */



/**
 *  \defgroup chip_IRQCTRL_if_api IRQ controller 
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_IRQCtrl_Init
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     12/12/2018
 *//**
 * 
 * \brief           This function will initialize the IRQ Control link to chip. 
 *
 * \param[in]       timingDevId       The SyncTiming Device ID
 * \param[in]       pOemData          OEM specific data, passed by the core
 * \param[in]       pOemIrqCallbackFn Core Driver Callback function to invoke when IRQ is triggered
 * \param[out]      ppOemHandle       Used to return the OEM context - passed for subsequent calls
 * \param[out]      pOemIRQCtrlStatus Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_IRQCtrl_Init(uint8_t                            timingDevId, 
                                           void                               *pOemData, 
                                           SYNC_TIMING_OEM_IRQ_CALLBACK       pOemIrqCallbackFn,
                                           void                               **ppOemHandle,
                                           SYNC_TIMING_OEM_IRQCTRLSTATUS_E    *pOemIRQCtrlStatus
                                          );

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OEM_IRQCtrl_Term
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     12/12/2018
 *//**
 * 
 * \brief           This function will terminate the IRQ Control link to chip. 
 *
 * \param[in]       pOemHdl   OEM IRQ Controller Handle - as returned in Init
 * \param[out]      pStatus   Pointer to storage to return the operation status
 *
 * \returns         SYNC_STATUS_SUCCESS, SYNC_STATUS_FAILURE
 *
 * ___
 ***************************************************************************************/
SYNC_STATUS_E Sync_Timing_OEM_IRQCtrl_Term(void                               *pOemHdl, 
                                           SYNC_TIMING_OEM_IRQCTRLSTATUS_E    *pStatus
                                          );

/** @} chip_IRQCTRL_if_api */






/** @} chip_if_api */
/** @} chip_if */
/** @} porting */

#endif // _SYNC_TIMING_OEM_DRIVER_H_

