/***********************************************************************************************/
 /*
 * FILE NAME          : sync_timing_oem_linux_i2cdev.c
 *
 * AUTHOR             : Ramprasath Pitta Sekar
 *
 * DATE CREATED       : 06/09/2021
 *
 * DESCRIPTION        : Low Level I2C Device driver Implementation for Linux OS
 *
 ****************************************************************************************/
  
/****************************************************************************************/
/**                  Copyright (c) 2021, 2021 Skyworks Solution Inc.                   **/
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


#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <linux/i2c-dev.h>
#include "sync_timing_oem_linux_i2cdev.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
******************************************************************************************/

static SYNC_TIMING_OEMLINUX_I2C_DEVICES_T 
syncTimingOemLinuxI2cDevices[SYNC_TIMING_OEM_I2CDEV_MAX_DEVS] = {0};

static uint8_t uNumOpenI2cDevices = 0;
int32_t i2cfd = 0;

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxI2cdev_Open
 *
 * AUTHOR        : Ramprasath Pitta Sekar
 *
 * DATE CREATED  : 06/09/2021
 *
 * DESCRIPTION   : This function is used to open the kernel I2C device
 *
 * IN PARAMS     : pI2cdevName  - Base name of the I2C device
 *                 i2cdevID     - Linux I2C Device ID
 *                 i2cDevAddr   - Bus address of the I2C Device
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                < 0 - Failure
 *
****************************************************************************************/

int32_t Sync_Timing_OemLinuxI2cdev_Open(int8_t* i2cDevName, uint8_t i2cDevId,
                                                        uint8_t i2cDevAddr)
{
    int32_t     ret     = 0;
    uint8_t     uIdx    = 0;
    //char i2cDevice[SYNC_TIMING_OEM_LINUX_MAX_DEVICE_NAME_SZ];
    if (uNumOpenI2cDevices == SYNC_TIMING_OEM_I2CDEV_MAX_DEVS)
    {
        ERROR_PRINT("Max Device count reached.\n");
        return (-1);
    }
    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_I2CDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxI2cDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxI2cDevices[uIdx].i2cFd != 0 &&
            syncTimingOemLinuxI2cDevices[uIdx].i2cDevId == i2cDevId)
        {
            ERROR_PRINT("device already opened\n");
            return (-2);
        }
    }
    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_I2CDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxI2cDevices[uIdx].bInUse == 0)
        {
            break;
        }
    }

    if (uIdx == SYNC_TIMING_OEM_I2CDEV_MAX_DEVS)
    {
        ERROR_PRINT("No free devices to open and use. \n");
        return (-3);
    }

    uNumOpenI2cDevices++;

    syncTimingOemLinuxI2cDevices[uIdx].bInUse = 1;
    syncTimingOemLinuxI2cDevices[uIdx].i2cDevId = i2cDevId;
    syncTimingOemLinuxI2cDevices[uIdx].i2cDevAddr = i2cDevAddr;
    sprintf(syncTimingOemLinuxI2cDevices[uIdx].i2cDevice, "%s%u", i2cDevName, i2cDevId);

    DEBUG_PRINT("i2cId = %u, device = %s\n", i2cDevId, syncTimingOemLinuxI2cDevices[uIdx].i2cDevice);
    syncTimingOemLinuxI2cDevices[uIdx].i2cFd = open(syncTimingOemLinuxI2cDevices[uIdx].i2cDevice,
                                                    O_RDWR);

    if (syncTimingOemLinuxI2cDevices[uIdx].i2cFd < 0)
    {
        ERROR_PRINT("can't open device\n");
        syncTimingOemLinuxI2cDevices[uIdx].bInUse = 0;
        syncTimingOemLinuxI2cDevices[uIdx].i2cFd = 0;
        syncTimingOemLinuxI2cDevices[uIdx].i2cDevId = 0xFF;
        uNumOpenI2cDevices--;
        return (-4);
    }

    ret = ioctl(syncTimingOemLinuxI2cDevices[uIdx].i2cFd, I2C_SLAVE_FORCE,
                syncTimingOemLinuxI2cDevices[uIdx].i2cDevAddr);
    if (ret < 0 )
    {
        ERROR_PRINT("Unable to set the i2c slave address.\n");
        return (-5);
    }
    return ret;
}

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxI2cdev_Transfer
 *
 * AUTHOR        : Ramprasath Pitta Sekar
 *
 * DATE CREATED  : 06/09/2021
 *
 * DESCRIPTION   : This function is used to transfer data over I2C device
 *
 * IN PARAMS     : i2cdevID     - Linux I2C Device ID
                   length       - Transfer size
                   tx           - tx buffer for sending
                   rxbuf        - rx buffer for reading
                   readLength   - Expected length of the reply
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                < 0 - Failure
 *
