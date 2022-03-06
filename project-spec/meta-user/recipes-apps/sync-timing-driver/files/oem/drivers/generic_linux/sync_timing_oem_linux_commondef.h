/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_commondef.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Common definitions for low-level Linux driver calls
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


#ifndef _SYNC_TIMING_OEM_LINUX_COMMONDEF_H_
#define _SYNC_TIMING_OEM_LINUX_COMMONDEF_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_config.h"
#include "sync_timing_common.h"
#include "sync_timing_osal.h"
#include "sync_timing_log.h"

/******************************************************************************
    Macros
******************************************************************************/
#define DBG_OUT 0
#define INFO_OUT 1

#define  DEBUG_PRINT(...) do\
{ \
if (DBG_OUT) SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  ERROR_PRINT(...) do\
{ \
SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  ALWAYS_PRINT(...) do\
{ \
SYNC_TIMING_ALWAYS(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  INFO_PRINT(...) do\
{ \
if (INFO_OUT) SYNC_TIMING_INFO3(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define SYNC_TIMING_OEM_LINUX_MAX_DEVICE_NAME_SZ 64

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

// IRQ types
typedef enum
{
   SYNC_TIMING_OEM_TRIG_DISABLE = 0,
   SYNC_TIMING_OEM_TRIG_RISING_EDGE,
   SYNC_TIMING_OEM_TRIG_FALLING_EDGE,
   SYNC_TIMING_OEM_TRIG_CHANGE_STATE,
   SYNC_TIMING_OEM_TRIG_TYPE_MAX,

} SYNC_TIMING_OEM_TRIG_TYPE_ID_E;

#endif //_SYNC_TIMING_OEM_LINUX_COMMONDEF_H_


