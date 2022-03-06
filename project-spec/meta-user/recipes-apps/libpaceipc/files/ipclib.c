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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ipc_base.h"
#include "ipc_ret.h"
#include "ipclib.h"
#include "socket.h"
#include "threads.h"
#include "protocol.h"

static pthread_mutex_t ipc_sync_mutex = PTHREAD_MUTEX_INITIALIZER;

void ipc_lock(void)
{
    pthread_mutex_lock(&ipc_sync_mutex);
}

void ipc_unlock(void)
{
    pthread_mutex_unlock(&ipc_sync_mutex);
}

void* ipc_create_cfg(void* api_table)
{
    IPC_INIT_PARAM*pcfg;

    if ((pcfg = malloc(sizeof(IPC_INIT_PARAM))) == NULL)
        return NULL;

    memset(pcfg, 0, sizeof(*pcfg));

    pcfg->transport_later_id = IPC_TRANSPORT_ID_TCP;
    pcfg->api_table = api_table;

    pcfg->threads_cfg.affinity = IPC_DEF_THREAD_AFFINITY;
    pcfg->threads_cfg.priority = IPC_DEF_THREAD_PRIO;

    pcfg->sockets_cfg.tcp_port = IPC_TCP_PORT;
    strcpy(pcfg->sockets_cfg.tcp_ip, "0.0.0.0");

    return pcfg;
}

int ipc_set_cfg(void* cfg, int param_id, void* value)
{
    IPC_INIT_PARAM* pcfg = (IPC_INIT_PARAM*)cfg;

    if (cfg == NULL)
        return RC_IPC_PARAM_ERROR;

    switch(param_id)
    {
        case IPC_CFG_TRANSPORT:
            pcfg->transport_later_id = (int)(long)value;
            break;
    
        case IPC_CFG_PORT:
            pcfg->sockets_cfg.tcp_port = (unsigned int)(long)value;
            break;

        case IPC_CFG_IP:
            strcpy(pcfg->sockets_cfg.tcp_ip, (char*)value);
            break;

        case IPC_CFG_THREAD_AFF:
            pcfg->threads_cfg.affinity = (unsigned int)(long)value;
            break;

        case IPC_CFG_THREAD_PRIO:
            pcfg->threads_cfg.priority = (int)(long)value;
            break;

        default:
            return RC_IPC_PARAM_ERROR;
    }

    return RC_OK;
}

void ipc_destroy_cfg(void* pcfg)
{
    free(pcfg);
}

/** @brief This function is an entry point of the IPC server
            and it should be called prior usage of any library API.
            This API creates a context and the thread for the IPC communication.

    @param pcfg [in]    - the configuration parameters or NULL
    @param h_ipc[out]   - the IPC handle, this handle should be used in all the API

    @return [int] an error code, RC_OK - OK
*/
int ipc_create(void* void_cfg, void**h_ipc)
{
    IPC_HANDLE *handle;
    int rc;
    int table_size, i;
    IPC_INIT_PARAM* pcfg = (void*)void_cfg;

    if (pcfg == NULL)
        return RC_IPC_PARAM_ERROR;

    if (pcfg->api_table == NULL)
        return RC_IPC_API_TABLE_ERROR;

    // let's calculate the size of the table
    i = 0;
    while(pcfg->api_table[i].proc != NULL)
    {
        i++;
    }
    if (i == 0)
        return RC_IPC_API_TABLE_EMPTY;

    table_size = i+1;

    handle = (IPC_HANDLE*)malloc(sizeof(*handle));
    if (handle == NULL)
        return RC_IPC_HANDLE_ALLOC_ERROR;

    memset(handle, 0, sizeof(*handle));

    handle->ipc_object_type = IPC_OBJ_TYPE_SERVER;
    handle->serv_api_table = pcfg->api_table;
    handle->serv_api_table_elm_num = table_size;

    // let's initialize all the layers

    if (pcfg->transport_later_id == IPC_TRANSPORT_ID_TCP)
    {
        if ((rc = sockets_init(&pcfg->sockets_cfg, handle)) < RC_OK)
        {
            free(handle);
            return rc;
        }

        handle->transport_handle = sockets_handle;
        handle->transport_client_close = sockets_client_close;
        handle->transport_read = sockets_read;
        handle->transport_write = sockets_write;
    }

    if ((rc = threads_init(&pcfg->threads_cfg, handle)) < RC_OK)
    {
        free(handle);
        return rc;
    }

    *h_ipc = handle;
    return RC_OK;
}

