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
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <sys/file.h>
#include <errno.h>

#ifdef STAND_ALONE
#include "ipclib.h"
#else
#include "paceipc/ipclib.h"
#endif

typedef struct _MY_TST_OBJ_
{
    int     a;
    int     b;
    int     c;
}MY_TST_OBJ;

int main(int argc, char** argv)
{
    char buf[128];
    void *ipc_cfg_client;
    void *h_ipc_client = NULL;
    int rc;
    MY_TST_OBJ obj;
    int major = 0, minor = 0;
    void* plist;
    long port = 50000;

    printf("IPC client test application v1.0\n");
    ipc_get_version(&major, &minor);
    printf("The library version is %d.%02d\n", major, minor);

    ipc_cfg_client = ipc_create_cfg(NULL);
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_PORT, IPC_CFG_VAL(port));
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_IP, IPC_CFG_VAL("127.0.0.1"));
    rc = ipc_open(ipc_cfg_client, &h_ipc_client);
    printf("ipc_open rc:%d\n", rc);
    ipc_destroy_cfg(ipc_cfg_client);

    if (rc < 0)
        return rc;

    printf("\n\n");
    strcpy(buf, "Hello, this is my text parameter for IPC API");

    #if 1
    void* ret = ipc_call(h_ipc_client, 1, 11, 22, 33, buf);
    if (ret != NULL)
    {
        printf("API call error code:%d\n", ipc_return_get_error_code(ret));
        printf("API call ret type: %d\n", ipc_return_get_data_type(ret));
        printf("API call ret size: %d\n", ipc_return_get_data_size(ret));
        printf("out-param: %s\n", buf);

        if (ipc_return_get_data_type(ret) == IPC_APT_U32)
            printf("API call ret data: %d\n", *(uint32_t*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_STR)
            printf("API call ret data: '%s'\n", (char*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_VOID)
            printf("API call ret data: VOID\n");
    }
    ipc_call_free_return(ret);
    printf("\n\n");
    #endif

    obj.a = 1;
    obj.b = 2;
    obj.c = 3;

    printf("To call the API with list parameters\n");
    plist = ipc_create_param_list(IPC_APT_STR, "Let's call API#2");
    if (plist == NULL)
    {
	printf("Error to create list of parameters\n");
	ipc_close(h_ipc_client);
	return -1;
    }

    if (ipc_add_param(plist, IPC_APT_PVOID, &obj) < 0)
    {
	printf("Error to create list of parameters\n");
	ipc_close(h_ipc_client);
	return -1;
    }

    ret = ipc_call_l(h_ipc_client, 2, plist);
    ipc_destroy_param_list(plist);

    if (ret != NULL)
    {
        printf("API call error code:%d\n", ipc_return_get_error_code(ret));
        printf("API call ret type: %d\n", ipc_return_get_data_type(ret));
        printf("API call ret size: %d\n", ipc_return_get_data_size(ret));

        if (ipc_return_get_data_type(ret) == IPC_APT_U32)
            printf("API call ret data: %d\n", *(uint32_t*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_STR)
            printf("API call ret data: '%s'\n", (char*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_VOID)
            printf("API call ret data: VOID\n");
        printf("   obj.a: %d\n", obj.a);
        printf("   obj.b: %d\n", obj.b);
        printf("   obj.c: %d\n", obj.c);
    }
    ipc_call_free_return(ret);

    printf("Press enter to exit...\n");
    fgets(buf, sizeof(buf), stdin);
    rc = ipc_close(h_ipc_client);
    h_ipc_client = NULL;
    return 0;
}
