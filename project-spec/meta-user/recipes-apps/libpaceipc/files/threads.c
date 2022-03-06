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

#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "threads.h"
#include "socket.h"
#include "protocol.h"

static int proc_client(IPC_HANDLE*phandle, int h)
{
    int rc = protocol_process_req(phandle, h);
    return rc;
}

static void* ipc_thread(void*p)
{
    IPC_HANDLE*phandle = (IPC_HANDLE*)p;
    int rc, h;

    while (1)
    {
        rc = phandle->transport_handle(phandle);
        if (rc < RC_OK)
            break;

        if (rc == RC_OK)
            continue;

        // Here we are going to process the client
        // returned by the socket API
        if ((rc = proc_client(phandle, (h = rc))) < 0)
        {
            phandle->transport_client_close(phandle, h);
        }
    }
    return NULL;
}

int threads_init(IPC_THREAD_INIT_PARAM* pcfg, IPC_HANDLE*phandle)
{
    int core;
    int rc;
    int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
    pthread_attr_t attr;
    cpu_set_t cpuset;
    struct sched_param param;

    if ((rc = pthread_attr_init(&attr)) != 0)
        return RC_THREAD_INIT_ATTR_ERROR;

    if (pcfg->priority > 0)
    {
        if ((rc = pthread_attr_setschedpolicy(&attr, SCHED_OTHER)) != 0)
            return RC_THREAD_INIT_POLICY_ERROR;

        param.sched_priority = 0;
        if ((rc = pthread_attr_setschedparam(&attr, &param)) != 0)
            return RC_THREAD_INIT_PRIO_ERROR;
    }
    else
    {
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if ((rc = pthread_attr_setschedpolicy(&attr, SCHED_RR)) != 0)
            return RC_THREAD_INIT_POLICY_ERROR;

        param.sched_priority = pcfg->priority * (-1);
        if ((rc = pthread_attr_setschedparam(&attr, &param)) != 0)
            return RC_THREAD_INIT_PRIO_ERROR;
    }

    if((rc = pthread_create(&phandle->ipc_thread, &attr, ipc_thread, phandle)))
        return RC_THREAD_CREATE_ERROR;

    CPU_ZERO(&cpuset);
    for (core = 0; core < numCPU; core++)
    {
        if ((pcfg->affinity&(1<<core)) || (pcfg->affinity == 0))
        {
            CPU_SET(core, &cpuset);
        }
    }

    if ((rc = pthread_setaffinity_np(phandle->ipc_thread, sizeof(cpu_set_t), &cpuset)) != 0)
        return RC_THREAD_AFFINITY_ERROR;
    return 0;
}

int threads_deinit(IPC_HANDLE*phandle)
{
    if (phandle == NULL)
        return RC_OK;

    if (phandle->ipc_object_type == IPC_OBJ_TYPE_SERVER)
    {
        void* ret;
        pthread_cancel(phandle->ipc_thread);
        pthread_join(phandle->ipc_thread, &ret);
    }

    return RC_OK;
}
