/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This file defines CLI-IPC 'CONTROL' interface API ID
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

#ifndef _CLI_IPC_CTRL_INF_H_
#define _CLI_IPC_CTRL_INF_H_

#define IPC_CLI_DEF_hLIB(hlib)      void* __cli_ipc_client_lib_handle__ = hlib
#ifdef __cplusplus
#define IPC_CLI_NAMESPACE   ::
#else
#define IPC_CLI_NAMESPACE
#endif


// This API prints the message to the CLI console
//    PARAMETERS:
//      [STR] - the ASCIIZ string to print to
//
//    RETURN
//      S32 - error code,  >=0 - OK
#define IPC_CLI_CTRL_INF_API_ID_PRINT          1
#define ipc_cli_api_print(msg)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_PRINT, msg)

// This API runs the CLI command or set of commands
// The output is printed to the CLI console
//    PARAMETERS:
//      [STR] - the ASCIIZ string: CLI commands:  native/extended/lua scripts
//
//    RETURN
//      S32 - error code,  >=0 - OK
#define IPC_CLI_CTRL_INF_API_ID_COMMAND        2
#define ipc_cli_api_command(cmd)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_COMMAND, cmd)

// This API sets HW register
//
//    PARAMETERS:
//      [U32] - 8 - 8Bits, 16 - 16Bits, 32 - 32Bits, 64 - 64Bits register type
//      [U64] - The register address
//      [U64] - The register value
//
//    RETURN
//      S32 - error code,  >=0 - OK

#define IPC_CLI_CTRL_INF_API_ID_SETREG        3
#define ipc_cli_api_setreg(bits, addr, val)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_SETREG, IPC_U32(bits), IPC_U64(addr), IPC_U64(val))

// This API gets HW register
//
//    PARAMETERS:
//      [U32]   - 8 - 8Bits, 16 - 16Bits, 32 - 32Bits, 64 - 64Bits register type
//      [U64]   - The register address
//      [U64*]  - [OUTPUT] - the register value
//
//    RETURN
//      S32 - error code,  >=0 - OK

#define IPC_CLI_CTRL_INF_API_ID_GETREG        4
#define ipc_cli_api_getreg(bits, addr, pval)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_GETREG, IPC_U32(bits), IPC_U64(addr), (unsigned long long*)pval)

// This API saves memory region (or AXIS-FIFO) to the file
//
//    PARAMETERS:
//      [U64]  - The memory region address
//      [U32]  - The size in bytes
//      [STR]  - The file name
//
//    RETURN
//      S32 - error code,  >=0 - OK

#define IPC_CLI_CTRL_INF_API_ID_SAVE2FILE      5
#define ipc_cli_api_save2file(addr, size, name)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_SAVE2FILE, IPC_U64(addr), IPC_U32(size), name)

// This API loads memory region (or AXIS-FIFO) from the file
//
//    PARAMETERS:
//      [STR]  - The file name
//      [U64]  - The memory region address
//
//    RETURN
//      S32 - error code,  >=0 - OK

#define IPC_CLI_CTRL_INF_API_ID_LOADFROMFILE   6
#define ipc_cli_api_loadfromfile(name, addr)  IPC_CLI_NAMESPACE ipc_call(__cli_ipc_client_lib_handle__, IPC_CLI_CTRL_INF_API_ID_LOADFROMFILE, name, IPC_U64(addr))

#endif // _CLI_IPC_CTRL_INF_H_

