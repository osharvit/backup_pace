/*!***************************************************************************************
 *  \addtogroup api
 *  @{
 *  \addtogroup common
 *  @{
 *  \addtogroup common_ds
 *  @{
 *
 *  \file          sync_timing_config.h
 *
 *  \details       Header file for defining common configuration information
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

#ifndef _SYNC_TIMING_CONFIG_H_
#define _SYNC_TIMING_CONFIG_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*! Defines maximum number of unique client applications supported by the core driver */
#define SYNC_TIMING_MAX_APPLICATIONS                1

/*! Defines the total number of client application instances (for all application put together) 
    that can be registered with the core driver at a given time */
#define SYNC_TIMING_MAX_APPLICATION_INSTANCES       1

/*! Defines maximum number of timing devices on the system */
#define SYNC_TIMING_MAX_DEVICES                     2

/*! Defines maximum number of OEM DATAPATH CTRL devices per Timing Device */
#define SYNC_TIMING_MAX_OEM_DATAPATH_DEVICES        1

/*! Defines maximum number of OEM RESET CTRL devices per Timing Device */
#define SYNC_TIMING_MAX_OEM_RESETCTRL_DEVICES       1


/*! Defines maximum number of OEM IRQ CTRL devices per Timing Device */
#define SYNC_TIMING_MAX_OEM_IRQCTRL_DEVICES         1




/*! Defines maximum SPI Speed in Hertz when SPI is used for datapath 
    - common for all datapath devices - varies from chipset to chipset 
    - refer to sync_timing_driver.conf for chipset specific recommended values */
#define SYNC_TIMING_MAX_SPI_SPEED                   50000000

/*! Defines maximum SPI Bits per word when SPI is used for datapath 
    - common for all datapath devices*/
#define SYNC_TIMING_MAX_SPI_BPW                     8

/*! Defines maximum SPI HDR size when SPI is used for datapath 
    - common for all datapath devices*/
#define SYNC_TIMING_MAX_SPI_HDR_SIZE                4

/*! Defines maximum number of bytes that can be transferred using SPI for read/write registers
    when SPI is used for datapath - common for all datapath devices*/
#define SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE      32

/*! Defines maximum number of command data bytes that can be transferred using SPI when SPI is used for datapath 
    - common for all datapath devices*/
#define SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE      255

/*! Defines maximum number of bytes that can be transferred using I2C for read/write registers
    when I2C is used for datapath - common for all datapath devices*/

#define SYNC_TIMING_MAX_I2C_DATA_TRANSFER_SIZE      16

/*! Defines maximum I2C HDR size when I2C is used for datapath
    - common for all datapath I2C devices*/
#define SYNC_TIMING_MAX_I2C_HDR_SIZE                4

/*! Defines the no of I2C error bytes that proceeds the MCU Status during read reply */
#define SYNC_TIMING_I2C_CMD_READ_REPLY_ERROR_SIZE   2


/*! Defines maximum number of device download bootfiles */
#define SYNC_TIMING_MAX_DEVICE_DOWNLOAD_BOOTFILES   4

/*! Defines maximum size of the device update file name */
#define SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ  256

/*! Defines maximum size of the any file name (LOG, CFG, etc) */
#define SYNC_TIMING_MAX_FILE_NAME_SZ                64

/*! Defines maximum number of unique log modules supported */
#define SYNC_TIMING_MAX_GLOBAL_LOG_MODULES          SYNC_TIMING_MAX_APPLICATION_INSTANCES + 1

/*! Defines maximum number of unique log modules supported */
#define SYNC_TIMING_MAX_LOCAL_LOG_INSTANCES         1

/*! Defines maximum size for any name (MsgQ, Application, etc) */
#define SYNC_TIMING_CFG_MAX_NAME_SZ                 256

/*! Defines maximum size for any application name */
#define SYNC_TIMING_CFG_MAX_APP_NAME_SZ             128

/*! Defines maximum size for log formatting */
#define SYNC_TIMING_MAX_LOG_FORMAT_SZ               128

/*! Defines an application ID base */
#define SYNC_TIMING_APPLN_ID_BASE                   1000

#define SYNC_TIMING_HARDWARE_PRESENT                1



#define SYNC_TIMING_LOG_THREAD_MAX_LOG_MSG_SIZE     864

#define SYNC_TIMING_MIN_LOG_ROTATE_FILES            1

#define SYNC_TIMING_MAX_LOG_ROTATE_FILES            50

#define SYNC_TIMING_MIN_LOG_FILE_SIZE               200*1024

#define SYNC_TIMING_MAX_LOG_FILE_SIZE               20*1024*1024

#define SYNC_TIMING_MAX_PORTS                       3


/*! Defines the default filename of the driver config file - change as per your need or based on selected OS */
#define SYNC_TIMING_DRIVER_CONF_FILE                "/etc/sync_timing_driver.conf"

/*! Defines the name of the default log folder - change as per your need or based on selected OS */
#define SYNC_TIMING_DEFAULT_LOG_FOLDER              "/var/log"

/*! Defines the name of the default driver log file - change as per your need or based on selected OS */
#define SYNC_TIMING_DEFAULT_DRIVER_LOG              "/var/log/synctimingdriver.log"

/*! Defines the SPIDEV base - change as per your need or based on selected OS or platform */
#define SYNC_TIMING_DEFAULT_SPIDEV_BASE              "/dev/spidev1."




#define SI5388 1
#define SI5348 2
#define ARUBA 3


/** @} common_ds */
/** @} common */
/** @} api */

#endif // _SYNC_TIMING_CONFIG_H_

