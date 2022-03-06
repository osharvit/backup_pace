/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager handles LUA IPC requests
 *    In this case LUA is an IPC client
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

#ifndef _LUA_IPC_MGR_H_
#define _LUA_IPC_MGR_H_

#include <stdint.h>
#include <vector>
#include <string>
#include "paceipc/ipclib.h"
#include "retcodes.h"

struct ipc_channel
{
    ipc_channel()
    {
        h_id = -1;
        h_ipc_channel = NULL;
        h_ipc_ret_val = NULL;
    }

    int     h_id;
    void*   h_ipc_channel;
    void*   h_ipc_ret_val;
};

struct lua_ipc_parameter
{
    lua_ipc_parameter()
    {
        ipc_data_type_id = IPC_APT_NONE;
        ipc_data.u64 = 0;
        ipc_data_size = 0;
    }

    int     ipc_data_type_id;
    int     ipc_data_size;
    union
    {
            void*       ipc_void_ptr;
            const char* ipc_str;
            uint64_t    u64;
            uint32_t    u32;
            uint16_t    u16;
            uint8_t     u8;
    } ipc_data;
};

struct lua_ipc_params
{
    std::vector<lua_ipc_parameter> list;
};

class class_lua_ipc_mgr
{
public:

                    class_lua_ipc_mgr(void);
                    ~class_lua_ipc_mgr(void);


                    int ipc_open(const char* ip, unsigned long port);
                    int ipc_close(int hipc);
                    int ipc_call(int hipc, int api_id, lua_ipc_params & params, lua_ipc_parameter& ret);
                    int ipc_call_done(unsigned int hipc);
protected:

                    int         get_free_id(void);
                    ipc_channel get_ipc_by_id(int id, int& idx);

private:

        int                             m_free_id;                  // The counter used to find the next free channel ID
        std::vector<ipc_channel>        m_cons;                     // The array of the connections
};


#endif // _LUA_IPC_MGR_H_