/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_i2cdev.h
 *
 * AUTHOR             : Ramprasath Pitta Sekar
 *
 * DATE CREATED       : 06/09/2021
 *
 * DESCRIPTION        : Low Level I2C Device driver header file for Linux OS
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

#ifndef _SYNC_TIMING_OEM_LINUX_I2CDEV_H_
#define _SYNC_TIMING_OEM_LINUX_I2CDEV_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include <stdio.h>
#include "sync_timing_oem_linux_commondef.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_OEM_I2CDEV_MAX_DEVS 1

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

typedef struct
{
    uint32_t bInUse;
    uint8_t  i2cDevId;
    char     i2cDevice[SYNC_TIMING_OEM_LINUX_MAX_DEVICE_NAME_SZ];
    int32_t  i2cFd;
    uint8_t  i2cDevAddr;

} SYNC_TIMING_OEMLINUX_I2C_DEVICES_T;

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

int32_t Sync_Timing_OemLinuxI2cdev_Open(int8_t* i2cDevName, uint8_t i2cDevId,
                                                        uint8_t i2cDevAddr);
int32_t Sync_Timing_OemLinuxI2cdev_Transfer(uint8_t i2cdevID, uint16_t length, uint8_t *tx,
                                            uint8_t *rxbuf, uint16_t readLength);
int32_t Sync_Timing_OemLinuxI2cdev_Close(uint8_t i2cdevID);

#endif //_SYNC_TIMING_OEM_LINUX_I2CDEV_H_
