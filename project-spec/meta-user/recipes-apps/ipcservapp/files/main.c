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

void proc_api_1(void* h)
{
    uint32_t* pu32 = (uint32_t*)ipc_param(h, 0);
    uint8_t* pu8 = (uint8_t*)ipc_param(h, 1);
    uint16_t* pu16 = (uint16_t*)ipc_param(h, 2);
    char* pstr = (char*)ipc_param(h, 3);

    printf("\nClient called: 'api_1'\n");
    printf("   param-1: (%d)\n", *pu32);
    printf("   param-2: (%d)\n", *pu8);
    printf("   param-3: (%d)\n", *pu16);
    printf("   param-4: '%s'\n", pstr);

    //strcpy(pstr, "Param#4: This text was set by proc_api_1 API");

    #if 1
    {
        int val;
        val = 123;
	    printf("   [RETURN]I'm returning U32:%d\n",val);
        ipc_return(h, IPC_APT_U32, 0, &val);
    }
    #else
    printf("   [RETURN]I'm returning text: '%s'\n", "proc_api_1 (returns): The text value");
    ipc_return(h, IPC_APT_STR, 0, "proc_api_1 (returns): The text value");
    #endif
}

typedef struct _MY_TST_OBJ_
{
    int     a;
    int     b;
    int     c;
}MY_TST_OBJ;

void proc_api_2(void* h)
{
    int val = 12345;
    char* pstr = (char*)ipc_param(h, 0);
    MY_TST_OBJ* pobj= (MY_TST_OBJ*)ipc_param(h, 1);

    (void)val;

    printf("\nClient called: 'api_2'\n");
    printf("   param-1: '%s'\n", pstr);
    printf("   param-2.a: %d\n", pobj->a);
    printf("   param-2.b: %d\n", pobj->b);
    printf("   param-2.c: %d\n", pobj->c);

    printf("   To update the second one parameter (MY_TST_OBJ*)\n");

    pobj->a = 1000;
    pobj->b = 2000;
    pobj->c = 3000;

#if 0
    printf("   [RETURN]I'm returning U32: %d\n", val);
    ipc_return(h, IPC_APT_U32, 0, &val);
#else
    printf("   [RETURN]I'm returning STRING: %s\n", "This is my ret text");
    ipc_return(h, IPC_APT_STR, 0, "This is my ret text");
#endif
}


IPC_API_TABLE_BEGIN(my_ipc_table)

    IPC_API_BEGIN(1, proc_api_1)
        IPC_API_PARAM(IPC_APT_U32, 0, 0)
        IPC_API_PARAM(IPC_APT_U8,  0, 0)
        IPC_API_PARAM(IPC_APT_U16, 0, 0)
        IPC_API_PARAM(IPC_APT_STR, 1, 0)
    IPC_API_END()

    IPC_API_BEGIN(2, proc_api_2)
        IPC_API_PARAM(IPC_APT_STR,   0, 0)
        IPC_API_PARAM(IPC_APT_PVOID, 1, sizeof(MY_TST_OBJ))
    IPC_API_END()

IPC_API_TABLE_END();


int main(int argc, char** argv)
{
    char buf[128];
    void* ipc_cfg_serv;
    void* h_ipc_serv = NULL;
    int rc;

    printf("IPC server test application v1.0\n");

    ipc_cfg_serv = ipc_create_cfg(my_ipc_table);
    ipc_set_cfg(ipc_cfg_serv, IPC_CFG_PORT, IPC_CFG_VAL(50000));
    rc = ipc_create(ipc_cfg_serv, &h_ipc_serv);
    if (rc < 0)
    {
	printf("Error to open IPC server, rc=%d\n", rc);
	ipc_destroy_cfg(ipc_cfg_serv);
	return rc;
    }

    ipc_destroy_cfg(ipc_cfg_serv);
    ipc_cfg_serv = NULL;

    printf("SERV-PORT is %d\n", ipc_get_port(h_ipc_serv));

    fgets(buf, sizeof(buf), stdin);

    rc = ipc_close(h_ipc_serv);
    h_ipc_serv = NULL;
    return 0;
}
