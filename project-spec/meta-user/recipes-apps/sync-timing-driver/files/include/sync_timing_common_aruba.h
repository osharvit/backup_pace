/*!***************************************************************************************
 *  \defgroup common_ds_aruba  Control Data Structures (Aruba specific)
 *   @brief    Control Data Structures available for the Aruba Timing Chipset. 
 *  \addtogroup api
 *  @{
 *
 *  \file          sync_timing_common_aruba.h
 *
 *  \details       Header file for Sync Timing Driver Common Definitions which are Aruba specific
 *
 *  \date          Created: 05/18/2020
 *
 ****************************************************************************************/

/****************************************************************************************/
/**                  Copyright (c) 2020, 2021 Skyworks Solution Inc.                   **/
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

#ifndef _SYNC_TIMING_COMMON_ARUBA_H_
#define _SYNC_TIMING_COMMON_ARUBA_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_osal.h"
#include "sync_timing_config.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/

/**
 *  \addtogroup common
 *  @{
 *  \addtogroup common_ds
 *  @{
 *  \addtogroup common_ds_aruba
 *  @{
 */

/*! Enumeration definition for the supported device INPUT events on Aruba Chipset */
typedef enum
{
    SYNC_TIMING_DEVICE_EVENT_ROS_IN0                = 0x1 << 0,
    //!< Recovery of Signal on IN0
    SYNC_TIMING_DEVICE_EVENT_ROS_IN1                = 0x1 << 2,
    //!< Recovery of Signal on IN1
    SYNC_TIMING_DEVICE_EVENT_ROS_IN2                = 0x1 << 4,
    //!< Recovery of Signal on IN2
    SYNC_TIMING_DEVICE_EVENT_ROS_IN2B               = 0x1 << 5,
    //!< Recovery of Signal on IN2B
    SYNC_TIMING_DEVICE_EVENT_ROS_IN3                = 0x1 << 6,
    //!< Recovery of Signal on IN3
    SYNC_TIMING_DEVICE_EVENT_ROS_IN3B               = 0x1 << 7,
    //!< Recovery of Signal on IN3B
       
    SYNC_TIMING_DEVICE_EVENT_LOS_IN0                = 0x1 << 8,
    //!< Loss of Signal on IN0
    SYNC_TIMING_DEVICE_EVENT_LOS_IN1                = 0x1 << 10,
    //!< Loss of Signal on IN1
    SYNC_TIMING_DEVICE_EVENT_LOS_IN2                = 0x1 << 12,
    //!< Loss of Signal on IN2
    SYNC_TIMING_DEVICE_EVENT_LOS_IN2B               = 0x1 << 13,
    //!< Loss of Signal on IN2B   
    SYNC_TIMING_DEVICE_EVENT_LOS_IN3                = 0x1 << 14,
    //!< Loss of Signal on IN3
    SYNC_TIMING_DEVICE_EVENT_LOS_IN3B               = 0x1 << 15,
    //!< Loss of Signal on IN3B
    
    SYNC_TIMING_DEVICE_EVENT_OOF_IN0                = 0x1 << 16,
    //!< Out of Frequency on IN0
    SYNC_TIMING_DEVICE_EVENT_OOF_IN1                = 0x1 << 18,
    //!< Out of Frequency on IN1
    SYNC_TIMING_DEVICE_EVENT_OOF_IN2                = 0x1 << 20,
    //!< Out of Frequency on IN2
    SYNC_TIMING_DEVICE_EVENT_OOF_IN2B               = 0x1 << 21,
    //!< Out of Frequency on IN2B    
    SYNC_TIMING_DEVICE_EVENT_OOF_IN3                = 0x1 << 22,
    //!< Out of Frequency on IN3
    SYNC_TIMING_DEVICE_EVENT_OOF_IN3B               = 0x1 << 23,
    //!< Out of Frequency on IN3B

    SYNC_TIMING_DEVICE_EVENT_INVALID_IN0            = 0x1 << 24,
    //!< Phase Error on IN0
    SYNC_TIMING_DEVICE_EVENT_INVALID_IN1            = 0x1 << 26,
    //!< Phase Error on IN1
    SYNC_TIMING_DEVICE_EVENT_INVALID_IN2            = 0x1 << 28,
    //!< Phase Error on IN2
    SYNC_TIMING_DEVICE_EVENT_INVALID_IN2B           = 0x1 << 29,
    //!< Phase Error on IN2B    
    SYNC_TIMING_DEVICE_EVENT_INVALID_IN3            = 0x1 << 30,
    //!< Phase Error on IN3
    SYNC_TIMING_DEVICE_EVENT_INVALID_IN3B           = 0x1 << 31,
    //!< Phase Error on IN3B

} SYNC_TIMING_DEVICE_EVENT_INPUT_E;