static int ipc_req_for_api(IPC_HANDLE *handle)
{
    int i, j;
    void* papi_table = protocol_get_api_table(handle);
    IPC_API_PARAM* param;

    if (papi_table == NULL)
        return RC_IPC_GETTING_TABLE_ERROR;

    // let's calculate the size of the table
    i = 0;
    while(handle->serv_api_table[i].proc != NULL)
    {
        j = 0;
        param = &handle->serv_api_table[i].params[0];
        while (param[j].type != 0)
        {
            j++;
        }
        handle->serv_api_table[i].param_num = j;
        i++;
    }
    handle->serv_api_table_elm_num = i+1;
    return RC_OK;
}

int ipc_open(void* void_cfg, void**h_ipc)
{
    IPC_HANDLE *handle;
    int rc;
    IPC_INIT_PARAM* pcfg = (void*)void_cfg;

    if (pcfg == NULL)
        return RC_IPC_PARAM_ERROR;

    handle = (IPC_HANDLE*)malloc(sizeof(*handle));
    if (handle == NULL)
        return RC_IPC_HANDLE_ALLOC_ERROR;

    memset(handle, 0, sizeof(*handle));

    handle->ipc_object_type = IPC_OBJ_TYPE_CLIENT;

    if (pcfg->transport_later_id == IPC_TRANSPORT_ID_TCP)
    {
        if ((rc = sockets_init(&pcfg->sockets_cfg, handle)) < RC_OK)
        {
            free(handle);
            return rc;
        }

        handle->transport_handle = sockets_handle;
        handle->transport_client_close = sockets_client_close;
        handle->transport_read = sockets_read;
        handle->transport_write = sockets_write;
    }

    // To ask the server to provide the table of the registered API
    // and information about every API
    // such table will be used to find the information about API and parameters
    rc = ipc_req_for_api(handle);

    *h_ipc = handle;
    return RC_OK;
}

int ipc_close(void* h_ipc)
{
    IPC_HANDLE *handle = (IPC_HANDLE *)h_ipc;

    if (h_ipc == NULL)
        return RC_OK;

    if (handle->ipc_object_type == IPC_OBJ_TYPE_SERVER)
    {
        threads_deinit((IPC_HANDLE *)h_ipc);
        sockets_deinit((IPC_HANDLE *)h_ipc);
    }
    else if (handle->ipc_object_type == IPC_OBJ_TYPE_CLIENT)
    {
        sockets_deinit((IPC_HANDLE *)h_ipc);

        // The client asks for the table
        // and keep this table in the dynamically allocated table
        // -------------------------------------------------------
        if (handle->serv_api_table != NULL)
        {
            free(handle->serv_api_table);
            handle->serv_api_table = NULL;
            handle->serv_api_table_elm_num = 0;
        }

        if (handle->client_call_resp_buf != NULL)
        {
            free(handle->client_call_resp_buf);
            handle->client_call_resp_buf = NULL;
        }
    }

    free(h_ipc);
    return RC_OK;
}

int ipc_get_port(void* h_ipc)
{
    IPC_HANDLE *handle = (IPC_HANDLE *)h_ipc;

    if (h_ipc == NULL)
        return RC_IPC_PARAM_ERROR;

    return ipc_socket_get_port(handle);
}

int ipc_get_version(int* major, int* minor)
{
    if (major == NULL || minor == NULL)
        return RC_IPC_PARAM_ERROR;

    *major = IPC_MAJOR_VERSION;
    *minor = IPC_MINOR_VERSION;

    return RC_OK;
}

