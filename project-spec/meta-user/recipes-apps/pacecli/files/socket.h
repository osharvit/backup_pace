/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This file implements TCP socket API
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

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "retcodes.h"

#define SOCKET_TCP                  0
#define SOCKET_UDP                  1

class class_socket
{
public:
                class_socket(void);
                class_socket(int sk, int type = SOCKET_TCP);
                ~class_socket(void);

                int create(int type = 0);
                int reuse(int ena);
                int bind(const char * ip, int port);
                int connect(const char * ip, int port);
                int listen(int num);
                class_socket* accept(void);

                int send(void* pdata, int len);
                int recv(void* pdata, int len);
                int recv_data(void* pdata, int len);
                int select(unsigned int ms);

                int is_connected(void);

                int             set_ipv4_address(struct sockaddr_in* in, socklen_t len);
                const char*     get_ipv4_address(void);
                unsigned int    get_port(void);

                void            close(void);
                int             get_socket(void);
protected:


private:

        int                     m_socket;
        int                     m_type;         // 0 - tcp, 1 - udp
        int                     m_connected;

        struct sockaddr_in      m_conn_addr;
};


#endif //_SOCKET_H_

