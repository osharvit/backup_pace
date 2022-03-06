/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager is designed to control native & extended CLI commands
 *     to pars the input stream, to keep the commands, to provide 
 *     all the commands parameters, etc.
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
#include <ctype.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "cmdmgr.h"
#include "gen-types.h"
#include "lexer.h"
#include "math.h"
#include "helpmgr.h"
#include "clicpmgr.h"
#include "file.h"
#include "json.h"
#include "rc_cmdmgr.h"

static struct {
		int		id;
		const char*	name;
		const char*	con_name;
	}
	native_cmds [] = {
			{NCID_REG_SET, 	"reg.set",		"reg"		},
			{NCID_REG_GET, 	"reg.get",		""			},
			{NCID_REG_OR, 	"reg.or",		""			},
			{NCID_REG_XOR, 	"reg.xor",		""			},
			{NCID_REG_AND, 	"reg.and",		""			},
			{NCID_SLEEP,	"sleep",		"sleep"		},
			{NCID_PRINT,	"print",		"print"		},
			{NCID_DUMP, 	"dump",			"dump"		},
			{NCID_WRITE,	"write",		"write"		},
			{NCID_CALC,     "=",			"="			},
			{NCID_UPLOAD, 	"upload",		"upload"	},
			{NCID_DOWNLOAD,	"download",		"download"	},
			{NCID_SET_VAR,	"set",			"set"		},
			{NCID_HELP,		"help",			"help"		},
			{NCID_LS,		"ls",			"ls"		},
			{NCID_CD,		"cd",			"cd"		},
			{NCID_PWD,		"pwd",			"pwd"		},
			{NCID_MATH,		"math",			"math"		},
			{NCID_SCHED,	"sched",		"sched"		},
			{NCID_GET_FILE,	"getfile", 		"getfile"	},
			{NCID_EXIT,		"exit",			"exit"		},

			{0, 			NULL,			NULL		},
	};

class_cmdmgr::class_cmdmgr(void)
{
	m_sched_time = 0;
	m_sched_time_cur = 0;
	m_sched_cmd = "";
	m_stdout_fd = 0;
	memset(&m_stdout_pos, 0, sizeof(m_stdout_pos));
	m_tab_files = 0;
}

class_cmdmgr::~class_cmdmgr(void)
{
	release_cmds();
}

int class_cmdmgr::init_native_cmds(CLF_PARAMS_TYPE* p)
{
	class_native_cmd * pcmd;

	// to create the list of the native commands supported by CLI
	// ----------------------------------------------------------

	int i = 0;
	while (native_cmds[i].id != 0)
	{
		pcmd = new class_native_cmd(native_cmds[i].id);
		if (pcmd == NULL)
		{
			PRN_ERROR("ERROR to create native command:%s\n", native_cmds[i].name);
			return RC_CMDMGR_CREATECMD_ERROR;
		}
		m_cmds.push_back(pcmd);
		i++;
	}

	return RC_OK;
}

int class_cmdmgr::init_extended_cmds(CLF_PARAMS_TYPE* p)
{
	DIR *d;
	struct dirent *dir_elm;

	int r;

	// To scan the optional file that may include extended commaqnds
	// -------------------------------------------------------------
	r = m_extcmdmgr.load_from_file(CLI_EXT_CMD_FILENAME);
	if (r < 0 && r != RC_EXTCMDMGR_NO_FILE)
	{
		PRN_ERROR("FILE:%s, line:%d\n", CLI_EXT_CMD_FILENAME, 0);
		return r;
	}

	// To scan optional folder with the set of files
	// ---------------------------------------------
	if ((d = opendir(CLI_EXT_CMD_FOLDER_NAME)) != NULL)
	{
	    while ((dir_elm = readdir(d)) != NULL)
	    {
		if (dir_elm->d_type != DT_DIR)
		{
			std::string path = "./extcmd/";
			path += dir_elm->d_name;

			r = m_extcmdmgr.load_from_file(path.c_str());

			if (r < 0 && r != RC_EXTCMDMGR_NO_FILE)
			{
				PRN_ERROR("FILE:%s, line:%d\n", path.c_str(), m_extcmdmgr.get_err_line());
				return r;
			}
		}
	    }
	    closedir(d);
	}

	PRN_INFO("[%d loaded]", m_extcmdmgr.get_num());
	return RC_OK;
}

int class_cmdmgr::init_hw_comp(CLF_PARAMS_TYPE* p)
{
	int r;
	DIR *d;
	struct dirent *dir_elm;

	// To scan optional folder with the set of files
	// ---------------------------------------------
	if ((d = opendir(CLI_HW_DESCR_FOLDER_NAME)) != NULL)
	{
	    while ((dir_elm = readdir(d)) != NULL)
	    {
		if (dir_elm->d_type != DT_DIR)
		{
			std::string path = "./hw/";
			path += dir_elm->d_name;

			r = m_hwmgr.load_from_file(path.c_str());

			if (r < 0 && r != RC_HWMGR_NO_FILE)
			{
				PRN_ERROR("FILE:%s, line:%d\n", path.c_str(), m_hwmgr.get_err_line());
				return r;
			}
		}
        }
        closedir(d);
	}

	PRN_INFO("[%d loaded]", m_hwmgr.get_num());
	return r;
}

int class_cmdmgr::init_lua(CLF_PARAMS_TYPE* p)
{
	int r;
	if ((r = m_luamgr.init(p)) < 0)
		return r;
	r = m_luamgr.load_scripts();
	PRN_INFO("[%d loaded]", m_luamgr.get_scripts_num());
	return r;
}

int class_cmdmgr::init_vars(CLF_PARAMS_TYPE* p)
{
	int r;
	if ((r = m_vars.load_from_file(CLI_VARS_FILENAME)) >= 0)
	{
		PRN_INFO("[%d loaded]", m_vars.get_var_num());

		// Here we need to add some set of the system (CLI internal variables)
		m_vars.set("H2UID",  "0", 1);
		m_vars.set("retu", "0", 1);
		m_vars.set("retd", "0", 1);
		m_vars.set("retx", "0x0", 1);
	}

	return r;
}

int class_cmdmgr::init_cli_serv_conn(CLF_PARAMS_TYPE* p)
{
	int r = RC_OK;
	if (p->serv_port)
	{
		r = m_connection.connect(p->serv_ip, p->serv_port);
		if (r < 0)
		{
			PRN_ERROR("{%s:%d}", p->serv_ip, p->serv_port);
		}
		else
		{
			int ver = 0;
			r = m_connection.get_version(ver);
			if (r < 0)
			{
				PRN_ERROR("Error to read version");
				return r;
			}
			PRN_INFO("(version: %d.%02d)", ver >> 8, ver & 0xFF);

			// Here we need to ack CLI server to map HW components regions
			// -----------------------------------------------------------

			clicp_cmd* pcpcmd;
			clicp_cmd* pcpcmd_ack; 
			if((pcpcmd = m_connection.alloc_cmd(NCID_MAP, 0))== NULL)
			{
				PRN_ERROR("Error to allocate the NET CLI command");
				return RC_CMDMGR_ALLOC_NETCMD_ERROR;
			}

			for (int i = 0; i < m_hwmgr.get_num(); i++)
			{
				const hw_block& blk = m_hwmgr[i];
				pcpcmd->cmd_drv_id = blk.m_drvid;
				pcpcmd->params.map.address = blk.m_base;
				pcpcmd->params.map.size = blk.m_size;

				pcpcmd_ack = NULL;
				r = m_connection.send_cmd(pcpcmd, pcpcmd_ack);

				if (r != RC_OK)
				{
					PRN_INFO("Send MAP regs error(%d)", r);
					break;
				}
				else
				{
					if (pcpcmd_ack->res_code != RC_OK)
					{
						r = pcpcmd_ack->res_code;
						PRN_INFO("MAP regs error(%d)", r);
						free(pcpcmd_ack);
						pcpcmd_ack = NULL;
						break;
					}
				}

				free(pcpcmd_ack);
				pcpcmd_ack = NULL;
			}

			if (r == RC_OK)
			{
				PRN_INFO("(Mapped: (%d) region(s))", m_hwmgr.get_num());
			}
		}
	}
	else
	{
		PRN_INFO("{disabled}");
	}

	return r;
}

int class_cmdmgr::init_h2app_conn(CLF_PARAMS_TYPE* p)
{
	int r = RC_OK;
	if (p->h2_port)
	{
		r = m_h2mgr.connect(p->serv_ip, p->h2_port);
		if (r < 0)
		{
			PRN_ERROR("{%s:%d}", p->serv_ip, p->h2_port);
		}
	}
	else
	{
		PRN_INFO("{disabled}");
	}
	return r;
}

int class_cmdmgr::init_rc_cmdmgr(CLF_PARAMS_TYPE* p)
{
	int rc = RC_OK;

	if (p->flags & CLF_REMOTE_CTRL)
		rc = m_rccmdmgr.init(p, this);
	else
		PRN_INFO("[disabled by configuration]");
	return rc;
}

int class_cmdmgr::init_ipc_mgr(CLF_PARAMS_TYPE* p)
{
	int rc = RC_OK;
	if (p->flags&CLF_IPC_PORT)
		rc = m_ipcmgr.init(p, this);
	else
		PRN_INFO("[disabled by configuration]");
	return rc;
}

int class_cmdmgr::init_autorun(CLF_PARAMS_TYPE* p)
{
	int rc = RC_OK;

	// This code runs LUA scripts which are marked with option "init"
	// and also runs the commands mentioned in the variable "autorun"

	int idx = m_vars.find_var(CLI_AUTOVAR_NAME);
	if (idx >= 0)
	{
		PRN_INFO("[AUTOVAR:YES]");
	}

	int num = m_luamgr.autorun_get_num();
	if (num > 0)
	{
		PRN_INFO("[SCRIPTS:%d]", num);
	}
	return rc;
}

int class_cmdmgr::cmd_name2id(const char* name)
{
	std::string cmd;

	if (name == NULL)
		return 0;

	cmd = name;

	int i = 0;
	while (native_cmds[i].id != 0)
	{
		if (cmd == native_cmds[i].name)
			return native_cmds[i].id;

		i++;
	}
	return 0;
}

int class_cmdmgr::set_scr_cur_path(const char* path)
{
	m_luamgr.set_rel_path(path);
	return RC_OK;
}

std::string class_cmdmgr::get_scr_cur_path(void)
{
	return m_luamgr.get_rel_path();
}

int class_cmdmgr::redirect_console_to_file(void)
{
	fflush(stdout);
	fgetpos(stdout, &m_stdout_pos);

	m_stdout_fd = dup(fileno(stdout));
	if (freopen("cli.stdout", "w", stdout) == NULL)
	    return RC_CMDMGR_OPEN_FILE_ERROR;
	return RC_OK;
}

int class_cmdmgr::restore_console(std::string * pstr)
{
	fflush(stdout);
    dup2(m_stdout_fd, fileno(stdout));
    close(m_stdout_fd);
    clearerr(stdout);
    fsetpos(stdout, &m_stdout_pos);

	if (pstr != NULL)
	{
		class_file file("cli.stdout");
		*pstr = file.read_to_str();
	}
	return RC_OK;
}

int class_cmdmgr::set_var(const char* name, const char* value)
{
	int res = m_vars.set(name, value);
	return res;
}

const char* class_cmdmgr::get_var(const char* name)
{
	std::string val = m_vars.get(name);
	return val.c_str();
}

int class_cmdmgr::set_reg(int oprid, const char* bits, const char* addr, const char* value, const char* msg)
{
	int rc;
	sync_cmd_begin();
	rc = set_reg_nosync(oprid, bits, addr, value, msg);
	sync_cmd_end();
	return rc;
}

int class_cmdmgr::set_reg_nosync(int oprid, const char* bits, const char* addr, const char* value, const char* msg)
{
	if (bits == NULL || addr == NULL || value == NULL)
		return RC_CMDMGR_PARAM_ERROR;

	class_native_cmd cmd(oprid);
	int res = cmd.set_bits((int)class_helpmgr::to_num(bits));
	if (res < 0)
	{
		PRN_ERROR("[reg.set]: num bits (%d) error\n", (int)class_helpmgr::to_num(bits));
		return res;
	}

	if (m_hwmgr.is_reg_name(addr) >= 0)
	{
		std::string compname, regname;
		m_hwmgr.split_hw_name(addr, compname, regname);
		cmd.set_compreg_name(compname.c_str(), regname.c_str());
	}
	else
	{
		cmd.set_address(addr);
	}
	cmd.set_value(value);

	if (msg != NULL)
	{
		cmd.set_message(msg);
	}
	else
	{
		cmd.enable_message(0);
	}

	return exec_cmd_reg(&cmd);
}

int class_cmdmgr::get_reg(const char* bits, const char* addr, uint64_t& value, const char* msg)
{
	int rc;
	sync_cmd_begin();
	rc = get_reg_nosync(bits, addr, value, msg);
	sync_cmd_end();
	return rc;
}

