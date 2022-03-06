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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"

static int protocol_calc_out_params_size(IPC_API* pApi, IPC_REQ_CALL* pcmd)
{
    int size = 0;
    int i = 0;
    int num = pcmd->param_num;
    IPC_CALL_PARAM* param = (IPC_CALL_PARAM*)&pcmd->params[0];

    while (num > 0)
    {
        if (pApi->params[i].output)
        {
            size += IPC_PARAM_HDR_SIZE() + ipc_round8(param->data_len);
        }
        param = (IPC_CALL_PARAM*)(((unsigned char*)param) + IPC_PARAM_HDR_SIZE() + ipc_round8(param->data_len));
        num --;
        i++;
    }

    return size;
}

static int protocol_proc_send_return(IPC_HANDLE* phandle, int client_h, IPC_API* pApi, IPC_REQ_CALL* pcmd)
{
    int size = 0, rc, i;
    int out_param_size = protocol_calc_out_params_size(pApi, pcmd);
    IPC_CMD_HEADER* resp;
    IPC_CALL_PARAM* out_param, *in_param;

    size = ipc_round8(phandle->current_api_ret_size);
    size += sizeof(IPC_REQ_CALL_RESP);
    size += out_param_size;

    resp = malloc(IPC_HDR_SIZE()+size);
    if (resp == NULL)
        return RC_PROTOCOL_ALLOC_ERROR;

    memset(resp, 0, IPC_HDR_SIZE()+size);

    resp->req_id = IPC_CMD_ID_REQ_CALL_RESP;
    resp->cmd_size = size;

    resp->cmd.call_resp.ret_type = phandle->current_api_ret_type;
    resp->cmd.call_resp.ret_size = phandle->current_api_ret_size;
    if (phandle->current_api_ret_data != NULL)
    {
        memcpy(resp->cmd.call_resp.ret_data, phandle->current_api_ret_data, phandle->current_api_ret_size);
    }

    // To process the output parameters
    // ---------------------------------

    in_param  = (IPC_CALL_PARAM*)(pcmd->params);
    out_param = (IPC_CALL_PARAM*)(resp->cmd.call_resp.ret_data + ipc_round8(resp->cmd.call_resp.ret_size));

    i = 0;
    while (pApi->params[i].type != IPC_APT_NONE && out_param_size != 0)
    {
        if (pApi->params[i].output)
        {
            out_param->type_id  = in_param->type_id;
            out_param->data_len = in_param->data_len;
            memcpy(out_param->data, in_param->data, in_param->data_len);
            out_param = (IPC_CALL_PARAM*)(((unsigned char*)out_param) + IPC_PARAM_HDR_SIZE() + ipc_round8(out_param->data_len));

            resp->cmd.call_resp.out_param_num++;
        }

        in_param = (IPC_CALL_PARAM*)(((unsigned char*)in_param) + IPC_PARAM_HDR_SIZE() + ipc_round8(in_param->data_len));
        i++;
    }

    rc = phandle->transport_write(phandle, client_h, resp, IPC_CMD_SIZE(resp));

    free(resp);
    resp = NULL;

    // The phandle->current_api_ret_data was used, so, let's free this memory
    if (phandle->current_api_ret_data != NULL)
    {
        free(phandle->current_api_ret_data);
        phandle->current_api_ret_data = NULL;
    }

    return rc;
}

static int protocol_proc_req_call(IPC_HANDLE* phandle, int client_h, IPC_CMD_HEADER*pHdr)
{
    int rc, i;
    IPC_REQ_CALL* pcmd = NULL;

    pcmd = (IPC_REQ_CALL*)malloc(pHdr->cmd_size);
    if (pcmd == NULL)
        return RC_PROTOCOL_ALLOC_ERROR;

    if ((rc = phandle->transport_read(phandle, client_h, pcmd, pHdr->cmd_size)) <= 0)
    {
        free(pcmd);
        pcmd = NULL;
        return rc;
    }

    rc = RC_OK;

    // Here we need to find the specific API,
    // when the search is OK, to call that API

    i = 0;
    while (phandle->serv_api_table[i].proc != NULL)
    {
        if (phandle->serv_api_table[i].api_id == pcmd->api_id)
        {
            phandle->current_api_ret_type = IPC_APT_VOID;
            phandle->current_api_ret_size = 0;
            phandle->current_api_ret_data = NULL;
            phandle->current_api_call = &phandle->serv_api_table[i];
            phandle->current_api_params = pcmd;

            phandle->serv_api_table[i].proc(phandle);

            // Here we need to process the return value of the API
            // to process the output parameters
            // and to deliver it to the IPC client
            rc = protocol_proc_send_return(phandle, client_h, phandle->current_api_call, pcmd);

            phandle->current_api_call = NULL;
            break;
        }

        i++;
    }

    free(pcmd);
    pcmd = NULL;
    return rc;
}

