/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_gpio.c 
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/29/2018
 *
 * DESCRIPTION        : Low Level GPIO Device driver Implementation for Linux OS
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

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>

#include "sync_timing_config.h"
#include "sync_timing_oem_linux_gpio.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

static SYNC_TIMING_GPIO_DATA_T gSyncTimingGPIOCfg[SYNC_TIMING_MAX_GPIO_SUPPORTED] ;

static int32_t Sync_Timing_Internal_OemLinuxGpiodev_Config(
                                                  uint16_t gpioNum, 
                                                  uint8_t gpioDirection, 
                                                  SYNC_TIMING_OEM_TRIG_TYPE_ID_E trigMask);


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxGpiodev_Open
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to open the Linux kernel GPIO device
 *
 * IN PARAMS     : gpioNum - gpio device number that needs to be opened
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxGpiodev_Open(uint16_t gpioNum)
{
    int32_t fdg = 0, index = 0, ret = 0;
    char    gpioPath[512];
    DIR     *dir;

    while ( index < SYNC_TIMING_MAX_GPIO_SUPPORTED)
    {
        DEBUG_PRINT("Sync_Timing_OemLinuxGpiodev_Open: gSyncTimingGPIOCfg[%d].gpioNum: %d\n", index, gSyncTimingGPIOCfg[index].gpioNum);
        if (gSyncTimingGPIOCfg[index].gpioNum == 0)
        {
            break; 
        }  
        else if(gSyncTimingGPIOCfg[index].gpioNum == gpioNum)
        {
            ERROR_PRINT("gpioNum %u already opened\n", gpioNum);
            return -1;
        }
        
        index++;
    }
    
    if (index >= SYNC_TIMING_MAX_GPIO_SUPPORTED)
    {
        ERROR_PRINT("Attempting to open more than max supported GPIOs\n");
        return (-1);
    }
    
    sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%d", gpioNum);  //format GPIO dir name
    dir = opendir(gpioPath);

    if (dir) 
    {
        /* Directory exists. */
        closedir(dir);
        DEBUG_PRINT("%s exists; No need to export\n", gpioPath);
    } 
    else if (ENOENT == errno) 
    {
        /* Directory does not exist - so export to sysfs*/
        fdg = open(SYSFS_GPIO_PATH"export", O_WRONLY);
        sprintf(gpioPath, "%d", gpioNum);
        if (-1 == write(fdg, gpioPath, strlen(gpioPath)))
        {
            ERROR_PRINT("GPIO %u export ERROR: %s\n", gpioNum, strerror(errno));
            return (-2);
        }
        else
        {
            close(fdg);
        }
    } 
    else 
    {
        ERROR_PRINT("GPIO %u ERROR: %s\n", gpioNum, strerror(errno));
        return (-3);
    } 
    
    sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%d/value", gpioNum);  //format GPIO dir name
    
    fdg = open(gpioPath, O_RDONLY | O_NONBLOCK );
    if (fdg < 0) 
    {
        ERROR_PRINT("gpio/fd_open: %s failed\n",gpioPath);
        return (-4);
    }
    
    gSyncTimingGPIOCfg[index].fdg = fdg;
    gSyncTimingGPIOCfg[index].gpioNum = gpioNum;

    DEBUG_PRINT("GPIO Device %d Opened successfully\n", gpioNum);

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxGpiodev_Close
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to close the Linux kernel GPIO device
 *
 * IN PARAMS     : gpioNum - gpio device number that needs to be closed
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxGpiodev_Close(uint16_t gpioNum)
{
    int32_t fdg, index = 0, ret = 0;
    char    gpioPath[512];
   
    sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%d", gpioNum);  //format GPIO dir name

    while (index < SYNC_TIMING_MAX_GPIO_SUPPORTED)
    {
        if (gSyncTimingGPIOCfg[index].gpioNum == gpioNum)
        {
            break; 
        }  
        index++;
    }
    
    if( index == SYNC_TIMING_MAX_GPIO_SUPPORTED)
    {
        ERROR_PRINT("gpioNum %u already closed or never opened\n", gpioNum);
        return (-1);
    }
      
    DIR *dir = opendir(gpioPath);
    if (dir) 
    {
        /* Directory exists. */
        DEBUG_PRINT("%s exists\n", gpioPath);

        closedir(dir);
        close(gSyncTimingGPIOCfg[index].fdg);

        fdg = open(SYSFS_GPIO_PATH"unexport", O_WRONLY); // unexport to sysfs
        sprintf(gpioPath, "%d", gpioNum);
        ret = write(fdg, gpioPath, strlen(gpioPath));
        close(fdg);
    } 
    else if (ENOENT == errno) 
    {
        /* Directory does not exist - Don't do anything for now */
        ;
    } 
    else 
    {
        ERROR_PRINT("GPIO %u ERROR: %s\n", gpioNum,strerror(errno));
        return (-1);
    } 
    
    gSyncTimingGPIOCfg[index].gpioNum = 0;
    gSyncTimingGPIOCfg[index].fdg = 0;
    
    DEBUG_PRINT("GPIO Device %d Closed successfully\n", gpioNum);
    ret = 0;

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxGpiodev_Config
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to configure the Linux kernel GPIO device
 *
 * IN PARAMS     : gpioNum       - gpio device number that needs to be configured
 *               : directionOut  - gpio direction in or out
 *               : trigMask      - <For Future Use >
 *               : callback      - <For Future Use >
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxGpiodev_Config(uint16_t gpioNum, uint8_t gpioDirection,
                                        SYNC_TIMING_OEM_TRIG_TYPE_ID_E trigMask, 
                                        SYNC_TIMING_OEM_GPIO_CALLBACK callback)
{
    uint8_t index = 0;
    int32_t ret;
    uint8_t gpio_index = 0;

    while ( index < SYNC_TIMING_MAX_GPIO_SUPPORTED)
    {
        if (gSyncTimingGPIOCfg[index].gpioNum == gpioNum)
            break;  
        index++;
    }
    
    if (index >= SYNC_TIMING_MAX_GPIO_SUPPORTED)
        return (-1);
    
    gpio_index = index;
    
    if (trigMask < SYNC_TIMING_OEM_TRIG_TYPE_MAX && gpioDirection <= SYNC_TIMING_GPIO_DIR_OUT)
    {
        // set IO mode
        ret = Sync_Timing_Internal_OemLinuxGpiodev_Config(gpioNum, gpioDirection, trigMask);
      
        if (ret == 0) 
        {
            // configure Trigger mask
            DEBUG_PRINT("\nConfigure IRQ mask - gpioNum %d direction %d edge %d\n", 
                             gpioNum, gpioDirection, trigMask);
            gSyncTimingGPIOCfg[gpio_index].IRQmask = (SYNC_TIMING_OEM_TRIG_TYPE_ID_E) (trigMask);
            gSyncTimingGPIOCfg[gpio_index].gpioDir = gpioDirection;
            gSyncTimingGPIOCfg[gpio_index].tag = gpioNum;
            ret = 0;
        } 
    }
    else 
    {
        ERROR_PRINT("Invalid gpio Trigger mask (%u) or gpioDirection (%u)\n", trigMask, gpioDirection);
        ret = -1;
    }

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxGpiodev_GetVal
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to get the Linux kernel GPIO device value
 *
 * IN PARAMS     : gpioNum       - gpio device number that needs to be configured
 *
 * OUT PARAMS    : gpioVal       - Pointer containing the gpio value read
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxGpiodev_GetVal(uint16_t gpioNum, uint8_t *gpioVal)
{
    int32_t fdg, ret = 0;
    char    gpioPath[512];
    uint8_t val;

    sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%u/value",gpioNum);

    fdg = open(gpioPath, O_RDONLY);    //open gpio value file in sysfs
    if (fdg < 0) 
    {
        ERROR_PRINT("Can't open %s file for IO %d\n", gpioPath, gpioNum);
        ret = -1;
    } 
    else 
    {                      
        // read gpio value from sysfs
        if (read(fdg, &val, 1) == 1) 
        {
            if (val == '0')
                *gpioVal = 0;
            else
                *gpioVal = 1;
            DEBUG_PRINT("GPIO (%u) Value read %c\n",gpioNum, val);
        } 
        else 
        {
            ERROR_PRINT("Can't read from file %s\n", gpioPath);
            ret = -1;
        }
        close(fdg);                 //close pin file in sysfs
    }
    return (ret);
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxGpiodev_SetVal
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : This function is used to set the Linux kernel GPIO device value
 *
 * IN PARAMS     : gpioNum       - gpio device number that needs to be configured
 *               : gpioVal       - The gpio value to be written
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxGpiodev_SetVal(uint16_t gpioNum, uint8_t gpioVal)
{
    int32_t fdg, ret;
    char    gpioPath[512];

    sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%u/value",gpioNum);

    fdg = open(gpioPath, O_WRONLY);    //open pin file in sysfs
    if (fdg < 0) 
    {
        ERROR_PRINT("Can't open %s file for IO %d\n", gpioPath, gpioNum);
        return (-1);
    } 
    else 
    {
        if (gpioVal)
        {
            ret = write(fdg, "1", 1);
        }
        else
        {
            ret = write(fdg, "0", 1);
        }
        close(fdg);

        if (ret == 1)
        {
            DEBUG_PRINT("GPIO (%u) Value written %u\n",gpioNum, gpioVal);
            return (0);
        }
        else
        {
            ERROR_PRINT("Can't write to file %s\n", gpioPath);
            return (-1);
        }
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OemLinuxGpiodev_Config
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/29/2018
 *
 * DESCRIPTION   : Internal function used to configure the Linux kernel GPIO device
 *
 * IN PARAMS     : gpioNum       - gpio device number that needs to be configured
 *               : gpioDirection - The gpio direction to be programmed
 *               : trigMask      - GPIO Trigger mask
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
static int32_t Sync_Timing_Internal_OemLinuxGpiodev_Config(uint16_t gpioNum, 
                                                           uint8_t gpioDirection, 
                                                           SYNC_TIMING_OEM_TRIG_TYPE_ID_E trigMask)
{
    int32_t    fdg, ret = 0;
    const char *edgeStr = NULL;
    char       gpioPath[512]; 
    char       buf[8] = {0};
    ssize_t    bytes_read;
    
    //Update GPIO Direction
    {
        sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%u/direction",gpioNum);

        fdg = open(gpioPath, O_RDWR);    //open pin file in sysfs
        if (fdg < 0) 
        {
            ERROR_PRINT("Can't open %s file for IO %d\n", gpioPath, gpioNum);
            return (-1);
        } 
        else 
        {
            if (gpioDirection == SYNC_TIMING_GPIO_DIR_OUT)
            {
                bytes_read = read(fdg, &(buf[0]), 3);
                if (bytes_read > 0)
                {
                    DEBUG_PRINT("buf = %s\n", buf);
                    if (strncmp(&(buf[0]), "in", 2) == 0)
                    {
                        INFO_PRINT("changing gpio direction to out\n");
                        lseek(fdg, -3, SEEK_CUR);
                        ret = write(fdg, "out", 3); //roni fix
                    }
                }
                else
                {
                    ERROR_PRINT("Error reading from gpio direction file.\n");
                }
            }
            else
            {
                bytes_read = read(fdg, &(buf[0]), 3);
                if (bytes_read > 0)
                {
                    DEBUG_PRINT("buf = %s\n", buf);
                    if (strncmp(&(buf[0]), "out", 3) == 0)
                    {
                        INFO_PRINT("changing gpio direction to in\n");
                        lseek(fdg, -3, SEEK_CUR);
                        ret = write(fdg, "in", 2); //roni fix
                    }
                }
                else
                {
                    ERROR_PRINT("Error reading from gpio direction file.\n");
                }                
            }

            close(fdg);

            if (ret < 0)
            {    
                ERROR_PRINT("Can't write to file %s\n", gpioPath);
                return (-1);
            }
        }
    }

    //Update GPIO Edge 
    {
        sprintf(gpioPath, SYSFS_GPIO_PATH"gpio%u/edge",gpioNum);
        fdg = open(gpioPath, O_WRONLY);
        if (fdg < 0) 
        {
            ERROR_PRINT("Can't open %s file for IO %d\n", gpioPath, gpioNum);
            return -1;
        }
        switch(trigMask)
        {
            case SYNC_TIMING_OEM_TRIG_RISING_EDGE:  
                edgeStr = "rising";
                break;
            case SYNC_TIMING_OEM_TRIG_FALLING_EDGE:  
                edgeStr = "falling";
                break;
            case SYNC_TIMING_OEM_TRIG_CHANGE_STATE:  
                edgeStr = "both";
                break;  
            default:
                edgeStr = "none";
                break;
        }
        ret = write(fdg, edgeStr, strlen(edgeStr) + 1);

        close(fdg);

        if (ret > 0)
        {
            DEBUG_PRINT("\ngpio_num %d direction %d edge %s\n", gpioNum, gpioDirection, edgeStr);
            return (0);
        }
        else
        {
            ERROR_PRINT("Can't write to file %s\n", gpioPath);
            return (-1);
        }
    }
}