int class_cmdmgr::get_reg_nosync(const char* bits, const char* addr, uint64_t& value, const char* msg)
{
	int bits_num;

	if (bits == NULL || addr == NULL)
		return RC_CMDMGR_PARAM_ERROR;

	bits_num = (int)class_helpmgr::to_num(bits);

	class_native_cmd cmd(NCID_REG_GET);
	int res = cmd.set_bits(bits_num);
	if (res < 0)
	{
		PRN_ERROR("[reg.get]: num bits: %d error, rc:%d\n", bits_num, res);
		return res;
	}

	if (m_hwmgr.is_reg_name(addr) >= 0)
	{
		std::string compname, regname;
		m_hwmgr.split_hw_name(addr, compname, regname);
		cmd.set_compreg_name(compname.c_str(), regname.c_str());
	}
	else
	{
		cmd.set_address(addr);
	}

	if (msg != NULL)
	{
		cmd.set_message(msg);
	}
	else
	{
		cmd.enable_message(0);
	}

	res = exec_cmd_reg(&cmd);

	if (res < RC_OK)
		return res;

	std::string val = m_vars.get("retx");
	value = strtoull(val.c_str(), (char**)0, 0);
	return res;
}

int class_cmdmgr::download(const char* bits, const char* addr, const char*len, const char* filename)
{
	int res;
	sync_cmd_begin();
	res = download_nosync(bits, addr, len, filename);
	sync_cmd_end();
	return res;
}

int class_cmdmgr::download_nosync(const char* bits, const char* addr, const char*len, const char* filename)
{
	if (bits == NULL || addr == NULL || len == NULL || filename == NULL)
		return RC_CMDMGR_PARAM_ERROR;

	class_native_cmd cmd(NCID_DOWNLOAD);
	int res = cmd.set_bits((int)class_helpmgr::to_num(bits));
	if (res < 0)
	{
		PRN_ERROR("[download]: unsupported bits:%d\n", (int)class_helpmgr::to_num(bits));
		return res;
	}

	if (m_hwmgr.is_reg_name(addr) >= 0)
	{
		std::string compname, regname;
		m_hwmgr.split_hw_name(addr, compname, regname);
		cmd.set_compreg_name(compname.c_str(), regname.c_str());
	}
	else
	{
		cmd.set_address(addr);
	}

	cmd.set_value(len);
	cmd.set_file_name(filename);
	
	res = exec_cmd_download(&cmd);
	return res;
}

int class_cmdmgr::upload(const char* bits, const char* filename, const char* addr)
{
	int res;
	sync_cmd_begin();
	res = upload_nosync(bits, filename, addr);
	sync_cmd_end();
	return res;
}

int class_cmdmgr::upload_nosync(const char* bits, const char* filename, const char* addr)
{
	if (bits == NULL || addr == NULL || filename == NULL)
		return RC_CMDMGR_PARAM_ERROR;

	class_native_cmd cmd(NCID_UPLOAD);
	int res = cmd.set_bits((int)class_helpmgr::to_num(bits));
	if (res < 0)
	{
		PRN_ERROR("[upload]: unsupported bits:%d\n", (int)class_helpmgr::to_num(bits));
		return res;
	}

	if (m_hwmgr.is_reg_name(addr) >= 0)
	{
		std::string compname, regname;
		m_hwmgr.split_hw_name(addr, compname, regname);
		cmd.set_compreg_name(compname.c_str(), regname.c_str());
	}
	else
	{
		cmd.set_address(addr);
	}

	cmd.set_file_name(filename);
	res = exec_cmd_upload(&cmd);
	return res;
}

int class_cmdmgr::h2(std::string& cmd, std::string&resp)
{
	unsigned short H2UID = m_vars.u16("H2UID");
	H2UID = H2UID + 1;
	char _buf_[32];
	sprintf(_buf_,"%u", H2UID);
	m_vars.set("H2UID", _buf_, 1, 1);
	//printf("H2 send: %s\n", cmd.c_str());
	cmd = expand_vars(cmd.c_str());
	int res = m_h2mgr.send(0, H2UID, cmd, resp);
	//printf("H2 resp: %s\n", resp.c_str());
	if (res >= 0)
	{
	}
	return res;
}

int class_cmdmgr::getfile(const char* filename, void*& poutdata)
{
	clicp_cmd* pcpcmd;
	clicp_cmd* pcpcmd_ack;

	if((pcpcmd = m_connection.alloc_cmd(NCID_GET_FILE, 0, strlen(filename)+1))== NULL)
	{
		PRN_ERROR("error to allocate the NET CLI command (getfile)\n");
		return RC_CMDMGR_ALLOC_NETCMD_ERROR;
	}

	data_ptr<clicp_cmd*> ptr(pcpcmd);
	data_ptr<clicp_cmd*> ptr_ack;

	strcpy((char*)pcpcmd->params.get_file.filename, filename);

	int res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
	if (res < RC_OK)
	{
		PRN_ERROR("error to send the getfile command(%d)\n", res);
		return res;
	}

	ptr_ack = pcpcmd_ack;
	res = pcpcmd_ack->res_code;
	if (res < RC_OK)
	{
		PRN_ERROR("CLI service error(%d)\n", res);
		return res;
	}

	poutdata = malloc(pcpcmd_ack->cmd_param_len);
	if (poutdata == NULL)
	{
		PRN_ERROR("error to allocate destination buffer, the command is getfile\n");
		return RC_CMDMGR_ALLOC_BUF_ERROR;
	}

	memcpy(poutdata, &pcpcmd_ack->params.get_file_ack.data[0], pcpcmd_ack->cmd_param_len);
	return pcpcmd_ack->cmd_param_len;
}

int class_cmdmgr::readdata(int bits, const char* text_addr, unsigned int size, void* poutdata)
{
	int res = 0;
	unsigned long long address;

	res = calc_value(text_addr, address);
	if (!is_addr_valid(address) || res < 0)
	{
		PRN_ERROR("readdata: the address(0x%llx) is invalid\n", address);
		return RC_CMDMGR_ADDR_ERROR;
	}

	if (size == 0)
	{
		PRN_ERROR("The size is zero, it's not allowed\n");
		return RC_CMDMGR_SIZE_ERROR;
	}

	switch(bits)
	{
		case 16:
			size *= 2;
			break;

		case 32:
			size *= 4;
			break;

		case 64:
			size *= 8;
			break;

		default:
			break;
	}

	return readdata(bits, address, size, poutdata);
}

int class_cmdmgr::readdata(int bits, unsigned long long address, unsigned int size, void* poutdata)
{
	sync_cmd_begin();
	int rc = readdata_nosync(bits, address, size, poutdata);
	sync_cmd_end();
	return rc;
}

int class_cmdmgr::readdata_nosync(int bits, unsigned long long address, unsigned int size, void* poutdata)
{
	clicp_cmd* pcpcmd;
	clicp_cmd* pcpcmd_ack;

	switch(bits)
	{
		case 8:
		case 16:
		case 32:
		case 64:
			break;

		default:
			return RC_CMDMGR_BITS_NUM_ERROR;
	}

	if (poutdata == NULL)
		return RC_CMDMGR_PARAM_ERROR;

	if((pcpcmd = m_connection.alloc_cmd(NCID_READ_DATA, addr_to_drvid(address)))== NULL)
	{
		PRN_ERROR("error to allocate the NET CLI command (read_data)\n");
		return RC_CMDMGR_ALLOC_NETCMD_ERROR;
	}

	data_ptr<clicp_cmd*> ptr(pcpcmd);
	data_ptr<clicp_cmd*> ptr_ack;

	pcpcmd->params.readdata.address = addr_bits_align(address, bits);
	pcpcmd->params.readdata.len     = addr_bits_align(size, bits);
	pcpcmd->params.readdata.bits    = bits;

	int res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
	if (res < RC_OK)
	{
		PRN_ERROR("error to send the read_data command(%d)\n", res);
		return res;
	}

	ptr_ack = pcpcmd_ack;
	res = pcpcmd_ack->res_code;
	if (res < RC_OK)
	{
		PRN_ERROR("CLI service error(%d)\n", res);
		return res;
	}

	memcpy(poutdata, &pcpcmd_ack->params.readdata_ack.opt[0], pcpcmd_ack->cmd_param_len);
	return pcpcmd_ack->cmd_param_len;
}

int class_cmdmgr::writedata(int bits, const char* text_addr, void* pindata, unsigned int size)
{
	int res = 0;
	unsigned long long address;

	res = calc_value(text_addr, address);
	if (!is_addr_valid(address) || res < 0)
	{
		PRN_ERROR("writedata: the address(0x%llx) is invalid\n", address);
		return RC_CMDMGR_ADDR_ERROR;
	}

	if (size == 0)
	{
		PRN_ERROR("The size is zero, it's not allowed\n");
		return RC_CMDMGR_SIZE_ERROR;
	}

	/*switch(bits)
	{
		case 16:
			size *= 2;
			break;

		case 32:
			size *= 4;
			break;

		case 64:
			size *= 8;
			break;

		default:
			break;
	}*/
	//printf("%llx\n", address);
	return writedata(bits, address, pindata, size);
}

int class_cmdmgr::writedata(int bits, unsigned long long address, void* pindata, unsigned int size)
{
	sync_cmd_begin();
	int rc = writedata_nosync(bits, address, pindata, size);
	sync_cmd_end();
	return rc;
}

int class_cmdmgr::writedata_nosync(int bits, unsigned long long address, void* pindata, unsigned int size)
{
	clicp_cmd* pcpcmd;
	clicp_cmd* pcpcmd_ack;

	if (!is_addr_valid(address))
	{
		PRN_ERROR("The address(0x%llx) is invalid\n", (unsigned long long)address);
		return RC_CMDMGR_ADDR_ERROR;
	}

	if((pcpcmd = m_connection.alloc_cmd(NCID_WRITE_DATA, addr_to_drvid(address), size))== NULL)
	{
		PRN_ERROR("Error to allocate the NET CLI command (upload)\n");
		return RC_CMDMGR_ALLOC_NETCMD_ERROR;
	}

	data_ptr<clicp_cmd*> ptr(pcpcmd);
	data_ptr<clicp_cmd*> ptr_ack;

	pcpcmd->params.writedata.address = addr_bits_align(address, bits);
	pcpcmd->params.writedata.len     = addr_bits_align(size, bits);
	pcpcmd->params.writedata.bits    = bits;

	if (pcpcmd->params.writedata.len == 0)
		return RC_CMDMGR_SIZE_ERROR;

	memcpy(pcpcmd->params.writedata.data, pindata, size);

	int res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
	if (res < RC_OK)
	{
		PRN_ERROR("writedata: error to send the command(%d)\n", res);
		return res;
	}

	// to de-allocate the pointer
	ptr_ack=pcpcmd_ack;
	return pcpcmd_ack->res_code;
}

class_luamgr* class_cmdmgr::get_luamgr(void)
{
	return &m_luamgr;
}

int class_cmdmgr::fifo_ctrl(const char* fifo_name, const char* opr)
{
	unsigned long long addr = 0;
	int res = calc_value(fifo_name, addr);
	if (res < RC_OK)
		return res;

	int idx = m_hwmgr.find_by_addr(addr);
	if (idx < 0)
		return RC_CMDMGR_ADDR_ERROR;

	const hw_block& hw = m_hwmgr.get(idx);

	if (hw.m_drvid != HW_DRV_ID_REG_AXIS_FIFO)
		return RC_CMDMGR_NOT_AXIS_FIFO_REGION;

	clicp_cmd* pcpcmd;
	clicp_cmd* pcpcmd_ack;

	if((pcpcmd = m_connection.alloc_cmd(NCID_FIFO_CTRL, HW_DRV_ID_REG_AXIS_FIFO))== NULL)
	{
		PRN_ERROR("error to allocate the NET CLI command (axis-fifo ctrl)\n");
		return RC_CMDMGR_ALLOC_NETCMD_ERROR;
	}

	data_ptr<clicp_cmd*> ptr(pcpcmd);
	data_ptr<clicp_cmd*> ptr_ack;

	if (strcasecmp(opr, "reset") == 0)
	{
		pcpcmd->params.fifo_ctrl.address = addr;
		pcpcmd->params.fifo_ctrl.operation = AXIS_FIFO_CTRL_RESET;

		int res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
		if (res < RC_OK)
		{
			PRN_ERROR("error to send the axis-fifo ctrl command(%d)\n", res);
			return res;
		}

		ptr_ack = pcpcmd_ack;
		res = pcpcmd_ack->res_code;
		if (res < RC_OK)
		{
			PRN_ERROR("CLI service error(%d), axis-fifo ctrl command\n", res);
			return res;
		}
	}
	else
	{
		return RC_CMDMGR_AXIS_FIFO_UNKNOWN_OPR;
	}

	return RC_OK;
}

int class_cmdmgr::load_cur_dir_files(const char* pat, std::vector<std::string> & list)
{
	DIR *d;
	struct dirent *dir_elm;

	// To scan optional folder with the set of files
	// ---------------------------------------------
	if ((d = opendir(".")) != NULL)
	{
		while ((dir_elm = readdir(d)) != NULL)
		{
			// for Linux NFS this does not work as dir_elm->d_type == 0 always
			// ---------------------------------------------------------------
			if (dir_elm->d_type != DT_DIR)
			{
				const char* name = dir_elm->d_name;
				if (strstr(name, pat) == name)
				{
					list.push_back(name);
				}
			}
		}
		closedir(d);
	}

	return RC_OK;
}

