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

#include <string.h>
#include "luasockets.h"

class_lua_sockets::class_lua_sockets()
{

}

class_lua_sockets::~class_lua_sockets()
{
	release_sockets();
}

int class_lua_sockets::init(void)
{
	release_sockets();
	return RC_OK;
}

int class_lua_sockets::socket(int type)
{
	if (type != 0 && type != 1)
		return RC_LUA_SOCKET_TYPE_ERROR;

	class_socket* psock = new class_socket();
	if (psock == NULL)
		return RC_LUA_SOCKET_CREATE_ERROR;

	int res = psock->create(type);
	if (res < 0)
	{
		delete (psock);
		return res;
	}

	m_sockets.push_back(psock);
	return psock->get_socket();
}

int class_lua_sockets::bind(int socket, const char* ip, int port)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	int res = psock->bind(ip, port);
	return res;
}

int class_lua_sockets::accept(int socket)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	struct sockaddr_in client_addr;
	socklen_t addrlen;

	memset(&client_addr, 0, sizeof(client_addr));
	addrlen = sizeof(client_addr);

	int client_sk = ::accept(psock->get_socket(), (struct sockaddr*)&client_addr, &addrlen);

	if (client_sk < 0)
		return RC_LUA_SOCKET_ACCEPT_ERROR;

	class_socket* pclient = new class_socket(client_sk);
	if (pclient == NULL)
	{
		close(client_sk);
		return RC_LUA_SOCKET_CREATE_ERROR;
	}

	m_sockets.push_back(pclient);
	return client_sk;
}

int class_lua_sockets::connect(int socket, const char* ip, int port)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	return psock->connect(ip, port);
}

int class_lua_sockets::send(int socket, void* pdata, int size)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	return psock->send(pdata, size);
}

int class_lua_sockets::recv(int socket, void* poutdata, int size, int wait_ms, int exact_size)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	int len;
	if (exact_size == 0)
	{
		if (wait_ms)
		{
			int offs = 0;
			int sub_len;
			while (size)
			{
				int res = psock->select(wait_ms);
				if (res != 1)
					break;

				sub_len = psock->recv_data((unsigned char*)poutdata+offs, size);
				len  += sub_len;
				size -= sub_len;
				offs += sub_len;
			}
		}
		else
		{
			len = psock->recv_data(poutdata, size);
		}
	}
	else
	{
		len = psock->recv(poutdata, size);
	}
	return len;
}

int class_lua_sockets::close(int socket)
{
	class_socket* psock = find_socket(socket);
	if (psock == NULL)
		return RC_LUA_SOCKET_HANDLER_ERROR;

	remove_socket(psock);
	psock->close();
	delete (psock);
	return RC_OK;
}

class_socket* class_lua_sockets::find_socket(int h)
{
	std::vector<class_socket*>::iterator i;
	for (i = m_sockets.begin(); i < m_sockets.end(); i++)
	{
		if ((*i)->get_socket() == h)
			return (*i);
	}

	return NULL;
}

int class_lua_sockets::remove_socket(class_socket* psock)
{
	std::vector<class_socket*>::iterator i;
	for (i = m_sockets.begin(); i < m_sockets.end(); i++)
	{
		if ((*i) == psock)
		{
			m_sockets.erase(i);
			return RC_OK;
		}
	}

	return RC_OK;
}

int class_lua_sockets::release_sockets(void)
{
	std::vector<class_socket*>::iterator i;
	for (i = m_sockets.begin(); i < m_sockets.end(); i++)
	{
		if ((*i) != NULL)
			delete (*i);
	}

	m_sockets.clear();
	return RC_OK;
}