static int protocol_proc_error_code(IPC_HANDLE* phandle, int client_h, IPC_CMD_HEADER*pHdr)
{
    return RC_OK;
}

static int protocol_proc_table_req(IPC_HANDLE* phandle, int client_h, IPC_CMD_HEADER*pHdr)
{
    int rc;
    int data_size = phandle->serv_api_table_elm_num*sizeof(IPC_API);

    IPC_CMD_HEADER cmd;
    IPC_CMD_HEADER* pcmd;

    if (pHdr->cmd_size != 0)
    {
        rc = phandle->transport_read(phandle, client_h, &cmd, pHdr->cmd_size);
        if (rc < 0)
            return rc;
    }

    if((pcmd = (IPC_CMD_HEADER*)malloc(sizeof(IPC_CMD_HEADER) + data_size)) == NULL)
        return RC_PROTOCOL_TABLE_REQ_ERROR;

    pcmd->req_id = IPC_CMD_ID_REQ_TABLE_RESP;
    pcmd->cmd_size = data_size;

    memcpy(pcmd->cmd.table_resp.data, phandle->serv_api_table, data_size);
    rc = phandle->transport_write(phandle, client_h, pcmd, IPC_CMD_SIZE(pcmd));

    free(pcmd);
    pcmd = NULL;
    return rc;
}

int protocol_process_req(IPC_HANDLE* phandle, int client_h)
{
    IPC_CMD_HEADER req;
    int rc;

    if ((rc = phandle->transport_read(phandle, client_h, &req, IPC_HDR_SIZE())) < 0)
        return rc;

    switch(req.req_id)
    {
        case IPC_CMD_ID_REQ_CALL:
            rc = protocol_proc_req_call(phandle, client_h, &req);
            break;

        case IPC_CMD_ID_ERROR_CODE:
            rc = protocol_proc_error_code(phandle, client_h, &req);
            break;

        case IPC_CMD_ID_REQ_TABLE:
            rc = protocol_proc_table_req(phandle, client_h, &req);
            break;

        default:
            return RC_PROTOCOL_UNKNOWN_CMD;
    }

    // let's find the required interface 

    return RC_OK;
}

void* protocol_get_api_table(IPC_HANDLE* phandle)
{
    void* ptable;
    int rc;
    IPC_CMD_HEADER cmd;
    #if 0
    int i,j;
    #endif

    memset(&cmd, 0, sizeof(cmd));
    cmd.req_id = IPC_CMD_ID_REQ_TABLE;
    cmd.cmd_size = sizeof(IPC_REQ_TABLE);

    if ((rc = phandle->transport_write(phandle, 0, &cmd, IPC_CMD_SIZE(&cmd))) <= 0)
        return NULL;

    if ((rc = phandle->transport_read(phandle, 0, &cmd, IPC_HDR_SIZE())) != IPC_HDR_SIZE())
        return NULL;

    ptable = malloc(cmd.cmd_size);
    if (ptable == NULL)
        return NULL;

    if ((rc = phandle->transport_read(phandle, 0, ptable, cmd.cmd_size)) != cmd.cmd_size)
        return NULL;

    phandle->serv_api_table = (IPC_API*)ptable;
    phandle->serv_api_table_elm_num =  cmd.cmd_size / sizeof(IPC_API);

    #if 0
    printf("OK, Let's scan the table\n");
    i = 0;
    while (phandle->serv_api_table[i].proc != NULL)
    {
        printf("api.id:%d,  proc:%p,  ret-type:%d, ret-size:%d, param_num:%d\n", 
                phandle->serv_api_table[i].api_id,
                phandle->serv_api_table[i].proc,
                phandle->serv_api_table[i].ret_data_type,
                phandle->serv_api_table[i].ret_data_size,
                phandle->serv_api_table[i].param_num
            );

        for (j = 0; j < phandle->serv_api_table[i].param_num; j++)
        {
            printf("    PARAM: type:%d,  out:%d,  size:%d\n",
                phandle->serv_api_table[i].params[j].type,
                phandle->serv_api_table[i].params[j].output,
                phandle->serv_api_table[i].params[j].size
                );
        }

        i++;
    }
    #endif

    return ptable;
}

