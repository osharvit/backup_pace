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

#ifndef _SPI_MGR_H_
#define _SPI_MGR_H_

#include <vector>
#include <string>
#include "rc.h"
#include "spidev.h"

#define SPI_MAX_TX_BUF_SIZE         1024
#define SPI_MAX_RX_BUF_SIZE         10240

class class_spimgr
{
public:
        class_spimgr(void);
        ~class_spimgr(void);

    int init(const char* spidev, unsigned char mode, unsigned int speed, unsigned int bits);
    int close(void);

    int send_data(void* pcmd, int cmd_size, void*presp, int resp_size, void* api_err);
    int send_cmd(void* pcmd, int cmd_size);
    int recv_resp(void*presp, int resp_size, void* api_err = NULL);

    int dumpena(int ena);

protected:
    int dumpdata(const char* msg, void* pdata, int size);

    int             m_hspi;
    std::string     m_dev;
    unsigned int    m_bits;
    unsigned int    m_speed;
    unsigned char   m_mode;
    int             m_dump;

    unsigned char   m_tx[SPI_MAX_TX_BUF_SIZE];
    unsigned char   m_rx[SPI_MAX_RX_BUF_SIZE];
};

#endif // _SPI_MGR_H_