int class_cmdmgr::get_cmd_list(const char* pat, std::vector<std::string> & list)
{
	// Let's go though the list of ALL commands
	// and to find the matching

	//int pat_len = strlen(pat);
	int dot_inside = strstr(pat, ".") != NULL;

	// -------------------------------------------------------------
	//               The list of variables
	// -------------------------------------------------------------
	for (unsigned int i = 0; i < m_vars.get_var_num(); i++)
	{
		const variable& var = m_vars.get_var(i);
		const char* name = var.m_name.c_str();
		if (strstr(name, pat) == name)
		{
			list.push_back(name);
		}
	}

	// -------------------------------------------------------------
	//               The list of NATIVE commands
	// -------------------------------------------------------------
	int k = 0;
	while (native_cmds[k].con_name != NULL)
	{
		const char* name = native_cmds[k].con_name;
		if (strstr(name, pat) == name)
		{
			list.push_back(name);
		}

		k++;
	}

	// -------------------------------------------------------------
	//               The HW components & HW components registers
	// -------------------------------------------------------------
	for (int i = 0; i < m_hwmgr.get_num(); i++)
	{
		const hw_block& hw = m_hwmgr[i];
		const char* name = hw.m_name.c_str();
		if (strstr(name, pat) == name)
		{
			list.push_back(name);
		}

		if (dot_inside)
		{
			for (unsigned int j = 0; j < hw.m_regs.size(); j++)
			{
				std::string reg_name = name;
				reg_name += ".";
				reg_name += hw.m_regs[j].m_regname;
				const char* preg_name = reg_name.c_str();
				if (strstr(preg_name, pat) == preg_name)
				{
					list.push_back(preg_name);
				}
			}
		}
	}

	// -------------------------------------------------------------
	//               The extended commands
	// -------------------------------------------------------------
	for (int i = 0; i < m_extcmdmgr.get_num(); i++)
	{
		class_ext_cmd* pcmd = m_extcmdmgr[i];
		const char* name = pcmd->get_name();
		if (strstr(name, pat) == name)
		{
			list.push_back(name);
		}
	}

	// -------------------------------------------------------------
	//               The LUA folders
	// -------------------------------------------------------------
	std::vector<std::string> lua_content;
	m_luamgr.load_content(pat, lua_content);
	std::copy (lua_content.begin(), lua_content.end(), std::back_inserter(list));

	if (m_tab_files)
		load_cur_dir_files(pat, list);
	return RC_OK;
}

int class_cmdmgr::is_lua_scr(const char* name)
{
	return m_luamgr.find_by_name(name);
}

int  class_cmdmgr::is_lua_folder(const char* path)
{
	return m_luamgr.is_folder(path);
}

void class_cmdmgr::release_cmds(void)
{
	std::vector<class_cmd*>::iterator iter;

	for(iter = m_cmds.begin(); iter < m_cmds.end(); ++iter)
	{
		delete (*iter);
	}
	m_cmds.clear();
}

int class_cmdmgr::is_digit(const char* val)
{
	if (val == NULL)
		return 0;

	// The finite state machine
	// ---------------------------
	// 0 by [0]		->	1
	// 0 by [NUM]	->	2
	// 1 by [x]		->  3
	// 2 by [NUM]	->	2
	// 2 by [eof]	->  [done], this is a DEC digit
	// 3 by [NUM]	->	3
	// 3 by [A...F]	->	3
	// 3 by [eof]	->  [done], this is HEX digit

	int state = 0;

	int i = 0;
	char ch;
	while ((ch = val[i++]) != 0)
	{
		switch (state)
		{
			case 0:
				if (ch == '0')
				{
					state = 1;
				}
				else if(isdigit(ch))
				{
					state = 2;
				}
				else
				{
					return 0;
				}
				break;

			case 1:
				if (ch == 'x')
				{
					state = 3;
				}
				else
				{
					return 0;
				}
				break;

			case 2:
				if (isdigit(ch))
				{
					state = 2;
				}
				else
				{
					return 0;
				}
				break;

			case 3:
				if (isdigit(ch)||(ch>='A'&& ch<='F')||(ch>='a'&& ch<='f'))
				{
					state = 3;
				}
				else
				{
					return 0;
				}
				break;

			default:
				return 0;
		}
	}

	if (state == 3)
		return 2;
	
	return 1;
}

int class_cmdmgr::is_alpha(const char* txt)
{
	if (txt == NULL)
		return 0;

	int i = 0;
	char ch;
	while ((ch = txt[i++]) != 0)
	{
		if (isalpha(ch) || ch == '_')
			continue;
		return 0;
	}
	return 1;
}

int class_cmdmgr::is_name(const char* txt)
{
	if (txt == NULL)
		return 0;

	int i = 0;
	char ch;
	while ((ch = txt[i++]) != 0)
	{
		if (isalpha(ch) || ch == '_')
			continue;

		if (i > 1)
		{
			if (isdigit(ch))
				continue;
		}
		return 0;
	}
	return 1;
}

int class_cmdmgr::is_bits_ok(int val)
{
	if (val == 8 ||
		val == 16||
		val == 32||
		val == 64)
		return 1;

	return 0;
}

int class_cmdmgr::is_var_ref(const char* pname)
{
	if (pname == NULL)
		return 0;

	if (pname[0] != '$')
		return 0;

	if (pname[1] == 0)
		return 0;

	int i = 1;
	char ch;
	while ( (ch=pname [i]) != 0)
	{
		if (isalpha(ch) || ch == '_')
		{
			i++;
			continue;
		}

		if (i > 1)
		{
			if (isdigit(ch))
				continue;
		}
		return 0;
	}

	return 1;
}

int class_cmdmgr::is_math_lex(std::string& lex)
{
	if (is_digit(lex.c_str()))
		return 1;

	if (lex == "+"||lex == "-"||lex == "*"||lex == "/"||lex == "("||lex == ")"||lex == "<<"||lex == ">>"||lex == "|"||lex == "&"||lex=="~")
		return 1;

	// here it can be the variable name:
	return is_var_ref(lex.c_str());
}

int class_cmdmgr::get_lex_num(const char* p, const char* br_sym, const char* sp_sym)
{
	class_lexer lexer(br_sym, sp_sym);
	lexer.init_text(p);

	int num = 0;
	while (!(lexer.lex_next()).empty())
	{
		num++;
	}
	return num;
}

unsigned long long class_cmdmgr::var2val(const char* pvariable_or_value)
{
	if (pvariable_or_value == NULL)
		return 0;

	if (is_digit(pvariable_or_value))
		return strtoull(pvariable_or_value, (char**)0, 0);

	if (is_var_ref(pvariable_or_value))
	{
		if (m_vars.find_var(pvariable_or_value+1) >= 0)
		{
			return strtoull(m_vars.get(pvariable_or_value+1).c_str(), (char**)0, 0);
		}
	}

	return 0;
}

std::string class_cmdmgr::expand_vars(const char*pvalue)
{
	std::string val;
	std::string ref_var;

	// to go through all the variables and to replace the used variables
	// with their values

	// The finite state machine 
	//
	// 0 		by [alpha]			->		0
	// 0		by [$]				->		1
	// 1		by [_ | alpha]		->		2
	// 2		by [_ | alpha | num]->		2
	// 3		by [other]			->		0

	int state = 0;
	int i = 0;
	char ch;

	while ((ch=pvalue[i]) != 0)
	{
		switch (state)
		{
			case 0:
				if (ch != '$')
				{
					val += ch;
				}
				else
				{
					ref_var = "$";
					state = 1;
				}
				break;

			case 1:
				if (ch == '_' || isalpha(ch))
				{
					ref_var += ch;
					state = 2;
				}
				else
				{
					state = 0;
					val += ref_var;
					ref_var = "";
				}
				break;

			case 2:
				if (ch == '_' || isalpha(ch) || isdigit(ch))
				{
					ref_var += ch;
					state = 2;
				}
				else
				{
					if (m_vars.find_var(ref_var.c_str()+1) >= 0)
					{
						val += m_vars.get(ref_var.c_str()+1);
						ref_var = "";
						state = 0;
					}
					else
					{
						//val += ref_var;
						val +="";
						ref_var = "";
						state = 0;
					}
					val += ch;
				}
				break;
		}
		i++;
	}

	if (!ref_var.empty())
	{
		if (m_vars.find_var(ref_var.c_str()+1) >= 0)
		{
			val += m_vars.get(ref_var.c_str()+1);
			ref_var = "";
		}
		else
		{
			//val += ref_var;
			val +="";
			ref_var = "";
		}
	}

	return val;
}

std::string class_cmdmgr::expand_ctrl_symb(const char*p)
{
	std::string val;
	int i = 0;

	while (p[i]!=0)
	{
		if (p[i] == '\\')
		{
			if (p[i+1]=='n')
			{
				val += "\n";
				i++;
			}
			else if (p[i+1]=='r')
			{
				val += "\r";
				i++;
			}
			else if (p[i+1]=='t')
			{
				val += "\t";
				i++;
			}
			else if (p[i+1]=='\\')
			{
				val += "\\";
				i++;
			}
		}
		else
		{
			val += p[i];
		}

		i++;
	}

	return val;
}

std::string class_cmdmgr::expand_hwname2addr(const char*p)
{
	/*
	std::string txt = p;

	// Here we need to replace the HW names (in a text),
	// with the real HW addresses

	for (int i = 0; i < m_hwmgr.get_num(); i++)
	{
		hw_block hw = m_hwmgr[i];

		// Here let's change all the reg names with the read addresses
		// -----------------------------------------------------------
		for (unsigned int j = 0; j < hw.m_regs.size(); j++)
		{
			std::string full_reg_name = hw.get_reg_name(j);
			std::string reg_addr_in_text = hw.get_reg_addr_as_text(j);

			while (1)
			{
				std::size_t found = txt.rfind(full_reg_name);
				if (found == std::string::npos)
					break;

				//printf("FOUND reg:%s, addr:%s\n", full_reg_name.c_str(), reg_addr_in_text.c_str());
				txt.replace(found, full_reg_name.length(), reg_addr_in_text);
			}
		}
	}

	for (int i = 0; i < m_hwmgr.get_num(); i++)
	{
		hw_block hw = m_hwmgr[i];

		// Here let's change the memory region with the real address
		// ---------------------------------------------------------
		while (1)
		{
			std::size_t found = txt.rfind(hw.m_name);
			if (found == std::string::npos)
				break;

			txt.replace(found, hw.m_name.length(), hw.get_base_as_text());
		}
	}*/

	std::string txt;

	class_lexer lexer("+-/*|^&()~", " \r\n\t", "<< >>");
	lexer.init_text(p);
	std::string lex;
	int replaced = 0;

	while (!(lex = lexer.lex_next()).empty())
	{
		for (int i = 0; i < m_hwmgr.get_num(); i++)
		{
			hw_block hw = m_hwmgr[i];

			// Here let's change all the reg names with the read addresses
			// -----------------------------------------------------------
			replaced = 0;
			for (unsigned int j = 0; j < hw.m_regs.size(); j++)
			{
				std::string full_reg_name = hw.get_reg_name(j);
				std::string reg_addr_in_text = hw.get_reg_addr_as_text(j);

				if (lex == full_reg_name)
				{
					replaced = 1;
					lex = reg_addr_in_text;
					break;
				}
			}

			if (replaced)
				break;

			if (lex == hw.m_name)
			{
				lex = hw.get_base_as_text();
				break;
			}
		}
		txt += lex;
	}

	return txt;
}

int class_cmdmgr::calc_value(const char*p, unsigned long long& result)
{
	std::string text;
	text = expand_vars(p);
	text = expand_hwname2addr(text.c_str());
	text = expand_ctrl_symb(text.c_str());

	result = 0;
	return calc_equation(text.c_str(), 64, result);
}

std::string class_cmdmgr::get_str_content(const char*p)
{
	std::string val;
	int i = 0;

	while (p[i]!=0)
	{
		if (p[i] == '\"')
		{
		}
		else
		{
			val += p[i];
		}

		i++;
	}

	return val;
}

int custom_func(int id, std::string name, void* mgr, long long& func_arg_func_res)
{
	class_cmdmgr* pmrg = (class_cmdmgr*)mgr;

	switch (id)
	{
		// To chech the proc name
		case 0:
		{
			if (name == "reg" || name == "REG")
				return 1;
			return 0;
		}

		// to calculate
		case 1:
		{
			// This case is a real calculation
			// ------------------------------------
			if (name == "reg" || name == "REG")
			{
				uint64_t result = 0;
				char buf[128];
				sprintf(buf, "0x%llx", (unsigned long long)func_arg_func_res);
				int res = pmrg->get_reg("32", buf, result, "");
				if (res == RC_OK)
					func_arg_func_res = (long long)result;
				return res;
			}
			return RC_CMDMGR_FUNC_NAME_ERROR;
		}

		// to check the HW reg name
		case 2:
		{
			unsigned long long addr = pmrg->get_hwreg_addr(name.c_str());
			if (addr == 0)
				return RC_CMDMGR_REG_NAME_ERROR;

			func_arg_func_res = (long long)addr;
			return RC_OK;
		}
	}

	return RC_CMDMGR_FUNC_NAME_ERROR;
}

int class_cmdmgr::calc_equation(const char* eq, unsigned int bits, unsigned long long & nval)
{
	class_math math(&m_vars, custom_func, this);
	int res = math.calculate(eq, nval);

	switch (bits)
	{
		case 8:
			nval &= 0xFF;
			break;

		case 16:
			nval &= 0xFFFF;
			break;

		case 32:
			nval &= 0xFFFFFFFF;
			break;

		default:
			break;
	}
	return res;
}

unsigned long long class_cmdmgr::get_hwreg_addr(const char* regname)
{
	return m_hwmgr.get_addr_by_name(regname);
}

unsigned int class_cmdmgr::addr_to_drvid(unsigned long long addr)
{
	return m_hwmgr.get_drvid_by_addr(addr);
}

int class_cmdmgr::is_addr_valid(unsigned long long addr)
{
	if (addr == 0)
		return 0;

	return 1;
}