****************************************************************************************/

int32_t Sync_Timing_OemLinuxI2cdev_Transfer(uint8_t i2cdevID, uint16_t length,
                                                       uint8_t *tx, uint8_t *rxbuf, uint16_t readLength)
{

    int32_t ret  = 0;
    uint8_t uIdx = 0;

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_I2CDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxI2cDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxI2cDevices[uIdx].i2cFd != 0 && 
            syncTimingOemLinuxI2cDevices[uIdx].i2cDevId == i2cdevID)
        {
            break;
        }
    }

    if (uIdx == uNumOpenI2cDevices)
    {
        ERROR_PRINT("Invalid I2C device Id\n");
        return (-1);
    }

    if (syncTimingOemLinuxI2cDevices[uIdx].i2cFd < 0) 
    {
        ERROR_PRINT("I2C is not opened\n");
        return (-2);
    }

    ret = ioctl(syncTimingOemLinuxI2cDevices[uIdx].i2cFd, I2C_SLAVE_FORCE,
                syncTimingOemLinuxI2cDevices[uIdx].i2cDevAddr);
    if (ret < 0)
    {
        ERROR_PRINT("Unable to set the slave address.\n");
        return ret;
    }

    if(readLength != 0)
    {
        ret = write(syncTimingOemLinuxI2cDevices[uIdx].i2cFd, tx, length - readLength);
        if(ret < 0){
            ERROR_PRINT("can't write address for i2c read : Error = %s\n", strerror(errno));
        }

        if(ret != length - readLength)
        {
            ERROR_PRINT("can't write address for i2c read : Write length(=%d) didn't match \n",ret);
            return -3;
        }

        ret = read(syncTimingOemLinuxI2cDevices[uIdx].i2cFd, rxbuf, readLength);
        if(ret < 0){
            ERROR_PRINT("can't read i2c message : Error = %s\n", strerror(errno));
        }

        if(ret != readLength)
        {
            ERROR_PRINT("can't read i2c message : Read length(=%d) didn't match \n",ret);
            return -3;
        }
    }
    else
    {
        ret = write(syncTimingOemLinuxI2cDevices[uIdx].i2cFd, tx, length);
        if(ret < 0){
            ERROR_PRINT("can't send i2c message : Error = %s\n", strerror(errno));
        }

        if(ret != length)
        {
            ERROR_PRINT("can't send i2c message : Write length(=%d) didn't match \n",ret);
            return -3;
        }
    }
    return (0);
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxI2Cdev_Close
 *
 * AUTHOR        : Ramprasath Pitta Sekar
 *
 * DATE CREATED  : 06/09/2021
 *
 * DESCRIPTION   : This function is used to close the kernel I2C device
 *
 * IN PARAMS     : i2cdevID - contains the I2CdevID to close
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                < 0 - Failure
 *
 ****************************************************************************************/

int32_t Sync_Timing_OemLinuxI2cdev_Close(uint8_t i2cDevID)
{
    uint8_t uIdx = 0;

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_I2CDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxI2cDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxI2cDevices[uIdx].i2cFd != 0 &&
            syncTimingOemLinuxI2cDevices[uIdx].i2cDevId == i2cDevID)
        {
            break;
        }
    }

    if (uIdx == SYNC_TIMING_OEM_I2CDEV_MAX_DEVS)
    {
        ERROR_PRINT("Invalid I2C device Id\n");
        return (-1);
    }

    if (syncTimingOemLinuxI2cDevices[uIdx].i2cFd > 0) 
    {
        close(syncTimingOemLinuxI2cDevices[uIdx].i2cFd);
        syncTimingOemLinuxI2cDevices[uIdx].i2cFd = 0;
        syncTimingOemLinuxI2cDevices[uIdx].bInUse = 0;
        uNumOpenI2cDevices--;
        return (0);
    }
    else 
    {
        ERROR_PRINT("I2C is not opened\n");
        return (-2);
    }
}

