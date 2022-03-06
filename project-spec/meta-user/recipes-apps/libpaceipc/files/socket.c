/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include "socket.h"

int sockets_init(IPC_SOCKET_INIT_PARAM*pcfg, IPC_HANDLE*phandle)
{
    struct sockaddr_in this_addr;
    int rc, enable, len;
    phandle->ipc_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (phandle->ipc_socket < 0)
        return RC_SOCKET_ERROR_TO_CREATE;

    if (phandle->ipc_object_type == IPC_OBJ_TYPE_SERVER)
    {
        enable = 1;
        rc = setsockopt(phandle->ipc_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        if (rc < 0)
            return RC_SOCKET_ERROR_SET_SOCK_OPT;

        memset(&this_addr, 0, sizeof(this_addr));

        this_addr.sin_family = AF_INET;
        this_addr.sin_port = htons(pcfg->tcp_port);
        if(inet_pton(AF_INET, pcfg->tcp_ip, &this_addr.sin_addr)<=0)
            return RC_SOCKET_IP_ERROR;

        len = sizeof(struct sockaddr_in);
        if ((rc = bind(phandle->ipc_socket, (struct sockaddr*)&this_addr, len)) < 0)
            return RC_SOCKET_BIND_ERROR;

        if ((rc = listen(phandle->ipc_socket, 10)) < 0)
            return RC_SOCKET_LISTEN_ERROR;
    }
    else if (phandle->ipc_object_type == IPC_OBJ_TYPE_CLIENT)
    {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));

        if(inet_pton(AF_INET, pcfg->tcp_ip, &serv_addr.sin_addr)<=0)
            return RC_SOCKET_IP_ERROR;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(pcfg->tcp_port);

        if (connect(phandle->ipc_socket,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
            return RC_SOCKET_CONNECT_ERROR;
    }
    return RC_OK;
}

int sockets_deinit(IPC_HANDLE*phandle)
{
    int i;
    if (phandle == NULL)
        return RC_OK;

    close(phandle->ipc_socket);
    phandle->ipc_socket = 0;

    for (i = 0; i < IPC_MAX_CLIENTS; i++)
    {
        if (phandle->clients.array[i].h != 0)
        {
            close(phandle->clients.array[i].h);
            phandle->clients.array[i].h = 0;
        }
    }
    phandle->clients.num = 0;
    return 0;
}

int sockets_accept(IPC_HANDLE* phandle)
{
    struct sockaddr_in client_addr;
    socklen_t addrlen;
    int client_sk, i;

    memset(&client_addr, 0, sizeof(client_addr));
    addrlen = sizeof(client_addr);

    client_sk = accept(phandle->ipc_socket, (struct sockaddr*)&client_addr, &addrlen);

    if (client_sk < 0)
        return RC_SOCKET_ACCEPT_ERROR;

    // To add new one socket to the array of the sockets
    // -------------------------------------------------
    if (phandle->clients.num >= IPC_MAX_CLIENTS)
    {
        close(client_sk);
        return RC_OK;
    }

    for(i = 0; i < IPC_MAX_CLIENTS; i++)
    {
        if (phandle->clients.array[i].h == 0)
        {
            phandle->clients.array[i].h = client_sk;
            phandle->clients.num++;
            break;
        }
    }
    return RC_OK;
}

int sockets_handle(IPC_HANDLE* phandle)
{
    int rc, i, num;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(phandle->ipc_socket, &set);

    num = (int)phandle->ipc_socket;

    for (i = 0; i < phandle->clients.num; i++)
    {
        FD_SET(phandle->clients.array[i].h, &set);
        num = (num < phandle->clients.array[i].h) ? phandle->clients.array[i].h : num;
    }

     if ((rc = select(num + 1, &set, NULL, NULL, NULL)) < 0)
        return rc;

    // Let's check what the system returned
    // -------------------------------------
    if (FD_ISSET(phandle->ipc_socket, &set))
    {
        rc = sockets_accept(phandle);
        if (rc < 0)
            return rc;
    }

    for (i = 0; i < phandle->clients.num; i++)
    {
        if (FD_ISSET(phandle->clients.array[i].h, &set))
            return phandle->clients.array[i].h;
    }

    return RC_OK;
}

int sockets_client_close(IPC_HANDLE* phandle, int h)
{
    int i;
    for (i = 0; i < phandle->clients.num; i++)
    {
        if (phandle->clients.array[i].h == h)
        {
            phandle->clients.array[i].h = 0;
            phandle->clients.num--;
            close(h);
            break;
        }
    }

    return RC_OK;
}

int sockets_read(IPC_HANDLE* phandle, int h, void*pdata, int size)
{
    unsigned int offs = 0;

    if (h < 0)
        return RC_SOCKET_ERROR;

    if (h == 0)
        h = phandle->ipc_socket;

    while (size > 0)
    {
        int rx_len = read(h, (char*)pdata+offs, size);
        if (rx_len <= 0)
            return RC_SOCKET_RECV_ERROR;

        offs += rx_len;
        size -= rx_len;
    }
    return offs;
}

int sockets_write(IPC_HANDLE* phandle, int h, void*pdata, int size)
{
    unsigned int offs = 0;

    if (h < 0)
        return RC_SOCKET_ERROR;

    if (h == 0)
        h = phandle->ipc_socket;

    while (size > 0)
    {
        int tx_len = write(h, (char*)pdata+offs, size);
        if (tx_len <= 0)
            return RC_SOCKET_SEND_ERROR;

        offs += tx_len;
        size -= tx_len;
    }
    return offs;
}

int ipc_socket_get_port(IPC_HANDLE* phandle)
{
    struct sockaddr my_addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    memset(&my_addr, 0, sizeof(my_addr));

    int rc = getsockname(phandle->ipc_socket, &my_addr, &addrlen);

    if (rc < 0)
        return RC_SOCKET_GET_PORT_ERROR;

    return htons(((struct sockaddr_in*)&my_addr)->sin_port);
}
