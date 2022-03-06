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

#ifndef _IPC_H_
#define _IPC_H_

#include <stdint.h>
#include <pthread.h>
#include "ipc_ret.h"
#include "ipclib.h"

#define IPC_MAJOR_VERSION               1
#define IPC_MINOR_VERSION               0

#define ipc_round8(val)                 (((val)+7)&~7)

typedef enum _IPC_OBJ_TYPE_
{
    IPC_OBJ_TYPE_SERVER     =   1,
    IPC_OBJ_TYPE_CLIENT     =   2

}IPC_OBJ_TYPE;

typedef struct _CLIENT_CTX_
{
    int     h;
    int     param;
}CLIENT_CTX;

typedef struct _CLIENTS_
{
    int             num;
    CLIENT_CTX      array[IPC_MAX_CLIENTS];
}CLIENTS;


typedef struct _IPC_HANDLE_
{
    int ipc_object_type;

    int (*transport_handle)(struct _IPC_HANDLE_*this);
    int (*transport_client_close)(struct _IPC_HANDLE_*this, int h);

    int (*transport_read)(struct _IPC_HANDLE_*this, int h, void*pdata, int size);
    int (*transport_write)(struct _IPC_HANDLE_*this, int h, void*pdata, int size);

    int             ipc_socket;
    pthread_t       ipc_thread;

    CLIENTS         clients;                    // The information about all clients connected to this IPC server

    int             serv_api_table_elm_num;     // The number of elements in the table 'serv_api_table' including 'ZERO' terminator
    IPC_API*        serv_api_table;             // The pointer to the SERVER API table (used by the CLIENT)

    IPC_API*        current_api_call;           // The pointer to the API currently called by the system or NULL
    void*           current_api_params;

    int             current_api_ret_type;
    int             current_api_ret_size;
    void*           current_api_ret_data;       // The pointer to the current API return value

    void*           client_call_resp_buf;

}IPC_HANDLE;

typedef struct _IPC_RETURN_
{
    int                         error_code;
    int                         ret_data_type;
    int                         ret_data_size;
    void*                       ret_data[];
}IPC_RETURN;

typedef struct _IPC_THREAD_INIT_PARAM_
{
    unsigned int        affinity;
    int                 priority;

}IPC_THREAD_INIT_PARAM;

typedef struct _IPC_SOCKET_INIT_PARAM_
{
    unsigned int        tcp_port;
    char                tcp_ip[64];
}IPC_SOCKET_INIT_PARAM;

typedef struct _IPC_INIT_PARAM_
{
    int                         transport_later_id; // The transport latyer ID, please see IPC_TRANSPORT_ID_xxx
    IPC_API*                    api_table;          // The pointer to the API table provided by this server

    IPC_THREAD_INIT_PARAM       threads_cfg;
    IPC_SOCKET_INIT_PARAM       sockets_cfg;
}IPC_INIT_PARAM;

typedef struct
{
    unsigned int    tcp_port;
    char            tcp_ip[64];
}IPC_SERVER_INFO;

#endif // _IPC_H_
