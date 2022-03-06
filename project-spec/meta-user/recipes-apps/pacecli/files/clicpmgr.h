/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This file implements the class: CLI control protocol
 *      this a command protocol used to communicate CLI-frontend and CLI-serv
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

#ifndef _CLICPMGR_H_
#define _CLICPMGR_H_

#include "retcodes.h"
#include <string>
#include <vector>
#include "varmgr.h"
#include "cmd.h"
#include "socket.h"
#include "gen-types.h"

struct clicpi_cmd_version
{
    unsigned int            version;
    unsigned int            res[2];
};

struct clicpi_cmd_map
{
    uint64_t                address;
    uint64_t                size;
};

struct clicpi_cmd_map_ack
{
    uint32_t                res;
};

struct clicpi_cmd_reg_set
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint64_t                value;              // up to 64-bits value (LE)
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_reg_set_ack
{
    int32_t                 res;
};

struct clicpi_cmd_reg_get
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint64_t                value;              // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_reg_get_ack
{
    uint64_t                value;              // 64-bits address (there is 64-bits platform) (LE)
};

struct clicpi_cmd_reg_and
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint64_t                value;              // up to 64-bits value (LE)
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_reg_and_ack
{
    uint64_t                read_val;
    uint64_t                write_val;
};

struct clicpi_cmd_reg_xor
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint64_t                value;              // up to 64-bits value (LE)
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage

    uint64_t                read_val;
    uint64_t                write_val;
};

struct clicpi_cmd_reg_xor_ack
{
    uint64_t                read_val;
    uint64_t                write_val;
};

struct clicpi_cmd_reg_or
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint64_t                value;              // up to 64-bits value (LE)
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_reg_or_ack
{
    uint64_t                read_val;
    uint64_t                write_val;
};

struct clicpi_cmd_dump
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                len;
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_dump_ack
{
    uint8_t                 opt[1];             // it's reserved for the future usage
};

struct clicpi_cmd_download
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                len;
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_download_ack
{
    uint8_t                 opt[1];             // it's a byte-array
};

struct clicpi_cmd_upload
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                len;
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
    uint8_t                 data[1];
};

struct clicpi_cmd_upload_ack
{
    uint8_t                 opt[1];             // it's a byte-array
};

struct clicpi_cmd_read_data
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                len;
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
};

struct clicpi_cmd_read_data_ack
{
    uint8_t                 opt[1];             // it's a byte-array
};

struct clicpi_cmd_write_data
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                len;
    uint32_t                bits;               // The bits
    uint32_t                opt[4];             // it's reserved for the future usage
    uint8_t                 data[1];
};

struct clicpi_cmd_write_data_ack
{
    uint8_t                 opt[1];             // it's a byte-array
};

struct clicpi_cmd_get_file
{
    uint8_t                 filename[1];
};

struct clicpi_cmd_get_file_ack
{
    uint8_t                 data[1];             // it's a byte-array (file data)
};

enum AXIS_FIFO_CTRL_OPERATIONS
{
    AXIS_FIFO_CTRL_RESET        =   1
};

struct clicpi_cmd_fifo_ctrl
{
    uint64_t                address;            // 64-bits address (there is 64-bits platform) (LE)
    uint32_t                operation;          // see: AXIS_FIFO_CTRL_OPERATIONS type
};

struct clicpi_cmd_fifo_ctrl_ack
{
    uint8_t                 data[1];             // it's a byte-array (file data)
};

union clicp_cmd_params
{
    clicpi_cmd_version          version;
    clicpi_cmd_map              map;
    clicpi_cmd_map_ack          map_ack;
    clicpi_cmd_reg_set          reg_set;
    clicpi_cmd_reg_set_ack      reg_set_ack;
    clicpi_cmd_reg_get          reg_get;
    clicpi_cmd_reg_get_ack      reg_get_ack;
    clicpi_cmd_reg_or           reg_or;
    clicpi_cmd_reg_or_ack       reg_or_ack;
    clicpi_cmd_reg_xor          reg_xor;
    clicpi_cmd_reg_xor_ack      reg_xor_ack;
    clicpi_cmd_reg_and          reg_and;
    clicpi_cmd_reg_and_ack      reg_and_ack;

    clicpi_cmd_dump             dump;
    clicpi_cmd_dump_ack         dump_ack;
    clicpi_cmd_download         download;
    clicpi_cmd_download_ack     download_ack;
    clicpi_cmd_upload           upload;
    clicpi_cmd_upload_ack       upload_ack;

    clicpi_cmd_read_data        readdata;
    clicpi_cmd_read_data_ack    readdata_ack;
    clicpi_cmd_write_data       writedata;
    clicpi_cmd_write_data_ack   writedata_ack;

    clicpi_cmd_get_file         get_file;
    clicpi_cmd_get_file_ack     get_file_ack;

    clicpi_cmd_fifo_ctrl        fifo_ctrl;
    clicpi_cmd_fifo_ctrl_ack    fifo_ctrl_ack;
};

#define CMD_MAIN_HDR_SIZE()     ((unsigned long)&((clicp_cmd*)NULL)->params)
#define CMD_SIZE(cmd)           (CMD_MAIN_HDR_SIZE() + (cmd)->cmd_param_len)
#define CMD_INIT(id, cmd, type) { memset((cmd), 0, sizeof(*(cmd))); (cmd)->cmd_id = (id); (cmd)->cmd_param_len = sizeof(type); }

struct clicp_cmd
{
    uint32_t            cmd_id;         // See the commands IDs
    uint32_t            cmd_param_len;  // The command parameters len, in bytes
    uint32_t            cmd_drv_id;     // The driver, that needs to handle such command, CLI serv needs to know about the driver
    uint32_t            cmd_uniq_idx;   // The uniqueue command identifier
    int32_t             res_code;       // The result of operation (CLI::serv -> CLI::frontend)
    uint32_t            reserved;       // reserved to have alignment for 64bits variables!!!

    clicp_cmd_params    params;
};

class class_clicpimgr
{
public:
                                class_clicpimgr();
                                ~class_clicpimgr();

    int                         connect(const char* ip = DEF_SERV_IP, int port = DEF_SERV_PORT);
    int                         get_version(int& version);

    clicp_cmd*                  alloc_cmd(int cmdid, int drvid, unsigned int extra_size = 0);
    int                         send_cmd(clicp_cmd* pclicmd, clicp_cmd*& pclicmd_ack);
    clicp_cmd*                  recv_cmd(void);

protected:

    unsigned int                m_cmd_trans_id;
    class_socket                m_socket;
};

#endif //_CLICPMGR_H_
