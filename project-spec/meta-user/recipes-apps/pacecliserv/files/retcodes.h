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

#ifndef _RET_CODES_H_
#define _RET_CODES_H_

#define RC_OK                                               0

#define RC_CMD_LIMIT                                        (-100)
#define RC_OPEN_FILE_ERROR                                  (-101)
#define RC_UNKNOWN_PARAMETER                                (-102)
#define RC_BUS_ERROR                                        (-103)

#define RC_CONN_MGR_CREATE_SERV_SOCKET_ERROR                (-1001)
#define RC_CONN_MGR_BIND_SERV_SOCKET_ERROR                  (-1002)
#define RC_CONN_MGR_LISTEN_SERV_SOCKET_ERROR                (-1003)

#define RC_CLIENT_MGR_CREATE_ERROR                          (-2001)

#define RC_CLIENT_INIT_ATTR_ERROR                           (-3001)
#define RC_CLIENT_THREAD_CREATE_ERROR                       (-3002)
#define RC_CLIENT_SEND_CMD_ERROR                            (-3003)
#define RC_CLIENT_RECV_CMD_ERROR                            (-3004)
#define RC_CLIENT_UNKNOWN_CMD                               (-3005)
#define RC_CLIENT_ACK_ALLOC_ERROR                           (-3006)
#define RC_CLIENT_NUM_BITS_ERROR                            (-3007)
#define RC_CLIENT_DEV_OPEN_ERROR                            (-3008)
#define RC_CLIENT_MAP_ERROR                                 (-3009)
#define RC_CLIENT_ALLOC_ERROR                               (-3010)
#define RC_CLIENT_UNSUPPORTED_DRVID                         (-3011)
#define RC_CLIENT_OPEN_ERROR                                (-3012)
#define RC_CLIENT_FIFO_OPERATION_ERROR                      (-3014)

#define RC_SOCKET_CREATE_ERROR                              (-7001)
#define RC_SOCKET_ERROR                                     (-7002)
#define RC_SOCKET_CONNECT_ERROR                             (-7003)
#define RC_SOCKET_SEND_ERROR                                (-7004)
#define RC_SOCKET_RECV_ERROR                                (-7005)
#define RC_SOCKET_IP_ERROR                                  (-7006)

#endif //_RET_CODES_H_