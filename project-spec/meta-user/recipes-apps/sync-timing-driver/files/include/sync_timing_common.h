/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *
 *  \file          sync_timing_common.h
 *
 *  \details       Header file for Sync Timing Driver Common Definitions
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

#ifndef _SYNC_TIMING_COMMON_H_
#define _SYNC_TIMING_COMMON_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_osal.h"
#include "sync_timing_config.h"
#if (SYNC_TIMING_CHIP_TYPE == SI5388)
#include "sync_timing_common_si5388.h"    
#elif (SYNC_TIMING_CHIP_TYPE == SI5348)
#include "sync_timing_common_si5348.h"
#elif (SYNC_TIMING_CHIP_TYPE == ARUBA)
#include "sync_timing_common_aruba.h"
#endif

/*****************************************************************************************
    Macros
*****************************************************************************************/

#if defined(SYNC_TIMING_HOST_BIG_ENDIAN)  //TBD
#define SYNC_TIMING_SWAP_L(i) (i)   //TBD
#define SYNC_TIMING_SWAP_I(i) (i)   //TBD
#define SYNC_TIMING_SWAP_S(s) (s)   //TBD
#else
#define SYNC_TIMING_SWAP_L(i) (i)
#define SYNC_TIMING_SWAP_I(i) (i)
#define SYNC_TIMING_SWAP_S(s) (s)
#endif

#define SYNC_TIMING_MAX_DRIVER_VERSION_STRING_SIZE          64
#define SYNC_TIMING_MAX_FW_VERSION_STRING_SIZE              64
#define SYNC_TIMING_MAX_FPLAN_VERSION_STRING_SIZE           64
#define SYNC_TIMING_MAX_CBPRO_VERSION_STRING_SIZE           64
#define SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE           256
#define SYNC_TIMING_MAX_BL_VERSION_STRING_SIZE              32
#define SYNC_TIMING_MAX_CHIPSET_VERSION_STRING_SIZE         16
#define SYNC_TIMING_MAX_FPLAN_DESIGN_ID_STRING_SIZE         16

#define SYNC_TIMING_INVALID_SERVO_ID                        UINT32_MAX

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/


/**
 *  \addtogroup common
 *  @{
 *  \addtogroup common_ds
 *  @{
 */
/*! Enum for various SYNC Timing Driver Return status */
typedef enum
{
    SYNC_STATUS_SUCCESS = 0,            //!< API call Succeeded 
    SYNC_STATUS_FAILURE,                //!< API call failed due to various reasons  
    SYNC_STATUS_ALREADY_INITIALIZED,    //!< API call returned since driver was already initialized
    SYNC_STATUS_INVALID_PARAMETER,      //!< API Call failed since Invalid parameters sent to the API
    SYNC_STATUS_NOT_SUPPORTED,          //!< API call not supported currently 
    SYNC_STATUS_NOT_READY,              //!< API call returned not ready status - typically returned when HW block is not ready
    SYNC_STATUS_NOT_INITIALIZED,        //!< API Call failed since driver was not initialized
    SYNC_STATUS_TIMEOUT,                //!< API Call Timed out
    SYNC_STATUS_NO_RESOURCES,           //!< API Call returned resource unavailability
    SYNC_STATUS_MAX

} SYNC_STATUS_E;

/*! Enum for various SYNC Timing Boolean Values */
typedef enum 
{
    SYNC_TIMING_FALSE = 0,     //!< False 
    SYNC_TIMING_TRUE  = 1,     //!< True 

    SYNC_TIMING_DISABLE = 0,   //!< Disable
    SYNC_TIMING_ENABLE  = 1,   //!< Enable

    SYNC_TIMING_INVALID = 0,   //!< Invalid
    SYNC_TIMING_VALID   = 1,   //!< Valid

    SYNC_TIMING_OFF     = 0,   //!< Off
    SYNC_TIMING_ON      = 1,   //!< On

    SYNC_TIMING_NO      = 0,   //!< No
    SYNC_TIMING_YES     = 1,   //!< Yes

    SYNC_TIMING_NOT_RUNNING     = 0,    //!< Not Running
    SYNC_TIMING_RUNNING         = 1,    //!< Running

    SYNC_TIMING_NOT_AVAILABLE   = 0,    //!< Not Available
    SYNC_TIMING_AVAILABLE       = 1,    //!< Available

} SYNC_TIMING_BOOL_E;

