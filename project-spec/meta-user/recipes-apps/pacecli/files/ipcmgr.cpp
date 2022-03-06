/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class implements IPC server interface
 *  and provides access to CLI API over this IPC mechanism
 *    set/get/or/xor registers
 *    save/load data to/from file
 *    print message to CLI console
 *    etc (please see the complete IPC API interface)
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


#include "ipcmgr.h"
#include "cmdmgr.h"
#include "paceipc/ipclib.h"
#include "helpmgr.h"

static class_cmdmgr* g_cmdmgr = NULL;

static void cli_ctrl_inf_print(void* hlib)
{
	const char* pmsg = (const char*)ipc_param(hlib, 0);
	if (pmsg != NULL)
	{
		PRN_INFO("%s", pmsg);
	}

	int rc = RC_OK;
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

static void cli_ctrl_inf_command(void* hlib)
{
	int rc;
	if (g_cmdmgr == NULL)
	{
		rc = RC_IPC_MGR_GENERAL_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
		return;
	}

	const char* pcmd = (const char*)ipc_param(hlib, 0);
	if (pcmd == NULL)
	{
		rc = RC_IPC_MGR_PARAM_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
	}

	rc = g_cmdmgr->run_commands(pcmd);
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

static void cli_ctrl_inf_setreg(void* hlib)
{
	int rc;
	unsigned int		bits = *(unsigned int*)ipc_param(hlib, 0);
	unsigned long long	addr = *(unsigned long long*)ipc_param(hlib, 1);
	unsigned long long	value= *(unsigned long long*)ipc_param(hlib, 2);

	if (g_cmdmgr == NULL)
	{
		rc = RC_IPC_MGR_GENERAL_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
		return;
	}

	std::string s_bits = class_helpmgr::to_string(bits, 1);
	std::string s_addr = class_helpmgr::to_string(addr, 1);
	std::string s_value = class_helpmgr::to_string(value, 1);
	rc = g_cmdmgr->set_reg(NCID_REG_SET, s_bits.c_str(), s_addr.c_str(), s_value.c_str(), NULL);
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

static void cli_ctrl_inf_getreg(void* hlib)
{
	int rc;
	unsigned int		bits = *(unsigned int*)ipc_param(hlib, 0);
	unsigned long long	addr = *(unsigned long long*)ipc_param(hlib, 1);
	unsigned long long*	pvalue= (unsigned long long*)ipc_param(hlib, 2);
	uint64_t value;

	if (g_cmdmgr == NULL)
	{
		rc = RC_IPC_MGR_GENERAL_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
		return;
	}

	std::string s_bits = class_helpmgr::to_string(bits);
	std::string s_addr = class_helpmgr::to_string(addr);

	rc = g_cmdmgr->get_reg(s_bits.c_str(), s_addr.c_str(), value, NULL);
	*pvalue = (unsigned long long)value;
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

static void cli_ctrl_inf_savetofile(void* hlib)
{
	int rc;
	unsigned long long	addr = *(unsigned long long*)ipc_param(hlib, 0);
	unsigned int		size= *(unsigned int*)ipc_param(hlib, 1);
	const char*		name= (const char*)ipc_param(hlib, 2);

	if (g_cmdmgr == NULL)
	{
		rc = RC_IPC_MGR_GENERAL_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
		return;
	}

	std::string s_addr = class_helpmgr::to_string(addr, 1);
	std::string s_size = class_helpmgr::to_string(size, 0);
	rc = g_cmdmgr->download("8", s_addr.c_str(), s_size.c_str(), name);
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

static void cli_ctrl_inf_loadfromfile(void* hlib)
{
	int rc;
	const char*		name= (const char*)ipc_param(hlib, 0);
	unsigned long long	addr = *(unsigned long long*)ipc_param(hlib, 1);

	if (g_cmdmgr == NULL)
	{
		rc = RC_IPC_MGR_GENERAL_ERROR;
		ipc_return(hlib, IPC_APT_U32, 0, &rc);
		return;
	}

	std::string s_addr = class_helpmgr::to_string(addr, 1);
	rc = g_cmdmgr->upload("8", name, s_addr.c_str());
	ipc_return(hlib, IPC_APT_U32, 0, &rc);
}

/**********************************************************************************/
/*             This is IPC 'CONTROL' interface definition                         */
/*             The interface name is CLI CONTROL INTERFACE                        */
/*             The set of API is:                                                 */
/**********************************************************************************/
IPC_API_TABLE_BEGIN(ipc_cli_ctrl_inf)

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_PRINT, cli_ctrl_inf_print)
	IPC_API_PARAM(IPC_APT_STR, 0, 0)
    IPC_API_END()

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_COMMAND, cli_ctrl_inf_command)
	IPC_API_PARAM(IPC_APT_STR, 0, 0)
    IPC_API_END()

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_SETREG, cli_ctrl_inf_setreg)
	IPC_API_PARAM(IPC_APT_U32, 0, 0)	// How to access the register: 8, 16, 32 or 64 bits register
	IPC_API_PARAM(IPC_APT_U64, 0, 0)	// The register address
	IPC_API_PARAM(IPC_APT_U64, 0, 0)	// The data
    IPC_API_END()

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_GETREG, cli_ctrl_inf_getreg)
	IPC_API_PARAM(IPC_APT_U32,   0, 0)					// How to access the register: 8, 16, 32 or 64 bits register
	IPC_API_PARAM(IPC_APT_U64,   0, 0)					// The register address
	IPC_API_PARAM(IPC_APT_PVOID, 1, sizeof(uint64_t))	// The register value [this is an output parameter]
    IPC_API_END()

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_SAVE2FILE, cli_ctrl_inf_savetofile)
	IPC_API_PARAM(IPC_APT_U64, 0, 0)	// The address
	IPC_API_PARAM(IPC_APT_U32, 0, 0)	// The size in bytes
	IPC_API_PARAM(IPC_APT_STR, 0, 0)	// The filename
    IPC_API_END()

    IPC_API_BEGIN(IPC_CLI_CTRL_INF_API_ID_LOADFROMFILE, cli_ctrl_inf_loadfromfile)
	IPC_API_PARAM(IPC_APT_STR, 0, 0)	// The filename
	IPC_API_PARAM(IPC_APT_U64, 0, 0)	// The address
    IPC_API_END()

