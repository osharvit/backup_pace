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
#include <stdlib.h>
#include "h2mgr.h"
#include "gen-types.h"
#include "json.h"

class_h2mgr::class_h2mgr(void)
{
	m_stopped = 0;
	m_thread_created = 0;
	pthread_mutex_init(&m_resp_mutex, NULL);
	pthread_mutex_init(&m_ind_mutex, NULL);
}

class_h2mgr::~class_h2mgr(void)
{
	close();
	pthread_mutex_destroy(&m_resp_mutex);
	pthread_mutex_destroy(&m_ind_mutex);
}

void* h2_thread(void* param)
{
	class_h2mgr* obj = (class_h2mgr*)param;
	int res;

	while (!obj->is_stopped())
	{
		res = obj->proc_input();
		if (res < 0)
			break;
	}

	PRN_INFO("H2 thread is stopped\n");
	return NULL;
}

int class_h2mgr::connect(const char* ip, int port)
{
	int res;
	if ((res = m_socket.create()) < 0)
	{
		PRN_ERROR("class_h2mgr::create socket error, rc:%d\n", res);
		return res;
	}

	res = m_socket.connect(ip, port);
	if (res < RC_OK)
		return res;

	if((res = pthread_create(&m_thread, NULL, h2_thread, this)))
    {
        PRN_ERROR("class_h2mgr::Thread creation error, rc:%d\n", res);
        return RC_H2_THREAD_CREATE_ERROR;
    }
	m_thread_created = 1;
	return res;
}

int class_h2mgr::send(int devid, int cmdid, std::string& cmd, std::string& resp)
{
	int found;
	int res = m_socket.send((void*)cmd.c_str(), cmd.length()+1);

	if (res < RC_OK)
		return res;

	// Let's wait for the response
	// ----------------------------
	res = 0;
	found = 0;
	extern volatile unsigned int g_Stop;
	while (!found && !g_Stop)
	{
		// Let's wait for 1ms
		usleep(1000);

		pthread_mutex_lock(&m_resp_mutex);

		// Here, let's scan the input queue
		int size = m_resp_queue.size();
		h2_q_data h2_msg;

		while (size > 0)
		{
			h2_msg = m_resp_queue.front();
			m_resp_queue.pop();

			//if (h2_msg.m_cmdid == cmdid)
			{
				found = 1;
				resp = h2_msg.m_data;
				break;
			}

			m_resp_queue.push(h2_msg);
			size --;
		}

		pthread_mutex_unlock(&m_resp_mutex);
	}

	return res;
}

int class_h2mgr::recv(int devid, std::string& resp)
{
	char buf[2048];

	// Here is to read the socket
	int res = m_socket.recv_data(buf, sizeof(buf)-1);

	if (res > 0)
	{
		buf[res] = 0;

		#if 0
		PRN_INFO("------------------------------------\n");
		PRN_INFO(" H2 response:\n");
		PRN_INFO("------------------------------------\n");
		PRN_INFO("%s\n", buf);
		PRN_INFO("------------------------------------\n");
		#endif

		resp = buf;
	}
	return res;
}

int class_h2mgr::proc_input(void)
{
	h2_q_data h2_msg;

	h2_msg.m_cmdid = 1;
	h2_msg.m_devid = 0;
	int res = recv(h2_msg.m_devid, h2_msg.m_data);

	if (res > 0)
	{
		// here, let's add this message to the input queue
		// and to wake up the main sleeping thread
		// this message is the <resp>
		// The indications should go to the specific queue 

		class_json json;
		res = json.create(h2_msg.m_data.c_str());
		if (res < RC_OK)
		{
			PRN_ERROR("------------------------------------------\n");
			PRN_ERROR("Error to parse JSON responce (error:%d)\n", res);
			PRN_ERROR("------------------------------------------\n");
			PRN_ERROR("%s\n", h2_msg.m_data.c_str());
			PRN_ERROR("------------------------------------------\n");
			return RC_OK;
		}

		// if this is the response
		// ------------------------
		if (json.getobjdata("header.type") == "rsp")
		{
			pthread_mutex_lock(&m_resp_mutex);
			h2_msg.m_cmdid = atoi(json.getobjdata("header.uid").c_str());
			m_resp_queue.push(h2_msg);
			pthread_mutex_unlock(&m_resp_mutex);
		}
		else
		{
			#if 0
			pthread_mutex_lock(&m_ind_mutex);
			m_ind_queue.push(h2_msg);
			pthread_mutex_lock(&m_ind_mutex);
			#else

			// To print to the console H2 indication
			PRN_INFO("\nH2-IND: %s\n", json.print_obj().c_str());
			#endif
		}
	}
	else
	{
		if (res == 0)
		{
			PRN_INFO("H2 socket is closed\n");
		}
		else
		{
			PRN_INFO("H2 read socket error, res:%d\n", res);
		}
		return RC_H2_READ_SOCKET_ERROR;
	}
	return RC_OK;
}

int class_h2mgr::close(void)
{
	m_stopped = 1;

	if (m_thread_created)
	{
		void*res;
		pthread_cancel(m_thread);
		pthread_join(m_thread, &res);
		m_thread_created = 0;
	}

	m_socket.close();
	return RC_OK;
}
