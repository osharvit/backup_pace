/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    To caclulate the mathematic operations
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "rc_cmdmgr.h"
#include "retcodes.h"
#include "gen-types.h"
#include "cmdmgr.h"
#include "helpmgr.h"

void* rc_cmdmgr_clients_handler(void* p)
{
	class_rccmdmgr* _this = (class_rccmdmgr*)p;
	int h_serv_socket = _this->m_socket.get_socket();
	class_socket* p_client;

	while(1)
	{
		int rc, i, num;
	    fd_set set;
	    FD_ZERO(&set);
	    FD_SET(h_serv_socket, &set);

	    num = (int)h_serv_socket;

	    for (i = 0; i < _this->get_client_num(); i++)
	    {
	        FD_SET(_this->m_clients[i].m_client->get_socket(), &set);
	        num = (num < _this->m_clients[i].m_client->get_socket()) ? _this->m_clients[i].m_client->get_socket() : num;
	    }

	     if ((rc = select(num + 1, &set, NULL, NULL, NULL)) < 0)
	     {
	        break;
	     }

	    // Let's check what the system returned
	    // -------------------------------------
	    if (FD_ISSET(h_serv_socket, &set))
	    {
	    	p_client = _this->m_socket.accept();
	        if (p_client != NULL)
	       	{
	       		_this->add_client(p_client);
				continue;
	        }
	    }

	    for (i = 0; i < _this->get_client_num(); i++)
	    {
			int h = _this->m_clients[i].m_client->get_socket();
		
	        if (FD_ISSET(h, &set))
	        {
	            if ( (rc = _this->proc_client(_this->m_clients[i].m_client)) < RC_OK)
	            {
					_this->del_client(_this->m_clients[i].m_client);
				}
	        }
	    }
	}

	return NULL;
}

class_rccmdmgr::class_rccmdmgr(void)
{
	m_pcmdmgr = NULL;
}


class_rccmdmgr::~class_rccmdmgr(void)
{
	m_thread.destroy();
	del_all_clients();
}

int class_rccmdmgr::init(CLF_PARAMS_TYPE* p, class_cmdmgr* pmgr)
{
	m_pcmdmgr = pmgr;

	int rc = m_socket.create(0);
	if (rc < RC_OK)
		return rc;

	m_socket.reuse(1);

	rc = m_socket.bind("0.0.0.0", p->rc_port);
	if (rc < RC_OK)
		return rc;

	PRN_INFO("[port:%d]", p->rc_port);

	rc = m_socket.listen(10);
	if (rc < RC_OK)
		return rc;

	// Here we need to create the thread that will handle the client connections
	// and to handle their commands

	rc = m_thread.create(rc_cmdmgr_clients_handler, this);
	if (rc < RC_OK)
		return rc;

	return RC_OK;
}

int class_rccmdmgr::add_client(class_socket* pclient)
{
	rc_client rc;
	rc.m_client = pclient;
	m_clients.push_back(rc);
	return RC_OK;
}

int class_rccmdmgr::del_client(class_socket* pclient)
{
	std::vector<rc_client>::iterator i;
	for (i = m_clients.begin(); i < m_clients.end(); i++)
	{
		if ((*i).m_client == pclient)
		{
			delete ((*i).m_client);
			m_clients.erase(i);
			break;
		}
	}

	return RC_OK;
}

void class_rccmdmgr::del_all_clients(void)
{
	std::vector<rc_client>::iterator i;
	for (i = m_clients.begin(); i < m_clients.end(); i++)
	{
		if ((*i).m_client != NULL)
			delete ((*i).m_client);

		(*i).m_client = NULL;
	}

	m_clients.clear();
}

int  class_rccmdmgr::get_client_num(void)
{
	return m_clients.size();
}