IPC_API_TABLE_END();

class_ipc_mgr::class_ipc_mgr(void)
{
	m_pcmdmgr = NULL;
	m_h_ipc_serv = NULL;
}

class_ipc_mgr::~class_ipc_mgr(void)
{
	if (m_h_ipc_serv != NULL)
		::ipc_close(m_h_ipc_serv);

	m_h_ipc_serv = NULL;
}

int class_ipc_mgr::init(CLF_PARAMS_TYPE* p, class_cmdmgr* p_cmdmgr)
{
	void* ipc_cfg_serv = NULL;
	g_cmdmgr  = p_cmdmgr;
	m_pcmdmgr = p_cmdmgr;
	PRN_INFO("[port:%d]", p->ipc_port);

	ipc_cfg_serv = ::ipc_create_cfg(ipc_cli_ctrl_inf);
	::ipc_set_cfg(ipc_cfg_serv, IPC_CFG_IP, IPC_CFG_VAL("127.0.0.1"));
	::ipc_set_cfg(ipc_cfg_serv, IPC_CFG_PORT, IPC_CFG_VAL(IPC_U64(p->ipc_port)));
	int rc = ::ipc_create(ipc_cfg_serv, &m_h_ipc_serv);
	if (rc < 0)
	{
		PRN_ERROR("[IPC server open error, rc=%d]\n", rc);
		::ipc_destroy_cfg(ipc_cfg_serv);
		return rc;
	}

	ipc_destroy_cfg(ipc_cfg_serv);
	ipc_cfg_serv = NULL;
	return RC_OK;
}