static void* ipc_call_return(int error_code, int ret_data_type, int ret_data_size, void* ret_data)
{
    IPC_RETURN* pRet = (IPC_RETURN*)malloc(sizeof(IPC_RETURN)+ret_data_size);
    if (pRet == NULL)
        return NULL;

    pRet->error_code        = error_code;
    pRet->ret_data_type     = ret_data_type;
    pRet->ret_data_size     = ret_data_size;

    if (pRet->ret_data_type != IPC_APT_VOID)
    {
        if (pRet->ret_data_size && ret_data != NULL)
        {
            memcpy(pRet->ret_data, ret_data, ret_data_size);
        }
    }
    return pRet;
}

static int ipc_calc_param_size(IPC_API* pApi, va_list params)
{
    int size = 0;
    int i;

    for (i = 0; i < pApi->param_num; i++)
    {
        switch (pApi->params[i].type)
        {
            case IPC_APT_U8:
                va_arg(params,  uint32_t);
                size += protocol_calc_param_size(1);
                break;

            case IPC_APT_U16:
                va_arg(params,  uint32_t);
                size += protocol_calc_param_size(2);
                break;

            case IPC_APT_U32:
                va_arg(params,  uint32_t);
                size += protocol_calc_param_size(4);
                break;

            case IPC_APT_U64:
                va_arg(params,  uint64_t);
                size += protocol_calc_param_size(8);
                break;

            case IPC_APT_STR:
                {
                    char* pdata = (char*)va_arg(params,  char*);
                    if (pdata != NULL)
                        size += protocol_calc_param_size(strlen(pdata)+1);
                    else
                        size += 0;
                }
                break;

            case IPC_APT_PVOID:
                {
                    char* pdata = (char*)va_arg(params,  void*);
                    if (pdata != NULL)
                        size += protocol_calc_param_size(pApi->params[i].size);
                    else
                        size += 0;
                }
                break;
        }
    }
    return size;
}

static int ipc_is_number(int type)
{
    switch(type)
    {
        case IPC_APT_U8:
        case IPC_APT_U16:
        case IPC_APT_U32:
        case IPC_APT_U64:
            return 1;

        default:
            return 0;
    }
}

static int ipc_calc_param_size_l(IPC_API* pApi, IPC_CALL_PARAM_LIST* list)
{
    int size = 0;
    int i;
    IPC_CALL_PARAM_LIST* cur_param = list;

    for (i = 0; i < pApi->param_num; i++)
    {
        if (cur_param == NULL)
            return RC_IPC_API_INCORRECT_PARAMS;

        switch (pApi->params[i].type)
        {
            case IPC_APT_U8:
                {
                    if (!ipc_is_number(cur_param->type_id))
                        return RC_IPC_API_INCORRECT_PARAMS;
                    size += protocol_calc_param_size(1);
                }
                break;

            case IPC_APT_U16:
                {
                    if (!ipc_is_number(cur_param->type_id))
                        return RC_IPC_API_INCORRECT_PARAMS;
                    size += protocol_calc_param_size(2);
                }
                break;

            case IPC_APT_U32:
                {
                    if (!ipc_is_number(cur_param->type_id))
                        return RC_IPC_API_INCORRECT_PARAMS;
                    size += protocol_calc_param_size(4);
                }
                break;

            case IPC_APT_U64:
                {
                    if (!ipc_is_number(cur_param->type_id))
                        return RC_IPC_API_INCORRECT_PARAMS;
                    size += protocol_calc_param_size(8);
                }
                break;

            case IPC_APT_STR:
                {
                    if (cur_param->type_id != pApi->params[i].type)
                        return RC_IPC_API_INCORRECT_PARAMS;

                    char* pdata = (char*)cur_param->data_ptr;
                    if (pdata != NULL)
                        size += protocol_calc_param_size(strlen(pdata)+1);
                    else
                        size += 0;
                }
                break;

            case IPC_APT_PVOID:
                {
                    if (cur_param->type_id != pApi->params[i].type)
                        return RC_IPC_API_INCORRECT_PARAMS;

                    char* pdata = (char*)cur_param->data_ptr;
                    if (pdata != NULL)
                        size += protocol_calc_param_size(pApi->params[i].size);
                    else
                        size += 0;
                }
                break;
        }

        cur_param = cur_param->next;
    }
    return size;
}