int class_rccmdmgr::proc_client(class_socket* client)
{
	int rc = RC_OK;
	rc_cmd_header cmd;
	std::string output;

	rc = client->recv(&cmd, sizeof(cmd));
	if (rc < RC_OK)
		return rc;

    void* pdata = malloc(cmd.data_size + 1);
	if (pdata == NULL)
		return RC_REMOTE_CTRL_ALLOC_ERROR;

	data_ptr<void*> data(pdata);

	rc = client->recv(pdata, cmd.data_size);
	if (rc != (int)cmd.data_size)
		return RC_REMOTE_CTRL_RECV_ERROR;

	// From the safity point of view,
	// to add \0 terminator
	((char*)pdata)[cmd.data_size] = 0;

	switch(cmd.cmd_id)
	{
		case REMOTE_CMD_ID_RUN_LUA_SCRIPT:
		{
			m_pcmdmgr->redirect_console_to_file();
			rc = m_pcmdmgr->exec_lua_script((const char*)pdata);
			m_pcmdmgr->restore_console(&output);
			rc = send_resp(client, REMOTE_CMD_ID_RUN_LUA_SCRIPT, output.length()+1, output.c_str());
			break;
		}

		case REMOTE_CMD_ID_RUN_CMD:
		{
			m_pcmdmgr->redirect_console_to_file();
			rc = m_pcmdmgr->run_commands((const char*)pdata);
			m_pcmdmgr->restore_console(&output);
			rc = send_resp(client, REMOTE_CMD_ID_RUN_CMD, output.length()+1, output.c_str());
			break;
		}

		case REMOTE_CMD_ID_GET_REG:
		{
			uint64_t value;
			struct rc_cmd_get_reg_ack ack;
			struct rc_cmd_get_reg* pcmd = (struct rc_cmd_get_reg*)pdata;

			std::string bits = class_helpmgr::to_string(pcmd->bits);
			std::string addr = class_helpmgr::to_string(pcmd->addr);

			ack.rc = m_pcmdmgr->get_reg(bits.c_str(), addr.c_str(), value, NULL);
			ack.value = (unsigned long long)value;

			rc = send_resp(client, REMOTE_CMD_ID_GET_REG, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_SET_REG:
		{
			struct rc_cmd_set_reg_ack ack;
			struct rc_cmd_set_reg* pcmd = (struct rc_cmd_set_reg*)pdata;

			std::string bits = class_helpmgr::to_string(pcmd->bits, 1);
			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string value = class_helpmgr::to_string(pcmd->value, 1);
			ack.rc = m_pcmdmgr->set_reg(NCID_REG_SET, bits.c_str(), addr.c_str(), value.c_str(), NULL);
			rc = send_resp(client, REMOTE_CMD_ID_SET_REG, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_OR_REG:
		{
			struct rc_cmd_set_reg_ack ack;
			struct rc_cmd_set_reg* pcmd = (struct rc_cmd_set_reg*)pdata;

			std::string bits = class_helpmgr::to_string(pcmd->bits, 1);
			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string value = class_helpmgr::to_string(pcmd->value, 1);
			ack.rc = m_pcmdmgr->set_reg(NCID_REG_OR, bits.c_str(), addr.c_str(), value.c_str(), NULL);
			rc = send_resp(client, REMOTE_CMD_ID_OR_REG, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_XOR_REG:
		{
			struct rc_cmd_set_reg_ack ack;
			struct rc_cmd_set_reg* pcmd = (struct rc_cmd_set_reg*)pdata;

			std::string bits = class_helpmgr::to_string(pcmd->bits, 1);
			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string value = class_helpmgr::to_string(pcmd->value, 1);
			ack.rc = m_pcmdmgr->set_reg(NCID_REG_XOR, bits.c_str(), addr.c_str(), value.c_str(), NULL);
			rc = send_resp(client, REMOTE_CMD_ID_XOR_REG, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_AND_REG:
		{
			struct rc_cmd_set_reg_ack ack;
			struct rc_cmd_set_reg* pcmd = (struct rc_cmd_set_reg*)pdata;

			std::string bits = class_helpmgr::to_string(pcmd->bits, 1);
			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string value = class_helpmgr::to_string(pcmd->value, 1);
			ack.rc = m_pcmdmgr->set_reg(NCID_REG_AND, bits.c_str(), addr.c_str(), value.c_str(), NULL);
			rc = send_resp(client, REMOTE_CMD_ID_AND_REG, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_WRITE_DATA:
		{
			struct rc_cmd_write_data_ack ack;
			struct rc_cmd_write_data* pcmd = (struct rc_cmd_write_data*)pdata;

			ack.rc = m_pcmdmgr->writedata(pcmd->bits, pcmd->addr, pcmd+1, cmd.data_size - sizeof(struct rc_cmd_write_data));
			rc = send_resp(client, REMOTE_CMD_ID_WRITE_DATA, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_READ_DATA:
		{
			struct rc_cmd_read_data_ack ack;
			struct rc_cmd_read_data_ack* pack;
			struct rc_cmd_read_data* pcmd = (struct rc_cmd_read_data*)pdata;
			void* pdata;

			pack = (struct rc_cmd_read_data_ack*)malloc(sizeof(struct rc_cmd_read_data_ack) + pcmd->size);
			if (pack == NULL)
			{
				ack.rc = RC_REMOTE_CTRL_ALLOC_ERROR;
				rc = send_resp(client, REMOTE_CMD_ID_READ_DATA, sizeof(ack), &ack);
				break;
			}

			pdata = pack+1;
			pack->rc = m_pcmdmgr->readdata(pcmd->bits, pcmd->addr, pcmd->size, pdata);

			if (pack->rc < 0)
			{
				// to send just a header with error code
				// ------------------------------------------------------------------
				rc = send_resp(client, REMOTE_CMD_ID_READ_DATA, sizeof(struct rc_cmd_read_data_ack), pack);
			}
			else
			{
				// to send a header + data
				// ------------------------------------------------------------------
				rc = send_resp(client, REMOTE_CMD_ID_READ_DATA, sizeof(*pack) + pack->rc, pack);
			}
			free(pack);
			pack=NULL;
			break;
		}

		case REMOTE_CMD_ID_SAVE_FILE:
		{
			struct rc_cmd_save_file_ack ack;
			struct rc_cmd_save_file* pcmd = (struct rc_cmd_save_file*)pdata;

			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string size = class_helpmgr::to_string(pcmd->size, 0);
			std::string bits = class_helpmgr::to_string(pcmd->bits, 0);
			std::string name = pcmd->filename;

			ack.rc = m_pcmdmgr->download(bits.c_str(), addr.c_str(), size.c_str(), name.c_str());
			rc = send_resp(client, REMOTE_CMD_ID_SAVE_FILE, sizeof(ack), &ack);
			break;
		}

		case REMOTE_CMD_ID_LOAD_FILE:
		{
			struct rc_cmd_load_file_ack ack;
			struct rc_cmd_load_file* pcmd = (struct rc_cmd_load_file*)pdata;

			std::string addr = class_helpmgr::to_string(pcmd->addr, 1);
			std::string bits = class_helpmgr::to_string(pcmd->bits, 0);
			std::string name = pcmd->filename;

			ack.rc = m_pcmdmgr->upload(bits.c_str(), name.c_str(), addr.c_str());
			rc = send_resp(client, REMOTE_CMD_ID_LOAD_FILE, sizeof(ack), &ack);
			break;
		}

		default:
		{
			rc = send_resp(client, REMOTE_CMD_ID_UNKNOWN_CMD, 0, NULL);
			break;
		}
	}

	return RC_OK;
}

int class_rccmdmgr::send_resp(class_socket* client, unsigned int cmd_id, unsigned int data_size, const void* pdata)
{
	rc_cmd_header cmd;

	cmd.cmd_id = cmd_id;
	cmd.data_size = data_size;

	int rc = client->send(&cmd, sizeof(cmd));
	if ((unsigned int)rc != sizeof(cmd))
		return RC_REMOTE_CTRL_SEND_ERROR;

	if (data_size != 0 && pdata != NULL)
	{
		rc = client->send((void*)pdata, data_size);
		if ((unsigned int)rc != data_size)
			return RC_REMOTE_CTRL_SEND_ERROR;
	}
	return RC_OK;
}
