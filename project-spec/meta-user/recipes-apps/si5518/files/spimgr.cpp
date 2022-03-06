/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * Contact Information:
 * Parallel Wireless
 */
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include "general.h"
#include "spimgr.h"

class_spimgr::class_spimgr(void)
{
	m_dump = 0;
	m_hspi = -1;
	memset(m_tx, 0xCC, SPI_MAX_TX_BUF_SIZE);
	memset(m_rx, 0xCC, SPI_MAX_RX_BUF_SIZE);
}

class_spimgr::~class_spimgr(void)
{
	close();
}

int class_spimgr::init(const char* spidev, unsigned char mode, unsigned int speed, unsigned int bits)
{
	close();

	m_hspi = open(spidev, O_RDWR);
    if (m_hspi < 0)
    {
        PRN_ERROR("The device cannot be opened, rc:%d\n", m_hspi);
        return -1;
    }

    int rc = ioctl(m_hspi, SPI_IOC_WR_MODE, &mode); // SPI Mode
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to select WR SPI mode (0x%x), rc:%d\n", mode, rc);
        return rc;
    }

    rc = ioctl(m_hspi, SPI_IOC_RD_MODE, &mode); // SPI Mode
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to select RD SPI mode (0x%x), rc:%d\n", mode, rc);
        return rc;
    }

    rc = ioctl(m_hspi, SPI_IOC_WR_BITS_PER_WORD, &bits);// bits per word
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to select WR SPI bits(%d), rc:%d\n", bits, rc);
        return rc;
    }

    rc = ioctl(m_hspi, SPI_IOC_RD_BITS_PER_WORD, &bits);// bits per word
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to select RD SPI bits(%d), rc:%d\n", bits, rc);
        return rc;
    }

    rc = ioctl(m_hspi, SPI_IOC_WR_MAX_SPEED_HZ, &speed); //max write speed
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to set WR SPI speed (%d), rc:%d\n", speed, rc);
        return rc;
    }

    rc = ioctl(m_hspi, SPI_IOC_RD_MAX_SPEED_HZ, &speed); //max write speed
    if (rc < 0)
    {
        PRN_ERROR("SPI: error to set RD SPI speed (%d), rc:%d\n", speed, rc);
        return rc;
    }

	m_bits = bits;
	m_speed = speed;
	m_mode = mode;
	m_dev = spidev;
	return RC_OK;
}

int class_spimgr::close(void)
{
	if (m_hspi > -1)
	{
		::close(m_hspi);
		m_hspi = -1;
	}

	return RC_OK;
}

int class_spimgr::send_data(void* pcmd, int cmd_size, void*presp, int resp_size, void* api_err)
{
	if (m_hspi < 0)
		return RC_SPIMGR_DEV_ERROR;

	struct spi_ioc_transfer msg[4] = {0};
	memset(&msg, 0, sizeof(msg));

	m_tx[0] = 0xC0;
	memcpy(m_tx+1, pcmd, cmd_size);
	memset(m_rx, 0xCC, sizeof(m_rx));

	msg[0].tx_buf        = (unsigned long)m_tx;
    msg[0].rx_buf        = (unsigned long)0;
    msg[0].len           = 1+cmd_size;
    msg[0].delay_usecs   = 0;
    msg[0].speed_hz      = m_speed;
    msg[0].bits_per_word = m_bits;

	int res = ioctl(m_hspi, SPI_IOC_MESSAGE(1), msg); // perform duplex transfer
	if (res < 0)
	{
		PRN_ERROR("SPIMGR: error to write the data, rc:%d\n", res);
		return RC_SPIMGR_DEV_ERROR;
	}

	int num = 0;
	while (num < 100)
	{
		memset(&msg, 0, sizeof(msg));

		// to request the responce for the forwarded command
		// -------------------------------------------------
		m_tx[0] = 0xD0;

	    msg[0].tx_buf        = (unsigned long)m_tx;
	    msg[0].rx_buf        = (unsigned long)0;
	    msg[0].len           = 1;
	    msg[0].delay_usecs   = 0;
	    msg[0].speed_hz      = m_speed;
	    msg[0].bits_per_word = m_bits;

		msg[1].tx_buf        = (unsigned long)0;
	    msg[1].rx_buf        = (unsigned long)m_rx;
	    msg[1].len           = resp_size+32;
	    msg[1].delay_usecs   = 0;
	    msg[1].speed_hz      = m_speed;
	    msg[1].bits_per_word = m_bits;
	    res = ioctl(m_hspi, SPI_IOC_MESSAGE(2), msg); // perform duplex transfer

		if (res < 0)
		{
			PRN_ERROR("SPIMGR2: error to write the data, rc:%d\n", res);
			return RC_SPIMGR_DEV_ERROR;
		}	    

		// if CTS (Clear-to-send) bit is set
		if ((m_rx[0] & (1<<7)) == 0)
		{
			usleep(1000);
			num ++;
			continue;
		}
		break;
	}

	if (num > 100)
	{
		PRN_ERROR("SPI: timeout to wait for the STATUS field\n");
		return RC_SPIMGR_TIMEOUT_ERROR;
	}

	if (m_dump)
		dumpdata("[command]", m_tx, 1+cmd_size);

	if (m_dump)
		dumpdata("[responce]", m_rx, 64);

	// let's analyze the STATUS 
	// if HW error = Host attempted to send a command when serial port wasn't ready
	// Watchdog timeout occurred
	// SPI speed is too fast
	if (m_rx[0] & (1<<6))
		return RC_SPIMGR_HW_ERROR;

	// let's analyze the STATUS 
	// API error
	// Either a command was sent while CTS was low or the command is not a valid command in this mode,
	// had an invalid argument, or is otherwise not allowed.
	// See READ_REPLY for a description of what error code in byte 2 represents unless it was caused by sending a command while CTS was low
	if (m_rx[0] & (1<<5))
	{
		if (api_err)
			memcpy(api_err, m_rx, 3);
		return RC_SPIMGR_API_ERROR;
	}

	// let's analyze the STATUS
	// FW error
	// A problem has occurred. Please check NVM_STATUS for possible causes.
	if (m_rx[0] & (1<<4))
		return RC_SPIMGR_FW_ERROR;

	memcpy(presp, m_rx, resp_size);
	return RC_OK;
}