unsigned long long class_cmdmgr::addr_bits_align(unsigned long long addr, int bits)
{
	switch (bits)
	{
		case 64:
			addr &= ~7;
			break;

		case 32:
			addr &= ~3;
			break;

		case 16:
			addr &= ~1;
			break;

		default:
			break;
	}

	return addr;
}

int class_cmdmgr::sync_cmd_begin(void)
{
	pthread_mutex_lock(&m_cmd_mutex);
	return 0;
}

int class_cmdmgr::sync_cmd_end(void)
{
	pthread_mutex_unlock(&m_cmd_mutex);
	return 0;
}

int class_cmdmgr::init(CLF_PARAMS_TYPE* p)
{
	int r, first_error = RC_OK;
	struct
	{
		const char * opr_name;
		int (class_cmdmgr::*proc)(CLF_PARAMS_TYPE* p);

	} inits[]=
	{
			{"Native commands initialization ... ",				&class_cmdmgr::init_native_cmds},
			{"Extended commands initialization ... ",			&class_cmdmgr::init_extended_cmds},
			{"Loading variables ... ", 							&class_cmdmgr::init_vars},
			{"Loading HW components ... ", 						&class_cmdmgr::init_hw_comp},
			{"LUA scripts initialization ... ", 				&class_cmdmgr::init_lua},
			{"CLI service connection ... ", 					&class_cmdmgr::init_cli_serv_conn},
			{"H2 application connection ... ", 					&class_cmdmgr::init_h2app_conn},
			{"Remote control manager ... ", 					&class_cmdmgr::init_rc_cmdmgr},
			{"IPC components initialization ... ", 				&class_cmdmgr::init_ipc_mgr},
			{"Autorunning ... ", 								&class_cmdmgr::init_autorun},
			{NULL,												NULL}
	};

	int i = 0;
	while (inits[i].proc != NULL)
	{
		PRN_INFO("%40s", inits[i].opr_name);
		if ((r = (this->*inits[i].proc)(p)) < 0)
		{
			PRN_ERROR("[Error, %d]\n", r);
			if (first_error != RC_OK)
			{
				first_error = r;
			}
		}
		else
		{
			PRN_INFO("[OK]\n");
		}
		i++;
	}

	pthread_mutex_init(&m_runcmd_mutex, NULL);
	pthread_mutex_init(&m_cmd_mutex, NULL);

	// -----------------------
	// to handle autorun code
	// -----------------------

	int idx = m_vars.find_var(CLI_AUTOVAR_NAME);
	if (idx>=0)
	{
		PRN_INFO("\n\n");
		PRN_INFO("----------------------- AutoRun commands ----------------------\n");
		run_commands(m_vars[idx].get_val());
		PRN_INFO("---------------------------------------------------------------\n");
	}

	if (m_luamgr.autorun_get_num())
	{
		PRN_INFO("\n");
		PRN_INFO("----------------------- AutoRun scripts -----------------------\n");
		m_luamgr.autorun();
		PRN_INFO("---------------------------------------------------------------\n");
	}

	m_tab_files = p->tab_files;
	return RC_OK;
}

int class_cmdmgr::run_commands(const char* cmds)
{
	pthread_mutex_lock(&m_runcmd_mutex);
	int res = internal_run_commands(cmds);
	pthread_mutex_unlock(&m_runcmd_mutex);
	return res;
}

int class_cmdmgr::internal_run_commands(const char* cmds)
{
	std::vector<class_cmd*> objcmds; // The array of the object the system created from the input text, see: parse_commands(...)

	int res = parse_commands(cmds, objcmds);
	if (res < 0)
		return res;

	res = exec_commands(objcmds);

	// we need to destroy the created objects
	std::vector<class_cmd*>::iterator iter;
	for (iter = objcmds.begin(); iter < objcmds.end(); ++iter)
	{
		delete (*iter);
	}
	objcmds.clear();
	return res;
}

int class_cmdmgr::exec_lua_script(const char* plua_scr_text)
{
	pthread_mutex_lock(&m_runcmd_mutex);
	m_luamgr.run_buffer(plua_scr_text);
	pthread_mutex_unlock(&m_runcmd_mutex);
	return RC_OK;
}

int class_cmdmgr::sched_time_event(void)
{
	m_sched_time_cur++;
	return RC_OK;
}

int class_cmdmgr::run_sched_tasks(void)
{
	if (m_sched_time == 0 || m_sched_cmd == "")
		return RC_OK;

	if (m_sched_time_cur < m_sched_time)
		return RC_OK;

	m_sched_time_cur = 0;
	int res = run_commands(m_sched_cmd.c_str());
	return res;
}

int class_cmdmgr::exec_commands(std::vector<class_cmd*>& objcmds)
{
	int r = 0;
	class_cmd* pcmd;
	class_native_cmd* pnative_cmd;
	class_ext_cmd* pext_cmd;
	class_lua_cmd*    plua_cmd;
	
	std::vector<class_cmd*>::iterator iter;
	for (iter = objcmds.begin(); iter < objcmds.end(); ++iter)
	{
		pcmd = (*iter);

		pnative_cmd = (class_native_cmd*)pcmd;
		pext_cmd = (class_ext_cmd*)pcmd;
		plua_cmd = (class_lua_cmd*)pcmd;

		// Some of the commands can be executed loccally within CLI
		// ---------------------------------------------------------
		switch (pcmd->get_id())
		{
			case NCID_REG_SET:
			case NCID_REG_GET:
			case NCID_REG_OR:
			case NCID_REG_XOR:
			case NCID_REG_AND:
				r = exec_cmd_reg(pnative_cmd);
				break;

			case NCID_SET_VAR:
				r = exec_cmd_set_var(pnative_cmd);
				break;

			case NCID_KILL_VAR:
				r = exec_cmd_remove_var(pnative_cmd);
				break;

			case NCID_VAR_LIST:
				r = exec_cmd_varlist(pnative_cmd);
				break;

			case NCID_PRINT:
				r = exec_cmd_print(pnative_cmd);
				break;

			case NCID_HELP:
				r = exec_cmd_help(pnative_cmd);
				break;

			case NCID_DUMP:
				r = exec_cmd_dump(pnative_cmd);
				break;

			case NCID_WRITE:
				r = exec_cmd_write(pnative_cmd);
				break;

			case NCID_SLEEP:
				r = exec_cmd_sleep(pnative_cmd);
				break;

			case NCID_UPLOAD:
				r = exec_cmd_upload(pnative_cmd);
				break;

			case NCID_DOWNLOAD:
				r = exec_cmd_download(pnative_cmd);
				break;

			case NCID_SCHED:
				r = exec_cmd_sched(pnative_cmd);
				break;

			case NCID_EXIT:
				r = exec_cmd_exit(pnative_cmd);
				break;

			case NCID_LS:
				r = exec_cmd_ls(pnative_cmd);
				break;

			case NCID_CD:
				r = exec_cmd_cd(pnative_cmd);
				break;

			case NCID_PWD:
				r = exec_cmd_pwd(pnative_cmd);
				break;

			case NCID_MATH:
				r = exec_cmd_math(pnative_cmd);
				break;

			case NCID_CALC:
				r = exec_cmd_calc(pnative_cmd);
				break;

			case NCID_GET_FILE:
				r = exec_cmd_getfile(pnative_cmd);
				break;

			case NCID_EXT_CMD:
				r = exec_cmd_ext(pext_cmd);
				break;

			case NCID_LUA_CMD:
				r = exec_cmd_lua(plua_cmd);
				break;

			case NCID_CD_SCR_FOLDER:
				r = exec_cd_lua_scr_folder(pnative_cmd);
				break;

			case NCID_OS_SYS_CMD:
				r = exec_cmd_os_sys_cmd(pnative_cmd);
				break;

			default:
				break;
		}

		if (r < 0)
			return r;
	}

	return RC_OK;
}

int class_cmdmgr::parse_commands(const char* cmds, std::vector<class_cmd*>& objcmds)
{
	int res;

	// to parse the commands separated by the ';'
	std::string lex;
	class_lexer cmds_lex("", ";");
	cmds_lex.lex_init(cmds);

	while (!(lex = cmds_lex.lex_next(0)).empty())
	{
		// Here we have some text (a single command)
		// let's parse this command in the separate routine
		if ((res = parse_single_command(lex.c_str(), objcmds)) < 0)
		{
			PRN_ERROR("parser error(%d)\n", res);
			break;
		}
	}

	if (res < 0)
	{
		// we need to destroy the created objects
		std::vector<class_cmd*>::iterator iter;
		for (iter = objcmds.begin(); iter < objcmds.end(); ++iter)
		{
			delete (*iter);
		}
		objcmds.clear();
		return res;
	}

	return res;
}

int class_cmdmgr::parse_single_command(const char* pcmd, std::vector<class_cmd*>& objcmds)
{
	int r = RC_OK;
	int varidx, cmdidx;

	std::string cmd_name;
	std::string cmd_body;

	class_lexer cmd_lex(":!=", " \t\n");
	cmd_lex.lex_init(pcmd);

	// The grammar is:
	// cmd ::= name {param}

	cmd_name = cmd_lex.lex_next();
	cmd_body = cmd_lex.lex_rest_text(" \r\n\t", " \r\n\t");

	//printf("body: %s\n", cmd_body.c_str());

	if (cmd_name == "reg" || cmd_name == "!")
	{
		r = pars_reg_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "set")
	{
		r = pars_set_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "math")
	{
		r = pars_math_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "print")
	{
		r = pars_print_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "help"||cmd_name == "?")
	{
		r = pars_help_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "dump")
	{
		r = pars_dump_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "write")
	{
		r = pars_write_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "sleep")
	{
		r = pars_sleep_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "upload")
	{
		r = pars_upload_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "download")
	{
		r = pars_download_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "exit")
	{
		r = pars_exit_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "ls")
	{
		r = pars_ls_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "pwd")
	{
		r = pars_pwd_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "cd")
	{
		r = pars_cd_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "sched")
	{
		r = pars_sched_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "=")
	{
		r = pars_calc_cmd(cmd_body, objcmds);
	}
	else if (cmd_name == "getfile")
	{
		r = pars_getfile_cmd(cmd_body, objcmds);
	}
	else if ((varidx = m_vars.find_var(cmd_name.c_str())) >= 0)
	{
		//printf("Running the variable[%s] commands\n", cmd_name.c_str());
		r = parse_commands(m_vars.get((unsigned int )varidx).c_str(), objcmds);
		//r = run_commands(m_vars.get((unsigned int )varidx).c_str());
	}
	else if ((cmdidx = m_extcmdmgr.find_by_name(cmd_name.c_str())) >= 0)
	{
		r = pars_extended_cmd(cmdidx, cmd_body, objcmds);
	}
	else if ((cmdidx = is_lua_scr(cmd_name.c_str())) >= 0)
	{
		r = pars_lua_cmd(cmdidx, cmd_name, cmd_body, objcmds);
	}
	else if (is_lua_folder(cmd_name.c_str()))
	{
		r = pars_lua_folder(cmd_name, cmd_body, objcmds);
	}
	else if (1) // probably this is Linux system command
	{
		 r = pars_os_system_cmd(cmd_name, cmd_body, objcmds);
	}
	else if (cmd_name.length())
	{
		PRN_ERROR("Unknown command: %s\n", cmd_name.c_str());
		r = RC_CMDMGR_UNKNOWN_ERROR;
	}
	return r;
}

