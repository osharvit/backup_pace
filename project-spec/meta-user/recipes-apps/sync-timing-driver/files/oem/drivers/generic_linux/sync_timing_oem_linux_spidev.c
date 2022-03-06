/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_oem_linux_spidev.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/26/2018
 *
 * DESCRIPTION        : Low Level SPI Device driver Implementation for Linux OS
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
#include <linux/spi/spidev.h>
#include "sync_timing_oem_linux_spidev.h"
#include <errno.h>
#include <err.h>

/*****************************************************************************************
    Macros
*****************************************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
 ****************************************************************************************/

static SYNC_TIMING_OEMLINUX_SPI_DEVICES_T syncTimingOemLinuxSpiDevices[SYNC_TIMING_OEM_SPIDEV_MAX_DEVS] = {0};

static uint8_t uNumOpenSpiDevices = 0;

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxSpidev_Open
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/26/2018
 *
 * DESCRIPTION   : This function is used to open the kernel SPI device
 *
 * IN PARAMS     : pSpidevName  - Base name of the spi device
 *                 spidevID     - Linux SPI Device ID
 *                 spiSpeed     - Bit Rate of the SPI device to open
 *                 bitsPerWord  - number of bits per word
 *                 spiMode      - SPI Mode
 *                 lsbFirst     - LSB First or MSB First
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxSpidev_Open(int8_t *pSpidevName, uint8_t spidevID, uint32_t spiSpeed, 
                                    uint8_t bitsPerWord, uint8_t spiMode, 
                                    uint8_t lsbFirst)
{
    int32_t         ret     = 0;
    uint8_t         uIdx    = 0;

    if (uNumOpenSpiDevices == SYNC_TIMING_OEM_SPIDEV_MAX_DEVS)
    {
        ERROR_PRINT("Max Device count reached.\n");
        return (-1);
    }

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_SPIDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxSpiDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxSpiDevices[uIdx].spiFd != 0 && 
            syncTimingOemLinuxSpiDevices[uIdx].spidevID == spidevID)
        {
            ERROR_PRINT("device already opened\n");
            return (-2);
        }
    }

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_SPIDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxSpiDevices[uIdx].bInUse == 0)
        {
            break;
        }
    }

    if (uIdx == SYNC_TIMING_OEM_SPIDEV_MAX_DEVS)
    {
        ERROR_PRINT("No free devices to open and use.\n");
        return (-3);
    }

    uNumOpenSpiDevices++;

    syncTimingOemLinuxSpiDevices[uIdx].bInUse = 1;   
    syncTimingOemLinuxSpiDevices[uIdx].spiSpeed = spiSpeed;
    syncTimingOemLinuxSpiDevices[uIdx].spiMode = spiMode;
    syncTimingOemLinuxSpiDevices[uIdx].spiBitsPerWord = bitsPerWord;
    syncTimingOemLinuxSpiDevices[uIdx].delay = 0;
    syncTimingOemLinuxSpiDevices[uIdx].spidevID = spidevID;
    sprintf(syncTimingOemLinuxSpiDevices[uIdx].spiDevice, "%s%u", pSpidevName, spidevID);
    
    if (lsbFirst)
        syncTimingOemLinuxSpiDevices[uIdx].spiMode |= SPI_LSB_FIRST;

    DEBUG_PRINT("spiId = %u, device = %s\n", spidevID, syncTimingOemLinuxSpiDevices[uIdx].spiDevice);

    syncTimingOemLinuxSpiDevices[uIdx].spiFd = open(syncTimingOemLinuxSpiDevices[uIdx].spiDevice, 
                                                    O_RDWR);
    if (syncTimingOemLinuxSpiDevices[uIdx].spiFd < 0) 
    {
        ERROR_PRINT("can't open device\n");
        syncTimingOemLinuxSpiDevices[uIdx].bInUse = 0;   
        syncTimingOemLinuxSpiDevices[uIdx].spiFd = 0;
        syncTimingOemLinuxSpiDevices[uIdx].spidevID = 0xFF;
        uNumOpenSpiDevices--;
        return (-4);
    }

    /*
     * spi mode
     */
    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_WR_MODE, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiMode);
    if (ret == -1) 
    {
        ERROR_PRINT("can't set spi wr mode(%d):%s\n", ret, strerror(errno));
        Sync_Timing_OemLinuxSpidev_Close(syncTimingOemLinuxSpiDevices[uIdx].spiFd);
        return (-5);
    }

    

    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_RD_MODE, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiMode);
    DEBUG_PRINT("mode %.2x\n", syncTimingOemLinuxSpiDevices[uIdx].spiMode);
    if (ret == -1) 
    {
        ERROR_PRINT("can't set spi rd mode(%d):%s\n", ret, strerror(errno));
        Sync_Timing_OemLinuxSpidev_Close(spidevID);
        return (-6);
    }

    /*
     * bits per word
     */
    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_WR_BITS_PER_WORD, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiBitsPerWord);
    if (ret == -1) 
    {
        ERROR_PRINT("can't set bits per word\n");
        Sync_Timing_OemLinuxSpidev_Close(spidevID);
        return (-7);
    }

    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_RD_BITS_PER_WORD, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiBitsPerWord);
    DEBUG_PRINT("bits %.2x\n", syncTimingOemLinuxSpiDevices[uIdx].spiBitsPerWord);
    if (ret == -1) 
    {
        ERROR_PRINT("can't get bits per word\n");
        Sync_Timing_OemLinuxSpidev_Close(spidevID);
        return (-8);
    }

    /*
     * max speed hz
     */
    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_WR_MAX_SPEED_HZ, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiSpeed);
    if (ret == -1) 
    {
        ERROR_PRINT("can't set max speed hz\n");
        Sync_Timing_OemLinuxSpidev_Close(spidevID);
        return (-9);
    }

    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_RD_MAX_SPEED_HZ, 
                &syncTimingOemLinuxSpiDevices[uIdx].spiSpeed);
    DEBUG_PRINT("speed %d\n\n", syncTimingOemLinuxSpiDevices[uIdx].spiSpeed);
    if (ret == -1) 
    {
        ERROR_PRINT("can't get max speed hz\n");
        Sync_Timing_OemLinuxSpidev_Close(spidevID);
        return (-10);
    }
    return ret;
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxSpidev_Transfer
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/26/2018
 *
 * DESCRIPTION   : This function is used to transfer data over the SPI device
 *
 * IN PARAMS     : spidevID   - Linux SPI Device Id
 *                 length     - Transfer size
 *                 tx         - tx buffer
 *                 rxbuf      - rx buffer
                   readLength - Expected length of the reply
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxSpidev_Transfer(uint8_t spidevID, uint16_t length, 
                                                 uint8_t * tx, uint8_t *rxbuf, uint16_t readLength )
{
    int32_t ret, idx = 0;
    uint8_t uIdx = 0;

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_SPIDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxSpiDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxSpiDevices[uIdx].spiFd != 0 && 
            syncTimingOemLinuxSpiDevices[uIdx].spidevID == spidevID)
        {
            break;
        }
    }

    if (uIdx == uNumOpenSpiDevices)
    {
        ERROR_PRINT("Invalid SPI device Id\n");
        return (-1);
    }
    
    if (syncTimingOemLinuxSpiDevices[uIdx].spiFd < 0) 
    {
        ERROR_PRINT("SPI is not opened\n");
        return (-2);
    }

    // SPI transfer parameters for IOCTL
    struct spi_ioc_transfer tr = 
    {
        .tx_buf = (unsigned long) tx,
        .rx_buf = (unsigned long) rxbuf,
        .len = length,
        .delay_usecs = syncTimingOemLinuxSpiDevices[uIdx].delay,
        .speed_hz = syncTimingOemLinuxSpiDevices[uIdx].spiSpeed,
        .bits_per_word = syncTimingOemLinuxSpiDevices[uIdx].spiBitsPerWord,
    };

    // transfer IOCTL
    ret = ioctl(syncTimingOemLinuxSpiDevices[uIdx].spiFd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) 
    {
        printf("can't send spi message : Error = %s\n", strerror(errno));
        return (-3);
    }

    DEBUG_PRINT("len = %u\n", length);

    DEBUG_PRINT("tx = ");
    for (idx = 0; idx < length; idx++)
    {
        DEBUG_PRINT("%.2x ", tx[idx]);
    }

    DEBUG_PRINT("\n");
    DEBUG_PRINT("rx = ");
    for (idx = 0; idx < length; idx++)
    {
        DEBUG_PRINT("%.2x ", rxbuf[idx]);
    }
    DEBUG_PRINT("\n");
    
    return (0);
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_OemLinuxSpidev_Close
 *
 * AUTHOR        : Srini Venkataraman
 *
 * DATE CREATED  : 06/26/2018
 *
 * DESCRIPTION   : This function is used to close the kernel SPI device
 *
 * IN PARAMS     : spidevID - contains the spidevID to close
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  :  0 - Success
 *                 -1 - Failure
 *
 ****************************************************************************************/
