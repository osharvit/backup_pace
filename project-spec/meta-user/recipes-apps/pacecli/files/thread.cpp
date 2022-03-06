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

#include <stdlib.h>
#include <string.h>
#include "thread.h"
#include "retcodes.h"
#include "gen-types.h"

class_thread::class_thread(void)
{
	memset(&m_thread, 0, sizeof(m_thread));
	m_thread_created = 0;
}

class_thread::~class_thread(void)
{
	destroy();
}

int class_thread::create(void* (*thread_proc)(void* param), void* param)
{
	int res;
	if((res = pthread_create(&m_thread, NULL, thread_proc, param)))
	{
	    PRN_ERROR("class_thread::Thread creation error, rc:%d\n", res);
	    return RC_THREAD_CREATE_ERROR;
	}
	m_thread_created = 1;
	return RC_OK;
}

int class_thread::destroy(void)
{
	if (m_thread_created == 0)
		return RC_OK;

	void*res;
	pthread_cancel(m_thread);
	pthread_join(m_thread, &res);
	m_thread_created = 0;
	return RC_OK;
}
