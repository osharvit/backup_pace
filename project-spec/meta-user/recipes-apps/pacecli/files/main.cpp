/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <spawn.h>
#include <wait.h>
#include <stdint.h>
#include <errno.h>
#include "gen-types.h"
#include "lexer.h"
#include "varmgr.h"
#include "cmd.h"
#include "cmdmgr.h"
#include "math.h"
#include "file.h"
#include "json.h"
#include "lua.hpp"
#include "sync.h"
#include "helpmgr.h"
#include "cmdeditor.h"
#include "path.h"

std::string* 				g_putputdev = NULL;
volatile unsigned int		g_Stop = 0;
const char*					g_command_ptr = NULL;
class_cmdmgr 				g_mgr;
pthread_t					g_mainthread;
class_sync					g_sync_run_cmd;
class_sync					g_sync_done_cmd;
CLF_PARAMS_TYPE 			g_params;
class_cmd_editor			g_cmdedit;

void PRN_SET_DEV(std::string* ptr)
{
	g_putputdev = ptr;
}

int PRN_IS_DEV(void)
{
	return g_putputdev != NULL;
}

int PRN_SEND_TO_DEV(const char* pdata)
{
	if (g_putputdev != NULL)
	{
		(*g_putputdev) += pdata;
	}

	return 0;
}

int usage (void)
{
	printf ("<pacecli> application usage:\n");
	printf (" pacecli [options]\n");
	printf ("  where options are:\n");
	printf ("  -h (--help)               - To see the help\n");
	printf ("  --ip <ip>                 - To set CLI server IP address, by def: %s\n", DEF_SERV_IP);
	printf ("  --port <port>             - To set CLI server port, by def: %d\n", DEF_SERV_PORT);
	printf ("  --h2 <port>               - To set H2 application port, if set to 0, the connection will not be set, by def: %d\n", DEF_H2_PORT);
	printf ("  -f <fiename>              - To run the commands from the files instead of taking it from the console\n");
	printf ("  --file <fiename>          - To run the commands from the files instead of taking it from the console\n");
	printf ("  -c <cli command>          - To run the CLI command, it's possible to runmany commands: --cmd <cmd1> --cmd <cmd2> ...\n");
	printf ("  --cmd <cli command>       - To run the CLI command, it's possible to runmany commands: --cmd <cmd1> --cmd <cmd2> ...\n");
	printf ("  --logfile <fiename>       - To set the log file\n");
	printf ("  --logctrl <mask>          - To control the logging: 1 - to enable, 0 - to disable, by def:0x%x\n", DEF_LOG_CTRL);
	printf ("  --rc <port>               - To enable/disable remote commands manager, by default it is %s, the def port is %d\n", DEF_REMOTE_CTRL ? "enabled":"disabled", DEF_REMOTE_CTRL_PORT);
	printf ("  --ipc <port>              - To enable/disable IPC manager, by default it is %s, the def port is %d\n", DEF_IPC_PORT ? "enabled":"disabled", DEF_IPC_PORT);
	printf ("  --tabfile <1|0>           - To enable/disable printing of current folder files in case of [tab][tab] command, by default it's %s (%d)\n", DEF_TAB_FILE_LIST ? "enabled":"disabled", DEF_TAB_FILE_LIST);
	printf ("  --input 0                 - To use the limited input text instead of the extended command editor\n");
	return 0;
}

void print_app_header(void)
{
	PRN_INFO("---------------------------------------------------------------\n");
	PRN_INFO("          PACE CLI application, version:%s\n", VERSION);
	PRN_INFO("---------------------------------------------------------------\n");
}

unsigned int param_get_num(char * pVal)
{
   unsigned int val = 0;

   if (optarg[0]=='0' && (optarg[1]=='x'||optarg[1]=='X'))
   {
        sscanf (optarg, "%x", &val);
   }
   else
   {
        val = (unsigned int)atoi(optarg);
   }
   return val;
}

