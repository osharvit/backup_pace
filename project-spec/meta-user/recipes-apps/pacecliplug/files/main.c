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
#include "cli_ipc_ctrl_inf.h"
#else
#include "paceipc/ipclib.h"
#include "pacecli/cli_ipc_ctrl_inf.h"
#endif


void *h_ipc_client = NULL;

int open_test_cli_ipc(void)
{
    void *ipc_cfg_client;
    int rc;
    ipc_cfg_client = ipc_create_cfg(NULL);
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_PORT, IPC_CFG_VAL(50007));
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_IP, IPC_CFG_VAL("127.0.0.1"));
    rc = ipc_open(ipc_cfg_client, &h_ipc_client);
    printf("cli ipc_open rc:%d\n", rc);
    ipc_destroy_cfg(ipc_cfg_client);

    if (rc < 0)
        return rc;

    IPC_CLI_DEF_hLIB(h_ipc_client);
    void* ret;

    ret = ipc_cli_api_print("Hello from my external application\n");
    if (ret)
        printf("CLI-PRINT - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);

    ret = ipc_cli_api_print("Going to run my command: print Hello CLI;\n");
    if (ret)
        printf("CLI-PRINT - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);

    ret = ipc_cli_api_command("print Hello CLI;\n");
    if (ret)
        printf("CLI-API - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);

    ret = ipc_cli_api_save2file(0x1000000, 128, "dump_0x1000000_128.dat");
    if (ret)
        printf("save2file - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);

    ret = ipc_cli_api_loadfromfile("math.o", 0x100000);
    if (ret)
        printf("loadfromfile - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);

    return 0;
}

int main(int argc, char** argv)
{
    printf("CLI plugin parameters:\n");
    for (int i = 0; i < argc; i++)
    {
        printf("  param[%d] == %s\n", i, argv[i]);
    }
    printf("\n");

    const void* pinfo = ipc_scan_server_info(argc, argv);
    if (pinfo != NULL)
    {
        printf("ipc-cli-ip: %s\n", (const char*)ipc_get_server_param(pinfo, IPC_SERV_INFO_IP));
        printf("ipc-cli-port: %d\n", *(unsigned int*)ipc_get_server_param(pinfo, IPC_SERV_INFO_PORT));
        ipc_destroy_server_info(pinfo);
    }

    printf("Test CLI plugin application v1.0\n");
    open_test_cli_ipc();

    printf("Going to while(1) with 1sec prints\n");
    printf("Please stop me with TERMSIG 2, in LUA proc_kill(pid, 2) or in a console kill -2 <pid>");
    while (1)
    {
        usleep(1000000);
        printf("I'm alive \n");
    }
    return 0;
}
