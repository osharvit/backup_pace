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
#define RC_PARAM_ERROR                                      (-102)
#define RC_REG_SIGNAL_ERROR                                 (-103)
#define RC_REG_TIMER_ERROR                                  (-104)
#define RC_CREATE_THREAD_ERROR                              (-105)
#define RC_ALLOC_ERROR                                      (-106)
#define RC_MGR_ERROR                                        (-107)
#define RC_GET_LUA_MGR_ERROR                                (-108)
#define RC_CREATE_PROC_ERROR                                (-109)
#define RC_PROC_WAIT_ERROR                                  (-110)

#define RC_LEXER_PARAM_ERROR                                (-1001)
#define RC_LEXER_INIT_SYMBS_ERROR                           (-1002)
#define RC_LEXER_INIT_TEXT_ERROR                            (-1003)

#define RC_VARMGR_SYNTAX_ERROR                              (-2001)
#define RC_VARMGR_STATE_ERROR                               (-2002)
#define RC_VARMGR_DOUBLE_DEF                                (-2003)
#define RC_VARMGR_PARAM_ERROR                               (-2004)
#define RC_VARMGR_ALLOC_ERROR                               (-2005)
#define RC_VARMGR_READFILE_ERROR                            (-2006)
#define RC_VARMGR_OPENFILE_ERROR                            (-2007)
#define RC_VARMGR_WRITEFILE_ERROR                           (-2008)
#define RC_VARMGR_SYSVAR_PROTECTED                          (-2009)

#define RC_CMD_PARAM_ERROR                                  (-3001)
#define RC_CMD_BITS_ERROR                                   (-3002)

#define RC_CMDMGR_CREATECMD_ERROR                           (-4001)
#define RC_CMDMGR_PARSCMD_ERROR                             (-4002)
#define RC_CMDMGR_ALLOCCMD_ERROR                            (-4003)
#define RC_CMDMGR_REGCMDPARS_ERROR                          (-4004)
#define RC_CMDMGR_SETCMDPARS_ERROR                          (-4005)
#define RC_CMDMGR_PRNPARAM_ERROR                            (-4006)
#define RC_CMDMGR_DUMPPARAM_ERROR                           (-4007)
#define RC_CMDMGR_UNSUPPORTED_ERROR                         (-4008)
#define RC_CMDMGR_DOWNLOAD_PARAM_ERROR                      (-4009)
#define RC_CMDMGR_UPLOADLOAD_PARAM_ERROR                    (-4010)
#define RC_CMDMGR_ALLOC_NETCMD_ERROR                        (-4011)
#define RC_CMDMGR_UNKNOWN_ERROR                             (-4012)
#define RC_CMDMGR_ADDR_ERROR                                (-4014)
#define RC_CMDMGR_SYSVAR_USAGE_ERROR                        (-4015)
#define RC_CMDMGR_PARAM_NUM_ERROR                           (-4016)
#define RC_CMDMGR_BITS_NUM_ERROR                            (-4017)
#define RC_CMDMGR_READ_FILE_ERROR                           (-4018)
#define RC_CMDMGR_PARAM_ERROR                               (-4019)
#define RC_CMDMGR_NAME_ERROR                                (-4020)
#define RC_CMDMGR_ALLOC_BUF_ERROR                           (-4021)
#define RC_CMDMGR_SIZE_ERROR                                (-4022)
#define RC_CMDMGR_WRITE_PARAM_ERROR                         (-4023)
#define RC_CMDMGR_CALC_ERROR                                (-4024)
#define RC_CMDMGR_FUNC_NAME_ERROR                           (-4025)
#define RC_CMDMGR_REG_NAME_ERROR                            (-4026)
#define RC_CMDMGR_NOT_AXIS_FIFO_REGION                      (-4027)
#define RC_CMDMGR_AXIS_FIFO_UNKNOWN_OPR                     (-4028)
#define RC_CMDMGR_OPEN_FILE_ERROR                           (-4029)
#define RC_CMDMGR_CHDIR_ERROR                               (-4030)