int param_pars (int argc, char ** argv, CLF_PARAMS_TYPE * params)
{
    int c = 0;
    memset (params, 0, sizeof (*params));

	params->h2_port = DEF_H2_PORT;
	params->serv_port = DEF_SERV_PORT;
	strcpy (params->serv_ip, DEF_SERV_IP);
	params->log_ctrl = DEF_LOG_CTRL;

	params->rc_port = DEF_REMOTE_CTRL_PORT;
	params->ipc_port= DEF_IPC_PORT;

	params->flags |= (DEF_REMOTE_CTRL) ? (CLF_REMOTE_CTRL) : (0);
	params->flags |= (DEF_IPC_PORT) ? (CLF_IPC_PORT) : (0);

	params->flags |= (DEF_TAB_FILE_LIST) ? (CLF_TAB_FILE_LIST):0;
	params->tab_files = DEF_TAB_FILE_LIST;

    while (1)
    {
        //int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        static struct option long_options[] = 
        {
            {"help",            no_argument,       0, 'h'},
	    {"ip",              required_argument, 0, CLF_SERV_IP},
	    {"port",            required_argument, 0, CLF_SERV_PORT},
	    {"h2port",          required_argument, 0, CLF_H2_PORT}, 
            {"file",            required_argument, 0, 'f'},
            {"cmd",             required_argument, 0, 'c'},
            {"logfile",         required_argument, 0, 'l'},
            {"logctr",          required_argument, 0, CLF_LOG_CTRL},
            {"rc",          	required_argument, 0, CLF_REMOTE_CTRL},
            {"ipc",          	required_argument, 0, CLF_IPC_PORT},
            {"tabfile",        	required_argument, 0, CLF_TAB_FILE_LIST},
            {"input",        	required_argument, 0, CLF_INPUT_0_MODE},

			{0,                 0,                 0,  0}
		};

		c = getopt_long(argc, argv, "f:c:h", long_options, &option_index);

		if (c == -1)
			break;

		switch(c)
		{
			case 'h':
				params->flags |= CLF_HELP;
				break;

			case 'f':
				params->flags |= CLF_CMD_FILE_NAME;
				strncpy(params->cmd_file, optarg, sizeof(params->cmd_file));
				break;

			case CLF_SERV_IP:
				params->flags |= CLF_SERV_IP;
				strncpy(params->serv_ip, optarg, sizeof(params->serv_ip));
				break;

			case CLF_SERV_PORT:
				params->flags |= CLF_SERV_PORT;
				params->serv_port = param_get_num(optarg);
				break;

			case CLF_H2_PORT:
				params->flags |= CLF_H2_PORT;
				params->h2_port = param_get_num(optarg);
				break;

			case 'c':

				if (params->cmd_num>=CLF_MAX_NUM_OF_CMD)
				{
					PRN_ERROR("cmd limit(%d) is over\n", params->cmd_num);
					return RC_CMD_LIMIT;
				}
				
				params->flags |= CLF_CMD;
				strncpy(params->cmd[params->cmd_num], optarg, PATH_MAX);
				params->cmd_num++;
				break;

			case 'l':
				params->flags |= CLF_LOG_FILE;
				strncpy(params->log_file, optarg, sizeof(params->log_file));
				break;	

			case CLF_LOG_CTRL:
				params->flags |= CLF_LOG_CTRL;
				params->log_ctrl = param_get_num(optarg);
				break;

			case CLF_REMOTE_CTRL:
				params->rc_port = param_get_num(optarg);
				if (params->rc_port)
					params->flags |= CLF_REMOTE_CTRL;
				else
					params->flags &= ~CLF_REMOTE_CTRL;
				break;

			case CLF_IPC_PORT:
				params->ipc_port = param_get_num(optarg);
				if (params->ipc_port)
					params->flags |= CLF_IPC_PORT;
				else
					params->flags &= ~CLF_IPC_PORT;
				break;

			case CLF_TAB_FILE_LIST:
				params->tab_files = param_get_num(optarg);
				if (params->tab_files)
					params->flags |= CLF_TAB_FILE_LIST;
				else
					params->flags &= ~CLF_TAB_FILE_LIST;
				break;

			case CLF_INPUT_0_MODE:
				if (param_get_num(optarg) == 0)
					params->flags |= CLF_INPUT_0_MODE;
				break;
		}
    }

	return 0;
}

int exec_file_cmd(const char* filename, class_cmdmgr& mgr, int & line_err)
{
	char*buf = NULL;
	size_t len = 0;
	ssize_t nread;

	FILE* pfile = fopen(filename, "r");
	if (pfile == NULL)
		return RC_OPEN_FILE_ERROR;

	line_err = 1;
	while (1)
	{
		nread = getline(&buf, &len, pfile);
		if (nread < 0)
			break;

		//printf("Line(%p):%s\n", buf, buf);
		int r = mgr.run_commands(buf);
		if (r < 0)
			return r;

		line_err++;
	}

	fclose(pfile);
	if (buf!= NULL)
		free(buf);
	return RC_OK;
}

