/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_common.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/26/2018
 *
 * DESCRIPTION        : Common header file for various OEM wrappers; Sync reference
 *                      implementation; 
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

#ifndef _SYNC_TIMING_OEM_COMMON_H_
#define _SYNC_TIMING_OEM_COMMON_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/
#include "sync_timing_oem_driver.h"

#include "sync_timing_oem_linux_commondef.h"

/**
 *  \defgroup porting SYNC TIMING DRIVER PORTING GUIDE
 *  @{
 *  \defgroup chip_if Hardware Interface Porting
 *  @{
 *  \defgroup chip_if_api Hardware Interface Data Structures and Functions 
 *  @{
 *
 */

/**
*  \defgroup chip_OEM_Common_if_api COMMON
*  @brief OEM COMMON Interface Functions - These are Sync Reference implementation;
*                                          OEM can customize and implement for their hardware
*  @{
*/

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*! Macro defining maximum IRQs supported by the OEM driver  - Sync reference implementation */
#define SYNC_TIMING_OEM_MAX_IRQS 1

/*! Macro defining the maximum size of a device name - Sync reference implementation */
#define SYNC_TIMING_OEM_MAX_DEVICE_NAME_SZ 64

/*! Macro defining maximum LEDs supported by the OEM driver  - Sync reference implementation */
#define SYNC_TIMING_OEM_MAX_LEDS 7

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

/*! Enum defining possible OEM datapaths  - Sync reference implementation */
typedef enum
{
    SYNC_TIMING_OEM_DATAPATH_SPI,
    SYNC_TIMING_OEM_DATAPATH_I2C,
    SYNC_TIMING_OEM_DATAPATH_INVALID,

} SYNC_TIMING_OEM_DATAPATH_E;

 /*! Enum defining possible OEM devices  - Sync reference implementation - Future use*/
typedef enum
{
    SYNC_TIMING_OEM_DEVICE_REF_EVB = 1,
    SYNC_TIMING_OEM_DEVICE_INVALID,

} SYNC_TIMING_OEM_DEVICE_E;

/*! OEM all connection specific configration stucture - Sync reference implementation */
typedef struct
{
    SYNC_TIMING_OEM_DEVICE_E                oemDeviceType;   
    //!< Oem Device Type - Can be used for hw identification
    SYNC_TIMING_OEM_DATAPATH_E              OemDataPath;
    //!< Oem Data Path Type 

    union {
        void                                *pSpiCfg;
        //!< Device SPI Configuration e.g., of type SYNC_TIMING_OEM_SPI_CFG_T*
        void                                *pI2cCfg;
        //!< Device I2C Configuration e.g., of type SYNC_TIMING_OEM_I2C_CFG_T*
    } dataPathCfg;

    void                                    *pIRQCtrlCfg;
    //!< Device IRQ Configuration e.g., of type SYNC_TIMING_OEM_IRQCTRL_CFG_T*
    void                                    *pResetCtrlCfg;
    //!< Device RESET CONTROLLER Configuration e.g., of type SYNC_TIMING_OEM_RESETCTRL_CFG_T*

} SYNC_TIMING_OEM_CFG_DATA_T;

/*! Structure Specifying OEM SPI Configuration - Sync reference implementation */
typedef struct
{
    int8_t                                  spiDevName[SYNC_TIMING_OEM_MAX_DEVICE_NAME_SZ];
    //!< SPI device name
    uint8_t                                 spiDevId;        
    //!< SPI Device ID - Linux specific
    uint32_t                                spiMode;         
    //!< SPI Mode 0-3
    uint32_t                                spiSpeed;        
    //!< SPI Speed in Hz
    uint32_t                                spiBitsPerWord;  
    //!< SPI BITS per word

} SYNC_TIMING_OEM_SPI_CFG_T;

/*! Structure Specifying OEM I2C Configuration - Sync reference implementation */
typedef struct
{
    int8_t                                  i2cDevName[SYNC_TIMING_OEM_MAX_DEVICE_NAME_SZ];
    //!< I2C device name
    uint8_t                                 i2cDevId;
    //!< I2C Device ID - Linux specific
    uint8_t                                 i2cDevAddr;
    //!< I2C device address

} SYNC_TIMING_OEM_I2C_CFG_T;

