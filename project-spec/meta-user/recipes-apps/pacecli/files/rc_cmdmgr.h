/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This is remote control command manager, the class that allows to communicate
 *    to the class_cmdmgr remotely
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


#ifndef _RC_CMD_MGR_H_
#define _RC_CMD_MGR_H_

#include <vector>
#include "socket.h"
#include "thread.h"
#include "gen-types.h"

class class_cmdmgr;
void* rc_cmdmgr_clients_handler(void* p);


struct rc_client
{
    class_socket*       m_client;
};


enum REMOTE_CND_IDS
{
    REMOTE_CMD_ID_RUN_LUA_SCRIPT  = 1,
    REMOTE_CMD_ID_RUN_CMD         = 2,

    REMOTE_CMD_ID_GET_REG         = 3,
    REMOTE_CMD_ID_SET_REG         = 4,
    REMOTE_CMD_ID_OR_REG          = 5,
    REMOTE_CMD_ID_XOR_REG         = 6,
    REMOTE_CMD_ID_AND_REG         = 7,

    REMOTE_CMD_ID_WRITE_DATA      = 8,
    REMOTE_CMD_ID_READ_DATA       = 9,

    REMOTE_CMD_ID_SAVE_FILE       = 10,
    REMOTE_CMD_ID_LOAD_FILE       = 11,

    REMOTE_CMD_ID_UNKNOWN_CMD     = 100
};


struct rc_cmd_header
{
    unsigned int        cmd_id;
    unsigned int        data_size;
}__attribute__((packed));

struct rc_cmd_get_reg
{
    unsigned long long  addr;
    unsigned int        bits;
}__attribute__((packed));
struct rc_cmd_get_reg_ack
{
    unsigned long long  value;
    int                 rc;
}__attribute__((packed));


struct rc_cmd_set_reg
{
    unsigned long long  addr;
    unsigned long long  value;
    unsigned int        bits;
}__attribute__((packed));
struct rc_cmd_set_reg_ack
{
    unsigned int        rc;
}__attribute__((packed));

struct rc_cmd_write_data
{
    unsigned long long  addr;
    unsigned int        bits;
}__attribute__((packed));

struct rc_cmd_write_data_ack
{
    int                 rc;
}__attribute__((packed));

struct rc_cmd_read_data
{
    unsigned long long  addr;
    unsigned int        size;
    unsigned int        bits;
}__attribute__((packed));

struct rc_cmd_read_data_ack
{
    int                 rc;
}__attribute__((packed));

struct rc_cmd_save_file
{
    unsigned long long  addr;
    unsigned int        size;
    unsigned int        bits;
    char                filename[1];
}__attribute__((packed));

struct rc_cmd_save_file_ack
{
    int                 rc;
}__attribute__((packed));

struct rc_cmd_load_file
{
    unsigned long long  addr;
    unsigned int        bits;
    char                filename[1];
}__attribute__((packed));

struct rc_cmd_load_file_ack
{
    int                 rc;
}__attribute__((packed));

class class_rccmdmgr
{

    friend class_cmdmgr;
    friend void* rc_cmdmgr_clients_handler(void* p);

public:
                                class_rccmdmgr();
                                ~class_rccmdmgr();

            int                 init(CLF_PARAMS_TYPE* p, class_cmdmgr* pmgr);

protected:

            int                 add_client(class_socket* pclient);
            int                 del_client(class_socket* pclient);
            void                del_all_clients(void);
            int                 get_client_num(void);
            int                 proc_client(class_socket* client);
            int                 send_resp(class_socket* client, unsigned int cmd_id, unsigned int data_size, const void* pdata);

private:

    class_cmdmgr*               m_pcmdmgr;
    class_socket                m_socket;           // The server socket, used to handle the clients
    class_thread                m_thread;

    std::vector<rc_client>      m_clients;
};


#endif // _RC_CMD_MGR_H_
