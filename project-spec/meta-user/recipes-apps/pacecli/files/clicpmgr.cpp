/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This file implements the class: CLI control protocol
 *      this a command protocol used to communicate CLI-frontend and CLI-serv
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clicpmgr.h"
#include "gen-types.h"

class_clicpimgr::class_clicpimgr()
{
	m_cmd_trans_id = 0;
}

class_clicpimgr::~class_clicpimgr()
{
}

int class_clicpimgr::connect(const char* ip, int port)
{
	int rc;
	if ((rc = m_socket.create()) < 0)
	{
		PRN_ERROR("create socket error, rc:%d\n", rc);
		return rc;
	}

    //PRN_INFO("Connecting to cli-serv: IP:[%s], port:[%d]\n", ip, port);
    if ((rc = m_socket.connect(ip, port)) < 0)
    {
		//PRN_ERROR("socket connection error, rc:%d\n", rc);
		return rc;
	}

	return RC_OK;
}

int class_clicpimgr::get_version(int& version)
{
	clicp_cmd* pcmd = recv_cmd();

	if (pcmd == NULL)
		return RC_CLICP_RECV_CMD_ERROR;

	if (pcmd->cmd_id == NCID_VERSION)
	{
		version = pcmd->params.version.version;
	}

	free(pcmd);
	return RC_OK;
}

clicp_cmd* class_clicpimgr::alloc_cmd(int cmdid, int drvid, unsigned int extra_size)
{
	int param_size = 0;
	switch (cmdid)
	{
		case NCID_REG_SET:
			param_size = sizeof(clicpi_cmd_reg_set);
			break;

		case NCID_REG_GET:
			param_size = sizeof(clicpi_cmd_reg_get);
			break;

		case NCID_REG_OR:
			param_size = sizeof(clicpi_cmd_reg_or);
			break;

		case NCID_REG_XOR:
			param_size = sizeof(clicpi_cmd_reg_xor);
			break;

		case NCID_REG_AND:
			param_size = sizeof(clicpi_cmd_reg_and);
			break;

		case NCID_DUMP:
			param_size = sizeof(clicpi_cmd_dump);
			break;

		case NCID_DOWNLOAD:
			param_size = sizeof(clicpi_cmd_download);
			break;

		case NCID_UPLOAD:
			param_size = sizeof(clicpi_cmd_upload);
			break;

		case NCID_MAP:
			param_size = sizeof(clicpi_cmd_map);
			break;

		case NCID_READ_DATA:
			param_size = sizeof(clicpi_cmd_read_data);
			break;

		case NCID_WRITE_DATA:
			param_size = sizeof(clicpi_cmd_read_data);
			break;

		case NCID_GET_FILE:
			param_size = sizeof(clicpi_cmd_get_file);
			break;

		case NCID_FIFO_CTRL:
			param_size = sizeof(clicpi_cmd_fifo_ctrl);
			break;

		default:
			return NULL;
	}

	clicp_cmd* pclicmd = (clicp_cmd*)malloc(CMD_MAIN_HDR_SIZE()+param_size+extra_size);
	if (pclicmd == NULL)
		return NULL;

	memset(pclicmd, 0, CMD_MAIN_HDR_SIZE()+param_size+extra_size);

	pclicmd->cmd_id = cmdid;
	pclicmd->cmd_drv_id = drvid;
	pclicmd->cmd_param_len = param_size+extra_size;

	return pclicmd;
}

int class_clicpimgr::send_cmd(clicp_cmd* pclicmd, clicp_cmd*& pclicmd_ack)
{
	if (pclicmd == NULL)
		return RC_CLICP_PARAM_ERROR;

	if (!m_socket.is_connected())
		return RC_CLICP_CONNECTION_ERROR;

	int res = m_socket.send(pclicmd, CMD_SIZE(pclicmd));
	if (res != (int)CMD_SIZE(pclicmd))
		return RC_CLICP_SEND_CMD_ERROR;

	pclicmd_ack = recv_cmd();
	return pclicmd_ack != NULL ? RC_OK : RC_CLICP_RECV_ACK_ERROR;
}

clicp_cmd* class_clicpimgr::recv_cmd(void)
{
	clicp_cmd* pcmd;
	clicp_cmd  header;

	if (!m_socket.is_connected())
		return NULL;

	int res = m_socket.recv(&header, CMD_MAIN_HDR_SIZE());
	if (res < 0)
		return NULL;

	pcmd = (clicp_cmd*)malloc(header.cmd_param_len + CMD_MAIN_HDR_SIZE());
	if (pcmd == NULL)
		return NULL;

	memcpy(pcmd, &header, CMD_MAIN_HDR_SIZE());

	if (pcmd->cmd_param_len > 0)
	{
		res = m_socket.recv((char*)pcmd + CMD_MAIN_HDR_SIZE(), header.cmd_param_len);
		if (res != (int)header.cmd_param_len)
		{
			free(pcmd);
			return NULL;
		}
	}
	return pcmd;
}

int send_to_cliserv(clicp_cmd* pclicmd)
{
	return RC_OK;
}

int send_to_h2(clicp_cmd* pclicmd)
{
	return RC_OK;
}

