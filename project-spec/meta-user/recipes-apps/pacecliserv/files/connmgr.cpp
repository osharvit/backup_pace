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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "connmgr.h"
#include "socket.h"

class_connection_mgr::class_connection_mgr(void)
{
	m_serv_socket = -1;
}

class_connection_mgr::~class_connection_mgr()
{
	close();
}

int class_connection_mgr::init(CLF_PARAMS_TYPE* p)
{
	sockaddr_in serv_addr;
	int r;
	int len;

	m_serv_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_serv_socket < 0)
	{
		return RC_CONN_MGR_CREATE_SERV_SOCKET_ERROR;
	}

	int enable = 1;
	if (::setsockopt(m_serv_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
    	PRN_ERROR("setsockopt(SO_REUSEADDR) failed");
	}

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(p->serv_port); 

	len = sizeof(sockaddr_in);
    if ((r=::bind(m_serv_socket, (struct sockaddr*)&serv_addr, len)) < 0)
    {
    	PRN_ERROR( "bind error: %s ", gai_strerror(r));
		return RC_CONN_MGR_BIND_SERV_SOCKET_ERROR;
    }

    if ((r = ::listen(m_serv_socket, 10)) < 0)
    {
    	PRN_ERROR( "listen error: %s ", gai_strerror(r));
		return RC_CONN_MGR_LISTEN_SERV_SOCKET_ERROR;
    }

	return RC_OK;
}

class_socket* class_connection_mgr::listen(void)
{
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	if (m_serv_socket < 0)
		return NULL;

	memset(&client_addr, 0, sizeof(client_addr));
	addrlen = sizeof(client_addr);

	int client_sk = accept(m_serv_socket, (struct sockaddr*)&client_addr, &addrlen);

	if (client_sk < 0)
		return NULL;

	class_socket* psock = new class_socket(client_sk);
	if (psock != NULL)
	{
		psock->set_ipv4_address(&client_addr, addrlen);
	}
	return psock;
}

void class_connection_mgr::close(void)
{
	if (m_serv_socket != -1)
	{
		::close(m_serv_socket);
		m_serv_socket = -1;
	}
}