/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_irq.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/11/2018
 *
 * DESCRIPTION        : Low Level IRQ Device driver Implementation for Linux OS
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

#ifndef _SYNC_TIMING_OEM_LINUX_IRQ_H_
#define _SYNC_TIMING_OEM_LINUX_IRQ_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include <sys/types.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

#include "sync_timing_oem_driver.h"
#include "sync_timing_oem_linux_commondef.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/ 

#define INVALID_HANDLER                    -1
#define SYSFS_IRQ_PATH                    "/sys/class/gpio/"
#define MAX_BUFF_SIZE                      81

// number of available IO
#define SYNC_TIMING_MAX_IRQ_SUPPORTED          1
#define SYNC_TIMING_IRQ_POLL_TIMEOUT          1000

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/
/* sturucture for OEM device specific configration */
typedef struct
{
    uint16_t                          irqPinNum;
    uint32_t                          irqIdTag;
    uint32_t                          irqCount;
    uint8_t                           irqValue;
    SYNC_TIMING_OEM_TRIG_TYPE_ID_E    irqTrigType;
    pthread_mutex_t                   irqIOlock;
    int32_t                           irqFd;
    SYNC_TIMING_OEM_IRQ_CALLBACK      irqCallback;
} SYNC_TIMING_IRQ_DATA_T;

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_GlobalInit();
int32_t Sync_Timing_OemLinuxIrqdev_GlobalTerm();
int32_t Sync_Timing_OemLinuxIrqdev_Open(uint16_t irqPinNum);
int32_t Sync_Timing_OemLinuxIrqdev_Close(uint16_t irqPinNum);
int32_t Sync_Timing_OemLinuxIrqdev_Term();
int32_t Sync_Timing_OemLinuxIrqdev_Config(uint32_t irqIdTag,
                                          uint16_t irqPinNum,              
                                          SYNC_TIMING_OEM_TRIG_TYPE_ID_E irqTrigType, 
                                          SYNC_TIMING_OEM_IRQ_CALLBACK pOemIrqCallbackFn);

#endif //_SYNC_TIMING_OEM_LINUX_IRQ_H_

