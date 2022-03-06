/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_spidev.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/26/2018
 *
 * DESCRIPTION        : Low Level SPI Device driver header file for Linux OS
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

#ifndef _SYNC_TIMING_OEM_LINUX_SPIDEV_H_
#define _SYNC_TIMING_OEM_LINUX_SPIDEV_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 

#include "sync_timing_oem_linux_commondef.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define MAX_DEV_NM_SZ 64

// max data size restricted by spidev buffer size
#define SYNC_TIMING_OEM_SPIDEV_MAX_MSG_BUF    4096

#define SYNC_TIMING_OEM_SPIDEV_MAX_DEVS 1

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

typedef struct
{
    uint32_t bInUse;
    uint8_t  spidevID;
    char     spiDevice[SYNC_TIMING_OEM_LINUX_MAX_DEVICE_NAME_SZ];
    int32_t  spiFd;
    uint32_t spiSpeed;
    uint8_t  spiBitsPerWord;
    uint8_t  spiMode;
    uint16_t delay;
    
} SYNC_TIMING_OEMLINUX_SPI_DEVICES_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

int32_t Sync_Timing_OemLinuxSpidev_Open(int8_t *pSpiDevName, uint8_t spidevID, uint32_t spiSpeed, 
                                    uint8_t bitsPerWord, uint8_t spiMode, 
                                    uint8_t lsbFirst);
int32_t Sync_Timing_OemLinuxSpidev_Close(uint8_t spidevID);
int32_t Sync_Timing_OemLinuxSpidev_Transfer(uint8_t spidevID, uint16_t length, 
                                            uint8_t * tx, uint8_t *rx, uint16_t readLength);

#endif //_SYNC_TIMING_OEM_LINUX_SPIDEV_H_