/*! Driver Event Callback Notify Function prototype
    driverEvent is of type SYNC_TIMING_DEVICE_DRIVER_EVENT_E
    eventPayload is of type SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T and is event specific 
    and chipset specific.
    deviceEventInfo is to be used for SYNC_TIMING_DEVICE_CHIP_EVENT
    ppsEventData is to be used for 1PPS_TS_AVAIL and 1PPS_TS_TIMEOUT events - ACCUTIME Support Only
    ptpclockEventInfo is to be used for PTP Clock events - ACCUTIME Support Only
 */
typedef uint32_t (*SYNC_TIMING_CALLBACK_FN_T)(uint8_t timingDevId, 
                                              uint32_t driverEvent, 
                                              void * eventPayload);

/*! Enumeration definition the various supported interfaces */
typedef enum
{
    SYNC_TIMING_CHIP_INTERFACE_SPI,      //!< Chip interface SPI
    SYNC_TIMING_CHIP_INTERFACE_I2C,      //!< Chip interface I2C
    SYNC_TIMING_CHIP_INTERFACE_INVALID,  //!< Invalid chip interface - used for range check

} SYNC_TIMING_CHIP_INTERFACE_E;

/*! Structure definition for providing the application information - for future use */
typedef struct
{
    char        AppName[SYNC_TIMING_CFG_MAX_APP_NAME_SZ];   
    //!< Application Name
    uint32_t    AppInstanceId;                              
    //!< Application instance ID. E.g. PID

} SYNC_TIMING_APPLN_INFO_T;

/*! Enumeration definition for the supported chip reset types */
typedef enum
{
    SYNC_TIMING_DEVICE_RESET_TOGGLE = 0xC0,      
    //!< Toggle and bring device out of reset in regular firmware mode - HARD RESET
    SYNC_TIMING_DEVICE_RESET_HOLD,               
    //!< Put device into reset - HARD RESET
    SYNC_TIMING_DEVICE_RESET_RELEASE,            
    //!< Release reset line and bring device out of reset in regular firmware mode - HARD RESET
    SYNC_TIMING_DEVICE_RESET_BOOTLOADER_MODE,    
    //!< Bring device out of reset but keep it in bootloader mode
    SYNC_TIMING_DEVICE_RESET_SOFT,
    //!< Toggle and bring device out of reset in regular firmware mode - SOFT RESET
    SYNC_TIMING_DEVICE_RESET_INVALID,
    //!< Invalid Reset Type - used only for range check; DONOT use in application

} SYNC_TIMING_DEVICE_RESET_TYPE_E;

/*! Structure definition for presenting the version info of driver, firmware and the bootloader */
typedef struct
{
    char  driverVersion[SYNC_TIMING_MAX_DRIVER_VERSION_STRING_SIZE]; 
    //!< Driver Version String
    char driverBuildInfo[SYNC_TIMING_MAX_FW_BUILD_INFO_STRING_SIZE];
    //!< Driver Build Info
    char  fwVersion[SYNC_TIMING_MAX_FW_VERSION_STRING_SIZE];         
    //!< Firmware Version String
    char  blVersion[SYNC_TIMING_MAX_BL_VERSION_STRING_SIZE];         
    //!< Bootloader Version String
    char  fplanVersion[SYNC_TIMING_MAX_FPLAN_VERSION_STRING_SIZE];   
    //!< Frequency Planner Version String (only for Si55xx chipsets)
    char  fplanDesignId[SYNC_TIMING_MAX_FPLAN_DESIGN_ID_STRING_SIZE];
    //!< Frequency Planner Design ID String (only for Si55xx chipsets)
    char  chipsetRevision[SYNC_TIMING_MAX_CHIPSET_VERSION_STRING_SIZE];
    //!< Chipset Revision (only for Si55xx chipsets)
    char  cbproVersion[SYNC_TIMING_MAX_CBPRO_VERSION_STRING_SIZE];
    //!< CBPro Version String (only for si55xx chipsets)

} SYNC_TIMING_DEVICE_VERSION_T;

