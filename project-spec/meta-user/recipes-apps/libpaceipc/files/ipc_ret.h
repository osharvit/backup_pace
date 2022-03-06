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


#ifndef _IPC_RET_H_
#define _IPC_RET_H_

#define RC_OK                                   0
#define RC_API_REQUEST                          1

#define RC_IPC_HANDLE_ALLOC_ERROR               (-100)
#define RC_IPC_OBJ_TYPE_ERROR                   (-101)
#define RC_IPC_PARAM_ERROR                      (-102)
#define RC_IPC_API_TABLE_ERROR                  (-103)
#define RC_IPC_API_TABLE_EMPTY                  (-104)
#define RC_IPC_GETTING_TABLE_ERROR              (-105)
#define RC_IPC_API_ID_ERROR                     (-106)
#define RC_IPC_API_CREATE_REQ_ERROR             (-106)
#define RC_IPC_API_NULL_PARAM_ERROR             (-107)
#define RC_IPC_API_ALLOC_ERROR                  (-108)
#define RC_IPC_API_INCORRECT_PARAMS             (-109)
#define RC_IPC_CREATE_DAEMON_ERROR              (-110)

#define RC_SOCKET_ERROR_TO_CREATE               (-200)
#define RC_SOCKET_ERROR_SET_SOCK_OPT            (-201)
#define RC_SOCKET_IP_ERROR                      (-202)
#define RC_SOCKET_BIND_ERROR                    (-203)
#define RC_SOCKET_LISTEN_ERROR                  (-204)
#define RC_SOCKET_ACCEPT_ERROR                  (-205)
#define RC_SOCKET_ERROR                         (-206)
#define RC_SOCKET_RECV_ERROR                    (-207)
#define RC_SOCKET_SEND_ERROR                    (-208)
#define RC_SOCKET_CONNECT_ERROR                 (-209)
#define RC_SOCKET_GET_PORT_ERROR                (-210)

#define RC_THREAD_CREATE_ERROR                  (-300)
#define RC_THREAD_INIT_ATTR_ERROR               (-301)
#define RC_THREAD_INIT_POLICY_ERROR             (-302)
#define RC_THREAD_INIT_PRIO_ERROR               (-303)
#define RC_THREAD_AFFINITY_ERROR                (-304)

#define RC_PROTOCOL_ALLOC_ERROR                 (-400)
#define RC_PROTOCOL_READ_ERROR                  (-401)
#define RC_PROTOCOL_UNKNOWN_CMD                 (-402)
#define RC_PROTOCOL_TABLE_REQ_ERROR             (-403)
#define RC_PROTOCOL_PARAM_ERROR                 (-404)
#define RC_PROTOCOL_WRITE_DATA_ERROR            (-405)
#define RC_PROTOCOL_API_CTX_ERROR               (-406)
#define RC_PROTOCOL_READ_RESP_ERROR             (-407)


#endif // _IPC_RET_H_

