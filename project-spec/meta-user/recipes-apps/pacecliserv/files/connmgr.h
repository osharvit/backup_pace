/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  The connection manager listens to the TCP connection from CLI clients
 *  and creates the socket class
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

#ifndef _CONN_MGR_H_
#define _CONN_MGR_H_

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socket.h"
#include "gen-types.h"
#include "retcodes.h"

class class_connection_mgr
{
public:

                        class_connection_mgr();
                        ~class_connection_mgr();

                        int init(CLF_PARAMS_TYPE* p);

                        class_socket*   listen(void);
                        void            close(void);

protected:


private:
        int             m_serv_socket;
};

#endif // _CONN_MGR_H_
