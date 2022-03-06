/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This file defines 3 classes to implement 3 types of the commands
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
#include <string.h>
#include "cmd.h"
#include "gen-types.h"

/****************************************************************************************************
*
*                  BASE CLASS (COMMANDS)
*
*****************************************************************************************************/


class_cmd::class_cmd(void)
{
}

class_cmd::~class_cmd(void)
{
}

int class_cmd::set_name(const char* name)
{
	if (name == NULL)
		return RC_CMD_PARAM_ERROR;

	if (strcmp(name, "") == 0)
		return RC_CMD_PARAM_ERROR;

	m_name = name;
	return RC_OK;
}

const char* class_cmd::get_name(void)
{
	return m_name.c_str();
}

int class_cmd::set_id(int id)
{
	m_id = id;
	return RC_OK;
}

int class_cmd::get_id(void)
{
	return m_id;
}

int class_cmd::set_descr(const char* descr)
{
	if (descr == NULL)
		return RC_CMD_PARAM_ERROR;

	m_descr = descr;
	return RC_OK;
}

const char* class_cmd::get_descr(void)
{
	return m_descr.c_str();
}


/****************************************************************************************************
*
*                  NATIVE COMMANDS
*
*****************************************************************************************************/

class_native_cmd::class_native_cmd(void)
{
	m_addr_type = 0;
	m_id = 0;
	m_value_type = 0;
	m_bits = 32;
	m_filename = "";
	m_address = "";
	m_hwname = "";
	m_regname = "";
	m_message = "";
	m_value = "";
	m_is_message = 1;
}

class_native_cmd::class_native_cmd(int id)
{
	m_addr_type = 0;
	m_value_type = 0;
	m_id = id;
	m_name = id_to_name(id);
	m_bits = 32;
	m_address = "";
	m_hwname = "";
	m_regname = "";
	m_message = "";
	m_value = "";
	m_is_message = 1;
}

class_native_cmd::~class_native_cmd(void)
{
}

cmd_type class_native_cmd::get_cmd_type(void)
{
	return CMD_NATIVE;
}


void* class_native_cmd::packcmd(void)
{
	

	return 0;
}

int class_native_cmd::unpack(void* pdata)
{
	return 0;
}

int class_native_cmd::set_address(const char* addr)
{
	if (addr == NULL)
		return RC_CMD_PARAM_ERROR;

	m_address = addr;
	m_addr_type = 1;
	return RC_OK;
}

int class_native_cmd::set_compreg_name(const char* comp, const char* reg)
{
	if (comp == NULL || reg == NULL)
		return RC_CMD_PARAM_ERROR;

	m_address = comp;
	m_address += ".";
	m_address += reg;

	m_hwname = comp;
	m_regname = reg;
	m_addr_type = 2;
	return RC_OK;
}

int class_native_cmd::set_message(const char* msg)
{
	if (msg == NULL)
		return RC_CMD_PARAM_ERROR;

	m_message = msg;
	return RC_OK;
}

const char* class_native_cmd::get_message(void)
{
	return m_message.c_str();
}

int class_native_cmd::enable_message(int ena)
{
	m_is_message = ena;
	return RC_OK;
}

int class_native_cmd::is_message(void)
{
	return m_is_message;
}

int class_native_cmd::set_value(const char* val, int this_is_variable)
{
	if (val == NULL)
		return RC_CMD_PARAM_ERROR;

	m_value_type = this_is_variable;
	m_value = val;
	return RC_OK;
}

void class_native_cmd::set_file_name(const char* name)
{
	m_filename = name;
}

const char* class_native_cmd::get_file_name(void)
{
	return m_filename.c_str();
}

const char* class_native_cmd::id_to_name(int id)
{
	switch (id)
	{
		case NCID_REG_SET:
			return "reg.set";

		case NCID_REG_GET:
			return "reg.get";

		case NCID_REG_OR:
			return "reg.or";

		case NCID_REG_XOR:
			return "reg.xor";

		case NCID_REG_AND:
			return "reg.and";

		case NCID_SLEEP:
			return "sleep";

		case NCID_PRINT:
			return "print";

		case NCID_DUMP:
			return "dump";

		case NCID_UPLOAD:
			return "upload";

		case NCID_DOWNLOAD:
			return "download";

		default:
			return "unknown";
	}
}

