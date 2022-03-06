/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_irq.c 
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
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>


#include "sync_timing_config.h"
#include "sync_timing_oem_linux_irq.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

static SYNC_TIMING_IRQ_DATA_T   gSyncTimingIRQCfg[SYNC_TIMING_MAX_IRQ_SUPPORTED] ;
static pthread_t                sync_timing_oem_irq_thread  = 0;
static uint32_t                 exitThread                  = 0;
static uint8_t                  bIrqGlobalInitDone          = 0;


static int32_t Sync_Timing_Internal_OemLinuxIrqdev_Config(uint16_t                       irqPinNum, 
                                                          SYNC_TIMING_OEM_TRIG_TYPE_ID_E irqTrigType);
static void*   Sync_Timing_Internal_OemLinuxIrqThread(void *arg);
static int32_t Sync_Timing_Internal_OemLinuxCheckIrqState(int irqIndex,int irqValue);
              

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxIrqdev_GlobalInit
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used for initializing the global data structure for IRQ control
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_GlobalInit()
{

    uint32_t            uIdx                = 0, 
                        ret                 = 0;

    if(!bIrqGlobalInitDone)
    {
        // Initialize mutex for all IO
        for (uIdx = 0; uIdx < SYNC_TIMING_MAX_IRQ_SUPPORTED; uIdx++)
        {
            if (pthread_mutex_init(&gSyncTimingIRQCfg[uIdx].irqIOlock, NULL) != 0) 
            {
                ERROR_PRINT("Mutex init %d failed. \n", uIdx);
                return -1;
            }
        }

        // Create IRQ Handler thread.
        if (pthread_create(&sync_timing_oem_irq_thread, NULL, Sync_Timing_Internal_OemLinuxIrqThread, NULL) != 0) 
        {
            ERROR_PRINT("Cannot create sync_timing_oem_irq_thread \n");
            return -1;
        }

        bIrqGlobalInitDone = 1;
        DEBUG_PRINT("Sync_Timing_OemLinuxIrqdev_GlobalInit Done\n");
    }

    return ret;
}  

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxIrqdev_Open
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used to open the Linux kernel GPIO device
 *
 * IN PARAMS     : irqPinNum - IRQ device pin number that needs to be opened
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_Open(uint16_t irqPinNum)
{
    int32_t irqFd = 0, index = 0, ret = 0;
    char    irqPath[512];
    DIR     *dir;

    while ( index < SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        DEBUG_PRINT("Sync_Timing_OemLinuxIrqdev_Open: gSyncTimingIRQCfg[%d].gpioNum: %d\n", 
                     index, gSyncTimingIRQCfg[index].irqPinNum);
        if (gSyncTimingIRQCfg[index].irqPinNum == 0)
        {
            break; 
        }  
        else if(gSyncTimingIRQCfg[index].irqPinNum == irqPinNum)
        {
            ERROR_PRINT("irqPinNum %u already opened\n", irqPinNum);
            return -1;
        }
        index++;
    }
    
    if (index > SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        ERROR_PRINT("Attempting to open more than max supported GPIOs\n");
        return (-1);
    }
    
    sprintf(irqPath, SYSFS_IRQ_PATH"gpio%d", irqPinNum);  //format GPIO dir name
    dir = opendir(irqPath);

    if (dir) 
    {
        /* Directory exists. */
        closedir(dir);
        DEBUG_PRINT("%s exists; No need to export\n", irqPath);
    } 
    else if (ENOENT == errno) 
    {
        /* Directory does not exist - so export to sysfs*/
        irqFd = open(SYSFS_IRQ_PATH"export", O_WRONLY);
        sprintf(irqPath, "%d", irqPinNum);
        if (-1 == write(irqFd, irqPath, strlen(irqPath)))
        {
            ERROR_PRINT("GPIO %u export ERROR: %s\n", irqPinNum, strerror(errno));
            return (-1);
        }
        else
        {
            close(irqFd);
        }
    } 
    else 
    {
        ERROR_PRINT("GPIO %u ERROR: %s\n", irqPinNum, strerror(errno));
        return (-1);
    } 

    //format IRQ dir name
    sprintf(irqPath, SYSFS_IRQ_PATH"gpio%d/value", irqPinNum);  
    
    irqFd = open(irqPath, O_RDONLY | O_NONBLOCK );
    if (irqFd < 0) 
    {
        ERROR_PRINT("gpio/fd_open: %s failed\n",irqPath);
        return (-1);
    }
    
    gSyncTimingIRQCfg[index].irqFd = irqFd;
    gSyncTimingIRQCfg[index].irqPinNum = irqPinNum;

    DEBUG_PRINT("IRQ Device PinNum %d Opened successfully; irqPath = %s, irqFd = %u\n", 
                 irqPinNum, irqPath, irqFd);

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxIrqdev_Close
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used to close the Linux kernel GPIO device
 *
 * IN PARAMS     : irqPinNum - IRQ device pin number that needs to be closed
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_Close(uint16_t irqPinNum)
{
    int32_t     irqFd, index = 0, ret = 0;
    char        irqPath[512];
    ssize_t     bytesWritten = 0;    
   
    sprintf(irqPath, SYSFS_IRQ_PATH"gpio%d", irqPinNum);  //format GPIO dir name

    while (index < SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        if (gSyncTimingIRQCfg[index].irqPinNum == irqPinNum)
        {
            break; 
        }  
        index++;
    }
    
    if( index == SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        ERROR_PRINT("gpioNum %u already closed or never opened\n", irqPinNum);
        return (-1);
    }
      
    DIR *dir = opendir(irqPath);
    if (dir) 
    {
        /* Directory exists. */
        DEBUG_PRINT("%s exists\n", irqPath);

        closedir(dir);
        close(gSyncTimingIRQCfg[index].irqFd);
        irqFd = open(SYSFS_IRQ_PATH"unexport", O_WRONLY); // unexport to sysfs
        sprintf(irqPath, "%d", irqPinNum);
        bytesWritten = write(irqFd, irqPath, strlen(irqPath));
        if (bytesWritten == -1)
        {
            ERROR_PRINT("Failed to unexport IRQ.\n");
        }
        close(irqFd);
    } 
    else if (ENOENT == errno) 
    {
        /* Directory does not exist - Don't do anything for now */
        ;
    } 
    else 
    {
        ERROR_PRINT("GPIO %u ERROR: %s\n", irqPinNum, strerror(errno));
        return (-1);
    } 
    
    gSyncTimingIRQCfg[index].irqPinNum = 0;
    gSyncTimingIRQCfg[index].irqFd = 0;
    
    DEBUG_PRINT("IRQ Device Pin Num %d Closed successfully\n", irqPinNum);

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxIrqdev_Config
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used to configure the Linux kernel IRQ device
 *
 * IN PARAMS     : irqIdTag      - IRQ ID tag to be sent back when notifying
 *               : irqPinNum     - IRQ device pin number that needs to be configured
 *               : irqTrigType   - IRQ Trigger Type
 *               : callback      - IRQ callback function
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_Config(uint32_t irqIdTag,
                                          uint16_t irqPinNum,              
                                          SYNC_TIMING_OEM_TRIG_TYPE_ID_E irqTrigType, 
                                          SYNC_TIMING_OEM_IRQ_CALLBACK pOemIrqCallbackFn)
{
    uint8_t index  = 0;
    int32_t ret    = 0;
    uint8_t irqIdx = 0;

    while ( index < SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        if (gSyncTimingIRQCfg[index].irqPinNum == irqPinNum)
            break;  
        index++;
    }
    
    if (index >  SYNC_TIMING_MAX_IRQ_SUPPORTED)
    {
        ERROR_PRINT("Attempting to configure more than max supported GPIOs\n");
        return (-1);
    }
    
    irqIdx = index;
    
    if (irqTrigType < SYNC_TIMING_OEM_TRIG_TYPE_MAX)
    {
        // Configure IRQ pin
        ret = Sync_Timing_Internal_OemLinuxIrqdev_Config(irqPinNum, irqTrigType);
      
        if (ret == 0) 
        {
            // configure Trigger mask
            pthread_mutex_lock(&gSyncTimingIRQCfg[irqIdx].irqIOlock);
            DEBUG_PRINT("Configure IRQ mask - irqPinNum %u irqIdTag %u irqTrigType %u\n", 
                         irqPinNum, irqIdTag, irqTrigType);
            gSyncTimingIRQCfg[irqIdx].irqTrigType = (SYNC_TIMING_OEM_TRIG_TYPE_ID_E) (irqTrigType);
            gSyncTimingIRQCfg[irqIdx].irqIdTag = irqIdTag;
            gSyncTimingIRQCfg[irqIdx].irqCallback = pOemIrqCallbackFn;
            pthread_mutex_unlock(&gSyncTimingIRQCfg[irqIdx].irqIOlock);
            ret = 0;
        } 
    }
    else 
    {
        ERROR_PRINT("Invalid IRQ Trigger Type (%u) \n", irqTrigType);
        ret = -1;
    }

    return ret;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxIrqdev_GlobalTerm
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : This function is used for terminating the global data structure for IRQ control
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxIrqdev_GlobalTerm()
{
    int32_t     ret     = 0;
    uint32_t    uIdx    = 0;

    // Terminate the thread
    exitThread = 1;
    
    // Delete all mutexes
    for (uIdx = 0; uIdx < SYNC_TIMING_MAX_IRQ_SUPPORTED; uIdx++)
    {
        pthread_mutex_destroy(&gSyncTimingIRQCfg[uIdx].irqIOlock);
    }
    
    bIrqGlobalInitDone = 0;
    return ret;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OemLinuxIrqdev_Config
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : Internal function used to configure the Linux kernel GPIO device
 *
 * IN PARAMS     : irqPinNum       - gpio device number that needs to be configured
 *               : IrqTrigType     - IRQ Trigger Type
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
static int32_t Sync_Timing_Internal_OemLinuxIrqdev_Config(uint16_t                       irqPinNum, 
                                                          SYNC_TIMING_OEM_TRIG_TYPE_ID_E irqTrigType)
{
    int32_t    irqFd, 
               ret              = 0;
    const char *edgeStr         = NULL;
    char       irqPath[512]     = {0}; 
    char       buf[8]           = {0};
    ssize_t    bytes_read       = -1;
    
    
    //Update GPIO Direction to IN since it is an IRQ
    {
        sprintf(irqPath, SYSFS_IRQ_PATH"gpio%u/direction",irqPinNum);

        //open pin file in sysfs
        irqFd = open(irqPath, O_RDWR);    
        if (irqFd < 0) 
        {
            ERROR_PRINT("Can't open %s file for IO %d\n", irqPath, irqPinNum);
            return (-1);
        } 
        else 
        {
            bytes_read = read(irqFd, &(buf[0]), 3);
            if (bytes_read > 0)
            {
                DEBUG_PRINT("buf = %s\n", buf);
                // Change dir to IN if it is set to OUT
                if (strncmp(&(buf[0]), "out", 3) == 0)
                {
                    INFO_PRINT("changing gpio direction to in\n");
                    lseek(irqFd, -3, SEEK_CUR);
                    ret = write(irqFd, "in", 2);
                }
            }
            else
            {
                ERROR_PRINT("Error reading from gpio direction file.\n");
            }

            close(irqFd);

            if (ret < 0)
            {    
                ERROR_PRINT("Can't write to file %s\n", irqPath);
                return (-1);
            }
        }
    }

    //Update GPIO Edge 
    {
        sprintf(irqPath, SYSFS_IRQ_PATH"gpio%u/edge",irqPinNum);
        irqFd = open(irqPath, O_WRONLY);
        if (irqFd < 0) 
        {
            ERROR_PRINT("Can't open %s file for IO %d\n", irqPath, irqPinNum);
            return -1;
        }
        switch(irqTrigType)
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
        ret = write(irqFd, edgeStr, strlen(edgeStr) + 1);

        close(irqFd);

        if (ret > 0)
        {
            DEBUG_PRINT("\ngpio_num %d edge %s\n", irqPinNum, edgeStr);
            return (0);
        }
        else
        {
            ERROR_PRINT("Can't write to file %s\n", irqPath);
            return (-1);
        }
    }
    return 0;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OemLinuxIrqThread
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : Internal thread function used to poll the IRQ and store the state in IRQval.
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
static void *Sync_Timing_Internal_OemLinuxIrqThread(void *arg)
{
    uint32_t        irqIndex            = 0;
    int32_t         state_changed       = 0, 
                    rc                  = 0;
    char            buf[MAX_BUFF_SIZE]  = {0};
    uint32_t        value_read          = 0;
    size_t          bytesRead           = 0;
    struct pollfd   irqfds[SYNC_TIMING_MAX_IRQ_SUPPORTED];

    do 
    { 
        memset((void*)&irqfds, 0, sizeof(irqfds));    
        for (irqIndex =0; irqIndex < SYNC_TIMING_MAX_IRQ_SUPPORTED; irqIndex++)
        {
            irqfds[irqIndex].fd = gSyncTimingIRQCfg[irqIndex].irqFd ;   
            irqfds[irqIndex].events = POLLPRI | POLLERR | POLLHUP | POLLNVAL;      
        }

        rc = poll(irqfds, SYNC_TIMING_MAX_IRQ_SUPPORTED, SYNC_TIMING_IRQ_POLL_TIMEOUT);
        if (rc < 0) 
        {
            ERROR_PRINT("\n poll() failed!\n");
            continue;
        }

        for (irqIndex =0; irqIndex < SYNC_TIMING_MAX_IRQ_SUPPORTED; irqIndex++)
        {
            INFO_PRINT("Poll Unblocked : irqfds[irqIndex].fd = %u, irqfds[irqIndex].events = 0x%x, "
                        "irqIndex =  %u, revents = 0x%x\n", 
                        irqfds[irqIndex].fd, irqfds[irqIndex].events,
                        irqIndex, irqfds[irqIndex].revents);

            if (irqfds[irqIndex].revents & POLLPRI)
            {
                lseek(irqfds[irqIndex].fd, 0, SEEK_SET);

                DEBUG_PRINT("IRQ Pin %d interrupt occurred\n", gSyncTimingIRQCfg[irqIndex].irqPinNum);
                bytesRead = read(irqfds[irqIndex].fd, buf, MAX_BUFF_SIZE);

                DEBUG_PRINT("IRQ bytesRead %d \n", (int32_t)bytesRead);

                value_read = atoi(buf);
                DEBUG_PRINT("IRQ value_read before CB %d \n", value_read);

                state_changed = Sync_Timing_Internal_OemLinuxCheckIrqState(irqIndex, value_read);
                DEBUG_PRINT("state_changed = %d\n", state_changed);
                if(state_changed == 1)
                {
                    if (pthread_mutex_trylock(&gSyncTimingIRQCfg[irqIndex].irqIOlock) == 0) 
                    {
                        // pin value has been changed, trigger callback
                        if(gSyncTimingIRQCfg[irqIndex].irqCallback)
                        {
                            // Trigger callback to ensure that all new interrupts have  been 
                            // processed as well. Note - these are level based interrupts.
                            while(value_read)
                            {
                                (gSyncTimingIRQCfg[irqIndex].irqCallback)(gSyncTimingIRQCfg[irqIndex].irqIdTag);
                                usleep(10 * 1000);
                                lseek(irqfds[irqIndex].fd, 0, SEEK_SET);
                                bytesRead = read(irqfds[irqIndex].fd, buf, MAX_BUFF_SIZE);
                                DEBUG_PRINT("IRQ bytesRead %d \n", (int32_t)bytesRead);
                                value_read = atoi(buf);
                                DEBUG_PRINT("IRQ value_read after CB %d \n", value_read);
                                gSyncTimingIRQCfg[irqIndex].irqValue = (uint8_t) value_read;
                            }
                        }
                        else
                        {
                            // TBD - How do we ensure the Interrupt is cleared if there is no CB registered
                        }
                    }
                    pthread_mutex_unlock(&gSyncTimingIRQCfg[irqIndex].irqIOlock);
                }
            }
        }
    }
    while (exitThread == 0);

    return 0;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_Internal_OemLinuxCheckIrqState
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 12/12/2018
 *
 * DESCRIPTION   : Internal thread function used to read IO state and verify change state for IRQ.
 *
 * IN PARAMS     : gpio_index  - IRQ Pin Number 
 *               : gpio_value  - Value read from the PIN
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
static int32_t Sync_Timing_Internal_OemLinuxCheckIrqState(int irqIndex,int newIrqValue)
{
    int ret = 0, state_changed = 0;

    if (pthread_mutex_trylock(&gSyncTimingIRQCfg[irqIndex].irqIOlock) == 0) 
    {
        switch (gSyncTimingIRQCfg[irqIndex].irqTrigType)// Check IRQ mask
        {
            case SYNC_TIMING_OEM_TRIG_DISABLE:          // IRQ disabled
                break;
            case SYNC_TIMING_OEM_TRIG_RISING_EDGE:      // IRQ in raising edge
                if (ret == 0 && 
                    newIrqValue == 1 && 
                    gSyncTimingIRQCfg[irqIndex].irqValue == 0) //detect edge
                {
                    gSyncTimingIRQCfg[irqIndex].irqCount++;        //increase event counter
                    state_changed = 1;
                }
                break;
            case SYNC_TIMING_OEM_TRIG_FALLING_EDGE:     // IRQ in falling edge
                if (ret == 0 && 
                    newIrqValue == 0 && 
                    gSyncTimingIRQCfg[irqIndex].irqValue == 1) //detect edge
                {
                    gSyncTimingIRQCfg[irqIndex].irqCount ++;        //increase event counter
                    state_changed = 1;
                }
                break;
            case SYNC_TIMING_OEM_TRIG_CHANGE_STATE:     // IRQ on both edges
                if ((((uint8_t) newIrqValue) ^ gSyncTimingIRQCfg[irqIndex].irqValue) && 
                    ret == 0)                           //detect edge
                {
                    gSyncTimingIRQCfg[irqIndex].irqCount ++;        //increase event counter
                    state_changed = 1;
                 }
                break;
            default:                                   // error case - must not come here
                pthread_mutex_unlock(&gSyncTimingIRQCfg[irqIndex].irqIOlock);
                return (-1);
        }
        if (state_changed == 1 && ret == 0)                    // update IO value, if read correctly
        {
            gSyncTimingIRQCfg[irqIndex].irqValue = (uint8_t) newIrqValue;
        }
        pthread_mutex_unlock(&gSyncTimingIRQCfg[irqIndex].irqIOlock);
    }
    return state_changed;
}