/*! Structure definition for presenting the active timing device information */
typedef struct
{
    uint8_t  uNumActiveTimingDevices;
    //!< Number of active timing Devices on this system
    char     timingDeviceName[SYNC_TIMING_MAX_DEVICES][SYNC_TIMING_CFG_MAX_NAME_SZ];
    //!< Name of the timing device - will come through driver cfg file; 
    //!< E.g. "MainTimingCard", "BackupTimingCard", etc
    uint8_t  timingDeviceId[SYNC_TIMING_MAX_DEVICES];
    //!< A unique identifier that must be used by the application for performing any operation on 
    //!< the device
    SYNC_TIMING_BOOL_E          bTimingDeviceAccessible[SYNC_TIMING_MAX_DEVICES];
    //!< Indicates if Timing Device is accessible through the datapath */
    SYNC_TIMING_BOOL_E          bTimingDeviceResetAvailable[SYNC_TIMING_MAX_DEVICES];
    //!< Indicates if Timing Device reset is available */
    SYNC_TIMING_BOOL_E          bTimingDeviceInterruptsAvailable[SYNC_TIMING_MAX_DEVICES];
    //!< Indicates if Timing Device interrupts are available */    
} SYNC_TIMING_DEVICE_INFO_T;

/** @} common_ds */
/** @} common */

/**
 *  \addtogroup debug
 *  @{
 *  \addtogroup debug_ds
 *  @{
 */

/*! Enumeration definition for the supported device modes */
typedef enum
{
    SYNC_TIMING_DEVICE_MODE_APPLN = 0,
    //!< Device in application mode - run regular FW
    SYNC_TIMING_DEVICE_MODE_BOOTLOADER = 1,
    //!< Device in bootloader mode - accepts bootloader commands
    SYNC_TIMING_DEVICE_MODE_INVALID,
    //!< Invalid device mode - used only for range check; DONOT use in application

} SYNC_TIMING_DEVICE_MODE_E;


/** @} debug_ds */
/** @} debug */


/**
 *  \addtogroup common
 *  @{
 *  \addtogroup common_ds
 *  @{
 */


/*! Enumeration definition for the driver event types */
typedef enum
{
    SYNC_TIMING_DEVICE_CHIP_EVENT                   = 0x1 << 0, 
    //!< Device Chipset (Si5388 or Aruba) event - 0x1 
    SYNC_TIMING_DEVICE_EVENT_MAX                    = 0xFFFFFFFF
    //!< Invalid Event - used only for range check; DONOT use in application

} SYNC_TIMING_DEVICE_DRIVER_EVENT_E;


/*! Union of the driver event data - depending on the driver event the corresponding data will 
    need to be accessed */
typedef union
{
    SYNC_TIMING_DEVICE_EVENT_INFO_T         deviceEventInfo;
    //!< Device chipset event - Provided for SYNC_TIMING_DEVICE_CHIP_EVENT and is chipset specific
} SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T;
    
/*! Union of the driver event filters - depending on the driver event the corresponding filter will 
    need to be provided */
typedef union
{
    SYNC_TIMING_DEVICE_EVENT_INFO_T         deviceEventFilter;
    //!< Device chipset event filter being registered for - chipset specific filter
} SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T;

/** @} common_ds */
/** @} common */


/**
*  \addtogroup clockadj
*  @{
*  \addtogroup clockadj_ds
*  @{
*/

/*! Enumeration definition for the supported PLL IDs */
typedef enum
{
    SYNC_TIMING_CLOCKADJ_PLL_ID_A               = 0,
    //!< PLL A     
    SYNC_TIMING_CLOCKADJ_PLL_ID_B               = 1,
    //!< PLL B 
    SYNC_TIMING_CLOCKADJ_PLL_ID_C               = 2,
    //!< PLL C - Si538x chipsets only
    SYNC_TIMING_CLOCKADJ_PLL_ID_D               = 3,
    //!< PLL D - Si538x chipsets only
    SYNC_TIMING_CLOCKADJ_PLL_ID_R               = 4,
    //!< PLL R - Si55xx chipsets only
    SYNC_TIMING_CLOCKADJ_PLL_ID_INVALID,
    //!< Invalid PLL ID - used only for range check; DONOT use in application

} SYNC_TIMING_CLOCKADJ_PLL_ID_E;