void* ipc_create_param_list(int type_id, void* data)
{
    IPC_CALL_PARAM_LIST* list;
    list = (IPC_CALL_PARAM_LIST*)malloc(sizeof(IPC_CALL_PARAM_LIST));

    if (list == NULL)
        return NULL;

    memset(list, 0, sizeof(*list));

    list->type_id = type_id;
    list->data_ptr = data;

    return list;
}

int ipc_destroy_param_list(void* p)
{
    // Here we need to go thought the all elements
    // and to destroy everyone

    IPC_CALL_PARAM_LIST* list = (IPC_CALL_PARAM_LIST*)p;
    IPC_CALL_PARAM_LIST* tmp;

    if (list == NULL)
        return RC_IPC_PARAM_ERROR;

    while (list != NULL)
    {
        tmp = list;
        list= list->next;
        free(tmp);
    }
    return RC_OK;
}

int ipc_add_param(void* list, int type_id, void* data)
{
    IPC_CALL_PARAM_LIST* last = (IPC_CALL_PARAM_LIST*)list;

    if (last == NULL)
        return RC_IPC_PARAM_ERROR;

    // To find the last element
    while (last->next != NULL)
    {
        last= last->next;
    }

    last->next = (IPC_CALL_PARAM_LIST*)malloc(sizeof(IPC_CALL_PARAM_LIST));
    if (last->next == NULL)
        return RC_IPC_API_ALLOC_ERROR;

    last = last->next;
    memset(last, 0, sizeof(*last));
    last->type_id = type_id;
    last->data_ptr = data;

    return RC_OK;
}

