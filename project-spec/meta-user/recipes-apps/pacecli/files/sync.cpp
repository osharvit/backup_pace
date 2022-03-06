
/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This is base OS thread synchronization
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

#include "sync.h"

class_sync::class_sync(void)
{
	pthread_cond_init(&m_cond, NULL);
	pthread_mutex_init(&m_mutex, NULL);
}

class_sync::~class_sync(void)
{

}

int class_sync::wait(void)
{
	pthread_mutex_lock(&m_mutex);
	while (m_counter == 0)
	    pthread_cond_wait(&m_cond, &m_mutex);
	m_counter = 0;
	pthread_mutex_unlock(&m_mutex);
	return RC_OK;
}

int class_sync::wakeup (void)
{
	pthread_mutex_lock(&m_mutex);
	if (m_counter == 0)
	{
		m_counter = 1;
		pthread_cond_signal(&m_cond);
	}
	pthread_mutex_unlock(&m_mutex);
	return RC_OK;
}