/*! Enumeration definition for the supported PLL Inputs */
typedef enum
{
    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN0           = 0,
    //!< Use IN0 as input for PLL
    
    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN1           = 2,
    //!< Use IN1 as input for PLL

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2           = 4,
    //!< Use IN2 as input for PLL

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2B          = 5,
    //!< Use IN2B as input for PLL - Aruba chipset only

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3           = 6,
    //!< IN3 as input - Aruba chipset only

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3B          = 7,
    //!< Use IN3B as input for PLL - Aruba chipset only    

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_RESERVED      = 0xFE,
    //!< Reserved input -  DONOT use in application

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE          = 0xFF,
    //!< No SyncE input; Put PLL in holdover

    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_INVALID,
    //!< Invalid PLL Input - used only for range check; DONOT use in application

} SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E;

/*! Enumeration definition for the supported status IDs */
typedef enum
{
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_A               = 0,
    //!< PLL A Status    
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_B               = 1,
    //!< PLL B Status
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_C               = 2,
    //!< PLL C Status
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_D               = 3,
    //!< PLL D Status
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_R               = 4,
    //!< PLL R Status
    SYNC_TIMING_CLOCKADJ_STATUS_IN0                 = 10,
    //!< INPUT 0 Status
    SYNC_TIMING_CLOCKADJ_STATUS_IN1                 = 12,
    //!< INPUT 1 Status
    SYNC_TIMING_CLOCKADJ_STATUS_IN2                 = 14,
    //!< INPUT 2 Status
    SYNC_TIMING_CLOCKADJ_STATUS_IN2B                = 15,
    //!< INPUT 2B Status - Aruba chipset only
    SYNC_TIMING_CLOCKADJ_STATUS_IN3                 = 16,
    //!< INPUT 3 Status
    SYNC_TIMING_CLOCKADJ_STATUS_IN3B                = 17,
    //!< INPUT 3B Status - Aruba chipset only
    SYNC_TIMING_CLOCKADJ_STATUS_REF_INPUT           = 18,
    //!< Ref (XO/VCXO) Status - Aruba chipset only 
   
    SYNC_TIMING_CLOCKADJ_STATUS_INVALID,
    //!< Invalid Input - used only for range check; DONOT use in application

} SYNC_TIMING_CLOCKADJ_STATUS_ID_E;

/*! Enumeration definition for the supported status values */
typedef enum
{
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED                      = 0x1 << 0,
    //!< PLL Locked        
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_NOT_LOCKED                  = 0x1 << 1,    
    //!< PLL Not Locked    
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_HOLDOVER                    = 0x1 << 2,
    //!< PLL In Holdover
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_FREQ_ERR                    = 0x1 << 3,
    //!< PLL has frequency lock error
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_PHASE_ERR                   = 0x1 << 4,
    //!< PLL has phase lock error
    SYNC_TIMING_CLOCKADJ_STATUS_PLL_INIT_LOCK                   = 0x1 << 5,
    //!< PLL still attempting initial lock
    
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_HAS_SIGNAL                = 0x1 << 8,
    //!< INPUT Signal Good
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_NO_SIGNAL                 = 0x1 << 9,
    //!< INPUT Status Loss of Signal            
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_OOF                       = 0x1 << 10,
    //!< INPUT Out of Frequency
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_INVALID                   = 0x1 << 11,
    //!< INPUT is invalid - phase_err/lol/oof
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_PENDING_SHORT_TERM_FAULT  = 0x1 << 12,
    //!< Input is in a state pending short term fault
    SYNC_TIMING_CLOCKADJ_STATUS_INPUT_UNDER_VALIDATION          = 0x1 << 13,
    //!< Input is uner validation

} SYNC_TIMING_CLOCKADJ_STATUS_E;

/** @} clockadj_ds */
/** @} clockadj */


/**
*  \addtogroup clockctrl
*  @{
*  \addtogroup clockctrl_ds
*  @{
*/



/** @} clockctrl_ds */
/** @} clockctrl */
/** @} api */

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
    \cond header_prototypes  Defined so the function prototypes from header files don't
    duplicate the entries from the 'c' files.
*****************************************************************************************/

/** \endcond */

#endif // _SYNC_TIMING_COMMON_H_