void* protocol_create_api_req(int api_id, int total_param_size)
{
    IPC_CMD_HEADER* pcmd;

    pcmd = (IPC_CMD_HEADER*)malloc(sizeof(IPC_CMD_HEADER) + total_param_size);

    if(pcmd != NULL)
    {
        memset(pcmd, 0, sizeof(*pcmd));

        pcmd->req_id    = IPC_CMD_ID_REQ_CALL;
        pcmd->cmd_size  = sizeof(IPC_REQ_CALL) + total_param_size;

        pcmd->cmd.call.api_id = api_id;
        pcmd->cmd.call.param_num = 0;
    }

    return pcmd;
}

IPC_CALL_PARAM* protocol_find_param(IPC_REQ_CALL* pApi, int idx)
{
    int i = 0;
    int num = pApi->param_num;
    IPC_CALL_PARAM* param = (IPC_CALL_PARAM*)&pApi->params[0];

    if (idx >= num)
        return NULL;

    while (num > 0)
    {
        if (i == idx)
            break;
        param = (IPC_CALL_PARAM*)(((unsigned char*)param) + IPC_PARAM_HDR_SIZE() + ipc_round8(param->data_len));
        num --;
        i++;
    }

    if (param->data_len == 0)
        return NULL;
    return param;
}

static IPC_CALL_PARAM* protocol_find_free_place(IPC_CMD_HEADER* pcmd)
{
    int num = pcmd->cmd.call.param_num;
    IPC_CALL_PARAM* param = (IPC_CALL_PARAM*)&pcmd->cmd.call.params[0];

    while (num > 0)
    {
        param = (IPC_CALL_PARAM*)(((unsigned char*)param) + IPC_PARAM_HDR_SIZE() + ipc_round8(param->data_len));
        num --;
    }

    return param;
}

int protocol_api_add_param(void* papi, int type, int size, void* data)
{
    IPC_CMD_HEADER* pcmd = (IPC_CMD_HEADER*)papi;
    IPC_CALL_PARAM* free_param;

    if (pcmd == NULL)
        return RC_PROTOCOL_PARAM_ERROR;

    free_param = protocol_find_free_place(pcmd);
    pcmd->cmd.call.param_num++;

    switch(type)
    {
        case IPC_APT_U8:
            *((uint8_t*)free_param->data) = *(uint8_t*)data;
            break;

        case IPC_APT_U16:
            *((uint16_t*)free_param->data) = *(uint16_t*)data;
            break;

        case IPC_APT_U32:
            *((uint32_t*)free_param->data) = *(uint32_t*)data;
            break;

        case IPC_APT_U64:
            *((uint64_t*)free_param->data) = *(uint64_t*)data;
            break;

        case IPC_APT_STR:
        case IPC_APT_PVOID:
            if (data!=NULL)
            {
                memcpy(free_param->data, data, size);
            }
            break;
    }

    free_param->type_id = type;
    free_param->data_len = /*ipc_round8*/(size);

    return RC_OK;
}

int protocol_calc_param_size(int data_size)
{
    return IPC_PARAM_HDR_SIZE() + ipc_round8(data_size);
}

