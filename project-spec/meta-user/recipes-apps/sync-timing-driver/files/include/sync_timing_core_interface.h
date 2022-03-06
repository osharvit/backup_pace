/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_core_interface.h
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Defines the common communication interface between the API layer 
 *                      and the Core Driver
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

#ifndef _SYNC_TIMING_CORE_INTERFACE_H_
#define _SYNC_TIMING_CORE_INTERFACE_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_common.h"
#include "sync_timing_oem_driver.h"
#include "sync_timing_log.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define SYNC_TIMING_CORE_MAX_MSG_DATA_SIZE 768
#define SYNC_TIMING_CORE_MAX_MSG_SIZE 8192  
#define SYNC_TIMING_CORE_MAX_MSG_MEM_RW_DATA_SIZE 512
#define SYNC_TIMING_CORE_MSG_CMD_STRING_SIZE 64

#define SYNC_TIMING_1PPS_OUT_FEEDBACK_IDX   0
#define SYNC_TIMING_GPS_1PPS_IDX   1

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/
/* Message Type Enumeration */
typedef enum
{
    SYNC_TIMING_CORE_MSG_TYPE_REQ = 0x10,  /* Request message from client */
    SYNC_TIMING_CORE_MSG_TYPE_RESP,        /* Response messages to client */
    SYNC_TIMING_CORE_MSG_TYPE_EVENT,       /* Event notification message from core to client */

} SYNC_TIMING_CORE_MSG_TYPE_E;

/* Message Commands that the core can receive from applications/clients */
typedef enum
{
    SYNC_TIMING_CORE_MSG_INIT                     = 0x100,
    SYNC_TIMING_CORE_MSG_TERM,
    SYNC_TIMING_CORE_MSG_REG_EVENTS,
    SYNC_TIMING_CORE_MSG_DEVICE_RESET             = 0x110,
    SYNC_TIMING_CORE_MSG_DEVICE_LOCK,
    SYNC_TIMING_CORE_MSG_DEVICE_UNLOCK,
    SYNC_TIMING_CORE_MSG_DEVICE_UPDATE,
    SYNC_TIMING_CORE_MSG_DEVICE_VERSION,
    SYNC_TIMING_CORE_MSG_DRIVER_EVENT,
    SYNC_TIMING_CORE_MSG_GET_GLOBAL_LOG_INFO,
    SYNC_TIMING_CORE_MSG_CHANGE_MODULE_LOG_CFG,
    SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD,
    SYNC_TIMING_CORE_MSG_READ_DIRECT              = 0x120,
    SYNC_TIMING_CORE_MSG_WRITE_DIRECT,
    SYNC_TIMING_CORE_MSG_READ_INDIRECT,
    SYNC_TIMING_CORE_MSG_WRITE_INDIRECT,
    SYNC_TIMING_CORE_MSG_API_COMMAND,
    SYNC_TIMING_CORE_MSG_CLKADJ_GET_PLLINPUT      = 0x130,
    SYNC_TIMING_CORE_MSG_CLKADJ_SET_PLLINPUT,
    SYNC_TIMING_CORE_MSG_CLKADJ_GET_STATUS,
    SYNC_TIMING_CORE_MSG_CLKCTRL_INIT             = 0x140,
    SYNC_TIMING_CORE_MSG_CLKCTRL_GETINFO,
    SYNC_TIMING_CORE_MSG_CLKCTRL_SETFREQ,
    SYNC_TIMING_CORE_MSG_CLKCTRL_GETFREQ,
    SYNC_TIMING_CORE_MSG_CLKCTRL_STEP,
    SYNC_TIMING_CORE_MSG_CLKCTRL_SETTIME,
    SYNC_TIMING_CORE_MSG_CLKCTRL_GETTIME,
    SYNC_TIMING_CORE_MSG_CLKCTRL_ACTIVATE_1PPSTS,
    SYNC_TIMING_CORE_MSG_CLKCTRL_TRANSMIT_TOD     = 0x150,       
    SYNC_TIMING_CORE_MSG_CLKCTRL_ACTIVATE_GPSTODREAD,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_RESET          = 0x160,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_INIT_SERVO,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_REINIT_SERVO,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_SET_MODE,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_SET_UNITPARAM,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_GET_UNITPARAM,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_SET_TSPAIR,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_GET_PTPSTATS,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_GET_ERRSTATS,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_SET_PPSINFO,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_GET_METRICS,
    SYNC_TIMING_CORE_MSG_SERVOCTRL_TERM_SERVO,
    SYNC_TIMING_CORE_MSG_DBG_DEVICE_SET_MODE      = 0x200,
    SYNC_TIMING_CORE_MSG_DBG_DEVICE_GET_MODE,
    SYNC_TIMING_CORE_MSG_DBG_TRANSFER_TO_BL,
    SYNC_TIMING_CORE_MSG_DBG_GET_BOOTFILE_INFO,
    SYNC_TIMING_CORE_MSG_DBG_VERIFY_FLASH_SEG,

} SYNC_TIMING_CORE_MSG_CMD_E;