int class_cmdmgr::pars_reg_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	int state = 0;
	int bits = 32;

	int cmd_id = NCID_REG_GET;

	std::string val;

	std::string address, message, value;
	std::string hw_name;
	std::string reg_name;

	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*> ptr(pcmd);

	class_lexer cmd_lex("=&^|.:+-*/(),~", " \t\n", "<< >> |= &= ^=");
	cmd_lex.lex_init(cmd_body.c_str());

	// The grammar is:
	// reg <address | name.name> [: bits] ',' [message]
	// reg <address | name.name> [: bits] operation value
	//  operation:  '=' '^=' '&=' '|='
	//  value is the math expression: 1+2 << 3 ... 
	//  bits: 8, 16, 32, 64

	// THE FINITE STATE MACHINE
	// ---------------------------------------------
	// state:
	//  0 	by [address]		-> 	1
	//	0	by [name]			->	100
	//
	// To pars name.name
	//	100	by [.]				->	101
	//	101	by [name]			->	1
	//
	// To pars the operation
	// 1	by [eof]			->	1		[done:  REG.GET]
	// 1	by [:]				->  20
	// 1|21	by ','				->	1001
	// 1|21	by '='				->  2001
	// 1|21	by '|='				->  3001
	// 1|21	by '&='				->  4001
	// 1|21	by '^='				->  5001
	//
	// 1001	by	[message]		->	1002	[done:  REG.GET with message]
	//
	// 
	// 20 by [num]				-> 21
	//
	//
	//      [REG.SET]
	// 2001 by [value]			->	10000 [done: REG.SET command]
	// 2001 by [varname]		->	10000 [done: REG.SET command]
	// 2001 by ':'				->	2002
	// 2001 by [bits]			->  10000 [done: REG.SET command]
	//
	//
	//      [REG.OR]
	// 3001 by [val]			->	3003 [done: REG.OR command]
	// 3001 by [varname]		->	3004 [done: REG.OR command]
	//
	//
	//      [REG.AND]
	// 4001 by [val]			->	10000 [done: REG.AND command]
	// 4001 by [varname]		->	10000 [done: REG.AND command]
	//
	//
	//      [REG.XOR]
	// 5001 by [val]			->	10000[done: REG.XOR command]
	// 5001 by [varname]		->	10000[done: REG.XOR command]
	//
	// 10000 by [num]			-> 10000 [done]
	// 10000 by [var]			-> 10000 [done]
	// 10000 by [operation]		-> 10000 [done]
	
	while(!(val = cmd_lex.lex_next()).empty())
	{
		//printf("state:%d, lex:%s\n", state, val.c_str());
		switch (state)
		{
			case 0:
				if (is_digit(val.c_str())||is_var_ref(val.c_str()))
				{
					state = 1;
					address = val;
				}
				else
				{
					state = 100;
					hw_name = val;
				}
				break;

			case 100:
				if (val != ".")
				{
					PRN_ERROR("REG command format error, it's expected '.' for the name.name construction\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				state = 101;
				break;

			case 101:
				reg_name = val;
				state = 1;
				break;

			case 1:
			case 21:
				if (val == ":" && state == 1)
				{
					state = 20;
				}
				else if (val == ",")
				{
					state = 1002;
					message = cmd_lex.lex_rest_text(" \t");
					cmd_id = NCID_REG_GET;
				}
				else if (val == "=")
				{
					state = 2001;
				}
				else if (val == "|=")
				{
					state = 3001;
				}
				else if (val == "&=")
				{
					state = 4001;
				}
				else if (val == "^=")
				{
					state = 5001;
				}
				else
				{
					PRN_ERROR("REG command format error, unsupported operation: %s\n", val.c_str());
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			case 20:
				if (is_digit(val.c_str()))
				{
					state = 21;
					bits = (int)class_helpmgr::to_num(val.c_str());
					if (bits!=8 && bits != 16 && bits != 32 && bits != 64)
					{
						PRN_ERROR("REG command: unsupported number of bits\n");
						return RC_CMDMGR_REGCMDPARS_ERROR;
					}
					break;
				}
				PRN_ERROR("REG command format error, the message should be ONE word or the text in the \"\"\n");
				return RC_CMDMGR_REGCMDPARS_ERROR;

			case 1002:
				PRN_ERROR("REG command format error, the message should be ONE word or the text in the \"\"\n");
				return RC_CMDMGR_REGCMDPARS_ERROR;


			// ------------------------------------------------------
			//        To pars the REG.SET command
			// ------------------------------------------------------
			case 2001:
				cmd_id = NCID_REG_SET;
				if (val == ":")
				{
					state = 2002;
				}
				else if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG.SET command format error\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			case 2002:
				if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG.SET command format error\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			// ------------------------------------------------------
			//        To pars the REG.OR command
			// ------------------------------------------------------
			case 3001:
				cmd_id = NCID_REG_OR;
				if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG.OR command format error\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			// ------------------------------------------------------
			//        To pars the REG.AND command
			// ------------------------------------------------------
			case 4001:
				cmd_id = NCID_REG_AND;
				if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG.AND command format error\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			// ------------------------------------------------------
			//        To pars the REG.XOR command
			// ------------------------------------------------------
			case 5001:
				cmd_id = NCID_REG_XOR;
				if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG.XOR command format error\n");
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}
				break;

			case 10000:
				// Here we have some native math string parser
				// which checks sanity of the text
				if (is_math_lex(val) || 1)
				{
					value += val;
					state = 10000;
				}
				else
				{
					PRN_ERROR("REG command format error:%s\n", val.c_str());
					return RC_CMDMGR_REGCMDPARS_ERROR;
				}

				break;

			default:
				PRN_ERROR("REG command format error\n");
				return RC_CMDMGR_REGCMDPARS_ERROR;
		}
	}

	if (state == 0)
	{
		class_helpmgr::show_commad_info(NCID_REG_SET);
		class_helpmgr::show_commad_info(NCID_REG_GET);
		class_helpmgr::show_commad_info(NCID_REG_OR);
		class_helpmgr::show_commad_info(NCID_REG_XOR);
		class_helpmgr::show_commad_info(NCID_REG_AND);
		return RC_OK;
	}

	// ----------------------------------------------
	//       reg (get,set,or,and,xor) commands
	// ----------------------------------------------
	if (state == 1 || state == 21 || state == 1002 || state == 10000)
	{
		pcmd->set_id(cmd_id);
		if (!address.empty())
			pcmd->set_address(address.c_str());
		else
			pcmd->set_compreg_name(hw_name.c_str(), reg_name.c_str());

		pcmd->set_message(message.c_str());
		pcmd->set_value(value.c_str(), 0);
		pcmd->set_bits(bits);
		//printf("command-id:%d [%d], address:%s (%s.%s): %s\n", cmd_id, pcmd->get_bits(), address.c_str(), hw_name.c_str(), reg_name.c_str(), value.c_str());
	}
	else
	{
		PRN_ERROR("REG command format error\n");
		return RC_CMDMGR_REGCMDPARS_ERROR;
	}

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}


int class_cmdmgr::pars_set_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string varname, value;

	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*>ptr(pcmd);

	class_lexer cmd_lex("", " \t\n");
	cmd_lex.lex_init(cmd_body.c_str());

	// The finite state machine
	// ---------------------------------------------
	//
	// set						- to print all the variables
	// set [varname] = value	- to set the variable
	// set [varname]			- to kill the variable

	// 0 	by eof		->		0	[done] the list of the variables
	// 0 	by [name]	->		1	[done] to kill the var
	// 1	by [=]		->		2
	// 2 	by [value]	->		3
	// 3	by [value]	->		3
	// 3	by eof		->		3	[done] to set/update the var

	varname = cmd_lex.lex_next();
	value = cmd_lex.lex_rest_text(" \n\t\r\"", " \n\t\r\"");
	//value = cmd_lex.lex_next();

	if (varname == "")
	{
		pcmd->set_id(NCID_VAR_LIST);
	}
	else if (value == "")
	{
		if (!is_name(varname.c_str()))
		{
			PRN_ERROR("the variable name format is incorrect: [%s]\n", varname.c_str());
			return RC_CMDMGR_SETCMDPARS_ERROR;
		}
		pcmd->set_address(varname.c_str());
		pcmd->set_id(NCID_KILL_VAR);
	}
	else
	{
		if (!is_name(varname.c_str()))
		{
			PRN_ERROR("the variable name format is incorrect: [%s]\n", varname.c_str());
			return RC_CMDMGR_SETCMDPARS_ERROR;
		}

		pcmd->set_address(varname.c_str());
		pcmd->set_id(NCID_SET_VAR);
		pcmd->set_value(value.c_str());
	}

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_math_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string varname, value;

	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*>ptr(pcmd);
	class_lexer cmd_lex("", " \t\n");
	cmd_lex.lex_init(cmd_body.c_str());

	varname = cmd_lex.lex_next();
	value = cmd_lex.lex_rest_text(" \n\t\r\"", " \n\t\r\"");

	if (!is_name(varname.c_str()))
	{
		PRN_ERROR("the variable name format is incorrect: [%s]\n", varname.c_str());
		return RC_CMDMGR_SETCMDPARS_ERROR;
	}

	pcmd->set_address(varname.c_str());
	pcmd->set_id(NCID_MATH);
	pcmd->set_value(value.c_str());

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}


int class_cmdmgr::pars_print_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string value;

	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_lexer cmd_lex("", "");
	cmd_lex.lex_init(cmd_body.c_str());

	// The grammar:
	// ---------------------------------------------
	// print value
	
	value = cmd_lex.lex_rest_text(" \n\t\r", " \n\t\r");
	
	pcmd->set_id(NCID_PRINT);
	pcmd->set_value(value.c_str());

	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_help_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_lexer cmd_lex("", " \t\r\n");
	cmd_lex.lex_init(cmd_body.c_str());

	pcmd->set_id(NCID_HELP);
	pcmd->set_value(cmd_lex.lex_next().c_str());

	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_dump_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string lex;
	int state = 0;

	int bits = 8;

	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*> ptr(pcmd);

	class_lexer cmd_lex(":", " \t\r\n");
	cmd_lex.lex_init(cmd_body.c_str());

	pcmd->set_bits(bits);

	// The finite state machine for the grammar:
	//  dump[:bits] address len

	// 0 by ':'			->	1
	// 0 by [num|var]	->  10
	// 1 by [num]		->	2
	// 2 by [val]		->  3
	// 3 by [val]		->  4 [done]
	// 10 by [num]		-> 11 [done: dump addr len]

	while (!(lex = cmd_lex.lex_next()).empty())
	{
		switch (state)
		{
			case 0:
				if (lex == ":")
				{
					state = 1;
				}
				else //if (is_digit(lex.c_str()) || is_var_ref(lex.c_str()) || is_hw_name(lex.c_str()))
				{
					state = 10;
					pcmd->set_address(lex.c_str());
				}
				/*else
				{
					PRN_ERROR("the dump command format error\n");
					return RC_CMDMGR_DUMPPARAM_ERROR;
				}*/
				break;

			case 1:
				bits = (int)class_helpmgr::to_num(lex.c_str());
				if (!is_bits_ok(bits))
				{
					PRN_ERROR("the dump command format error, unsupported number of bits:%d\n", bits);
					return RC_CMDMGR_DUMPPARAM_ERROR;
				}
				state = 2;
				pcmd->set_bits(bits);
				break;

			case 2:
				pcmd->set_address(lex.c_str());
				state = 3;
				break;

			case 3:
				pcmd->set_value(lex.c_str());
				state = 4;
				break;

			case 10:
				pcmd->set_value(lex.c_str());
				state = 11;
				break;
		}
	}

	if (state == 0)
	{
		class_helpmgr::show_commad_info(NCID_DUMP);
		return RC_OK;
	}

	if (state != 4 && state != 11)
	{
		PRN_ERROR("the dump command format error\n");
		return RC_CMDMGR_DUMPPARAM_ERROR;
	}

	pcmd->set_id(NCID_DUMP);

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_write_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string lex;
	int state = 0;
	int bits = 8;
	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*> ptr(pcmd);

	class_lexer cmd_lex(":", ", \t\r\n");
	cmd_lex.lex_init(cmd_body.c_str());

	pcmd->set_bits(bits);

	// The finite state machine for the grammar:
	//  write[:bits] address , data {, data}

	// 0 by ':'			->	1
	// 0 by [num|var]	->  10
	// 1 by [num]		->	2
	// 2 by [val]		->  3
	// 3 by [val]		->  4 [done]
	// 10 by [num]		-> 11 [done: dump addr len]

	while (!(lex = cmd_lex.lex_next()).empty())
	{
		switch (state)
		{
			case 0:
				if (lex == ":")
				{
					state = 1;
				}
				else
				{
					state = 3;
					pcmd->set_address(lex.c_str());
					cmd_lex.set_delims(",");
				}
				break;

			case 1:
				bits = (int)class_helpmgr::to_num(lex.c_str());
				if (!is_bits_ok(bits))
				{
					PRN_ERROR("the dump command format error, unsupported number of bits:%d\n", bits);
					return RC_CMDMGR_WRITE_PARAM_ERROR;
				}
				state = 2;
				pcmd->set_bits(bits);
				cmd_lex.set_delims(",");
				break;

			case 2:
				pcmd->set_address(lex.c_str());
				state = 3;
				break;

			case 3:
				value = lex;
				state = 4;
				break;

			case 4:
				value += "," + lex;
				state = 4;
				break;
		}
	}

	if (state == 0)
	{
		class_helpmgr::show_commad_info(NCID_WRITE);
		return RC_OK;
	}

	if (state != 4)
	{
		PRN_ERROR("the write command format error\n");
		return RC_CMDMGR_WRITE_PARAM_ERROR;
	}

	pcmd->set_value(value.c_str());
	pcmd->set_bits(bits);
	pcmd->set_id(NCID_WRITE);

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}


