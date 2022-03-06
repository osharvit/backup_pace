/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_driver_version.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 08/07/2018
 *
 * DESCRIPTION        : Core Timing Driver definitions for the version number
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
 
#ifndef _SYNC_TIMING_CORE_DRIVER_VERSION_H_
#define _SYNC_TIMING_CORE_DRIVER_VERSION_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"

/*****************************************************************************************
    Macros
****************************************************************************************/
#ifndef SYNC_TIMING_DRIVER_VERSION_CHIP_TYPE
#define SYNC_TIMING_DRIVER_VERSION_CHIP_TYPE 3
#endif

#ifndef SYNC_TIMING_DRIVER_VERSION_MAJOR
#define SYNC_TIMING_DRIVER_VERSION_MAJOR 2
#endif

#ifndef SYNC_TIMING_DRIVER_VERSION_MINOR
#define SYNC_TIMING_DRIVER_VERSION_MINOR 4
#endif

#ifndef SYNC_TIMING_DRIVER_VERSION_BUILD_TYPE
#define SYNC_TIMING_DRIVER_VERSION_BUILD_TYPE 15
#endif

#ifndef SYNC_TIMING_DRIVER_VERSION_BUILD_NUM
#define SYNC_TIMING_DRIVER_VERSION_BUILD_NUM 0
#endif

#ifndef SYNC_TIMING_DRIVER_VERSION_BUILD_INFO
#define SYNC_TIMING_DRIVER_VERSION_BUILD_INFO "GIT_REPO: https://stash.sync.com/projects/SI5388FW/repos/sync_timing_driver/browse"
#endif

#ifndef SYNC_TIMING_MIN_FW_MAJOR_VERSION
#define SYNC_TIMING_MIN_FW_MAJOR_VERSION 0
#endif

#ifndef SYNC_TIMING_MIN_FW_MINOR_VERSION
#define SYNC_TIMING_MIN_FW_MINOR_VERSION 10
#endif

#ifndef SYNC_TIMING_MIN_FW_BUILD_NUM
#define SYNC_TIMING_MIN_FW_BUILD_NUM 4733
#endif

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/


#endif //_SYNC_TIMING_CORE_DRIVER_VERSION_H_

