/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This is remote control command manager, the class that allows to communicate
 *    to the class_cmdmgr remotely
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


#ifndef _RC_THREAD_H_
#define _RC_THREAD_H_

#include <pthread.h>

class class_thread
{
public:

                            class_thread(void);
                            ~class_thread(void);

        int                 create(void* (*thread_proc)(void* param), void* param);
        int                 destroy(void);

protected:


private:
    pthread_t               m_thread;
    int                     m_thread_created;
};

#endif //_RC_THREAD_H_