int class_cmdmgr::pars_sleep_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_lexer cmd_lex("", " \t\r\n");
	cmd_lex.lex_init(cmd_body.c_str());

	pcmd->set_id(NCID_SLEEP);
	pcmd->set_value(cmd_lex.lex_rest_text("", "").c_str());

	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_upload_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	// The command format is:
	// upload[:bits] <filename> <address>
	//
	// <filename> - the name or name of the variable
	// <address> - the address | variable | expression

	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*> ptr(pcmd);

	class_lexer cmd_lex(":", " \t\r\n", "<< >>");
	cmd_lex.lex_init(cmd_body.c_str());
	std::string lex, val;

	int state = 0;

	pcmd->set_bits(8);

	// ----------------------------------------------------
	// The state machine is:
	// ----------------------------------------------------
	// 0 	by [file name|variable]		->		1
	// 0	by :						->		100
	// 100	by [bits]					->		101
	// 101  by [file name|variable]		->		1
	// 1	by [digit| var | oper]		->		2	[done]
	// 2	by [digit| var | oper]		->		2	[done]
	// ----------------------------------------------------

	while (!(lex = cmd_lex.lex_next()).empty())
	{
		switch (state)
		{
			case 0:
				if (lex == ":")
				{
					state = 100;
				}
				else
				{
					pcmd->set_file_name(lex.c_str());
					state = 1;
				}
				break;

			case 100:
				{
					int bits = (int)class_helpmgr::to_num(lex.c_str());
					if (!is_bits_ok(bits))
					{
						PRN_ERROR("unsupported number of bits: %d\n", bits);
						return RC_CMDMGR_BITS_NUM_ERROR;
					}
					pcmd->set_bits(bits);
					state = 101;
				}
				break;

			case 101:
				pcmd->set_file_name(lex.c_str());
				state = 1;
				break;

			case 1:
				state = 2;
				val += lex;
				break;

			case 2:
				state = 2;
				val += lex;
				break;
		}
	}

	if (state == 0)
	{
		class_helpmgr::show_commad_info(NCID_UPLOAD);
		return RC_OK;
	}

	if (state != 2)
	{
		PRN_ERROR("the upload command format error\n");
		return RC_CMDMGR_UPLOADLOAD_PARAM_ERROR;
	}

	pcmd->set_id(NCID_UPLOAD);
	pcmd->set_address(val.c_str());
	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_download_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	// The command format is:
	// download [:bits] <address> <len> <filename>
	//
	// [bits] - how to access the memory region: as 8,16,32 or 64 bits
	// <filename> - the name or name of the variable
	// <address> - the address | variable | expression

	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_ptr<class_native_cmd*> ptr(pcmd);

	class_lexer cmd_lex(":", " \t\r\n", "<< >>");
	cmd_lex.lex_init(cmd_body.c_str());
	std::string lex;

	int state = 0;
	pcmd->set_bits(8);

	// ----------------------------------------------------
	// The state machine is:
	// ----------------------------------------------------
	// 0    by [:]						->		100
	// 100	by [digit]					->		101
	// 0 	by [variable|value]			->		1
	// 101  by [variable|value]			->		1
	// 1	by [digit| var]				->		2
	// 2	by [name]					->		3 [done]
	// ----------------------------------------------------

	while (!(lex = cmd_lex.lex_next()).empty())
	{
		switch (state)
		{
			case 0:
				if (lex == ":")
				{
					state = 100;
				}
				else
				{
					pcmd->set_address(lex.c_str());
					state = 1;
				}
				break;

			case 100:
				{
					int bits = (int)class_helpmgr::to_num(lex.c_str());
					if (!is_bits_ok(bits))
					{
						PRN_ERROR("unsupported number of bits: %d\n", bits);
						return RC_CMDMGR_BITS_NUM_ERROR;
					}
					pcmd->set_bits(bits);
					state = 101;
				}
				break;

			case 101:
				pcmd->set_address(lex.c_str());
				state = 1;
				break;

			case 1:
				pcmd->set_value(lex.c_str());
				state = 2;
				break;

			case 2:
				pcmd->set_file_name(lex.c_str());
				state = 3;
				break;

			case 3:
				PRN_ERROR("the download command format error\n");
				return RC_CMDMGR_DOWNLOAD_PARAM_ERROR;
		}
	}

	if (state == 0)
	{
		class_helpmgr::show_commad_info(NCID_DOWNLOAD);
		return RC_OK;
	}

	if (state != 3)
	{
		PRN_ERROR("the download command format error\n");
		return RC_CMDMGR_UPLOADLOAD_PARAM_ERROR;
	}

	pcmd->set_id(NCID_DOWNLOAD);

	ptr = NULL;
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_exit_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	// The command format is:
	// exit [code]
	//
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_value(cmd_body.c_str());
	pcmd->set_id(NCID_EXIT);
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_ls_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_LS);
	pcmd->set_value(cmd_body.c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_pwd_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_PWD);
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_cd_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_CD);
	pcmd->set_value(cmd_body.c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_extended_cmd(int cmdidx, std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string ext_cmd_text;
	class_ext_cmd* pext_cmd = m_extcmdmgr[cmdidx];

	int total_param_num = pext_cmd->get_param_num();
	int req_param_num = pext_cmd->get_req_param_num();
	int cli_param_num = 0;
	
	ext_cmd_text = m_extcmdmgr[cmdidx]->get_text();

	class class_lexer lexer("", " \r\n\t");
	std::vector<std::string> cli_params;
	std::string lex;
	lexer.lex_init(cmd_body.c_str());
	while (!(lex=lexer.lex_next()).empty())
	{
		cli_params.push_back(lex);
		cli_param_num++;
	}

	if (cli_param_num > total_param_num)
	{
		PRN_ERROR("'%s' command requires %d parameters only\n", pext_cmd->get_name(), req_param_num);
		//PRN_ERROR("The number of parameters (detected:%d) is more than expected(%d) for command:%s\n", cli_param_num, total_param_num, pext_cmd->get_name());
		return RC_CMDMGR_PARAM_NUM_ERROR;
	}

	if (cli_param_num < req_param_num)
	{
		PRN_ERROR("'%s' command requires at least %d parameters\n", pext_cmd->get_name(), req_param_num);
		//PRN_ERROR("The number of parameters (detected:%d) is less than expected(%d) for command:%s\n", cli_param_num, req_param_num, pext_cmd->get_name());
		return RC_CMDMGR_PARAM_NUM_ERROR;
	}

	// Here we need to replace the parameters in a text,
	// with the real values taken from the command line

	for (int i = 0; i < total_param_num; i++)
	{
		const cmd_param& param = pext_cmd->get_param(i);
		while (1)
		{
			std::size_t found = ext_cmd_text.rfind("$"+param.m_name);
			if (found == std::string::npos)
				break;

			if (i < cli_param_num)
			{
				ext_cmd_text.replace(found, ("$"+param.m_name).length(), cli_params[i]);
			}
			else
			{
				ext_cmd_text.replace(found, ("$"+param.m_name).length(), param.m_defval);
			}
		}
	}

	if (pext_cmd->get_interface() == EXT_CMD_H2)
	{
		class_ext_cmd* ph2_cmd = new class_ext_cmd();
		if (ph2_cmd == NULL)
		{
			PRN_ERROR("H2 command: allocation error\n");
			return RC_CMDMGR_ALLOCCMD_ERROR;
		}

		ph2_cmd->set_id(NCID_EXT_CMD);
		ph2_cmd->set_text(ext_cmd_text.c_str());
		ph2_cmd->set_json_resp_format(pext_cmd->get_json_resp_format());
		objcmds.push_back(ph2_cmd);
		return RC_OK;
	}
	return parse_commands(ext_cmd_text.c_str(), objcmds);
}


int class_cmdmgr::pars_lua_cmd(int cmdidx, std::string& fullname, std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_lua_cmd* plua_cmd = new class_lua_cmd();
	if (plua_cmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class class_lexer lexer("", " \r\n\t");
	std::string lex;
	lexer.lex_init(cmd_body.c_str());

	while (!(lex=lexer.lex_next()).empty())
	{
		plua_cmd->add_param(lex.c_str());
	}

	plua_cmd->set_name(fullname.c_str());
	plua_cmd->set_proc_name(m_luamgr.find_proc_name(fullname.c_str()).c_str());
	objcmds.push_back(plua_cmd);
	return RC_OK;
}

int class_cmdmgr::pars_lua_folder(std::string& fullname, std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_CD_SCR_FOLDER);

	pcmd->set_value(fullname.c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_sched_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string time, cmds;

	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_lexer cmd_lex("", " \t\n");
	cmd_lex.lex_init(cmd_body.c_str());

	time = cmd_lex.lex_next();
	cmds = cmd_lex.lex_rest_text(" \n\t\r\"", " \n\t\r\"");

	pcmd->set_address(time.c_str());
	pcmd->set_id(NCID_SCHED);
	pcmd->set_value(cmds.c_str());

	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_calc_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_CALC);
	pcmd->set_value(cmd_body.c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_getfile_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	std::string value;
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	class_lexer cmd_lex("", "");
	cmd_lex.lex_init(cmd_body.c_str());
	value = cmd_lex.lex_rest_text(" \n\t\r", " \n\t\r");
	pcmd->set_id(NCID_GET_FILE);
	pcmd->set_value(value.c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::pars_os_system_cmd(std::string& name, std::string& cmd_body, std::vector<class_cmd*>& objcmds)
{
	class_native_cmd* pcmd = new class_native_cmd();
	if (pcmd == NULL)
		return RC_CMDMGR_ALLOCCMD_ERROR;

	pcmd->set_id(NCID_OS_SYS_CMD);
	pcmd->set_value((name + " " + cmd_body).c_str());
	objcmds.push_back(pcmd);
	return RC_OK;
}

int class_cmdmgr::exec_cmd_set_var(class_native_cmd*pnative_cmd)
{
	if (m_vars.is_system(pnative_cmd->get_address()))
	{
		PRN_ERROR("[%s] is the system variable\n", pnative_cmd->get_address());
		return RC_CMDMGR_SYSVAR_USAGE_ERROR;
	}

	std::string val = expand_ctrl_symb(expand_vars(pnative_cmd->get_value()).c_str());
	int r = m_vars.set(pnative_cmd->get_address(), val.c_str());
	return r;
}

int class_cmdmgr::exec_cmd_remove_var(class_native_cmd*pnative_cmd)
{
	if (m_vars.is_system(pnative_cmd->get_address()))
	{
		PRN_ERROR("[%s] is the system variable\n", pnative_cmd->get_address());
		return RC_CMDMGR_SYSVAR_USAGE_ERROR;
	}
	int r = m_vars.set(pnative_cmd->get_address(), "");
	return r;
}

int class_cmdmgr::exec_cmd_varlist(class_native_cmd*pcmd)
{
	if (m_vars.get_var_num() == 0)
	{
		PRN_INFO("    There is no CLI variables\n");
		return RC_OK;
	}

	PRN_INFO("-------------------------------------\n");
	PRN_INFO("        CLI user vars/commands:\n");
	PRN_INFO("-------------------------------------\n");

	for (unsigned int i =0; i < m_vars.get_var_num(); i++)
	{
		const variable & v = m_vars.get_var(i);

		PRN_INFO("*[%6s] %-8s = %s\n", v.m_system ? "system":"user", v.m_name.c_str(), v.m_value.c_str());
	}

	PRN_INFO("-------------------------------------\n");
	return RC_OK;
}

int class_cmdmgr::exec_cmd_print(class_native_cmd*pcmd)
{
	PRN_INFO("%s\n", expand_ctrl_symb(expand_vars(pcmd->get_value()).c_str()).c_str());
	return RC_OK;
}

int class_cmdmgr::exec_cmd_help(class_native_cmd*pcmd)
{
	int id;
	if ((id = cmd_name2id(pcmd->get_value())) > 0)
	{
		class_helpmgr::show_commad_info(id);
	}
	else if ((id = m_extcmdmgr.find_by_name(pcmd->get_value()))> -1)
	{
		PRN_INFO(" %-12s %-20s  - %s\n", m_extcmdmgr[id]->get_name(), m_extcmdmgr[id]->get_params_info(), m_extcmdmgr[id]->get_descr());
	}
	else if (m_luamgr.find_by_name(pcmd->get_value())>-1)
	{
		// to provide the help about some script
		m_luamgr.show_scr_help(pcmd->get_value());
	}
	else if ((id = m_hwmgr.find_by_name(pcmd->get_value()))> -1)
	{
		const hw_block & hw = m_hwmgr[id];

		PRN_INFO("----------------------------------------\n");
		PRN_INFO("       NAME: %s [0x%llx .. 0x%llx]\n", hw.m_name.c_str(),hw.m_base, hw.m_base + hw.m_size);
		PRN_INFO("        VER: %s\n", hw.m_version.c_str());
		PRN_INFO("     DRV-ID: %d\n", hw.m_drvid);
		PRN_INFO("       INFO: %s\n", hw.m_descr.c_str());
		PRN_INFO("----------------------------------------\n");
		unsigned int name_len_max = 0;
		for (unsigned int i = 0; i < hw.m_regs.size(); i++)
		{
			if (name_len_max < hw.m_regs[i].m_regname.length())
				name_len_max = hw.m_regs[i].m_regname.length();
		}
		char buf[256];
		sprintf(buf, "  [0x%%03x]:%%02d(b) .%%-%02us - %%s\n", name_len_max);
		for (unsigned int i = 0; i < hw.m_regs.size(); i++)
		{
			//PRN_INFO("  [0x%03x]:%02d(b) .%-10s - %s\n", hw.m_regs[i].m_regoffs, hw.m_regs[i].m_regbits, hw.m_regs[i].m_regname.c_str(), hw.m_regs[i].m_desc.c_str());
			PRN_INFO(buf, hw.m_regs[i].m_regoffs, hw.m_regs[i].m_regbits, hw.m_regs[i].m_regname.c_str(), hw.m_regs[i].m_desc.c_str());
		}
		PRN_INFO("\n");
	}
	else
	{
		class_helpmgr::show_commad_list();

		// to show the list of extended commands
		// -------------------------------------------

		if (m_extcmdmgr.get_num())
		{
			PRN_INFO("-----------------------------------------------------\n");
			PRN_INFO(" Extended commands:\n");
			PRN_INFO("-----------------------------------------------------\n");

			for (int i = 0; i < m_extcmdmgr.get_num(); i++)
			{
				PRN_INFO(" %-12s %-20s  - %s\n", m_extcmdmgr[i]->get_name(), m_extcmdmgr[i]->get_params_info(), m_extcmdmgr[i]->get_descr());
			}
			PRN_INFO("\n");
		}

		if (m_hwmgr.get_num())
		{
			PRN_INFO("-----------------------------------------------------\n");
			PRN_INFO(" HW components:\n");
			PRN_INFO("-----------------------------------------------------\n");

			for (int i = 0; i < m_hwmgr.get_num(); i++)
			{
				PRN_INFO(" %-12s (ver:%s) - %s\n", m_hwmgr[i].m_name.c_str(), m_hwmgr[i].m_version.c_str(), m_hwmgr[i].m_descr.c_str());
			}
			PRN_INFO("\n");
		}

		if (m_luamgr.get_scripts_num())
		{
			PRN_INFO("-----------------------------------------------------\n");
			PRN_INFO(" LUA scripts:\n");
			PRN_INFO("-----------------------------------------------------\n");

			// firstly we need to find the maximum length of name/folder for the LUA scripts
			// to adjust the text on the screen

			std::vector<std::string> folders;
			std::vector<std::string> files;

			unsigned int max_len = 0;
			m_luamgr.load_folders(FOLDER_SET_FOLDERS, folders);
			std::sort(folders.begin(), folders.end());

			for (unsigned int i = 0; i < folders.size(); i++)
			{
				if (max_len < folders[i].length())
					max_len = folders[i].length();
			}

			m_luamgr.load_folders(FOLDER_SET_FILES, files);
			for (unsigned int i = 0; i < files.size(); i++)
			{
				if (max_len < files[i].length())
					max_len = files[i].length();
			}

			// Here to create the PRINTF pattern
			char pat_scr_folder[64];
			sprintf(pat_scr_folder, " %%-%ds\n", max_len+1);

			char pat_scr_name[64];
			sprintf(pat_scr_name, " %%-%ds : %%12s - %%s\n", max_len+1);

			// -------------------------------------------------------------
			// to print the list of folders
			// before to print the list of the scripts in the current folder
			// -------------------------------------------------------------
			for (unsigned int i = 0; i < folders.size(); i++)
			{
				PRN_INFO(pat_scr_folder, folders[i].c_str());
			}

			// -------------------------------------------------------------
			// To print the list of the scripts
			// -------------------------------------------------------------
			for (unsigned int i = 0; i < m_luamgr.get_scripts_num(); i++)
			{
				if (m_luamgr.is_scr_path(i))
					PRN_INFO(pat_scr_name, m_luamgr[i].m_name.c_str(), m_luamgr[i].m_textopt.c_str(), m_luamgr[i].m_descr.c_str());
			}
			PRN_INFO("\n");
		}
	}
	return RC_OK;
}

int class_cmdmgr::exec_cmd_dump(class_native_cmd*pcmd)
{
	int res = 0;
	int bits = 8;
	unsigned int i = 0;
	unsigned long long address;
	unsigned long long size;

	res = calc_value(pcmd->get_address(), address);
	if (!is_addr_valid(address) || res < 0)
	{
		PRN_ERROR("The address(0x%llx) is invalid\n", address);
		return RC_CMDMGR_ADDR_ERROR;
	}
	res = calc_value(pcmd->get_value(), size);
	bits = pcmd->get_bits();

	if (size == 0)
	{
		PRN_ERROR("The size is zero, it's not allowed\n");
		return RC_CMDMGR_SIZE_ERROR;
	}

	switch(bits)
	{
		case 16:
			size *= 2;
			break;

		case 32:
			size *= 4;
			break;

		case 64:
			size *= 8;
			break;

		default:
			break;
	}

	void* poutbuf = malloc(size + 128);
	if (poutbuf == NULL)
	{
		PRN_ERROR("Error to allocate the buffer\n");
		return RC_CMDMGR_ALLOC_BUF_ERROR;
	}
	memset(poutbuf, 0, size);

	res = readdata(pcmd->get_bits(), addr_bits_align(address, bits), size, poutbuf);
	if (res < RC_OK)
	{
		PRN_ERROR("read command error(%d)\n", res);
		free(poutbuf);
		return res;
	}

	// ---------------------------------------------------
	// to SET the size to the value returned by CLI server
	// ---------------------------------------------------
	size = res;
	// ---------------------------------------------------

	PRN_INFO("memory dump(%d bits mode): from [0x%llx] to [0x%llx] (data size is [%llu] bytes):\n", bits, address, address + size, size);

	uint8_t* 			p8  =  (uint8_t*)poutbuf;
	uint16_t* 			p16 = (uint16_t*)poutbuf;
	uint32_t* 			p32 = (uint32_t*)poutbuf;
	uint64_t* 			p64 = (uint64_t*)poutbuf;

	PRN_INFO("0x%04llx:  ", address);

	if (bits == 16)
	{
		size >>= 1;
	}
	else if (bits == 32)
	{
		size >>= 2;
	}
	else if (bits == 64)
	{
		size >>= 3;
	}

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

	free(poutbuf);
	poutbuf = NULL;
	return RC_OK;
}

int class_cmdmgr::exec_cmd_write(class_native_cmd*pcmd)
{
	unsigned long long address;
	unsigned long long size;

	int res = calc_value(pcmd->get_address(), address);
	if (res < 0)
	{
		PRN_ERROR("error to calculate destination address: %s\n", pcmd->get_address());
		return res;
	}
	if (!is_addr_valid(address))
	{
		PRN_ERROR("the address(0x%llx) is invalid\n", (unsigned long long)address);
		return RC_CMDMGR_ADDR_ERROR;
	}

	size = get_lex_num(pcmd->get_value(), "", ",");
	switch (pcmd->get_bits())
	{
		case 8:
		break;

		case 16:
			size *= 2;
		break;

		case 32:
			size *= 4;
		break;

		case 64:
			size *= 8;
		break;

		default:
			PRN_ERROR("FIFO write, the bits number error, bits:%d\n", pcmd->get_bits());
			return RC_CMDMGR_BITS_NUM_ERROR;
	}

	void* pdata = malloc(size);
	if (pdata == NULL)
	{
		PRN_ERROR("FIFO write, error to allocate buffer\n");
		return RC_CMDMGR_ALLOC_BUF_ERROR;
	}

	unsigned long long val;
	class_lexer lexer("", ",");
	std::string lex;

	int i = 0;

	lexer.init_text(pcmd->get_value());
	while(!(lex=lexer.lex_next()).empty())
	{
		res = calc_value(lex.c_str(), val);
		if (res < 0)
		{
			PRN_ERROR("FIFO-WRITE: error to calculate the value for '%s'\n", lex.c_str());
			return RC_CMDMGR_CALC_ERROR;
		}

		switch (pcmd->get_bits())
		{
			case 8:
				((uint8_t*)pdata)[i] = (uint8_t)val;
				break;

			case 16:
				((uint16_t*)pdata)[i] = (uint16_t)val;
				break;

			case 32:
				((uint32_t*)pdata)[i] = (uint32_t)val;
				break;

			case 64:
				((uint64_t*)pdata)[i] = (uint64_t)val;
				break;
		}

		i++;
	}

	res = writedata(pcmd->get_bits(), address, pdata, size);

	if (addr_to_drvid(address) == 1)
	{
		if (res < 0)
		{
			PRN_INFO("WRITE-FIFO(%llu bytes) at [0x%llx] - Error(%d)\n",
				size,
				(unsigned long long)address,
				res);
		}
		else
		{
			PRN_INFO("WRITE-FIFO(%llu bytes) at [0x%llx] - OK (written %d bytes)\n",
				size,
				(unsigned long long)address,
				res);
		}
	}
	else
	{
		if (res < 0)
		{
			PRN_INFO("WRITE data (%llu bytes) at [0x%llx ... 0x%llx] - Error(%d)\n",
				size,
				(unsigned long long)address,
				(unsigned long long)address+size,
				res);
		}
		else
		{
			PRN_INFO("WRITE data (%llu bytes) at [0x%llx ... 0x%llx] - OK (written %d bytes)\n",
				size,
				(unsigned long long)address,
				(unsigned long long)address+size,
				res);
		}
	}

	free(pdata);
	pdata = NULL;
	return res;
}


int class_cmdmgr::exec_cmd_sleep(class_native_cmd*pcmd)
{
	unsigned long long val = 0;
	int res = calc_value(pcmd->get_value(), val);
	if (res < 0)
	{
		PRN_ERROR("sleep: error to calculate the value, res:%d\n", res);
		return res;
	}
	usleep((unsigned int)val*1000);
	return res;
}

int class_cmdmgr::exec_cmd_reg(class_native_cmd*pcmd)
{
	clicp_cmd* pcpcmd = NULL;
	clicp_cmd* pcpcmd_ack = NULL;
	int res = RC_OK;
	std::string value;
	class_math math(&m_vars);
	data_ptr<clicp_cmd*> ptr;
	data_ptr<clicp_cmd*> ptr_ack;

	unsigned long long nval;
	unsigned long long reg_addr;

	if (pcmd->get_addr_type() == 1)
	{
		res = calc_value(pcmd->get_address(), reg_addr);
	}
	else
	{
		reg_addr = get_hwreg_addr(pcmd->get_address());
	}

	reg_addr = addr_bits_align(reg_addr, pcmd->get_bits());

	if (!is_addr_valid(reg_addr))
	{
		PRN_ERROR("The address(0x%llx) is invalid\n", reg_addr);
		return RC_CMDMGR_ADDR_ERROR;
	}

	res = calc_equation(pcmd->get_value(), pcmd->get_bits(), nval);
	if (res < 0 && pcmd->get_id() != NCID_REG_GET)
	{
		PRN_ERROR("Error(%d) to handle REG command value '%s'\n", res, pcmd->get_value());
		return res;
	}

	switch (pcmd->get_id())
	{
		case NCID_REG_SET:

				if (pcmd->is_message())
					PRN_INFO("[reg.set] REG%d[0x%04llx]=0x%llx(%llu) ... ", pcmd->get_bits(), reg_addr, nval, nval);

				if((pcpcmd = m_connection.alloc_cmd(NCID_REG_SET, addr_to_drvid(reg_addr)))== NULL)
				{
					PRN_ERROR("Error to allocate the NET CLI command (reg.and)\n");
					return RC_CMDMGR_ALLOC_NETCMD_ERROR;
				}

				ptr=pcpcmd;
				pcpcmd->params.reg_set.address = reg_addr;
				pcpcmd->params.reg_set.value   = nval;
				pcpcmd->params.reg_set.bits    = pcmd->get_bits();
				res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
				res = (res == RC_OK) ? pcpcmd_ack->res_code : res;
				if (pcmd->is_message())
				{
					if (res == RC_OK)
						PRN_INFO("OK\n");
					else
						PRN_INFO("Error(%d)\n", res);
				}
				ptr_ack=pcpcmd_ack;
			break;

		case NCID_REG_AND:

				if (pcmd->is_message())
					PRN_INFO("[reg.and] REG%d[0x%04llx]&=0x%llx(%llu) ... ", pcmd->get_bits(), reg_addr, nval, nval);
				if((pcpcmd = m_connection.alloc_cmd(NCID_REG_AND, addr_to_drvid(reg_addr)))== NULL)
				{
					PRN_ERROR("Error to allocate the NET CLI command (reg.and)\n");
					return RC_CMDMGR_ALLOC_NETCMD_ERROR;
				}

				ptr = pcpcmd;
				pcpcmd->params.reg_and.address = reg_addr;
				pcpcmd->params.reg_and.value   = nval;
				pcpcmd->params.reg_and.bits    = pcmd->get_bits();
				res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
				res = (res == RC_OK) ? pcpcmd_ack->res_code : res;
				if (pcmd->is_message())
				{
					if (res == RC_OK)
						PRN_INFO("OK (read:0x%llx, write:0x%llx)\n", (unsigned long long)pcpcmd_ack->params.reg_and_ack.read_val, (unsigned long long)pcpcmd_ack->params.reg_and_ack.write_val);
					else
						PRN_INFO("Error(%d)\n", res);
				}
				ptr_ack=pcpcmd_ack;
			break;

		case NCID_REG_XOR:
				if (pcmd->is_message())
					PRN_INFO("[reg.xor] REG%d[0x%04llx]^=0x%llx(%llu) ... ", pcmd->get_bits(), reg_addr, nval, nval);
				if((pcpcmd = m_connection.alloc_cmd(NCID_REG_XOR, addr_to_drvid(reg_addr)))== NULL)
				{
					PRN_ERROR("Error to allocate the NET CLI command (reg.xor)\n");
					return RC_CMDMGR_ALLOC_NETCMD_ERROR;
				}

				ptr = pcpcmd;
				pcpcmd->params.reg_xor.address = reg_addr;
				pcpcmd->params.reg_xor.value   = nval;
				pcpcmd->params.reg_xor.bits    = pcmd->get_bits();
				res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
				res = (res == RC_OK) ? pcpcmd_ack->res_code : res;
				if (pcmd->is_message())
				{
					if (res == RC_OK)
						PRN_INFO("OK (read:0x%llx, write:0x%llx)\n", (unsigned long long)pcpcmd_ack->params.reg_xor_ack.read_val, (unsigned long long)pcpcmd_ack->params.reg_xor_ack.write_val);
					else
						PRN_INFO("Error(%d)\n", res);
				}
				ptr_ack=pcpcmd_ack;
			break;

		case NCID_REG_OR:

				if (pcmd->is_message())
					PRN_INFO("[reg.or] REG%d[0x%04llx]|=0x%llx(%llu) ... ", pcmd->get_bits(), (unsigned long long)reg_addr, (unsigned long long)nval, (unsigned long long)nval);
				if((pcpcmd = m_connection.alloc_cmd(NCID_REG_OR, addr_to_drvid(reg_addr)))== NULL)
				{
					PRN_ERROR("Error to allocate the NET CLI command (reg.or)\n");
					return RC_CMDMGR_ALLOC_NETCMD_ERROR;
				}

				ptr = pcpcmd;
				pcpcmd->params.reg_or.address = reg_addr;
				pcpcmd->params.reg_or.value   = nval;
				pcpcmd->params.reg_or.bits    = pcmd->get_bits();
				res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
				res = (res == RC_OK) ? pcpcmd_ack->res_code : res;
				if (pcmd->is_message())
				{
					if (res == RC_OK)
						PRN_INFO("OK (read:0x%llx, write:0x%llx)\n", (unsigned long long)pcpcmd_ack->params.reg_or_ack.read_val, (unsigned long long)pcpcmd_ack->params.reg_or_ack.write_val);
					else
						PRN_INFO("Error(%d)\n", res);
				}
				ptr_ack=pcpcmd_ack;
			break;

		case NCID_REG_GET:
				if((pcpcmd = m_connection.alloc_cmd(NCID_REG_GET, addr_to_drvid(reg_addr)))== NULL)
				{
					PRN_ERROR("Error to allocate the NET CLI command (reg.get)\n");
					return RC_CMDMGR_ALLOC_NETCMD_ERROR;
				}

				ptr = pcpcmd;
				pcpcmd->params.reg_get.address = reg_addr;
				pcpcmd->params.reg_get.bits    = pcmd->get_bits();
				res = m_connection.send_cmd(pcpcmd, pcpcmd_ack);
				res = (res == RC_OK) ? pcpcmd_ack->res_code : res;
				if (res == RC_OK)
				{
					nval = pcpcmd_ack->params.reg_get_ack.value;
					char _buf_[128];
					sprintf(_buf_, "%llu", nval);
					m_vars.set("retu", _buf_, 1, 1);
					sprintf(_buf_, "%lld", nval);
					m_vars.set("retd", _buf_, 1, 1);
					sprintf(_buf_, "0x%llx", nval);
					m_vars.set("retx", _buf_, 1, 1);
					if (pcmd->is_message())
					{
						if (pcmd->get_message()[0] != 0)
						{
							PRN_INFO("%s\n", expand_ctrl_symb(expand_vars(pcmd->get_message()).c_str()).c_str());
						}
						else
						{
							PRN_INFO("[reg.get] REG%d[0x%04llx]==0x%llx(%llu)\n", pcmd->get_bits(), (unsigned long long)reg_addr, (unsigned long long)nval, (unsigned long long)nval);
						}
					}
				}
				else
				{
					if (pcmd->is_message())
						PRN_INFO("[reg.get] REG%d:[0x%04llx]==ERROR(%d)\n", pcmd->get_bits(), reg_addr, res);
				}
				ptr_ack = pcpcmd_ack;
			break;
		default:
			return RC_CMDMGR_UNSUPPORTED_ERROR;
	}

	return res;
}

int class_cmdmgr::exec_cmd_upload(class_native_cmd*pcmd)
{
	class_file file;
	int res = file.open(pcmd->get_file_name(), "r");
	if (res < RC_OK)
	{
		PRN_ERROR("error to open file[%s]\n", pcmd->get_file_name());
		return res;
	}

	unsigned long long address;
	unsigned long long size;

	//res = calc_equation(pcmd->get_address(), 64, address);
	res = calc_value(pcmd->get_address(), address);
	size = (uint64_t)file.length();
	size = addr_bits_align(size, pcmd->get_bits());

	if (res < 0)
	{
		PRN_ERROR("error to calculate destination address: %s\n", pcmd->get_address());
		return res;
	}
	
	if (!is_addr_valid(address))
	{
		PRN_ERROR("the address(0x%llx) is invalid\n", (unsigned long long)address);
		return RC_CMDMGR_ADDR_ERROR;
	}

	if (size == 0)
	{
		PRN_ERROR("the data size is zero\n");
		return RC_CMDMGR_SIZE_ERROR;
	}

	void* pdata = malloc(size + 128);
	if (pdata == NULL)
	{
		PRN_ERROR("error to allocate buffer\n");
		return RC_CMDMGR_ALLOC_BUF_ERROR;
	}

	res = file.read(pdata, size);
	if (res < RC_OK)
	{
		PRN_ERROR("upload: error to read the file:%s\n", pcmd->get_file_name());
		return RC_CMDMGR_READ_FILE_ERROR;
	}

	res = writedata_nosync(pcmd->get_bits(), address, pdata, size);

	if (res < 0)
	{
		PRN_INFO("Uploading %s (%llu bytes) at [0x%llx ... 0x%llx] - Error(%d)\n",
			pcmd->get_file_name(),
			size,
			(unsigned long long)address,
			(unsigned long long)address+size,
			res);
	}
	else
	{
		PRN_INFO("Uploading %s (%llu bytes) at [0x%llx ... 0x%llx] - OK (written %d bytes)\n",
			pcmd->get_file_name(),
			size,
			(unsigned long long)address,
			(unsigned long long)address+size,
			res);
	}

	free(pdata);
	pdata = NULL;
	return res;
}

int class_cmdmgr::exec_cmd_download(class_native_cmd*pcmd)
{
	int res = 0;
	unsigned long long address, size;
	unsigned int bits;

	res = calc_value(pcmd->get_address(), address);
	if (!is_addr_valid(address))
	{
		PRN_ERROR("The address(0x%llx) is invalid\n", address);
		return RC_CMDMGR_ADDR_ERROR;
	}
	res = calc_value(pcmd->get_value(), size);
	bits = pcmd->get_bits();
	if (size == 0)
	{
		PRN_ERROR("The size is zero, it's not allowed\n");
		return RC_CMDMGR_SIZE_ERROR;
	}

	switch(bits)
	{
		case 16:
			size *= 2;
			break;

		case 32:
			size *= 4;
			break;

		case 64:
			size *= 8;
			break;

		default:
			break;
	}

	void* poutbuf = malloc(size + 128);
	if (poutbuf == NULL)
	{
		PRN_ERROR("Error to allocate the buffer\n");
		return RC_CMDMGR_ALLOC_BUF_ERROR;
	}
	memset(poutbuf, 0, size);
	res = readdata_nosync(bits, addr_bits_align(address, bits), size, poutbuf);
	if (res < RC_OK)
	{
		PRN_ERROR("read command error(%d)\n", res);
		free(poutbuf);
		return res;
	}

	// ---------------------------------------------------
	// to SET the size to the value returned by CLI server
	// ---------------------------------------------------
	size = res;
	// ---------------------------------------------------

	PRN_INFO("saving [0x%llx...0x%llx] (%llu bytes) into the file:{%s} - ", address, address + size, size, pcmd->get_file_name());

	class_file file(pcmd->get_file_name(), "w+");
	res = file.write((uint8_t*)poutbuf, size);
	file.close();

	if (res == RC_OK)
	{
		PRN_INFO("OK\n");
	}
	else
	{
		PRN_INFO("Error(%d)\n", res);
	}
	free(poutbuf);
	poutbuf = NULL;
	return res;
}

int class_cmdmgr::exec_cmd_sched(class_native_cmd*pcmd)
{
	unsigned long long period = 0;

	std::string txt_period = pcmd->get_address();
	if (txt_period == "")
	{
		if (m_sched_cmd == "")
		{
			PRN_INFO("nothing is scheduled\n");
		}
		else
		{
			PRN_INFO("running '%s' every %d sec(s)\n", m_sched_cmd.c_str(), m_sched_time);
		}
		return RC_OK;
	}

	calc_equation(pcmd->get_address(), 32, period);
	m_sched_time_cur = 0;
	m_sched_time = (unsigned int)period;
	m_sched_cmd  = pcmd->get_value();

	if (m_sched_cmd == "")
	{
		PRN_INFO("the scheduler is stopped\n");
	}
	else
	{
		PRN_INFO("to run '%s' every %d sec(s)\n", m_sched_cmd.c_str(), (unsigned int)period);
	}
	return RC_OK;
}

int class_cmdmgr::exec_cmd_exit(class_native_cmd*pcmd)
{
	unsigned long long reg_addr = 0;
	calc_equation(pcmd->get_value(), 64, reg_addr);
	PRN_INFO("The application is stopped with ret_code:%d\n", (int)reg_addr);
	exit((int)reg_addr);
	return RC_OK;
}

int class_cmdmgr::exec_cmd_ls(class_native_cmd*pcmd)
{
	std::string cmd = "ls ";
	cmd += pcmd->get_value();
	system(cmd.c_str());
	return RC_OK;
}

int class_cmdmgr::exec_cmd_pwd(class_native_cmd*pcmd)
{
	char dir[PATH_MAX];
	char* path = getcwd(dir, sizeof(dir));
	if (path)
	{
		PRN_INFO("%s\n", path);
	}
	else
	{
		PRN_ERROR("Error to get current working directory\n");
	}
		
	return RC_OK;
}

int class_cmdmgr::exec_cmd_math(class_native_cmd*pnative_cmd)
{
	if (m_vars.is_system(pnative_cmd->get_address()))
	{
		PRN_ERROR("[%s] is the system variable\n", pnative_cmd->get_address());
		return RC_CMDMGR_SYSVAR_USAGE_ERROR;
	}
	unsigned long long nval;
	int r = calc_equation(pnative_cmd->get_value(), 64, nval);
	if (r < 0)
	{
		PRN_ERROR("Error(%d) error to handle <math> command value %s (as the equation is incorrect)\n", r, pnative_cmd->get_value());
		return r;
	}
	
	char text_value[64];
	sprintf(text_value, "%llu", nval);
	r = m_vars.set(pnative_cmd->get_address(), text_value);
	return r;
}

int class_cmdmgr::exec_cmd_calc(class_native_cmd*pcmd)
{
	unsigned long long val = 0;
	int res = calc_equation(pcmd->get_value(), 64, val);
	if (res < 0)
	{
		PRN_ERROR("Error(%d) to calculate math expression: %s\n", res, pcmd->get_value());
	}
	else
	{
		char _buf_[128];
		sprintf(_buf_, "%llu", val);
		m_vars.set("retu", _buf_, 1, 1);
		sprintf(_buf_, "%lld", val);
		m_vars.set("retd", _buf_, 1, 1);
		sprintf(_buf_, "0x%llx", val);
		m_vars.set("retx", _buf_, 1, 1);
		PRN_INFO("%lld (0x%llx)\n", val, val);
	}
	return 0;
}

int class_cmdmgr::exec_cmd_getfile(class_native_cmd*pcmd)
{
	std::string filename = expand_vars(pcmd->get_value()).c_str();

	void* pdata = NULL;
	int len = getfile(filename.c_str(), pdata);
	data_ptr<void*> smartprt(pdata);

	if (len < 0)
	{
		PRN_ERROR("Error(%d) to read file: %s\n", len, filename.c_str());
		return len;
	}

	std::string lex;
	class_lexer lexer;
	lexer.lex_init(filename.c_str(), "", "/");

	while (!(lex = lexer.lex_next()).empty())
	{
		filename = lex;
	}

	class_file file;
	int file_rc = file.open(filename.c_str(), "w+b");

	if (file_rc < 0)
	{
		PRN_ERROR("Error(%d) to create local file(%s)\n", file_rc, filename.c_str());
		return file_rc;
	}

	file_rc = file.write(pdata, len);

	if (file_rc < RC_OK)
	{
		PRN_ERROR("Error(%d) to save data in local file\n", file_rc);
	}
	else
	{
		PRN_INFO("the file(%s) is copied\n", filename.c_str());
	}

	return RC_OK;
}

int class_cmdmgr::exec_cmd_os_sys_cmd(class_native_cmd*pcmd)
{
	const char* pcommand = pcmd->get_value();
	int res = system(pcommand);

	if (res < 0)
	{
		PRN_ERROR("error to run OS command : %s\n", pcommand);
	}
	else if (res == 127)
	{
		PRN_INFO("The shell could not be executed in the child process\n");
	}
	else
	{

	}
	return RC_OK;
}

int class_cmdmgr::exec_cmd_cd(class_native_cmd*pcmd)
{
	if (chdir(pcmd->get_value()) < 0)
	{
		PRN_ERROR("Error to change the directory to %s\n", pcmd->get_value());
	}
	else
	{
		PRN_INFO("OK\n");
	}
	return RC_OK;
}

int class_cmdmgr::exec_cmd_ext(class_ext_cmd*pcmd)
{
	std::string cmd = pcmd->get_text();
	std::string resp;
	int res = h2(cmd, resp);

	if (res < RC_OK)
	{
		PRN_ERROR("Error to run the json command with H2 interface, error(%d)\n", res);
	}
	else
	{
		class_json json;
		res = json.create(resp.c_str());
		if (res < RC_OK)
		{
			PRN_ERROR("Error to parse JSON response:\n%s\n", resp.c_str());
		}
		else
		{
			//std::string format = get_str_content(pcmd->get_json_resp_format());
			class_lexer lexer("", ";\n\r\t");
			std::string fmt;
			lexer.lex_init(pcmd->get_json_resp_format());

			//printf("main-fmt:%s\n", pcmd->get_json_resp_format());
			while (!(fmt = lexer.lex_next(0)).empty())
			{
				//printf("fmt:'%s'\n", fmt.c_str());
				PRN_INFO("%s\n", json.getobjdata(fmt.c_str()).c_str());
			}
		}
	}
	return res;
}

int class_cmdmgr::exec_cmd_lua(class_lua_cmd*pcmd)
{
	int r = m_luamgr.run_script(pcmd->get_name(), pcmd->get_params(), pcmd->get_proc_name());
	return r;
}

int class_cmdmgr::exec_cd_lua_scr_folder(class_native_cmd*pcmd)
{
	std::string folder_name = pcmd->get_value();
	set_scr_cur_path(folder_name.c_str());
	return RC_OK;
}

