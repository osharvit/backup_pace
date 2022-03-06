/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This is sockets manager, it implements socket API used in LUA scripts
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

#ifndef _LUASOCKETS_H_
#define _LUASOCKETS_H_

#include <vector>
#include <string>
#include "gen-types.h"
#include "retcodes.h"
#include "socket.h"

class class_lua_sockets
{
public:
                    class_lua_sockets();
                    ~class_lua_sockets();


            int     init(void);

            int     socket(int type);
            int     bind(int socket, const char* ip, int port);
            int     accept(int socket);
            int     connect(int socket, const char* ip, int port);
            int     send(int socket, void* pdata, int size);
            int     recv(int socket, void* poutdata, int size, int wait_ms = 0, int exact_size = 0);
            int     close(int socket);

            class_socket*   find_socket(int h);
protected:

            int     remove_socket(class_socket* psock);
            int     release_sockets(void);

private:

    std::vector<class_socket*>       m_sockets;
};

#endif //_LUASOCKETS_H_