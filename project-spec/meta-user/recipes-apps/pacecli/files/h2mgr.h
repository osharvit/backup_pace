/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager is designed to communicate to H2 application
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

#ifndef _H2MGR_H_
#define _H2MGR_H_

#include "retcodes.h"
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include "cmd.h"
#include "retcodes.h"
#include "socket.h"

struct h2_q_data
{
    int         m_devid;
    int         m_cmdid;
    std::string m_data;
};

class class_h2mgr
{
    friend void* h2_thread(void* param);

public:
                class_h2mgr(void);
                ~class_h2mgr(void);

        int     connect(const char* ip, int port);
        int     send(int devid, int cmdid, std::string& cmd, std::string& resp);
        int     recv(int devid, std::string& resp);
        int     proc_input(void);

        int     close(void);
        int     is_stopped(void) {return m_stopped;};

protected:


private:
    class_socket            m_socket;
    pthread_t               m_thread;
    int                     m_thread_created;
    int                     m_stopped;

    std::queue<h2_q_data>   m_resp_queue;
    std::queue<h2_q_data>   m_ind_queue;
    pthread_mutex_t         m_resp_mutex;
    pthread_mutex_t         m_ind_mutex;
    
};

#endif //_H2MGR_H_