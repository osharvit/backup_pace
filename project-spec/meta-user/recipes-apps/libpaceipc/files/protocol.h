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

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "ipc_base.h"
#include "ipc_ret.h"

#define IPC_HDR_SIZE()          ((unsigned long)&(((IPC_CMD_HEADER*)0)->cmd))
#define IPC_CMD_SIZE(cmd)       (IPC_HDR_SIZE() + (cmd)->cmd_size)
#define IPC_PARAM_HDR_SIZE()    ((unsigned long)&(((IPC_CALL_PARAM*)0)->data))

enum
{
    IPC_CMD_ID_REQ_CALL         =   1,
    IPC_CMD_ID_REQ_CALL_RESP    =   100,
    IPC_CMD_ID_ERROR_CODE       =   2,
    IPC_CMD_ID_REQ_TABLE        =   3,
    IPC_CMD_ID_REQ_TABLE_RESP   =   300,
};

typedef struct _IPC_CALL_PARAM_
{
    int                 type_id;
    int                 data_len;
    unsigned char       data[1];
}IPC_CALL_PARAM;

typedef struct _IPC_REQ_CALL_
{
    int                 api_id;
    int                 param_num;      // The number of parameters
    unsigned char       params[1];      // Just buffer where all the parameters are packed/placed, the type is IPC_CALL_PARAM
}IPC_REQ_CALL;

typedef struct _IPC_REQ_CALL_RESP_
{
    int                 out_param_num;          // The number of output parameters this response provides for the caller
    int                 reserved;               // 8 bytes alignment
    int                 ret_type;
    int                 ret_size;
    char                ret_data[1];
}IPC_REQ_CALL_RESP;

typedef struct _IPC_REQ_ERROR_
{
    int                 error_code;
}IPC_REQ_ERROR;

typedef struct _IPC_REQ_TABLE_
{
    int                 res;
}IPC_REQ_TABLE;

typedef struct _IPC_REQ_TABLE_RESP_
{
    unsigned char       data[1];
}IPC_REQ_TABLE_RESP;

typedef union _IPC_CMD_
{
    IPC_REQ_CALL        call;
    IPC_REQ_CALL_RESP   call_resp;
    IPC_REQ_ERROR       error;

    IPC_REQ_TABLE       table_req;
    IPC_REQ_TABLE_RESP  table_resp;

}IPC_CMD;

typedef struct _IPC_CMD_HEADER_
{
    int                 req_id;
    int                 cmd_size;           // The total size of CMD object, with all optional parameters
    IPC_CMD             cmd;
}IPC_CMD_HEADER;

typedef struct _IPC_RETURN_DATA_
{
    int             out_param_num;
    int             ret_type;
    int             ret_size;
    void*           ret_data;

    struct _out_params_
    {
        int     type;
        int     size;
        void*   data;
    }out_params[IPC_MAX_API_PARAMS];

}IPC_RETURN_DATA;

int protocol_process_req(IPC_HANDLE* phandle, int client_h);
void* protocol_get_api_table(IPC_HANDLE* phandle);
void* protocol_create_api_req(int api_id, int total_param_size);
int protocol_destroy_api_req(void* papi);
int protocol_api_add_param(void* papi, int type, int size, void* data);
IPC_CALL_PARAM* protocol_find_param(IPC_REQ_CALL* pApi, int idx);
int protocol_calc_param_size(int data_size);

int protocol_call_req(IPC_HANDLE* phandle, void* papi, IPC_RETURN_DATA* ret_ctx);
int protocol_ipc_return(IPC_HANDLE *handle, int type, int size, void* data);

#endif // _PROTOCOL_H_
