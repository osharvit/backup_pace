/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager handles LUA IPC requests
 *    In this case LUA is an IPC client
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

#include "gen-types.h"
#include "luaipcmgr.h"
#include "paceipc/ipclib.h"

class_lua_ipc_mgr::class_lua_ipc_mgr(void)
{
	m_free_id = 0;
}

class_lua_ipc_mgr::~class_lua_ipc_mgr(void)
{

}

int class_lua_ipc_mgr::ipc_open(const char* ip, unsigned long port)
{
	void *ipc_cfg_client;
	void *h_ipc_client = NULL;

	ipc_cfg_client = ipc_create_cfg(NULL);
	ipc_set_cfg(ipc_cfg_client, IPC_CFG_PORT, IPC_CFG_VAL(port));
	ipc_set_cfg(ipc_cfg_client, IPC_CFG_IP, IPC_CFG_VAL(ip));
	int rc = ::ipc_open(ipc_cfg_client, &h_ipc_client);
	ipc_destroy_cfg(ipc_cfg_client);

	if (rc < 0)
		return RC_LUA_IPC_CONNECT_ERROR;

	ipc_channel chan;
	chan.h_ipc_channel = h_ipc_client;
	chan.h_id          = get_free_id();
	m_cons.push_back(chan);

	return chan.h_id;
}

int class_lua_ipc_mgr::ipc_close(int hipc)
{
	int idx;
	ipc_channel chan = get_ipc_by_id(hipc, idx);

	if (chan.h_ipc_channel != NULL)
	{
		::ipc_close(chan.h_ipc_channel);

		if (chan.h_ipc_ret_val != NULL)
		{
			::ipc_call_free_return(chan.h_ipc_ret_val);
			chan.h_ipc_ret_val = NULL;
		}
		m_cons.erase(m_cons.begin()+idx);
	}

	return RC_OK;
}

int class_lua_ipc_mgr::ipc_call(int hipc, int api_id, lua_ipc_params & params, lua_ipc_parameter& ret)
{
	int idx = 0;
	ipc_channel ipc_ch = get_ipc_by_id(hipc, idx);

	void* param_list = NULL;
	if (params.list.size() > 0)
	{
		for (unsigned int i = 0; i < params.list.size(); i++)
		{
			int type_id = params.list[i].ipc_data_type_id;
			if (i == 0)
			{
				switch(type_id)
				{
					case IPC_APT_U8:
					case IPC_APT_U16:
					case IPC_APT_U32:
					case IPC_APT_U64:
						param_list = ::ipc_create_param_list(type_id, &params.list[i].ipc_data.u64);
						break;

					case IPC_APT_STR:
						param_list = ::ipc_create_param_list(type_id, (void*)params.list[i].ipc_data.ipc_str);
						break;

					case IPC_APT_PVOID:
						param_list = ::ipc_create_param_list(type_id, params.list[i].ipc_data.ipc_void_ptr);
						break;

					default:
						return RC_LUA_IPC_PARAM_TYPE_ERROR;
				}

				if (param_list == NULL)
					return RC_LUA_IPC_CREATE_PARAM_ERROR;
			}

			else
			{
				switch(type_id)
				{
					case IPC_APT_U8:
					case IPC_APT_U16:
					case IPC_APT_U32:
					case IPC_APT_U64:
					{
						if (::ipc_add_param(param_list, type_id, &params.list[i].ipc_data.u64) < 0)
						{
							ipc_destroy_param_list(param_list);
							param_list = NULL;
							return RC_LUA_IPC_CREATE_PARAM_ERROR;
						}
					}
					break;

					case IPC_APT_STR:
					{
						if (::ipc_add_param(param_list, type_id, (void*)params.list[i].ipc_data.ipc_str) < 0)
						{
							ipc_destroy_param_list(param_list);
							param_list = NULL;
							return RC_LUA_IPC_CREATE_PARAM_ERROR;
						}
					}
					break;

					case IPC_APT_PVOID:
					{
						if (::ipc_add_param(param_list, type_id, params.list[i].ipc_data.ipc_void_ptr) < 0)
						{
							ipc_destroy_param_list(param_list);
							param_list = NULL;
							return RC_LUA_IPC_CREATE_PARAM_ERROR;
						}
					}
					break;

					default:
					{
						ipc_destroy_param_list(param_list);
						param_list = NULL;
						return RC_LUA_IPC_PARAM_TYPE_ERROR;
					}
				}
			}
		}
	}

	void* h_ret = ::ipc_call_l(ipc_ch.h_ipc_channel, api_id, param_list);
	m_cons[idx].h_ipc_ret_val = h_ret;

	::ipc_destroy_param_list(param_list);
	param_list = NULL;

	if (::ipc_return_get_error_code(h_ret) < 0)
		return ::ipc_return_get_error_code(h_ret);

	ret.ipc_data_size = ::ipc_return_get_data_size(h_ret);
	ret.ipc_data_type_id = ::ipc_return_get_data_type(h_ret);
	switch(ret.ipc_data_type_id)
	{
		case IPC_APT_U8:
			ret.ipc_data.u8 = *(uint8_t*)ipc_return_get_data(h_ret);
			break;
		case IPC_APT_U16:
			ret.ipc_data.u16 = *(uint16_t*)ipc_return_get_data(h_ret);
			break;
		case IPC_APT_U32:
			ret.ipc_data.u32 = *(uint32_t*)ipc_return_get_data(h_ret);
			break;
		case IPC_APT_U64:
			ret.ipc_data.u64 = *(uint64_t*)ipc_return_get_data(h_ret);
			break;

		case IPC_APT_STR:
			ret.ipc_data.ipc_str = (const char*)ipc_return_get_data(h_ret);
			break;

		default:
			ret.ipc_data.ipc_void_ptr = ipc_return_get_data(h_ret);
			break;
	}
	return RC_OK;
}

int class_lua_ipc_mgr::ipc_call_done(unsigned int hipc)
{
	int idx = 0;
	ipc_channel ipc_ch = get_ipc_by_id(hipc, idx);

	if (ipc_ch.h_ipc_channel != NULL)
	{
		if (ipc_ch.h_ipc_ret_val != NULL)
			::ipc_call_free_return(ipc_ch.h_ipc_ret_val);

		ipc_ch.h_ipc_ret_val = NULL;
	}
	return RC_OK;
}

int class_lua_ipc_mgr::get_free_id(void)
{
	int val = m_free_id++;
	if (m_free_id > 0xFFFF)
		m_free_id = 0;
	return val;
}

ipc_channel class_lua_ipc_mgr::get_ipc_by_id(int id, int& idx)
{
	std::vector<ipc_channel>::iterator i;
	for (i = m_cons.begin(); i < m_cons.end(); i++)
	{
		if ((*i).h_id == id)
		{
			idx = i - m_cons.begin();
			return *i;
		}
	}
	idx = -1;
	return ipc_channel();
}