/* Structure to map enumeration to string */
typedef struct
{
    SYNC_TIMING_CORE_MSG_CMD_E              msgCmd;
    char                                    msgCmdString[SYNC_TIMING_CORE_MSG_CMD_STRING_SIZE];
} SYNC_TIMING_CORE_MSG_CMD_STRING_T;

/* Message header */
typedef struct
{
    uint32_t                                uClientAppId; /* Unique identifier assigned to each Client */
    uint32_t                                uTimingDevId; /* Timing Device Id */
  
    SYNC_TIMING_CORE_MSG_TYPE_E             coreMsgType;  /* Message Type - see enumeration above */
    uint32_t                                coreMsgCmd;   /* Message Command - see enum above */
                                                    
    SYNC_TIMING_BOOL_E                      bRespReqd;    
    /* Application set field for SYNC_TIMING_CORE_MSG_TYPE_REQ */
    SYNC_STATUS_E                           reqStatus;    
    /* Status of the request as executed by the core; Client can communicate this back
       to the application code */

} SYNC_TIMING_CORE_MSG_HDR_T;

/* Message structure for init */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_APPLN_INFO_T                clientInfo;
    char                                    clientRecvMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    char                                    clientSendMsgQName[SYNC_TIMING_CFG_MAX_NAME_SZ];
    SYNC_TIMING_DEVICE_INFO_T               activeDeviceInfo;  // Returned back by the core driver

} SYNC_TIMING_CORE_MSG_INIT_T;

/* Message structure for term */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_APPLN_INFO_T                clientInfo;

} SYNC_TIMING_CORE_MSG_TERM_T;

/* Message structure for Registering Callback events */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T                      msgHdr;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_E               driverEvent;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T        driverEventFilter;

} SYNC_TIMING_CORE_MSG_REG_EVENT_T;

/* Message structure for lock/unlock */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    uint32_t                                uPlaceHolder;

} SYNC_TIMING_CORE_MSG_GEN_CTRL_T;

/* Message structure for device reset */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_DEVICE_RESET_TYPE_E         ResetType;

} SYNC_TIMING_CORE_MSG_DEVICE_RESET_T;

/* Message structure for device update */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    char                                    pBootFile[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ];

} SYNC_TIMING_CORE_MSG_DEVICE_UPDATE_T;

/* Message structure for device update */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    char pBootFileList[SYNC_TIMING_MAX_DEVICE_DOWNLOAD_BOOTFILES][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ];
    uint32_t                                uNumBootFiles;   

} SYNC_TIMING_CORE_MSG_DEVICE_DOWNLOAD_T;

/* Message structure for version info */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_DEVICE_VERSION_T            deviceVersionInfo;

} SYNC_TIMING_CORE_MSG_DEVICE_VERSION_T;

typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T                  msgHdr;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_E           event;
    SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T      eventData;

} SYNC_TIMING_CORE_MSG_DRIVER_EVENT_T;


/* Message structure read/writing memory - direct or indirect */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    uint16_t                                memAddr;
    uint32_t                                len;
    uint8_t                                 data[SYNC_TIMING_CORE_MAX_MSG_MEM_RW_DATA_SIZE];

} SYNC_TIMING_CORE_MSG_MEM_ACCESS_T;

/* Message structure sending command to PLL chipset FW */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    uint32_t                                uCmdLen;
    uint32_t                                uExpCmdRespLen;
    uint8_t                                 uCmdStatus;
    uint8_t                                 command[SYNC_TIMING_CORE_MAX_MSG_MEM_RW_DATA_SIZE];
    uint8_t                                 cmdResponse[SYNC_TIMING_CORE_MAX_MSG_MEM_RW_DATA_SIZE];

} SYNC_TIMING_CORE_MSG_API_CMD_T;

/* Message structure for Set/Get Current Mode */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_DEVICE_MODE_E               deviceMode;
} SYNC_TIMING_CORE_MSG_DEBUG_MODE_T;



/* Message structure for setting synce input */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_CLOCKADJ_PLL_ID_E           uPLLId;
    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInputSelect;
    
} SYNC_TIMING_CORE_MSG_CLKADJ_PLLINPUT_T;

/* Message structure for getting the pll or input status */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_CLOCKADJ_STATUS_ID_E        statusType;
    SYNC_TIMING_CLOCKADJ_STATUS_E           status;
    
} SYNC_TIMING_CORE_MSG_CLKADJ_STATUS_T;



/* Message structure for device mode set/get */
typedef struct
{
    SYNC_TIMING_CORE_MSG_HDR_T              msgHdr;
    SYNC_TIMING_DEVICE_MODE_E               DeviceMode;

} SYNC_TIMING_CORE_MSG_DEVICE_MODE_T;


/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

extern void *pSyncTimingCoreLogHandle;

/*****************************************************************************************
    Prototypes
 ****************************************************************************************/

#endif //_SYNC_TIMING_CORE_INTERFACE_H_

