/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class handles CLI client by using connected class_socket
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <pthread.h>
#include "socket.h"
#include "retcodes.h"
#include "gen-types.h"
#include <vector>

/**
    @brief This class defines the native CLI command

*/

enum NATIVE_CMD_ID
{
    NCID_REG_SET         =   1,
    NCID_REG_SET_ACK     =   101,
    NCID_REG_GET         =   2,
    NCID_REG_GET_ACK     =   102,
    NCID_REG_OR          =   3,
    NCID_REG_OR_ACK      =   103,
    NCID_REG_XOR         =   4,
    NCID_REG_XOR_ACK     =   104,
    NCID_REG_AND         =   5,
    NCID_REG_AND_ACK     =   105,
    NCID_SLEEP           =   6,
    NCID_PRINT           =   7,
    NCID_DUMP            =   8,
    NCID_DUMP_ACK        =   108,
    NCID_UPLOAD          =   9,
    NCID_UPLOAD_ACK      =   109,
    NCID_DOWNLOAD        =   10,
    NCID_DOWNLOAD_ACK    =   110,
    NCID_SET_VAR         =   11,
    NCID_KILL_VAR        =   12,        // This command is the special case of SET command with var name only
    NCID_VAR_LIST        =   14,        // This command is the special case of SET command without parameters
    NCID_HELP            =   15,
    NCID_LS              =   16,
    NCID_CD              =   17,
    NCID_PWD             =   18,
    NCID_MATH            =   19,
    NCID_VERSION         =   20,
    NCID_MAP             =   21,
    NCID_MAP_ACK         =   22,
    NCID_SCHED           =   23,
    NCID_READ_DATA       =   24,
    NCID_READ_DATA_ACK   =   124,
    NCID_WRITE_DATA      =   25,
    NCID_WRITE_DATA_ACK  =   125,
    NCID_WRITE           =   26,
    NCID_CALC            =   27,
    NCID_GET_FILE        =   28,
    NCID_GET_FILE_ACK    =   128,
    NCID_FIFO_CTRL       =   29,
    NCID_FIFO_CTRL_ACK   =   129,
    NCID_EXIT            =   1000,

    NCID_EXT_CMD         =   10000,
    NCID_LUA_CMD         =   20000,
    
};

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
    uint8_t                 opt[1];             // it's a byte-array
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

struct map_region
{
    uint64_t            m_addr_begin;
    uint64_t            m_addr_end;

    void*               m_pmapped_mem;
    uint32_t            m_mapped_size;
};

struct axis_fifo
{
    int                 devfile;
    uint64_t            m_addr;
};

class class_client
{
    friend void* client_thread(void* param);

public:
                    class_client(void);
                    ~class_client(void);

                int create(class_socket* psocket);
                int close(void);
                int is_stopped(void);
                void set_stop(void);
protected:
                int send_version(void);
                int send_cmd(clicp_cmd* pcmd);
                int proc_cmd(void);

                int proc_reg_set(clicp_cmd* pcmd);
                int proc_reg_get(clicp_cmd* pcmd);
                int proc_reg_xor(clicp_cmd* pcmd);
                int proc_reg_or(clicp_cmd* pcmd);
                int proc_reg_and(clicp_cmd* pcmd);
                int proc_read_data(clicp_cmd* pcmd);
                int proc_write_data(clicp_cmd* pcmd);
                int proc_map(clicp_cmd* pcmd);
                int proc_map_axi_fifo(clicp_cmd* pcmd);
                int proc_get_file(clicp_cmd* pcmd);
                int proc_fifo_ctrl(clicp_cmd* pcmd);


                int proc_read_data_direct_map(clicp_cmd* pcmd);
                int proc_read_data_axis_fifo(clicp_cmd* pcmd);

                int proc_write_data_direct_map(clicp_cmd* pcmd);
                int proc_write_data_axis_fifo(clicp_cmd* pcmd);

                int is_hw_region_mapped(uint64_t addr, uint64_t len);
                const map_region* find_hw_mapped_region(uint64_t addr, uint64_t len = 8);
                const axis_fifo & find_axis_fifo_info(uint64_t addr);
                void* phys2virt(uint64_t addr, uint64_t len);
                int map_hw_region(uint64_t addr, uint64_t len, int prn = 0);
                int open_axis_fifo(uint64_t addr);
                int hw_reg_write(uint64_t addr, uint64_t val, uint32_t bits);
                int hw_reg_read(uint64_t addr, uint32_t bits, uint64_t& val);
private:
    pthread_t                   m_thread;
    int                         m_thread_created;
    int                         m_stopped;
    class_socket*               m_psocket;

    std::vector<map_region>     m_regions;
    std::vector<axis_fifo>      m_axis_fifo;

    int                         m_map_dev;
};

#endif //_CLIENT_H_
