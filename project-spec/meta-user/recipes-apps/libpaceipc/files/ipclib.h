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

#ifndef _IPC_LIB_H_
#define _IPC_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_DEF_THREAD_AFFINITY         0       // 0 - meand all the cores (1,2,...64 ... 128, etc)
#define IPC_DEF_THREAD_PRIO             20      // if > 0 - the regular priority, if (<0) - RT priority, like (-80), (-90), etc
#define IPC_DEF_TRANSPORT_ADDRESS       "127.0.0.1"

#define IPC_TCP_PORT                    0       // The port must be defined by the IPC user(by the application)
#define IPC_MAX_CLIENTS                 64
#define IPC_MAX_API_PARAMS              16

#define IPC_CFG_VAL(x)                ((void*)(x))

#ifndef IPC_U64
#define IPC_U64(x)      ((unsigned long long)(x))
#endif

#ifndef IPC_S64
#define IPC_S64(x)      ((long long)(x))
#endif

#ifndef IPC_U32
#define IPC_U32(x)      ((unsigned int)(x))
#endif

#ifndef IPC_S32
#define IPC_S32(x)      ((int)(x))
#endif

#ifndef IPC_U16
#define IPC_U16(x)      ((unsigned short)(x))
#endif

#ifndef IPC_S16
#define IPC_S16(x)      ((short)(x))
#endif

#ifndef IPC_U8
#define IPC_U8(x)      ((unsigned char)(x))
#endif

#ifndef IPC_S8
#define IPC_S8(x)      ((char)(x))
#endif


#define IPC_API_TABLE_BEGIN(name) static IPC_API name[] = {
#define IPC_API_TABLE_END()                                     \
            {0, NULL,       0} };

#define IPC_API_BEGIN(id, api)  {                               \
            id, api, 0,                                         \
            {

#define IPC_API_END()  {0,             0,    0},                \
            }                                                   \
            },


#define IPC_API_PARAM(type, out, size)  {type,   out,    size},

typedef struct _IPC_API_PARAM_
{
    unsigned short  type;       // See IPC_API_PARAM_TYPE
    unsigned short  output;     // The flag that informs that this parameter is input and output at the same time
    int             size;       // The data size, used for the pointers
}IPC_API_PARAM;

typedef struct _IPC_API_
{
    int             api_id;
    void            (*proc)(void*);
    int             param_num;                      // The client sets this field! For server this is 0
    IPC_API_PARAM   params[IPC_MAX_API_PARAMS];

}IPC_API;

enum
{
    IPC_TRANSPORT_ID_TCP        =   0
};

typedef enum _IPC_API_PARAM_TYPE_
{
    IPC_APT_NONE            =   0,
    IPC_APT_VOID            =   1000,      // APT = API Parameter Type,  VOID
    IPC_APT_U8              =   2001,      // APT = API Parameter Type,  unsigned char
    IPC_APT_U16             =   3002,      // APT = API Parameter Type,  unsigned short (16 bits)
    IPC_APT_U32             =   4004,      // APT = API Parameter Type,  unsigned short (32 bits)
    IPC_APT_U64             =   5008,      // APT = API Parameter Type,  unsigned short (64 bits)

    IPC_APT_STR             =   6,          // APT = API Parameter Type,  ASCIIZ string (the string terminated by 0)
    IPC_APT_PVOID           =   7,          // APT = API Parameter Type,  void*

}IPC_API_PARAM_TYPE;

typedef enum _IPC_CFG_PARAMS_
{
    IPC_CFG_TRANSPORT       =   0,
    IPC_CFG_PORT            =   1,
    IPC_CFG_IP              =   2,
    IPC_CFG_THREAD_AFF      =   3,
    IPC_CFG_THREAD_PRIO     =   4,

}IPC_CFG_PARAMS;

typedef struct _IPC_CALL_PARAM_LIST_
{
    struct _IPC_CALL_PARAM_LIST_*   next;           // The pointer to the next parameter
    int                             type_id;        // The data type ID, see: IPC_API_PARAM_TYPE
    void*                           data_ptr;       // The pointer to the data (to U32, to U16, U8, STR, VOID* ... )
}IPC_CALL_PARAM_LIST;

typedef enum _IPC_SERV_INFO_PAR_ID_
{
    IPC_SERV_INFO_IP        =   1,
    IPC_SERV_INFO_PORT      =   2,

}IPC_SERV_INFO_PAR_ID;

void* ipc_create_cfg(void* api_table);
int ipc_set_cfg(void* cfg, int param_id, void* value);
void ipc_destroy_cfg(void* pCfg);
int ipc_create(void* void_cfg, void**h_ipc);
int ipc_open(void* void_cfg, void**h_ipc);
int ipc_close(void* h_ipc);

int ipc_get_port(void* h_ipc);
int ipc_get_version(int* major, int* minor);

void* ipc_create_param_list(int type_id, void* data);
int ipc_destroy_param_list(void* p);
int ipc_add_param(void* list, int type_id, void* data);

void* ipc_call_l(void* h_ipc, int api_id, void* plist);
void* ipc_call(void* h_ipc, int api_id, ...);
void ipc_call_free_return(void* p);
int ipc_return_get_error_code(void* p);
int ipc_return_get_data_type(void* p);
int ipc_return_get_data_size(void* p);
void* ipc_return_get_data(void* p);
void* ipc_param(void* hcmd, int idx);
int ipc_return(void* h, int type, int size, void* data);

int ipc_enable_daemon_mode(void);

const void* ipc_scan_server_info(int argc, char** argv);
const void* ipc_get_server_param(const void* pserv_info, int param_id);
void        ipc_destroy_server_info(const void* pserv_info);

#ifdef __cplusplus
}
#endif

#endif // _IPC_LIB_H_