int class_spimgr::send_cmd(void* pcmd, int cmd_size)
{
	if (m_hspi < 0)
		return RC_SPIMGR_DEV_ERROR;

	struct spi_ioc_transfer msg[1] = {0};
	memset(&msg, 0, sizeof(msg));

	m_tx[0] = 0xC0;
	memcpy(m_tx+1, pcmd, cmd_size);

	msg[0].tx_buf        = (unsigned long)m_tx;
    msg[0].rx_buf        = (unsigned long)0;
    msg[0].len           = 1+cmd_size;
    msg[0].delay_usecs   = 0;
    msg[0].speed_hz      = m_speed;
    msg[0].bits_per_word = m_bits;

	int res = ioctl(m_hspi, SPI_IOC_MESSAGE(1), msg); // perform duplex transfer
	if (res < 0)
	{
		PRN_ERROR("SPIMGR: error to write the data, rc:%d\n", res);
		return RC_SPIMGR_DEV_ERROR;
	}
	return RC_OK;
}

int class_spimgr::recv_resp(void*presp, int resp_size, void* api_err)
{
	struct spi_ioc_transfer msg[2] = {0};
	memset(&msg, 0, sizeof(msg));
	int num = 0;
	int res = 0;
	while (num < 100)
	{
		memset(&msg, 0, sizeof(msg));

		// to request the responce for the forwarded command
		// -------------------------------------------------
		m_tx[0] = 0xD0;

	    msg[0].tx_buf        = (unsigned long)m_tx;
	    msg[0].rx_buf        = (unsigned long)0;
	    msg[0].len           = 1;
	    msg[0].delay_usecs   = 0;
	    msg[0].speed_hz      = m_speed;
	    msg[0].bits_per_word = m_bits;

		msg[1].tx_buf        = (unsigned long)0;
	    msg[1].rx_buf        = (unsigned long)m_rx;
	    msg[1].len           = resp_size+32;
	    msg[1].delay_usecs   = 0;
	    msg[1].speed_hz      = m_speed;
	    msg[1].bits_per_word = m_bits;
	    res = ioctl(m_hspi, SPI_IOC_MESSAGE(2), msg); // perform duplex transfer

		if (res < 0)
		{
			PRN_ERROR("SPIMGR2: error to write the data, rc:%d\n", res);
			return RC_SPIMGR_DEV_ERROR;
		}	    

		// if CTS (Clear-to-send) bit is set
		if ((m_rx[0] & (1<<7)) == 0)
		{
			usleep(1000);
			num ++;
			continue;
		}
		break;
	}

	if (num > 100)
	{
		PRN_ERROR("SPI: timeout to wait for the STATUS field\n");
		return RC_SPIMGR_TIMEOUT_ERROR;
	}

	if (m_dump)
		dumpdata("[command]", m_tx, 1);

	if (m_dump)
		dumpdata("[responce]", m_rx, 64);

	// let's analyze the STATUS 
	// if HW error = Host attempted to send a command when serial port wasn't ready
	// Watchdog timeout occurred
	// SPI speed is too fast
	if (m_rx[0] & (1<<6))
		return RC_SPIMGR_HW_ERROR;

	// let's analyze the STATUS
	// API error
	// Either a command was sent while CTS was low or the command is not a valid command in this mode,
	// had an invalid argument, or is otherwise not allowed.
	// See READ_REPLY for a description of what error code in byte 2 represents unless it was caused by sending a command while CTS was low
	if (m_rx[0] & (1<<5))
	{
		if (api_err)
			memcpy(api_err, m_rx, 3);
		return RC_SPIMGR_API_ERROR;
	}

	// let's analyze the STATUS
	// FW error
	// A problem has occurred. Please check NVM_STATUS for possible causes.
	if (m_rx[0] & (1<<4))
		return RC_SPIMGR_FW_ERROR;

	memcpy(presp, m_rx, resp_size);
	return RC_OK;
}

int class_spimgr::dumpena(int ena)
{
	m_dump = ena;
	return RC_OK;
}

int class_spimgr::dumpdata(const char* msg, void* pdata, int size)
{
	unsigned char* d = (unsigned char*)pdata;
    PRN_INFO("SPI message dump for %s:\n", msg);
    for (int i = 0; i < size; i++)
    {
        if (i%32 == 0)
            printf("0x%04x ", i);

        printf("%02x ", d[i]);

        if (i && ((i+1)%32) == 0)
            printf("\n");
    }
    printf("\n");
	return 0;
}