/****************************************************************************************************
*
*                  EXTENDED COMMANDS
*
*****************************************************************************************************/
class_ext_cmd::class_ext_cmd(void)
{
	set_id(NCID_EXT_CMD);
	m_json_resp_format = "";
}

class_ext_cmd::~class_ext_cmd(void)
{

}

cmd_type class_ext_cmd::get_cmd_type(void)
{
	return CMD_EXTENDED;
}

void* class_ext_cmd::packcmd(void)
{
	return NULL;
}

int class_ext_cmd::unpack(void* pdata)
{
	return 0;
}

void class_ext_cmd::set_name(const char* name)
{
	m_name = name;
}

void class_ext_cmd::set_text(const char* text)
{
	m_text = text;
}

const char* class_ext_cmd::get_text(void)
{
	return m_text.c_str();
}

const char* class_ext_cmd::get_name(void)
{
	return m_name.c_str();
}

void class_ext_cmd::set_interface(extcmdinterface inf)
{
	m_interface = inf;
}

extcmdinterface class_ext_cmd::get_interface(void)
{
	return m_interface;
}

void class_ext_cmd::add_param(cmd_param& param)
{
	m_params.push_back(param);
}

const cmd_param& class_ext_cmd::get_param(unsigned int idx)
{
	static cmd_param m_fake_param;

	if (idx >= m_params.size())
		return m_fake_param;

	return m_params[idx];
}

int class_ext_cmd::find_param(const char* param_name)
{
	std::vector<cmd_param>::iterator iter;
	int idx;
	for (idx = 0,iter=m_params.begin(); iter < m_params.end(); ++iter, ++idx)
	{
		if (iter->m_name == param_name)
			return idx;
	}

	return -1;
}

int class_ext_cmd::get_req_param_num(void)
{
	std::vector<cmd_param>::iterator iter;
	int num = 0;
	for (iter=m_params.begin(); iter < m_params.end(); ++iter)
	{
		if (iter->m_defval == "")
		{
			num++;
		}
	}
	return num;
}

const char* class_ext_cmd::get_params_info(void)
{
	static std::string info;
	info = "";

	std::vector<cmd_param>::iterator iter;
	int idx;
	for (idx = 0,iter=m_params.begin(); iter < m_params.end(); ++iter, ++idx)
	{
		if (iter->m_defval != "")
		{
			info += "[" + iter->m_name + "=" + iter->m_defval + "] ";
		}
		else
		{
			info += "<" + iter->m_name + "> ";
		}
	}

	return info.c_str();
}

int class_ext_cmd::set_json_resp_format(const char* pformat)
{
	if (pformat != NULL)
		m_json_resp_format = pformat;
	return RC_OK;
}

const char* class_ext_cmd::get_json_resp_format(void)
{
	return m_json_resp_format.c_str();
}

const char* class_ext_cmd::id_to_name(int id)
{
	return "";
}


/****************************************************************************************************
*
*                  LUA COMMAND CLASS (COMMANDS)
*
*****************************************************************************************************/

class_lua_cmd::class_lua_cmd(void)
{
	m_id = NCID_LUA_CMD;
	m_proc_name = LUA_SCR_ENTRY_POINT_NAME;
}

class_lua_cmd::~class_lua_cmd(void)
{

}

cmd_type class_lua_cmd::get_cmd_type(void)
{
	return CMD_LUA;
}

void* class_lua_cmd::packcmd(void)
{
	return NULL;
}

int class_lua_cmd::unpack(void* pdata)
{
	return 0;
}

const char* class_lua_cmd::id_to_name(int id)
{
	return "lua-script";
}

const std::vector<std::string>& class_lua_cmd::get_params(void)
{
	return m_params;
}

int class_lua_cmd::add_param(const char* p)
{
	m_params.push_back(p);
	return RC_OK;
}

int class_lua_cmd::set_proc_name(const char* p)
{
	m_proc_name = p;
	return RC_OK;
}

const char* class_lua_cmd::get_proc_name(void)
{
	return m_proc_name.c_str();
}