int32_t Sync_Timing_OemLinuxSpidev_Close(uint8_t spidevID)
{
    uint8_t uIdx = 0;

    for (uIdx = 0; uIdx < SYNC_TIMING_OEM_SPIDEV_MAX_DEVS; uIdx++)
    {
        if (syncTimingOemLinuxSpiDevices[uIdx].bInUse == 1 &&
            syncTimingOemLinuxSpiDevices[uIdx].spiFd != 0 && 
            syncTimingOemLinuxSpiDevices[uIdx].spidevID == spidevID)
        {
            break;
        }
    }

    if (uIdx == SYNC_TIMING_OEM_SPIDEV_MAX_DEVS)
    {
        ERROR_PRINT("Invalid SPI device Id\n");
        return (-1);
    }

    if (syncTimingOemLinuxSpiDevices[uIdx].spiFd > 0) 
    {
        close(syncTimingOemLinuxSpiDevices[uIdx].spiFd);
        syncTimingOemLinuxSpiDevices[uIdx].spiFd = 0;
        syncTimingOemLinuxSpiDevices[uIdx].bInUse = 0;        
        uNumOpenSpiDevices--;
        return (0);
    } 
    else 
    {
        ERROR_PRINT("SPI is not opened\n");
        return (-2);
    } 
}

