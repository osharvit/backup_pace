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

#ifndef _SYNC_H_
#define _SYNC_H_

#include <pthread.h>
#include "retcodes.h"

class class_sync
{
public:
                            class_sync(void);
                            ~class_sync(void);

                int         wait  (void);
                int         wakeup (void);

protected:

    pthread_cond_t		m_cond;
    pthread_mutex_t		m_mutex;
    unsigned int		m_counter;
};

#endif // _SYNC_H_