#define RC_MATH_FORMAT_ERROR                                (-5001)
#define RC_MATH_OPERATION_ERROR                             (-5002)
#define RC_MATH_VARS_ACCESS_ERROR                           (-5003)
#define RC_MATH_VARS_NAME_ERROR                             (-5004)

#define RC_CLICP_PARAM_ERROR                                (-6001)
#define RC_CLICP_ALLOC_ERROR                                (-6002)
#define RC_CLICP_IP_ERROR                                   (-6003)
#define RC_CLICP_SOCKET_ERROR                               (-6004)
#define RC_CLICP_CONNECTION_ERROR                           (-6005)
#define RC_CLICP_RECV_CMD_ERROR                             (-6006)
#define RC_CLICP_UNKNOWN_CMD                                (-6007)
#define RC_CLICP_SEND_CMD_ERROR                             (-6008)
#define RC_CLICP_RECV_ACK_ERROR                             (-6009)

#define RC_SOCKET_CREATE_ERROR                              (-7001)
#define RC_SOCKET_ERROR                                     (-7002)
#define RC_SOCKET_CONNECT_ERROR                             (-7003)
#define RC_SOCKET_SEND_ERROR                                (-7004)
#define RC_SOCKET_RECV_ERROR                                (-7005)
#define RC_SOCKET_IP_ERROR                                  (-7006)
#define RC_SOCKET_LISTEN_ERROR                              (-7007)

#define RC_EXTCMDMGR_NO_FILE                                (-8001)
#define RC_EXTCMDMGR_PARAM_ERROR                            (-8002)
#define RC_EXTCMDMGR_PARSER_ERROR                           (-8003)
#define RC_EXTCMDMGR_ALLOC_ERROR                            (-8004)

#define RC_FILE_OPEN_ERROR                                  (-9001)
#define RC_FILE_ALLOC_ERROR                                 (-9002)
#define RC_FILE_READ_ERROR                                  (-9003)
#define RC_FILE_WRITE_ERROR                                 (-9004)

#define RC_JSON_PARAM_ERROR                                 (-10001)
#define RC_JSON_FORMAT_ERROR                                (-10002)
#define RC_JSON_ALLOC_OBJ_ERROR                             (-10003)

#define RC_HWMGR_NO_FILE                                    (-11001)
#define RC_HWMGR_READFILE_ERROR                             (-11002)
#define RC_HWMGR_PARSER_ERROR                               (-11003)

#define RC_LUAMGR_INIT_ERROR                                (-12001)
#define RC_LUAMGR_NAME_ERROR                                (-12002)
#define RC_LUAMGR_SCR_LOAD_ERROR                            (-12003)
#define RC_LUAMGR_ALLOC_STATE_ERROR                         (-12004)
#define RC_LUAMGR_CUR_DIR_ERROR                             (-12005)

#define RC_H2_THREAD_CREATE_ERROR                           (-14001)
#define RC_H2_READ_SOCKET_ERROR                             (-14002)

#define RC_LUA_SOCKET_TYPE_ERROR                            (-15001)
#define RC_LUA_SOCKET_CREATE_ERROR                          (-15002)
#define RC_LUA_SOCKET_HANDLER_ERROR                         (-15003)
#define RC_LUA_SOCKET_BIND_ERROR                            (-15004)
#define RC_LUA_SOCKET_LISTEN_ERROR                          (-15005)
#define RC_LUA_SOCKET_ACCEPT_ERROR                          (-15006)

#define RC_THREAD_CREATE_ERROR                              (-16001)

#define RC_REMOTE_CTRL_ALLOC_ERROR                          (-17001)
#define RC_REMOTE_CTRL_RECV_ERROR                           (-17002)
#define RC_REMOTE_CTRL_SEND_ERROR                           (-17003)

#define RC_LUA_IPC_CONNECT_ERROR                            (-18000)
#define RC_LUA_IPC_CREATE_PARAM_ERROR                       (-18001)
#define RC_LUA_IPC_PARAM_TYPE_ERROR                         (-18002)

#define RC_IPC_MGR_GENERAL_ERROR                            (-19000)
#define RC_IPC_MGR_PARAM_ERROR                              (-19001)

#endif //_RET_CODES_H_