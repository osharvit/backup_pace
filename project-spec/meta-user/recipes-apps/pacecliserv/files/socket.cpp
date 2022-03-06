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

#include <string.h>
#include "socket.h"
#include "gen-types.h"

class_socket::class_socket(void)
{
	m_socket = -1;
	m_connected = 0;
	memset (&m_conn_addr, 0, sizeof(m_conn_addr));
}

class_socket::class_socket(int sk)
{
	m_socket = sk;
	m_connected = 1;
	memset (&m_conn_addr, 0, sizeof(m_conn_addr));
}

class_socket::~class_socket(void)
{
	close();
}

int class_socket::create(int type)
{
	close();
    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        PRN_ERROR("open socket error, sock:%d\n", m_socket);
        return RC_SOCKET_CREATE_ERROR;
    }

	return RC_OK;
}

int class_socket::connect(const char * ip, int port)
{
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	m_connected = 0;

    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
    {
        PRN_ERROR("IP address error\n");
        close();
        return RC_SOCKET_IP_ERROR;
    }

	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

	if (::connect(m_socket,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        close();
        return RC_SOCKET_CONNECT_ERROR;
    }

	m_connected = 1;
	return RC_OK;
}


int class_socket::send(void* pdata, int len)
{
	if (m_socket < 0)
		return RC_SOCKET_ERROR;

	unsigned int offs = 0;
	while (len > 0)
	{
		int tx_len = ::write(m_socket, (char*)pdata+offs, len);

		if (tx_len <= 0)
			return RC_SOCKET_SEND_ERROR;

		offs += tx_len;
		len -= tx_len;
	}

	return offs;
}

int class_socket::recv(void* pdata, int len)
{
	if (m_socket < 0)
		return RC_SOCKET_ERROR;

	unsigned int offs = 0;

	while (len > 0)
	{
		int rx_len = ::read(m_socket, (char*)pdata+offs, len);

		if (rx_len <= 0)
			return RC_SOCKET_RECV_ERROR;

		offs += rx_len;
		len -= rx_len;
	}

	return offs;
}

int class_socket::recv_data(void* pdata, int len)
{
	if (m_socket < 0)
		return RC_SOCKET_ERROR;

	int rx_len = ::read(m_socket, (char*)pdata, len);
	return rx_len;
}

int class_socket::is_connected(void)
{
	return m_connected;
}

int class_socket::set_ipv4_address(struct sockaddr_in* in, socklen_t len)
{
	memcpy(&m_conn_addr, in, len);
	return 0;
}

const char* class_socket::get_ipv4_address(void)
{
	static char buf[32];

	if (m_connected == 0)
		return "0.0.0.0";

	inet_ntop(AF_INET, &m_conn_addr.sin_addr, buf, sizeof(m_conn_addr));
	return buf;
}

unsigned int class_socket::get_port(void)
{
	if (m_connected == 0)
		return 0;

	return htons(m_conn_addr.sin_port);
}

void class_socket::close(void)
{
	if (m_socket != -1)
	{
		::shutdown(m_socket, SHUT_RDWR);
		::close(m_socket);
		m_socket = -1;
	}

	m_connected = 0;
}