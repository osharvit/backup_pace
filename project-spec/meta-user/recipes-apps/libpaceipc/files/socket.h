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


#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "ipc_base.h"
#include "ipclib.h"
#include "ipc_ret.h"

int sockets_init(IPC_SOCKET_INIT_PARAM*pcfg, IPC_HANDLE*phandle);
int sockets_deinit(IPC_HANDLE*phandle);

int sockets_accept(IPC_HANDLE* phandle);
int sockets_handle(IPC_HANDLE* phandle);
int sockets_client_close(IPC_HANDLE* phandle, int h);

int sockets_read(IPC_HANDLE* phandle, int h, void*pdata, int size);
int sockets_write(IPC_HANDLE* phandle, int h, void*pdata, int size);

int ipc_socket_get_port(IPC_HANDLE* phandle);

#endif // _SOCKET_H_