// -----------------------------------------------------------------------------
//                   The C and LUA interconnections
// -----------------------------------------------------------------------------
extern "C"
{

void lua_dump_data(void* p, int size)
{
	uint8_t* 			p8  =  (uint8_t*)p;
	uint16_t* 			p16 = (uint16_t*)p;
	uint32_t* 			p32 = (uint32_t*)p;
	uint64_t* 			p64 = (uint64_t*)p;

	unsigned long long address = (unsigned long long)p;
	int i = 0;
	int bits = 8;

	PRN_INFO("0x%04llx:  ", address);

	while (i < size)
	{
		if (bits == 8)
		{
			PRN_INFO("%02x ", (unsigned int)p8[i]);
			i++;
			if ((i%32)==0)
				PRN_INFO("\n0x%04llx:  ", address+i);
		}
		else if (bits == 16)
		{
			PRN_INFO("%04x ", (uint32_t)p16[i]);
			i++;
			if ((i%16)==0)
				PRN_INFO("\n0x%04llx:  ", address+i*2);
		}
		else if (bits == 32)
		{
			PRN_INFO("%08x ", (uint32_t)p32[i]);
			i++;
			if ((i%8)==0)
				PRN_INFO("\n0x%04llx:  ", address+i*4);
		}
		else if (bits == 64)
		{
			PRN_INFO("%016llx ", (unsigned long long)p64[i]);
			i++;
			if ((i%4)==0)
				PRN_INFO("\n0x%04llx:  ", address+i*8);
		}
	}
	PRN_INFO("\n");

}

void* lua_pop_table(lua_State* L, int param_idx, int data_size)
{
	unsigned char* pdata = (unsigned char*)malloc(data_size);
	if (pdata == NULL)
	{
		PRN_ERROR("LUA: POP table alloc data error\n");
		return NULL;
	}
	memset(pdata, 0, data_size);
	for (int i = 1; i <= data_size; i++)
	{
	    lua_pushinteger(L, i);
	    lua_gettable(L, param_idx);
	    if (lua_isnil(L, -1))
	    {
		data_size = i-1;
		break;
	    }
	    if (!lua_isnumber(L, -1))
	    {
		luaL_error(L, "Lua array element[%d] is invalid (a number required, but type is %s)", i, luaL_typename(L, -1));
		free(pdata);
		return NULL;
	    }

	    lua_Integer val = lua_tointeger(L, -1);

	    pdata[i-1] = val;
	    lua_pop(L, 1);
	}
	return pdata;
}

int lua_push_table(lua_State* L, void* pdata, int size)
{
	lua_createtable(L, size, 0);
	for (int i = 0; i < size; i++)
	{
	  lua_pushinteger(L, ((unsigned char*)pdata)[i]);
	  lua_rawseti (L, -2, i+1);
	}
	return size;
}

int	lua_get_cli_version(void* lua)
{
	lua_State* L = (lua_State*)lua;
	lua_pushinteger(L, VERSION_H);
	lua_pushinteger(L, VERSION_L);
	return 2;
}

int	lua_set_variable(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* name  = lua_tostring (L, 1);
	const char* value = lua_tostring (L, 2);

	int res = g_mgr.set_var(name, value);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_get_variable(void* lua)
{
	lua_State* L = (lua_State*)lua;
	const char* name  = lua_tostring (L, 1);

	const char* val= g_mgr.get_var(name);
	lua_pushstring(L, val);
	return 1;
}

int	lua_set_reg(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* value = lua_tostring (L, 3);

	int res = g_mgr.set_reg(NCID_REG_SET, bits, addr, value);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_xor_reg(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* value = lua_tostring (L, 3);

	int res = g_mgr.set_reg(NCID_REG_XOR, bits, addr, value);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_or_reg(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* value = lua_tostring (L, 3);

	int res = g_mgr.set_reg(NCID_REG_OR, bits, addr, value);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_and_reg(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* value = lua_tostring (L, 3);

	int res = g_mgr.set_reg(NCID_REG_AND, bits, addr, value);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_get_reg(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* msg   = lua_tostring (L, 3);

	uint64_t value = 0;

	int res = g_mgr.get_reg(bits, addr, value, msg);
	lua_pushinteger(L, res);
	lua_pushinteger(L, value);
	return 2;
}

int	lua_download(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* addr  = lua_tostring (L, 2);
	const char* len   = lua_tostring (L, 3);
	const char* file  = lua_tostring (L, 4);

	int res = g_mgr.download(bits, addr, len, file);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_upload(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* bits  = lua_tostring (L, 1);
	const char* file  = lua_tostring (L, 2);
	const char* addr  = lua_tostring (L, 3);

	int res = g_mgr.upload(bits, file, addr);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_h2(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* pcmd  = lua_tostring (L, 1);

	std::string cmd, resp;
	if (pcmd == NULL)
	{
		lua_pushinteger(L, RC_PARAM_ERROR);
		lua_pushstring(L, "{}");
		return 2;
	}

	cmd = pcmd;

	int res = g_mgr.h2(cmd, resp);
	lua_pushinteger(L, res);
	lua_pushstring(L, resp.c_str());
	return 2;
}

int	lua_exec(void* lua)
{
	lua_State* L = (lua_State*)lua;
	std::string output;

	int argnum = lua_gettop(L);
	if (argnum != 1)
	{
		PRN_ERROR("cli needs 1 parameter");
		lua_pushinteger(L, RC_PARAM_ERROR);
		lua_pushstring(L, "");
		return 2;
	}

	PRN_SET_DEV(&output);

	const char* cmds  = lua_tostring (L, 1);
	int res = g_mgr.internal_run_commands(cmds);
	lua_pushinteger(L, res);
	lua_pushstring(L, output.c_str());

	PRN_SET_DEV(NULL);
	return 2;
}

int	lua_json_get_param(void* lua)
{
	lua_State* L = (lua_State*)lua;

	const char* json_text  = lua_tostring (L, 1);
	const char* json_param  = lua_tostring (L, 2);

	class_json json;
	int res = json.create(json_text);

	lua_pushinteger(L, res);
	lua_pushstring(L, json.getobjdata(json_param).c_str());

	return 2;
}

int	lua_read_data(void* lua)
{
	int res = 0;
	lua_State* L = (lua_State*)lua;

	int argnum = lua_gettop(L);
	if (argnum != 3)
	{
		PRN_ERROR("readdata requires 3 parameters: (bits, table, size)");
	}

	unsigned int bits  	= (unsigned int)lua_tointeger(L, 1);
	const char*  addr	= (const char*)lua_tostring(L, 2);
	unsigned long size  = (unsigned long)lua_tointeger(L, 3);
	unsigned long size_in_bytes = size;

	//printf("readdata(%d, %s, %ld)\n", bits, addr, size);

	switch(bits)
	{
		case 8:
			break;

		case 16:
			size_in_bytes *= 2;
			break;

		case 32:
			size_in_bytes *= 4;
			break;

		case 64:
			size_in_bytes *= 8;
			break;

		default:
		{
			res = RC_ALLOC_ERROR;
			size = 0;
			lua_pushinteger(L, res);
			lua_createtable(L, size, 0);
			return 2;
		}
	}

	unsigned char* data = (unsigned char*)malloc(size_in_bytes);
	if (data == NULL)
	{
		res = RC_ALLOC_ERROR;
		size = 0;
	}
	else
	{
		res = g_mgr.readdata(bits, addr, size, data);
	}

	size = (res >= 0) ? res : 0;

	lua_pushinteger(L, res);
	lua_createtable(L, size, 0);
	for (unsigned long i = 0; i < size; i++)
	{
	  lua_pushinteger(L, data[i]);
	  lua_rawseti (L, -2, i+1);
	}

	if (data != NULL)
	{
		free(data);
		data = NULL;
	}

	return 2;
}

int	lua_write_data(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 3 && argnum != 4)
	{
		PRN_ERROR("LUA: writedata expects 3 or 4 arguments\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	if (!lua_istable(L, 3))
	{
		PRN_ERROR("LUA: writedata expects a table for argument#3\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int data_size;
	int bits = lua_tointeger(L, 1);
	const char* addr = lua_tostring(L, 2);
	int size = lua_rawlen(L, 3);

	if (argnum == 4)
		data_size = lua_tointeger(L, 4);
	else
		data_size = size;

	unsigned char* pdata = (unsigned char*)malloc(data_size);
	if (pdata == NULL)
	{
		PRN_ERROR("LUA: alloc data error\n");
		lua_pushinteger(L, RC_ALLOC_ERROR);
		return 1;
	}
	memset(pdata, 0, data_size);

	for (int i = 1; i <= data_size; i++)
	{
        lua_pushinteger(L, i);
        lua_gettable(L, 3);
        if (lua_isnil(L, -1))
	{
	    data_size = i-1;
            break;
        }
        if (!lua_isnumber(L, -1))
        {
            return luaL_error(L, "Lua array element[%d] is invalid (a number required, but type is %s)", i, luaL_typename(L, -1));
        }

        lua_Integer val = lua_tointeger(L, -1);

        pdata[i-1] = val;
        lua_pop(L, 1);
    }

	int res = g_mgr.writedata(bits, addr, pdata, data_size);
	lua_pushinteger(L, res);

	if(pdata != NULL)
	{
		free(pdata);
		pdata = NULL;
	}
	return 1;
}

int	lua_socket(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	int type = 0;

	if (argnum == 1)
	{
		type = lua_tointeger(L, 1);
	}
	else if (argnum > 1)
	{
		PRN_ERROR("LUA: socket expects 1 argument: 0 - TCP, 1 - UDP socket\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: socket internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int h = lua_mgr->socket(type);
	lua_pushinteger(L, h);
	return 1;
}

int	lua_socket_bind(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 3)
	{
		PRN_ERROR("LUA: socket bind takes 3 arguments: <socket>, <ip address>, <port>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: socket internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int socket = lua_tointeger(L, 1);
	const char* ip = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	int res = lua_mgr->bind(socket, ip, port);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_socket_accept(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 1)
	{
		PRN_ERROR("LUA: socket accept takes 1 argument: <socket>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: socket internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int socket = lua_tointeger(L, 1);
	int res = lua_mgr->accept(socket);
	lua_pushinteger(L, res);
	return 1;
}

int lua_socket_connect(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 3)
	{
		PRN_ERROR("LUA: socket connect takes 3 arguments: <socket> <ip> <port>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int socket = lua_tointeger(L, 1);
	const char* ip = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	int res = lua_mgr->connect(socket, ip, port);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_socket_send(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 2)
	{
		PRN_ERROR("LUA: socket send takes 2 arguments: <socket> <data>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: socket internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int socket = lua_tointeger(L, 1);
	void* pdata = NULL;
	int data_len = 0;
	int alloc = 0;

	if (lua_isstring(L, 2))
	{
		pdata = (void*)lua_tostring(L, 2);
		data_len = strlen((const char*)pdata);
		//printf ("Send a string: %s\n", (char*)pdata);
	}
	else if (lua_istable(L, 2))
	{
		data_len = lua_rawlen(L, 2);

		pdata = (unsigned char*)malloc(data_len);
		if (pdata == NULL)
		{
			PRN_ERROR("LUA: alloc data error\n");
			lua_pushinteger(L, RC_ALLOC_ERROR);
			return 1;
		}
		alloc = 1;
		memset(pdata, 0, data_len);

		for (int i = 1; i <= data_len; i++)
		{
	        lua_pushinteger(L, i);
	        lua_gettable(L, 2);
	        if (lua_isnil(L, -1))
			{
				data_len = i-1;
	            break;
	        }
	        if (!lua_isnumber(L, -1))
	        {
	            return luaL_error(L, "Lua array element[%d] is invalid (a number required, but type is %s)", i, luaL_typename(L, -1));
	        }

	        lua_Integer val = lua_tointeger(L, -1);

	        ((unsigned char*)pdata)[i-1] = (unsigned char)val;
	        lua_pop(L, 1);
	    }
	}
	else
	{
		PRN_ERROR("LUA: socket send error, the data should be string/table type\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int res = lua_mgr->send(socket, pdata, data_len);

	if (alloc)
	{
		free(pdata);
	}
	pdata = NULL;

	lua_pushinteger(L, res);
	return 1;
}

int	lua_socket_receive(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 1 && argnum != 2 && argnum != 3)
	{
		PRN_ERROR("LUA: socket recv takes 1, 2 or 3 arguments: <socket> [data_size [wait time]]\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		lua_createtable(L, 0, 0);
		return 2;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		lua_createtable(L, 0, 0);
		return 2;
	}

	int socket = lua_tointeger(L, 1);
	int size = 1024*10;
	int wait_ms = 0;
	int exact_size = 0;

	if (argnum == 2)
	{
		size = lua_tointeger(L, 2);
	}

	if (argnum == 3)
	{
		wait_ms = lua_tointeger(L, 3);
	}
	
	void* pdata = malloc(size);
	if (pdata == NULL)
	{
		PRN_ERROR("LUA: error to allocate the table\n");
		lua_pushinteger(L, RC_ALLOC_ERROR);
		lua_createtable(L, 0, 0);
		return 2;
	}
	memset(pdata, 0, size);
	int res = lua_mgr->recv(socket, pdata, size, wait_ms, exact_size);

	if (res <= 0)
	{
		free(pdata);
		lua_pushinteger(L, res);
		lua_createtable(L, 0, 0);
		return 2;
	}

	lua_pushinteger(L, res);
	lua_createtable(L, res, 0);
	for (int i = 0; i < res; i++)
	{
	  lua_pushinteger(L, ((unsigned char*)pdata)[i]);
	  lua_rawseti (L, -2, i+1);
	}
	free(pdata);
	return 2;
}

int	lua_descr_close(void* lua)
{
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 1)
	{
		PRN_ERROR("LUA: close takes 1 argument: <id>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	class_luamgr* lua_mgr = g_mgr.get_luamgr();
	if (lua_mgr == NULL)
	{
		PRN_ERROR("LUA: close internal error\n");
		lua_pushinteger(L, RC_MGR_ERROR);
		return 1;
	}

	int h = lua_tointeger(L, 1);
	int res = lua_mgr->close(h);
	lua_pushinteger(L, res);
	return 1;
}

int	lua_fifo_ctrl(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 2)
	{
		PRN_ERROR("LUA: fifoctrl takes 2 parameters: <fifo addr> <operation>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	const char* fifo  = lua_tostring (L, 1);
	const char* opr   = lua_tostring (L, 2);
	res = g_mgr.fifo_ctrl(fifo, opr);
	lua_pushinteger(L, res);
	return 1;
}

/** @brief This function takes 2 LUA mandatory parameters:
			LUA: the IP address
			LUA: the TCP port

	@return [int] the IPC descriptor (if >= 0) or the error code (if < 0)
*/
int lua_ipc_open(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 2)
	{
		PRN_ERROR("LUA: ipc_open requires 2 parameters: <ip address> <tcp port>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	const char* ip    = lua_tostring (L, 1);
	unsigned int port = lua_tointeger(L, 2);

	class_luamgr* pLua = g_mgr.get_luamgr();

	if (pLua != NULL)
	{
		res = pLua->ipc_open(ip, port);
	}
	else
	{
		res = RC_GET_LUA_MGR_ERROR;
	}

	lua_pushinteger(L, res);
	return 1;
}

/** @brief This function takes 1 LUA mandatory parameter:
			LUA: IPC handle opended by lua_ipc_open

	@return [int] the error code (0 means OK)
*/
int lua_ipc_close(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 1)
	{
		PRN_ERROR("LUA: ipc_close requires 1 parameter: <IPC handel>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int hIPC = lua_tointeger (L, 1);

	class_luamgr* pLua = g_mgr.get_luamgr();

	if (pLua != NULL)
	{
		res = pLua->ipc_close(hIPC);
	}
	else
	{
		res = RC_GET_LUA_MGR_ERROR;
	}

	lua_pushinteger(L, res);
	return 1;
}

/** @brief This function takes 2+N LUA parameter:
			LUA: IPC handle opended by lua_ipc_open
			LUA: API-ID
			LUA: [set of API parameters if any]

	@return:
			[ERROR code]  - OK if >= 0
			[RESULT if THIS IS NON-VOID result]
				-[U32]
				-[U16]
				-[U8]
				-[STRING] 0020
				-[ARRAY in case of VOID*]
*/
int lua_ipc_call(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum < 2)
	{
		PRN_ERROR("LUA: ipc_call requires 2+N parameter: <IPC handel> <API ID> [parameters]\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int h_ipc  = lua_tointeger(L, 1);
	int api_id = lua_tointeger(L, 2);

	// to pop all the paramters and to create the IPC param list

	class_luamgr* pLua = g_mgr.get_luamgr();

	lua_ipc_params params;
	lua_ipc_parameter ret_val;

	for (int prm_idx = 3; prm_idx <= argnum; prm_idx++)
	{
		lua_ipc_parameter p;
		if (lua_isnumber(L, prm_idx) || lua_isinteger(L, prm_idx))
		{
			p.ipc_data_type_id = IPC_APT_U64;
			p.ipc_data.u64 = (uint64_t)lua_tonumber(L, prm_idx);
		}
		else if (lua_istable(L, prm_idx))
		{
			int size = lua_rawlen(L, prm_idx);
			p.ipc_data_type_id = IPC_APT_PVOID;
			p.ipc_data.ipc_void_ptr = lua_pop_table(L, prm_idx, size);
			//lua_dump_data(p.ipc_data.ipc_void_ptr, size);
		}
		else // to treat as a string
		{
			p.ipc_data_type_id = IPC_APT_STR;
			p.ipc_data.ipc_str = lua_tostring(L, prm_idx);
		}
		params.list.push_back(p);
	}

	res = pLua->ipc_call(h_ipc, api_id, params, ret_val);

	// to clean the allocated objects
	for (unsigned int i = 0; i < params.list.size(); i++)
	{
		if (params.list[i].ipc_data_type_id == IPC_APT_PVOID)
		{
			if (params.list[i].ipc_data.ipc_void_ptr != NULL)
			{
				free(params.list[i].ipc_data.ipc_void_ptr);
				params.list[i].ipc_data.ipc_void_ptr = NULL;
			}
		}
	}

	if (res < 0)
	{
		lua_pushinteger(L, res);
		return 1;
	}

	if (ret_val.ipc_data_type_id == IPC_APT_VOID)
	{
		lua_pushinteger(L, res);
		return 1;
	}

	lua_pushinteger(L, res);

	switch(ret_val.ipc_data_type_id)
	{
		case IPC_APT_U8:
		case IPC_APT_U16:
		case IPC_APT_U32:
		case IPC_APT_U64:
			lua_pushinteger(L, ret_val.ipc_data.u64);
			break;

		case IPC_APT_STR:
			lua_pushstring(L, ret_val.ipc_data.ipc_str);
			break;

		default:
			lua_push_table(L, ret_val.ipc_data.ipc_void_ptr, ret_val.ipc_data_size);
			break;
	}

	pLua->ipc_call_done(h_ipc);
	return 2;
}

/** @brief This API is used by LUA to create the process,
			the LUA script will be provided with the process ID

	LUA parameters :
	    - <filename>
	    - <cmd line parameters>
	    - <to keep process alive if CLI is stopped>

	@return [pid] or error code (error if < 0)
*/
int lua_proc_create(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum != 2)
	{
		PRN_ERROR("LUA: proc_create requires 2 parameter: <filename> <parameters>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	const char* name   = lua_tostring(L, 1);
	const char* params = lua_tostring(L, 2);

	int pid = 0;
	char**argv = NULL;

	// We need to create the arguments for the create child process
	// ------------------------------------------------------------

	std::vector <std::string> param_list;
	param_list.push_back(name);

	class_lexer lex("", " ");
	std::string lexem;
	lex.lex_init(params);
	while (! (lexem = lex.lex_next()).empty())
	{
		param_list.push_back(lexem);
	}

	param_list.push_back("--ipc_server_ip");
	param_list.push_back("127.0.0.1");
	param_list.push_back("--ipc_server_port");
	param_list.push_back(class_helpmgr::to_string(g_params.ipc_port));

	// +1 - the selfname of a process
	// +1 - NULL termination
	argv = (char**)malloc(sizeof(char*) * (1 + param_list.size() + 1));
	if (argv == NULL)
	{
		lua_pushinteger(L, RC_ALLOC_ERROR);
		return 1;
	}

	memset(argv, 0, sizeof(char*) * (1 + param_list.size() + 1));

	for (unsigned int i = 0; i < param_list.size(); i++)
	{
		argv[i] = (char*)param_list[i].c_str();
	}

	int status = posix_spawn(&pid, name, NULL, NULL, argv, NULL);

	free(argv);
	argv = NULL;

	if (status != 0)
		res = RC_CREATE_PROC_ERROR;
	else
		res = pid;

	lua_pushinteger(L, res);
	return 1;
}

/** @brief This API is used by LUA to wait process termination,
			Note: the process is opened/created by lua_proc_create

	LUA parameters :
	    - <PID>
	@return error code (OK if >= 0)
*/
int lua_proc_wait(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum < 1)
	{
		PRN_ERROR("LUA: proc_wait requires 1 parameter: <pid>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int pid = lua_tointeger(L, 1);
	int status = 0;

	do
	{
		int ret = waitpid(pid, &status, WUNTRACED | WCONTINUED);
		if (ret == -1)
		{
			res = RC_PROC_WAIT_ERROR;
			break;
		}
		/*if (WIFEXITED(status)){
		   printf("exited, status=%d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
		   printf("killed by signal %d\n", WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
		   printf("stopped by signal %d\n", WSTOPSIG(status));
		} else if (WIFCONTINUED(status)) {
		   printf("continued\n");
		}*/
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	lua_pushinteger(L, res);
	return 1;
}

/** @brief This API is used by LUA to kill the process,
			Note: opened/created by lua_proc_create

	LUA parameters :
	    - <PID>
	    - <SIGNAL ID>
	@return error code (OK if >= 0)
*/
int lua_proc_kill(void* lua)
{
	int res = RC_OK;
	lua_State* L = (lua_State*)lua;
	int argnum = lua_gettop(L);
	if (argnum < 2)
	{
		PRN_ERROR("LUA: proc_kill requires 2 parameter: <pid> <signal id>\n");
		lua_pushinteger(L, RC_PARAM_ERROR);
		return 1;
	}

	int pid = lua_tointeger(L, 1);
	int sig = lua_tointeger(L, 2);

	res = kill(pid, sig);
	lua_pushinteger(L, res);
	return 1;
}

}

lua_export_api lua_api[] = 
{
	{"get_cliver",			lua_get_cli_version},
	{"setvar",				lua_set_variable},
	{"getvar",				lua_get_variable},

	{"regset",				lua_set_reg},
	{"regor",				lua_or_reg},
	{"regxor",				lua_xor_reg},
	{"regand",				lua_and_reg},
	{"regget",				lua_get_reg},

	{"download",			lua_download},
	{"upload",				lua_upload},

	{"h2",					lua_h2},
	{"cli",					lua_exec},
	{"jsonparam",			lua_json_get_param},

	{"readdata",			lua_read_data},
	{"writedata",			lua_write_data},

	{"socket",				lua_socket},
	{"bind",				lua_socket_bind},
	{"accept",				lua_socket_accept},
	{"connect",				lua_socket_connect},
	{"send",				lua_socket_send},
	{"recv",				lua_socket_receive},
	{"close",				lua_descr_close},

	{"fifoctrl",			lua_fifo_ctrl},

	{"ipc_open",			lua_ipc_open},
	{"ipc_close",			lua_ipc_close},
	{"ipc_call",			lua_ipc_call},

	{"proc_create",			lua_proc_create},
	{"proc_wait",			lua_proc_wait},
	{"proc_kill",			lua_proc_kill},

	{NULL,					NULL}
};

static void sig_handler(int signum)
{
	if (SIGWINCH == signum)
	{
		g_cmdedit.resize_window();
	}
	else
	{
		if (g_Stop)
			return;

		g_cmdedit.stop();

		g_Stop = 1;
		pthread_kill(g_mainthread, SIGINT);
		g_sync_run_cmd.wakeup();
		PRN_INFO("\n[CTRL+C signal is received]\n");
	}
}

static void sigHandler_Timer(int sig)
{
	g_mgr.sched_time_event();
}

void* main_thread(void*)
{
	while (g_Stop == 0)
	{
		if (g_sync_run_cmd.wait() < 0)
			continue;

		if (g_command_ptr == NULL)
		{
			g_mgr.run_sched_tasks();
		}
		else
		{
			g_mgr.run_commands(g_command_ptr);
		}

		g_sync_done_cmd.wakeup();
	}
	return NULL;
}

static std::string find_common_part(std::vector<std::string>& list, int init_pos = 0)
{
	std::string val;
	unsigned int min_len = list[0].length();

	for (unsigned int i = 1; i < list.size(); i++)
	{
		if (list[i].length() < min_len)
			min_len = list[i].length();
	}

	for (unsigned int pos = init_pos; pos < min_len; pos ++)
	{
		int ch = list[0][pos];
		unsigned int i;

		for (i = 1; i < list.size(); i++)
		{
			if (list[i][pos] != ch)
				break;
		}

		if (i < list.size())
			break;

		val += ch;
	}
	return val;
}

static int cmd_tab_handler(const char* pattern, void* pdata, char** pins_text)
{
	if (pattern == NULL)
	return 0;

	int pat_len = strlen(pattern);

	std::vector<std::string> list;
	g_mgr.get_cmd_list(pattern, list);

	if (list.size() == 0)
	return 0;

	if (list.size() == 1)
	{
		*pins_text = strdup(list[0].c_str() + pat_len);
		return 1;
	}

	// Here we need to print the list of possible variants
	// ---------------------------------------------------
	const char* p_dot = strstr(pattern, ".");
	int dot_place = -1;
	int pat_offs = 0;

	if (p_dot != NULL)
	{
		dot_place = (p_dot - pattern);
		pat_offs = pat_len - (dot_place+1);
	}
	else
	{
		pat_offs = pat_len;
	}

	//printf("\n\npat_offs:%d, pat_len:%d\n", pat_offs, pat_len);

	std::sort(list.begin(), list.end());
	std::string common = find_common_part(list, (dot_place+1));

	if (common.length())
	{
		*pins_text = strdup(common.c_str() + pat_offs);
	}

	printf("\n");

	unsigned int max_len = 0;
	for (unsigned int i = 0; i < list.size(); i++)
	{
		if (max_len < list[i].length())
			max_len = list[i].length();
	}

	max_len += 2;  // 2 is the delimiter
	int cols = g_cmdedit.get_cols();
	int num_per_line = cols / max_len - 1;
	if (num_per_line <= 0)
		num_per_line = 1;

	char prn_pat[64];
	sprintf(prn_pat, "%%-%ds", max_len);

	for (unsigned int i = 0; i < list.size(); i++)
	{
		if (dot_place>=0)
		{
			printf("%s\n", list[i].c_str());
		}
		else
		{
			printf(prn_pat, list[i].c_str());
			if ((i % num_per_line) == 0 && i)
				printf("\n");
		}
	}
	if (dot_place<0)
	{
		printf("\n");
	}
	return 2;
}

int main(int argc, char** argv)
{
#if 0
	class_path path;
	path = "/1/2/3";
	printf("%s\n", (const char*)path);
	return 1;
#endif

	char buf[PATH_MAX];
	int r = param_pars(argc, argv, &g_params);
	if (r < 0)
	{
		PRN_ERROR("command line parameter error, rc:%d\n", r);
		return r; 
	}

	if (g_params.flags & CLF_HELP)
	{
		usage();
		return 0;
	}

	g_params.lua_api = lua_api;

	struct sigaction sa;
	struct itimerval itv;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sig_handler;
	sa.sa_flags = 0; //SA_RESTART;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sigaction(SIGWINCH, &sa, NULL);

	if (g_params.cmd_num != 0 || g_params.flags & CLF_CMD_FILE_NAME)
	{
		if ((r = g_mgr.init(&g_params)) < 0)
		{
			PRN_ERROR("initialization error, rc:%d\n", r);
			return r;
		}
	}

 	// to run the default command line commands
	// -----------------------------------------
	for (unsigned int i = 0; i < g_params.cmd_num; i++)
	{
		r = g_mgr.run_commands(g_params.cmd[i]);
		if (r < 0)
		{
			PRN_ERROR("Error to execute command:[%s], rc:%d\n", g_params.cmd[i], r);
			return r;
		}
	}

	// Here is to read the file and to sequentially execute the commands
	// -----------------------------------------------------------------
	if (g_params.flags & CLF_CMD_FILE_NAME)
	{
		int line_err = 0;
		r = exec_file_cmd(g_params.cmd_file, g_mgr, line_err);
		if (r < 0)
		{
			PRN_ERROR("Error to execute file commands, rc:%d, file:%s, line:%d\n", r, g_params.cmd_file, line_err);
			return r;
		}
	}

	// in case of command line parameters: "-c or -f", we need to stop execution the application
	// as the user requested to execute the set of commands and to stop it
	// ------------------------------------------------------------------------------------------
	if (g_params.cmd_num||g_params.flags & CLF_CMD_FILE_NAME)
		return RC_OK;

	print_app_header();

	if ((r = g_mgr.init(&g_params)) < 0)
	{
		PRN_ERROR("initialization error, rc:%d\n", r);
		return r;
	}

	if((r = pthread_create(&g_mainthread, NULL, main_thread, &g_params)))
	{
		PRN_ERROR("MainThread creation error, rc:%d\n", r);
		return RC_CREATE_THREAD_ERROR;
	}

	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sigHandler_Timer;
	if (sigaction(SIGALRM, &sa, NULL) < 0)
	{
		PRN_ERROR("Error to register signal alarm handler\n");
		return RC_REG_SIGNAL_ERROR;
	}

	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	itv.it_interval.tv_sec = 1;
	itv.it_interval.tv_usec = 0;

	#if 1
	if (setitimer(ITIMER_REAL, &itv, NULL) < 0)
	{
		PRN_ERROR("setitimer setting error\n");
		return RC_REG_TIMER_ERROR;
	}
	#endif

	if (g_params.flags & CLF_INPUT_0_MODE)
	{
		PRN_INFO("\n");
		PRN_INFO("Limited command editor is used\n");
		g_command_ptr = buf;
		while (g_Stop == 0)
		{
			if (g_command_ptr != NULL)
			{
				sprintf(buf, "%s:%d:%s>", g_params.serv_ip, g_params.serv_port, g_mgr.get_scr_cur_path().c_str());
				PRN_INFO("%s", buf);
			}
			g_command_ptr = fgets(buf, sizeof(buf), stdin);
			if (g_command_ptr == NULL && feof(stdin))
			{
				break;
			}

			if (g_Stop == 0)
			{
				g_sync_run_cmd.wakeup();
				g_sync_done_cmd.wait();
			}
		}
	}
	else
	{
		PRN_INFO("\n");
		g_command_ptr = buf;

		sprintf(buf, "%s:%d>", g_params.serv_ip, g_params.serv_port);
		g_cmdedit.init(CLI_MAX_CMD_LEN, CLI_MAX_CMD_HISTORY);
		g_cmdedit.setlogo(buf);
		g_cmdedit.set_tab_handler(cmd_tab_handler, NULL);
		//g_cmdedit.set_tab_delim(" \t@,;#$:!()%*+-~=[]&\"<>/", "/");
		g_cmdedit.set_tab_delim(" \t@,;#$:!()%*+~=[]&\"<>", "");

		while (g_Stop == 0)
		{
			sprintf(buf, "%s:%d:%s>", g_params.serv_ip, g_params.serv_port, g_mgr.get_scr_cur_path().c_str());
			g_cmdedit.setlogo(buf);
			if ((g_command_ptr = g_cmdedit.getcmd()) == NULL)
				break;

			if (g_Stop == 0)
			{
				g_sync_run_cmd.wakeup();
				g_sync_done_cmd.wait();
			}
		}
	}
	if (g_Stop == 0)
	{
		pthread_kill(g_mainthread, SIGINT);
		g_sync_run_cmd.wakeup();
	}

	pthread_join(g_mainthread, NULL);
	PRN_INFO("cli is stopped\n");
	return 0;
}