void* ipc_call_l(void* h_ipc, int api_id, void* plist)
{
    IPC_HANDLE *handle = (IPC_HANDLE *)h_ipc;
    IPC_API* pApi = NULL;
    int i = 0, rc, j, k;
    void* p_api_cmd;
    IPC_RETURN_DATA ret_data;
    void* return_obj;
    void* in_pointers[IPC_MAX_API_PARAMS] = {0};
    int in_pointers_num = 0;
    int param_size;
    IPC_CALL_PARAM_LIST* cur_param = (IPC_CALL_PARAM_LIST*)plist;

    if (handle == NULL)
        return ipc_call_return(RC_IPC_PARAM_ERROR, 0, 0, NULL);

    if (handle->ipc_object_type != IPC_OBJ_TYPE_CLIENT)
        return ipc_call_return(RC_IPC_OBJ_TYPE_ERROR, 0, 0, NULL);

    if (handle->serv_api_table == NULL)
        return ipc_call_return(RC_IPC_API_TABLE_ERROR, 0, 0, NULL);

    // Let's find the exact API descriptor
    // -----------------------------------
    i = 0;
    while (handle->serv_api_table[i].proc != NULL)
    {
        if (handle->serv_api_table[i].api_id == api_id)
        {
            pApi = &handle->serv_api_table[i];
            break;
        }
        i++;
    }

    if (pApi == NULL)
        return ipc_call_return(RC_IPC_API_ID_ERROR, 0, 0, NULL);

    // OK, so now we have all the information about this API
    // let's prepare the parameters
    // -----------------------------------------------------
    if ((param_size = ipc_calc_param_size_l(pApi, (IPC_CALL_PARAM_LIST*)plist)) < 0)
        return ipc_call_return(RC_IPC_API_INCORRECT_PARAMS, 0, 0, NULL);

    if ((p_api_cmd = protocol_create_api_req(api_id, param_size)) == NULL)
    {
        return ipc_call_return(RC_IPC_API_CREATE_REQ_ERROR, 0, 0, NULL);
    }

    rc = RC_OK;
    for (i = 0; (i < pApi->param_num) && (rc == RC_OK); i++)
    {
        if (cur_param == NULL)
        {
            rc = RC_IPC_API_INCORRECT_PARAMS;
            break;
        }

        switch (pApi->params[i].type)
        {
            case IPC_APT_U8:
                {
                    protocol_api_add_param(p_api_cmd, IPC_APT_U8, 1, cur_param->data_ptr);
                }
                break;

            case IPC_APT_U16:
                {
                    protocol_api_add_param(p_api_cmd, IPC_APT_U16, 2, cur_param->data_ptr);
                }
                break;

            case IPC_APT_U32:
                {
                    protocol_api_add_param(p_api_cmd, IPC_APT_U32, 4, cur_param->data_ptr);
                }
                break;

            case IPC_APT_U64:
                {
                    protocol_api_add_param(p_api_cmd, IPC_APT_U64, 8, cur_param->data_ptr);
                }
                break;

            case IPC_APT_STR:
                {
                    char* pstr = (char*)cur_param->data_ptr;
                    if (pstr == NULL && pApi->params[i].output)
                    {
                        rc = RC_IPC_API_NULL_PARAM_ERROR;
                        break;
                    }
                    in_pointers[in_pointers_num++] = pstr;
                    protocol_api_add_param(p_api_cmd, IPC_APT_STR, (pstr != NULL) ? strlen(pstr)+1 : 0, pstr);
                }
                break;

            case IPC_APT_PVOID:
                {
                    void* pdata = (void*)cur_param->data_ptr;
                    if (pdata == NULL && pApi->params[i].output)
                    {
                        rc = RC_IPC_API_NULL_PARAM_ERROR;
                        break;
                    }
                    in_pointers[in_pointers_num++] = pdata;
                    protocol_api_add_param(p_api_cmd, IPC_APT_PVOID, (pdata != NULL) ? (pApi->params[i].size) : 0, pdata);
                }
                break;
        }

        cur_param = cur_param->next;
    }

    if (rc != RC_OK)
    {
        protocol_destroy_api_req(p_api_cmd);
        return ipc_call_return(rc, 0, 0, NULL);
    }

    memset(&ret_data, 0, sizeof(ret_data));
    if ((rc = protocol_call_req(handle, p_api_cmd, &ret_data)) < RC_OK)
    {
        protocol_destroy_api_req(p_api_cmd);
        return ipc_call_return(rc, 0, 0, NULL);
    }

    // To process the output parameters
    // ----------------------------------
    for (i=0, j=0, k=0; i < pApi->param_num && ret_data.out_param_num != 0; i++)
    {
        if (pApi->params[i].type == IPC_APT_PVOID || pApi->params[i].type == IPC_APT_STR)
        {
            if (pApi->params[i].output)
            {
                memcpy(in_pointers[j], ret_data.out_params[k].data, ret_data.out_params[k].size);
                k++;
            }

            j++;
        }
    }

    return_obj = ipc_call_return(RC_OK, ret_data.ret_type, ret_data.ret_size, ret_data.ret_data);

    // Let's go thought the parameters and to update the parameters
    // that are defined with 'output' flag

    protocol_destroy_api_req(p_api_cmd);
    return return_obj;
}

