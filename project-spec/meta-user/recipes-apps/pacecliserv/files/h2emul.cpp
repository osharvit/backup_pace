/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class emulates H2 APP connection, it can receive the JSON commands
 *  to print them and to return some responces, also it may generate some indications
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
#include <string.h>
#include "h2emul.h"

static const char* p_h2_resp =
"{ \"header\": {\"type\": \"rsp\", \"uid\": 14105}, \n\
\"body\": {											\n\
\"create_obj_rsp\": {								\n\
\"rx_antport:0\": \"rx0\",							\n\
\"rx_antport:1\": \"rx1\",							\n\
\"tx_antport:0\": \"tx0\",							\n\
\"tx_antport:1\": \"tx1\"							\n\
}													\n\
}													\n\
}													\
";

#ifdef H2_SEND_IND
static const char* p_h2_ind =
"{ \"header\": {\"type\": \"ind\", \"uid\": 0}, \n\
\"body\": {										\n\
\"restarted_ind\": {							\n\
\"re:0\": {										\n\
\"reset_reason\": \"requested\"					\n\
}												\n\
}												\n\
}												\n\
}												\n\
";
#endif

class_h2_emulator::class_h2_emulator(void)
{

}

class_h2_emulator::~class_h2_emulator(void)
{

}

int class_h2_emulator::init(CLF_PARAMS_TYPE* p)
{
	return RC_OK;
}

int class_h2_emulator::run_client(class_socket* psocket)
{
	char buf[2048];

	int res;
	m_psocket = psocket;

	while (1)
	{
		res = m_psocket->recv_data(buf, sizeof(buf)-1);
		if (res <= 0)
			break;

		buf[res] = 0;
		PRN_INFO("-----------------------------------------\n");
		PRN_INFO("H2-CMD:\n");
		PRN_INFO("-----------------------------------------\n");
		PRN_INFO("%s\n", buf);
		PRN_INFO("-----------------------------------------\n");

		// to generate the fake responce
		// ----------------------------------------------------

		res = m_psocket->send((void*)p_h2_resp, strlen(p_h2_resp));
		if (res < 0)
		{
			PRN_ERROR("Error to send fake response, rc:%d\n", res);
			break;
		}

		#ifdef H2_SEND_IND
		res = m_psocket->send((void*)p_h2_ind, strlen(p_h2_ind));
		if (res < 0)
		{
			PRN_ERROR("Error to send fake response, rc:%d\n", res);
			break;
		}
		#endif
	}

	PRN_INFO("The H2 session is ended\n");

	delete (psocket);
	m_psocket = NULL;

	return RC_OK;
}

void class_h2_emulator::close_clients(void)
{
	
}

void class_h2_emulator::close_stopped(void)
{
	
}