/*! Enumeration definition for the supported device PLL events on Aruba Chipset */
typedef enum
{
    SYNC_TIMING_DEVICE_EVENT_LOL_PLL_R              = 0x1 << 0,
    //!< Loss of Lock on PLL R
    SYNC_TIMING_DEVICE_EVENT_ROL_PLL_R              = 0x1 << 1,
    //!< Recovery of Lock on PLL R
    SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_R       = 0x1 << 2,
    //!< Cycle Slip occurred on PLL R
    SYNC_TIMING_DEVICE_EVENT_HO_PLL_R               = 0x1 << 3,
    //!< PLL R in Holdover 
    SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_R   = 0x1 << 4,
    //!< Hitless Switch happened on PLL R 
    SYNC_TIMING_DEVICE_EVENT_OOF_PLL_R              = 0x1 << 5,
    //!< Out Of Frequency detected on PLL R
    SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_R             = 0x1 << 6,
    //!< Out Of Phase detected on PLL R
    
    SYNC_TIMING_DEVICE_EVENT_LOL_PLL_A              = 0x1 << 8,
    //!< Loss of Lock on PLL A
    SYNC_TIMING_DEVICE_EVENT_ROL_PLL_A              = 0x1 << 9,
    //!< Recovery of Lock on PLL A    
    SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_A       = 0x1 << 10,
    //!< Cycle Slip occurred on PLL A
    SYNC_TIMING_DEVICE_EVENT_HO_PLL_A               = 0x1 << 11,
    //!< PLL A in Holdover 
    SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_A   = 0x1 << 12,
    //!< Hitless Switch happened on PLL A
    SYNC_TIMING_DEVICE_EVENT_OOF_PLL_A              = 0x1 << 13,
    //!< Out Of Frequency detected on PLL A
    SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_A             = 0x1 << 14,
    //!< Out Of Phase detected on PLL A
    
    SYNC_TIMING_DEVICE_EVENT_LOL_PLL_B              = 0x1 << 16,
    //!< Loss of Lock on PLL B
    SYNC_TIMING_DEVICE_EVENT_ROL_PLL_B              = 0x1 << 17,
    //!< Recovery of Lock on PLL B    
    SYNC_TIMING_DEVICE_EVENT_CYCLE_SLIP_PLL_B       = 0x1 << 18,
    //!< Cycle Slip occurred on PLL B
    SYNC_TIMING_DEVICE_EVENT_HO_PLL_B               = 0x1 << 19,
    //!< PLL B in Holdover 
    SYNC_TIMING_DEVICE_EVENT_HITLESS_SWITCH_PLL_B   = 0x1 << 20,
    //!< Hitless Switch happened on PLL B 
    SYNC_TIMING_DEVICE_EVENT_OOF_PLL_B              = 0x1 << 21,
    //!< Out Of Frequency detected on PLL B
    SYNC_TIMING_DEVICE_EVENT_OOPH_PLL_B             = 0x1 << 22,
    //!< Out Of Phase detected on PLL B

} SYNC_TIMING_DEVICE_EVENT_PLL_E;

/*! Enumeration definition for the supported device general events on Aruba Chipset */
typedef enum
{
    SYNC_TIMING_DEVICE_EVENT_CAL_SYS                = 0x1 << 0,
    //!< System in Calibration Event
    SYNC_TIMING_DEVICE_EVENT_IL_LOL                 = 0x1 << 1,
    //!< Inner Loop Loss of Lock
    SYNC_TIMING_DEVICE_EVENT_IL_LOREF               = 0x1 << 2,
    //!< Inner Loop Loss of Reference
    SYNC_TIMING_DEVICE_EVENT_SYSTEM_FAULT           = 0x1 << 3,
    //!< System Fault detected
    SYNC_TIMING_DEVICE_EVENT_WATCHDOG_TIMEOUT       = 0x1 << 4,
    //!< Watchdog timeout error
    SYNC_TIMING_DEVICE_EVENT_I2C_SMBUS_TIMEOUT      = 0x1 << 5
    //! I2C or SMBUS Timeout occurred

} SYNC_TIMING_DEVICE_EVENT_GENERAL_E;

/*! Structure definition for the supported device specific events for Aruba chipset.
    Please see header file source sync_timing_common_aruba.h */
typedef struct
{
    SYNC_TIMING_DEVICE_EVENT_INPUT_E                deviceInputEvents;
    SYNC_TIMING_DEVICE_EVENT_PLL_E                  devicePllEvents;
    SYNC_TIMING_DEVICE_EVENT_GENERAL_E              deviceGenEvents;
    
} SYNC_TIMING_DEVICE_EVENT_INFO_T;

/** @} common_ds_aruba */
/** @} common_ds */
/** @} common */
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

#endif // _SYNC_TIMING_COMMON_ARUBA_H_