void* ipc_call(void* h_ipc, int api_id, ...)
{
    IPC_HANDLE *handle = (IPC_HANDLE *)h_ipc;
    IPC_API* pApi = NULL;
    int i = 0, rc, j, k;
    void* p_api_cmd;
    IPC_RETURN_DATA ret_data;
    void* return_obj;
    va_list params;
    void* in_pointers[IPC_MAX_API_PARAMS] = {0};
    int in_pointers_num = 0;

    if (handle == NULL)
        return ipc_call_return(RC_IPC_PARAM_ERROR, 0, 0, NULL);

    if (handle->ipc_object_type != IPC_OBJ_TYPE_CLIENT)
    {
        va_start(params, api_id);
        va_end(params);
        return ipc_call_return(RC_IPC_OBJ_TYPE_ERROR, 0, 0, NULL);
    }

    if (handle->serv_api_table == NULL)
        return ipc_call_return(RC_IPC_API_TABLE_ERROR, 0, 0, NULL);;

    // Let's find the exact API descriptor
    // -----------------------------------
    i = 0;
    while (handle->serv_api_table[i].proc != NULL)
    {
        if (handle->serv_api_table[i].api_id == api_id)
        {
            pApi = &handle->serv_api_table[i];
            break;
        }
        i++;
    }

    if (pApi == NULL)
        return ipc_call_return(RC_IPC_API_ID_ERROR, 0, 0, NULL);

    // OK, so now we have all the information about this API
    // let's prepare the parameters
    // -----------------------------------------------------
    va_start(params, api_id);
    if ((p_api_cmd = protocol_create_api_req(api_id, ipc_calc_param_size(pApi, params))) == NULL)
    {
        va_end(params);
        return ipc_call_return(RC_IPC_API_CREATE_REQ_ERROR, 0, 0, NULL);
    }
    va_end(params);

    rc = RC_OK;
    va_start(params, api_id);
    for (i = 0; (i < pApi->param_num) && (rc == RC_OK); i++)
    {
        switch (pApi->params[i].type)
        {
            case IPC_APT_U8:
                {
                    uint8_t data_u8 = (uint8_t)va_arg(params,  uint32_t);
                    protocol_api_add_param(p_api_cmd, IPC_APT_U8, 1, &data_u8);
                }
                break;

            case IPC_APT_U16:
                {
                    uint16_t data_u16 = (uint16_t)va_arg(params,  uint32_t);
                    protocol_api_add_param(p_api_cmd, IPC_APT_U16, 2, &data_u16);
                }
                break;

            case IPC_APT_U32:
                {
                    uint32_t data_u32 = (uint32_t)va_arg(params,  uint32_t);
                    protocol_api_add_param(p_api_cmd, IPC_APT_U32, 4, &data_u32);
                }
                break;

            case IPC_APT_U64:
                {
                    uint64_t data_u64 = (uint64_t)va_arg(params,  uint64_t);
                    protocol_api_add_param(p_api_cmd, IPC_APT_U64, 8, &data_u64);
                }
                break;

            case IPC_APT_STR:
                {
                    char* pstr = (char*)va_arg(params,  char*);
                    if (pstr == NULL && pApi->params[i].output)
                    {
                        rc = RC_IPC_API_NULL_PARAM_ERROR;
                        break;
                    }
                    in_pointers[in_pointers_num++] = pstr;
                    protocol_api_add_param(p_api_cmd, IPC_APT_STR, (pstr != NULL) ? strlen(pstr)+1 : 0, pstr);
                }
                break;

            case IPC_APT_PVOID:
                {
                    void* pdata = (void*)va_arg(params,  void*);
                    if (pdata == NULL && pApi->params[i].output)
                    {
                        rc = RC_IPC_API_NULL_PARAM_ERROR;
                        break;
                    }
                    in_pointers[in_pointers_num++] = pdata;
                    protocol_api_add_param(p_api_cmd, IPC_APT_PVOID, (pdata != NULL) ? (pApi->params[i].size) : 0, pdata);
                }
                break;
        }
    }
    va_end(params);

    if (rc != RC_OK)
    {
        protocol_destroy_api_req(p_api_cmd);
        return ipc_call_return(rc, 0, 0, NULL);
    }

    memset(&ret_data, 0, sizeof(ret_data));
    if ((rc = protocol_call_req(handle, p_api_cmd, &ret_data)) < RC_OK)
    {
        protocol_destroy_api_req(p_api_cmd);
        return ipc_call_return(rc, 0, 0, NULL);
    }

    // To process the output parameters
    // ----------------------------------
    for (i=0, j=0, k=0; i < pApi->param_num && ret_data.out_param_num != 0; i++)
    {
        if (pApi->params[i].type == IPC_APT_PVOID || pApi->params[i].type == IPC_APT_STR)
        {
            if (pApi->params[i].output)
            {
                memcpy(in_pointers[j], ret_data.out_params[k].data, ret_data.out_params[k].size);
                k++;
            }

            j++;
        }
    }

    return_obj = ipc_call_return(RC_OK, ret_data.ret_type, ret_data.ret_size, ret_data.ret_data);

    // Let's go thought the parameters and to update the parameters
    // that are defined with 'output' flag

    protocol_destroy_api_req(p_api_cmd);
    return return_obj;
}