static IPC_CMD_HEADER* protocol_read_cmd(IPC_HANDLE* phandle)
{
    int rc;
    void* pdata;
    IPC_CMD_HEADER resp_hdr;

    if ((rc = phandle->transport_read(phandle, 0, &resp_hdr, IPC_HDR_SIZE())) != IPC_HDR_SIZE())
        return NULL;

    pdata = malloc(IPC_HDR_SIZE() + resp_hdr.cmd_size);
    if (pdata == NULL)
        return NULL;

    memcpy(pdata, &resp_hdr, IPC_HDR_SIZE());

    if ((rc = phandle->transport_read(phandle, 0, (char*)pdata+IPC_HDR_SIZE(), resp_hdr.cmd_size)) != resp_hdr.cmd_size)
    {
        free(pdata);
        return NULL;
    }

    return (IPC_CMD_HEADER*)pdata;
}

int protocol_call_req(IPC_HANDLE* phandle, void* papi, IPC_RETURN_DATA* ret_ctx)
{
    int rc, i;
    IPC_CMD_HEADER*pcmd = (IPC_CMD_HEADER*)papi;
    IPC_CMD_HEADER*resp;
    IPC_CALL_PARAM* out_param;

    if (papi == NULL)
        return RC_PROTOCOL_PARAM_ERROR;

    if ((rc = phandle->transport_write(phandle, 0, pcmd, IPC_CMD_SIZE(pcmd))) != IPC_CMD_SIZE(pcmd))
        return RC_PROTOCOL_WRITE_DATA_ERROR;

    // Here we need to read the response from IPC server
    // -------------------------------------------------
    while(1)
    {
        if ((resp = protocol_read_cmd(phandle)) == NULL)
            return RC_PROTOCOL_READ_RESP_ERROR;

        if (resp->req_id != IPC_CMD_ID_REQ_CALL_RESP)
        {
            free(resp);
            continue;
        }

        break;
    }

    if (phandle->client_call_resp_buf != NULL)
    {
        free(phandle->client_call_resp_buf);
    }

    phandle->client_call_resp_buf = resp;

    // To parse the response
    // ---------------------
    // The data format is:
    // [RET VALUE] [OUT PARAM] [OUT PARAM] ... [OUT PARAM]

    ret_ctx->ret_type = resp->cmd.call_resp.ret_type;
    ret_ctx->ret_size = resp->cmd.call_resp.ret_size;
    ret_ctx->ret_data = resp->cmd.call_resp.ret_data;

    ret_ctx->out_param_num = resp->cmd.call_resp.out_param_num;

    // OUTPUT parameters
    // ---------------------
    out_param = (IPC_CALL_PARAM*)(resp->cmd.call_resp.ret_data + ipc_round8(resp->cmd.call_resp.ret_size));
    for (i = 0; i < resp->cmd.call_resp.out_param_num; i++)
    {
        ret_ctx->out_params[i].type = out_param->type_id;
        ret_ctx->out_params[i].size = out_param->data_len;
        ret_ctx->out_params[i].data = out_param->data;
        out_param = (IPC_CALL_PARAM*)(((unsigned char*)out_param) + IPC_PARAM_HDR_SIZE() + ipc_round8(out_param->data_len));
    }

    return RC_OK;
}

int protocol_ipc_return(IPC_HANDLE *phandle, int type, int size, void* data)
{
    if (phandle->current_api_call == NULL)
        return RC_PROTOCOL_API_CTX_ERROR;

    if (phandle->current_api_ret_data != NULL)
    {
        free(phandle->current_api_ret_data);
        phandle->current_api_ret_data = NULL;
    }

    switch (type)
    {
        case IPC_APT_U8:
            size = 1;
            break;

        case IPC_APT_U16:
            size = 2;
            break;

        case IPC_APT_U32:
            size = 4;
            break;

        case IPC_APT_U64:
            size = 8;
            break;

        case IPC_APT_STR:
            size = strlen((char*)data) + 1;
            break;

        case IPC_APT_PVOID:
            break;
    }

    phandle->current_api_ret_type = type;
    phandle->current_api_ret_size = size;

    if (data != NULL)
    {
        phandle->current_api_ret_data = malloc(size);
        if (phandle->current_api_ret_data == NULL)
            return RC_PROTOCOL_ALLOC_ERROR;

        memcpy(phandle->current_api_ret_data, data, size);
    }
    return RC_OK;
}

int protocol_destroy_api_req(void* papi)
{
    if (papi != NULL)
        free(papi);
    return RC_OK;
}
