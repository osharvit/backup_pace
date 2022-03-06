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
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include "socket.h"
#include "gen-types.h"

class_socket::class_socket(void)
{
	m_type = SOCKET_TCP;
	m_socket = -1;
	m_connected = 0;
	memset (&m_conn_addr, 0, sizeof(m_conn_addr));
}

class_socket::class_socket(int sk, int type)
{
	m_type = type;
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
	if ((m_socket = socket(AF_INET, type == 0 ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0)
	{
	    PRN_ERROR("open socket error, sock:%d\n", m_socket);
	    return RC_SOCKET_CREATE_ERROR;
	}

	m_type = type;
	return RC_OK;
}

int class_socket::reuse(int ena)
{
	int enable = ena;
	if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
	    PRN_ERROR("setsockopt(SO_REUSEADDR) failed");
	}
	return RC_OK;
}

int class_socket::bind(const char * ip, int port)
{
	sockaddr_in this_addr;
	int r;
	int len;

	if (m_type == SOCKET_TCP)
	{
		int enable = 1;
		if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		{
	    	PRN_ERROR("setsockopt(SO_REUSEADDR) failed");
		}
	}

	memset(&this_addr, 0, sizeof(this_addr));

	this_addr.sin_family = AF_INET;
	this_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, ip, &this_addr.sin_addr)<=0)
	{
	    PRN_ERROR("IP address error\n");
	    return RC_SOCKET_IP_ERROR;
	}

	len = sizeof(sockaddr_in);
	if ((r=::bind(m_socket, (struct sockaddr*)&this_addr, len)) < 0)
	{
	    PRN_ERROR( "bind error: %s ", gai_strerror(r));
	    return RC_LUA_SOCKET_BIND_ERROR;
	}

	if (m_type == SOCKET_TCP)
	{
	    if ((r = ::listen(m_socket, 10)) < 0)
	    {
		PRN_ERROR( "listen error: %s ", gai_strerror(r));
			return RC_LUA_SOCKET_LISTEN_ERROR;
	    }
	}

	return RC_OK;
}

int class_socket::connect(const char * ip, int port)
{
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	m_connected = 0;

	//printf("ip:%s, port:%d\n", ip, port);

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

int class_socket::listen(int num)
{
	int rc;
	if ((rc = ::listen(m_socket, num)) < 0)
	{
	    PRN_ERROR( "listen error: %s ", gai_strerror(rc));
	    return RC_SOCKET_LISTEN_ERROR;
	}

	return RC_OK;
}

class_socket* class_socket::accept(void)
{
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	if (m_socket < 0)
		return NULL;

	memset(&client_addr, 0, sizeof(client_addr));
	addrlen = sizeof(client_addr);

	int client_sk = ::accept(m_socket, (struct sockaddr*)&client_addr, &addrlen);

	if (client_sk < 0)
		return NULL;

	class_socket* psock = new class_socket(client_sk);
	if (psock != NULL)
	{
		psock->set_ipv4_address(&client_addr, addrlen);
	}
	return psock;
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

int class_socket::select(unsigned int ms)
{
	struct timeval time;
	fd_set rdset;

	FD_ZERO(&rdset);
	FD_SET(m_socket, &rdset);

	time.tv_sec  = ms / 1000;
	time.tv_usec = (ms % 1000) * 1000;  // to convert ms into microseconds
	
	int res = ::select(m_socket+1, &rdset, NULL, NULL, &time);
	if (res < 0)
		return res;
	if (FD_ISSET(m_socket, &rdset))
		return 1;
	return 0;
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

int class_socket::get_socket(void)
{
	return m_socket;
}