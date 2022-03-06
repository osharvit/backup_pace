/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_driver_app.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/07/2020
 *
 * DESCRIPTION        : Sync Timing Driver Application header
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

#ifndef _SYNC_TIMING_DRIVER_APP_H_
#define _SYNC_TIMING_DRIVER_APP_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_osal.h"
#include "sync_timing_log.h"
#include "sync_timing_driver.h"
#include "sync_timing_config.h"

#if (SYNC_TIMING_CHIP_TYPE == ARUBA)
#include "sync_timing_aruba_cmd.h"
#include "sync_timing_aruba_cmd_map.h"
#endif

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define SYNC_TIMING_DRIVERAPP_ERRCHECK(actual, expected)  \
    if ((actual) != (expected))  \
    { \
      printf("%s:%d, Expected = %d with Actual retval=%d\n", __FUNCTION__, __LINE__, (expected),(actual)); \
      printf("\n**************************************************************************\n"); \
      printf("\nNNNNNNNNNNNNNNNNNNNNNNNN SNIFF TEST FAILED NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN\n"); \
      printf("\n**************************************************************************\n"); \
    }

#define SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE 256

#define SYNC_TIMING_DRIVER_APP_MAX_LINE_LENGTH 256


/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/


/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

SYNC_STATUS_E   Sync_Timing_DriverApp_HandleDriverAPICmd(char *prog);

SYNC_STATUS_E Sync_Timing_DriverApp_ProcessDriverAPICmd(char *inputCmd);

SYNC_STATUS_E   Sync_Timing_DriverApp_HandleFWAPICmd(char *prog);

SYNC_STATUS_E Sync_Timing_DriverApp_ProcessFWAPICmd(char *inputCmd);

int32_t Sync_Timing_DriverApp_Is_Number(char *str, uint32_t *base);

SYNC_STATUS_E   Sync_Timing_DriverApp_ProcessScript(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

#endif //_SYNC_TIMING_DRIVER_APP_H_