void ipc_call_free_return(void* p)
{
    if (p != NULL)
    {
        free(p);
    }
}

int ipc_return_get_error_code(void* p)
{
    if (p == NULL)
        return RC_IPC_PARAM_ERROR;
    return ((IPC_RETURN*)p)->error_code;
}

int ipc_return_get_data_type(void* p)
{
    if (p == NULL)
        return RC_IPC_PARAM_ERROR;
    return ((IPC_RETURN*)p)->ret_data_type;
}

int ipc_return_get_data_size(void* p)
{
    if (p == NULL)
        return RC_IPC_PARAM_ERROR;
    return ((IPC_RETURN*)p)->ret_data_size;
}

void* ipc_return_get_data(void* p)
{
    if (p == NULL)
        return NULL;
    return ((IPC_RETURN*)p)->ret_data;
}

void* ipc_param(void* h, int idx)
{
    IPC_CALL_PARAM* pParam = protocol_find_param((IPC_REQ_CALL*) ((IPC_HANDLE *)h)->current_api_params, idx);
    if (pParam == NULL)
        return NULL;
    return pParam->data;
}

int ipc_return(void* h, int type, int size, void* data)
{
    return protocol_ipc_return((IPC_HANDLE *)h, type, size, data);
}

int ipc_enable_daemon_mode(void)
{
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        return RC_IPC_CREATE_DAEMON_ERROR;

    /* If we got a good PID, then
    we can exit the parent process.*/
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    if (1)
    {
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0)
            return RC_IPC_CREATE_DAEMON_ERROR;

        /* Change the current working directory */
        if ((chdir("/")) < 0)
            return RC_IPC_CREATE_DAEMON_ERROR;
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return RC_OK;
}

const void* ipc_scan_server_info(int argc, char** argv)
{
    int mask = 0;
    int opterr_back;
    IPC_SERVER_INFO* pinfo = malloc(sizeof(IPC_SERVER_INFO));
    if (pinfo == NULL)
        return NULL;

    memset(pinfo, 0, sizeof(*pinfo));
    strcpy(pinfo->tcp_ip, "127.0.0.1");

    opterr_back = opterr;
    opterr = 0;
    while (1)
    {
        //int this_option_optind = optind ? optind : 1;
        int option_index = 0, c;

        static struct option long_options[] =
        {
            {"ipc_server_ip",   required_argument, 0,   1},
			{"ipc_server_port", required_argument, 0,   2},
			{0,                 0,                 0,   0}
		};

		c = getopt_long(argc, argv, "", long_options, &option_index);

		if (c == -1)
			break;

		switch(c)
		{
			case 1: // Server IP
				strncpy(pinfo->tcp_ip, optarg, sizeof(pinfo->tcp_ip) - 1);
                mask |= (1<<0);
				break;

            case 2: // Server PORT
                pinfo->tcp_port = (unsigned int)atoi(optarg);
                mask |= (1<<1);
                break;
        }
    }

    opterr = opterr_back;
    // if we do not have the complete information about the server
    //    if port is missed
    //    Note: (IP may be missed, by default this is 127.0.0.1)
    if ((mask & (1<<1)) == 0)
    {
        free(pinfo);
        pinfo = NULL;
    }

    return pinfo;
}

const void* ipc_get_server_param(const void* pserv_info, int param_id)
{
    if (pserv_info == NULL)
        return NULL;

    switch (param_id)
    {
        case IPC_SERV_INFO_IP:
            return ((IPC_SERVER_INFO*)pserv_info)->tcp_ip;

        case IPC_SERV_INFO_PORT:
            return &((IPC_SERVER_INFO*)pserv_info)->tcp_port;

        default:
            return NULL;
    }
}

void ipc_destroy_server_info(const void* pserv_info)
{
    if (pserv_info != NULL)
        free((void*)pserv_info);
}