/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class handles CLI clients: class_client
 *
 * This program is free software; you can redistribute it and/or modify
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

#include "clientmgr.h"

class_client_mgr::class_client_mgr(void)
{

}

class_client_mgr::~class_client_mgr(void)
{
	close_clients();
}

int class_client_mgr::init(CLF_PARAMS_TYPE* p)
{
	return RC_OK;
}

int class_client_mgr::run_client(class_socket* psocket)
{
	close_stopped();

	class_client * pclient = new class_client();
	if (pclient == NULL)
		return RC_CLIENT_MGR_CREATE_ERROR;

	int r = pclient->create(psocket);
	if (r < 0)
		return r;

	m_clients.push_back(pclient);
	return RC_OK;
}

void class_client_mgr::close_stopped(void)
{
	std::vector<class_client*>::iterator i;
	int stop = 0;
	while (!stop)
	{
		stop = 1;
		for (i = m_clients.begin(); i < m_clients.end(); i++)
		{
			if ((*i)->is_stopped())
			{
				stop = 0;
				delete (*i);
				m_clients.erase(i);
				break;
			}
		}
	}
}

void class_client_mgr::close_clients(void)
{
	std::vector<class_client*>::iterator i;
	for (i = m_clients.begin(); i < m_clients.end(); i++)
	{
		delete (*i);
	}

	m_clients.clear();
}