/*! Oem RESETCTRL Configuration structure - Sync reference implementation*/
typedef struct
{
    uint16_t                                oemResetCtrlRSTNum;  
    //!< OEM RESET GPIO Pin - Linux GPIO number for Sync reference implementation
    uint16_t                                oemResetCtrlBLMDNum;  
    //!< OEM Boot Loader Mode GPIO Pin - Linux GPIO number for Sync reference implementation
    SYNC_TIMING_BOOL_E                      bUseBLMDNum;
    //!< Use of this PIN is required or not;

} SYNC_TIMING_OEM_RESETCTRL_CFG_T;


/*! Oem IRQ Cfg Information structure - Sync reference implementation*/
typedef struct
{
    uint32_t                                irqTag;
    /*! Tag or Identification for the IRQ triggered - returned through the Callback notification to the
        core driver from the OEM IRQ layer - for sync reference implementation this will be a 
        combination of the TimingDevId and the irqId */
    uint16_t                                oemIrqNum;
    //!< OEM Specific IRQ IO PIN number - Linux GPIO number for Sync reference implementation
    SYNC_TIMING_OEM_TRIG_TYPE_ID_E          irqTriggerType;
    //!< IRQ Trigger Type - Rising or Falling or change or disable
} SYNC_TIMING_OEM_IRQ_INFO_T;

/*! Oem IRQ Control Configuration structure - Sync reference implementation*/
typedef struct
{
    SYNC_TIMING_OEM_IRQ_INFO_T              irqInfo[SYNC_TIMING_OEM_MAX_IRQS];
    //!< Config Information for IRQs being configured
    uint16_t                                uNumIrqs;
    //!< Number of IRQs being configured
    
} SYNC_TIMING_OEM_IRQCTRL_CFG_T;




/** @} chip_OEM_Common_if_api */

/** @} chip_if_api */
/** @} chip_if */
/** @} porting */

/* Structure specifying the OEM SPI device handle 
   - pointer to a instance returned to the caller as Datapath handle 
   - Sync reference implementation*/
typedef struct
{
    uint8_t                                 timingDevId;      
    // Device ID with which this context is stored
    SYNC_TIMING_OEM_DATAPATH_E              timingDataPath;   
    // Device data path
    SYNC_TIMING_OEM_DEVICE_E                oemDeviceType;    
    // OEM Device Type - just one for now - future use
    uint32_t                                devCfgState;      
    // Device datapath configuration state
    uint32_t                                numDataAccesses;  
    // Total data Accesses made
    uint32_t                                numDataBytes;     
    // Total number of bytes transfered using the data interface

    union {
        SYNC_TIMING_OEM_SPI_CFG_T           spiCfg;
        SYNC_TIMING_OEM_I2C_CFG_T           i2cCfg;
    } dataPathCfg;
  
} SYNC_TIMING_OEM_DATAPATH_HANDLE_T;

/* Structure specifying the OEM RESET_CTRL device handle 
   - pointer to a instance returned to the caller as Reset ctrl handle 
   - Sync reference implementation*/
typedef struct
{
    uint8_t                                 timingDevId; 
    // Device ID with which this context is stored
    uint32_t                                devCfgState;
    SYNC_TIMING_OEM_DEVICE_E                oemDeviceType;
    // OEM Device Type - just one for now - future use
    SYNC_TIMING_OEM_RESETCTRL_CFG_T         resetCtrlCfg;

} SYNC_TIMING_OEM_RESETCTRL_HANDLE_T;


/* Structure specifying the OEM IRQ CTRL device handle 
   - pointer to a instance returned to the caller as IRQ Ctrl handle 
   - Sync reference implementation*/
typedef struct
{
    uint8_t                                 timingDevId;    
    // Device ID with which this context is stored
    uint32_t                                devCfgState;
    SYNC_TIMING_OEM_DEVICE_E                oemDeviceType;  
    // OEM Device Type - just one for now - future use
    SYNC_TIMING_OEM_IRQCTRL_CFG_T           oemIRQCtrlCfg;

} SYNC_TIMING_OEM_IRQCTRL_HANDLE_T;




/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/

/** \endcond */

#endif  //_SYNC_TIMING_OEM_COMMON_H_

