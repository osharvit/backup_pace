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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "cmdmgr.h"
#include "file.h"

class_cmdmgr::class_cmdmgr(class_spimgr* spi, int cmd_id)
{
	m_spi = spi;
	m_cmdid = cmd_id;

	m_pInStream = NULL;
	m_pOutStream= NULL;
	m_InStreamSize = 0;
	m_OutStreamSize = 0;
}

class_cmdmgr::~class_cmdmgr(void)
{
	if (m_pInStream != NULL)
		free(m_pInStream);
	m_pInStream = NULL;

	if (m_pOutStream != NULL)
		free(m_pOutStream );
	m_pOutStream = NULL;

	m_InStreamSize = 0;
	m_OutStreamSize = 0;
}

int class_cmdmgr::proc(std::vector<std::string>& params)
{
	int res = 0;

	if ((res = create_command(params)) < RC_OK)
		return res;

	if (m_pInStream != NULL && m_pOutStream != NULL)
	{
		cmd_READ_REPLY_ack error = {0};
		if((res = m_spi->send_data(m_pInStream, m_InStreamSize, m_pOutStream, m_OutStreamSize, &error)) < RC_OK)
		{
			if (res == RC_SPIMGR_API_ERROR)
				PRN_ERROR("CMD-MGR: status API error (0x%x) is received\n", error.err.value);
			if (res == RC_SPIMGR_HW_ERROR)
				PRN_ERROR("CMD-MGR: status HW error is detected\n");
			if (res == RC_SPIMGR_FW_ERROR)
				PRN_ERROR("CMD-MGR: status FW error is detected\n");
			return res;
		}
	}

	if (m_pOutStream != NULL)
	{
		// --------------------------------------------
		// To show the responce (ACK) for the command
		// --------------------------------------------
		switch(m_cmdid)
		{
			case SI5518_CMDID_READ_REPLY:
				show_READ_REPLY_ack();
				break;

			case SI5518_CMDID_SIO_TEST:
				show_SIO_TEST_ack();
				break;

			case SI5518_CMDID_SIO_INFO:
				show_SIO_INFO_ack();
				break;

			case SI5518_CMDID_HOST_LOAD:
				show_HOST_LOAD_ack();
				break;

			case SI5518_CMDID_BOOT:
				show_BOOT_ack();
				break;

			case SI5518_CMDID_DEVICE_INFO:
				show_DEVICE_INFO_ack();
				break;

			case SI5518_CMDID_NVM_STATUS:
				show_NVM_STATUS_ack();
				break;

			case SI5518_CMDID_RESTART:
				show_RESTART_ack();
				break;

			case SI5518_CMDID_APP_INFO:
				show_APP_INFO_ack();
				break;

			case SI5518_CMDID_PLL_ACTIVE_REFCLOCK:
				show_PLL_ACTIVE_REFCLOCK_ack();
				break;

			case SI5518_CMDID_INPUT_STATUS:
				show_INPUT_STATUS_ack();
				break;

			case SI5518_CMDID_PLL_STATUS:
				show_PLL_STATUS_ack();
				break;

			case SI5518_CMDID_INTERRUPT_STATUS:
				show_INTERRUPT_STATUS_ack();
				break;

			case SI5518_CMDID_METADATA:
				show_METADATA_ack();
				break;

			case SI5518_CMDID_REFERENCE_STATUS:
				show_REFERENCE_STATUS_ack();
				break;

			case SI5518_CMDID_PHASE_READOUT:
				show_PHASE_READOUT_ack();
				break;

			case SI5518_CMDID_INPUT_PERIOD_READOUT:
				show_INPUT_PERIOD_READOUT_ack();
				break;

			case SI5518_CMDID_TEMPERATURE_READOUT:
				show_TEMPERATURE_READOUT_ack();
				break;
		}
	}
	return RC_OK;
}

int class_cmdmgr::is_error(unsigned char status)
{
	return (status&(1<<6))|(status&(1<<5))|(status&(1<<4));
}

int class_cmdmgr::create_command(std::vector<std::string>& params)
{
	int res;

	if (m_pInStream != NULL)
		free(m_pInStream);
	m_pInStream = NULL;

	if (m_pOutStream != NULL)
		free(m_pOutStream );
	m_pOutStream = NULL;
	
	switch(m_cmdid)
	{
		case SI5518_CMDID_READ_REPLY:
			res = create_READ_REPLY();
			break;

		case SI5518_CMDID_SIO_TEST:
			res = create_SIO_TEST();
			break;

		case SI5518_CMDID_SIO_INFO:
			res = create_SIO_INFO();
			break;

		case SI5518_CMDID_HOST_LOAD:
			res = create_HOST_LOAD(params);
			break;

		case SI5518_CMDID_BOOT:
			res = create_BOOT();
			break;

		case SI5518_CMDID_DEVICE_INFO:
			res = create_DEVICE_INFO();
			break;

		case SI5518_CMDID_NVM_STATUS:
			res = create_NVM_STATUS();
			break;

		case SI5518_CMDID_RESTART:
			res = create_RESTART();
			break;

		case SI5518_CMDID_APP_INFO:
			res = create_APP_INFO();
			break;

		case SI5518_CMDID_PLL_ACTIVE_REFCLOCK:
			res = create_PLL_ACTIVE_REFCLOCK(params);
			break;

		case SI5518_CMDID_INPUT_STATUS:
			res = create_INPUT_STATUS(params);
			break;

		case SI5518_CMDID_PLL_STATUS:
			res = create_PLL_STATUS(params);
			break;

		case SI5518_CMDID_INTERRUPT_STATUS:
			res = create_INTERRUPT_STATUS(params);
			break;

		case SI5518_CMDID_METADATA:
			res = create_METADATA(params);
			break;

		case SI5518_CMDID_REFERENCE_STATUS:
			res = create_REFERENCE_STATUS(params);
			break;

		case SI5518_CMDID_PHASE_READOUT:
			res = create_PHASE_READOUT(params);
			break;

		case SI5518_CMDID_INPUT_PERIOD_READOUT:
			res = create_INPUT_PERIOD_READOUT(params);
			break;

		case SI5518_CMDID_TEMPERATURE_READOUT:
			res = create_TEMPERATURE_READOUT(params);
			break;
	}
	return res;
}

int class_cmdmgr::alloc_buffers(void)
{
	m_pInStream = (unsigned char*)malloc(m_InStreamSize);
	if (m_pInStream == NULL)
		return RC_CMDMGR_ALLOC_ERROR;

	m_pOutStream = (unsigned char*)malloc(m_OutStreamSize);
	if (m_pOutStream == NULL)
		return RC_CMDMGR_ALLOC_ERROR;
	return RC_OK;
}

int class_cmdmgr::create_READ_REPLY(void)
{
	m_InStreamSize = sizeof(cmd_READ_REPLY);
	m_OutStreamSize = sizeof(cmd_READ_REPLY_ack);

	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_READ_REPLY* cmd = (cmd_READ_REPLY*)m_pInStream;
	cmd->id = SI5518_CMDID_READ_REPLY;

	return RC_OK;
}

int class_cmdmgr::show_READ_REPLY_ack(void)
{
	cmd_READ_REPLY_ack* ack = (cmd_READ_REPLY_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("READ_REPLY_ack\n");
	PRN_INFO("  status: 0x%x\n", ack->status);
	PRN_INFO("  error code: 0x%x (0x%02x : 0x%02x)\n", ack->err.value, ack->err.bytes.val[1], ack->err.bytes.val[0]);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_SIO_TEST(void)
{
	m_InStreamSize = sizeof(cmd_SIO_TEST);
	m_OutStreamSize = sizeof(cmd_SIO_TEST_ack);

	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_SIO_TEST* cmd = (cmd_SIO_TEST*)m_pInStream;
	cmd->id = SI5518_CMDID_SIO_TEST;
	for (int i = 0; i < cmd_SIO_TEST_data_size; i++)
	{
		cmd->data[i] = 0x80 + i;
	}
	return RC_OK;
}

int class_cmdmgr::show_SIO_TEST_ack(void)
{
	cmd_SIO_TEST_ack* ack = (cmd_SIO_TEST_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("SIO_TEST_ack\n");
	PRN_INFO("  status: 0x%x\n", ack->status);
	PRN_INFO("  cmd_id: 0x%x\n", ack->cmd_id);
	PRN_INFO("    data: ");
	for (int i = 0; i < cmd_SIO_TEST_data_size; i++)
	{
		PRN_INFO("%02x ", ack->data[i]);
	}
	PRN_INFO("\n---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_SIO_INFO(void)
{
	m_InStreamSize = sizeof(cmd_SIO_INFO);
	m_OutStreamSize = sizeof(cmd_SIO_INFO_ack);

	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_SIO_INFO* cmd = (cmd_SIO_INFO*)m_pInStream;
	cmd->id = SI5518_CMDID_SIO_INFO;
	return RC_OK;
}

int class_cmdmgr::show_SIO_INFO_ack(void)
{
	cmd_SIO_INFO_ack* ack = (cmd_SIO_INFO_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("SIO_INFO_ack\n");
	PRN_INFO("         status: 0x%x\n", ack->status);
	PRN_INFO("   cmd_buf_size: %d\n", ack->cmd_buf_size);
	PRN_INFO(" reply_buf_size: %d\n", ack->repl_buf_size);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_HOST_LOAD(std::vector<std::string>& params)
{
	const char* filename = params[0].c_str();
	class_file file;
	int res = file.open(filename);

	if (res < RC_OK)
	{
		PRN_ERROR("CMDMGR: error to open a file (%s)\n", filename);
		return res;
	}

	void* data = NULL;
	res = file.readfile(data);
	if (res < RC_OK)
	{
		PRN_ERROR("CMDMGR: error to read a file (%s)\n", filename);
		return res;
	}

	// Let's check the size of CMD buffer by using SIO_INFO command
	if (create_SIO_INFO() < 0)
	{
		PRN_ERROR("CMDMGR: HOST_LOAD command, error to handle SIO_INFO command\n");
		free(data);
		return RC_CMDMGR_ALLOC_ERROR;
	}

	cmd_READ_REPLY_ack error = {0};
	if((res = m_spi->send_data(m_pInStream, m_InStreamSize, m_pOutStream, m_OutStreamSize, &error)) < RC_OK)
	{
		if (res == RC_SPIMGR_API_ERROR)
			PRN_ERROR("CMD-MGR: SIO_INFO: status API error (0x%x) is received\n", error.err.value);
		if (res == RC_SPIMGR_HW_ERROR)
			PRN_ERROR("CMD-MGR: SIO_INFO: status HW error is detected\n");
		if (res == RC_SPIMGR_FW_ERROR)
			PRN_ERROR("CMD-MGR: SIO_INFO: status FW error is detected\n");

		free(data);
		return res;
	}

	unsigned int cmd_buf_size = ((cmd_SIO_INFO_ack*)m_pOutStream)->cmd_buf_size;
	if (cmd_buf_size == 0)
	{
		free(data);
		return RC_CMDMGR_GET_INFO_ERROR;
	}

	free(m_pInStream);
	m_pInStream = NULL;

	free(m_pOutStream);
	m_pOutStream = NULL;

	// Let's create the chain of the commands
	// to load the file data
	unsigned int file_len = file.length();
	int num = file_len/(cmd_buf_size-1)+((file_len%(cmd_buf_size-1))?1:0);

	int err = 0;
	int offs = 0;

	std::vector<host_load_cmd> list;
	for (int i = 0; i < num; i++)
	{
		cmd_HOST_LOAD* pcmd = (cmd_HOST_LOAD*)malloc(1+cmd_buf_size);
		if (pcmd == NULL)
		{
			err = 1;
			break;
		}
		pcmd->id = SI5518_CMDID_HOST_LOAD;
		memcpy(pcmd->data, (unsigned char*)data+offs, MIN(cmd_buf_size-1, file_len));

		host_load_cmd cmd;
		cmd.cmd = pcmd;
		cmd.total_size = MIN(cmd_buf_size-1, file_len);
		list.push_back(cmd);

		offs += cmd.total_size /*cmd_buf_size-1*/;
		file_len -= MIN(cmd_buf_size-1, file_len);
	}

	if (!err)
	{
		for (int i = 0; i < num; i++)
		{
			res = m_spi->send_cmd(list[i].cmd, list[i].total_size);
			if (res < 0)
			{
				PRN_ERROR("CMDMGR: error to send HOST_LOAD chain command, rc:%d\n", res);
				break;
			}
			usleep(100000);

		}
		m_OutStreamSize = sizeof(cmd_HOST_ack);
		m_pOutStream = (unsigned char*)malloc(m_OutStreamSize);
		if (m_pOutStream != NULL)
		{
			res = m_spi->recv_resp(m_pOutStream, m_OutStreamSize);
			if (res < 0)
			{
				PRN_ERROR("CMDMGR: error to request HOST_LOAD ACK message\n");
			}
		}
		else
		{
			PRN_ERROR("CMDMGR: error to allocate memory for HOST_LOAD ACK message\n");
		}
	}

	// Let's free the allocated commands
	// ---------------------------------
	for (int i = 0; i < num; i++)
	{
		free(list[i].cmd);
		list[i].cmd = NULL;
	}

	list.clear();
	free(data);
	return res;
}

int class_cmdmgr::show_HOST_LOAD_ack(void)
{

	return RC_OK;
}

int class_cmdmgr::create_BOOT(void)
{
	m_InStreamSize = sizeof(cmd_BOOT);
	m_OutStreamSize = sizeof(cmd_BOOT_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_BOOT* cmd = (cmd_BOOT*)m_pInStream;
	cmd->id = SI5518_CMDID_BOOT;
	return RC_OK;
}

int class_cmdmgr::show_BOOT_ack(void)
{
	cmd_BOOT_ack* ack = (cmd_BOOT_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("BOOT_ack\n");
	PRN_INFO("  status: 0x%x  (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_DEVICE_INFO(void)
{
	m_InStreamSize = sizeof(cmd_DEVICE_INFO);
	m_OutStreamSize = sizeof(cmd_DEVICE_INFO_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_DEVICE_INFO* cmd = (cmd_DEVICE_INFO*)m_pInStream;
	cmd->id = SI5518_CMDID_DEVICE_INFO;
	return RC_OK;
}

int class_cmdmgr::show_DEVICE_INFO_ack(void)
{
	cmd_DEVICE_INFO_ack* ack = (cmd_DEVICE_INFO_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("DEVICE_INFO_ack\n");
	PRN_INFO("       status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("     part_num: 0x%x\n", ack->part_num);
	PRN_INFO("    dev_grade: 0x%x (%c)\n", ack->dev_grade, ack->dev_grade);
	PRN_INFO(" dev_revision: 0x%x (%c)\n", ack->dev_revision, ack->dev_revision);
	PRN_INFO("       opn_id: 0x%x\n", ack->opn_id);
	PRN_INFO("      opn_rev: 0x%x (%c)\n", ack->opn_rev, ack->opn_rev);
	PRN_INFO("   temp_grade: 0x%x (%c)\n", ack->temp_grade, ack->temp_grade);
	PRN_INFO("      package: 0x%x (%c)\n", ack->package, ack->package);
	PRN_INFO("      rom_rev: 0x%x (%c)\n", ack->rom_rev, ack->rom_rev);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_NVM_STATUS(void)
{
	m_InStreamSize = sizeof(cmd_NVM_STATUS);
	m_OutStreamSize = sizeof(cmd_NVM_STATUS_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_NVM_STATUS* cmd = (cmd_NVM_STATUS*)m_pInStream;
	cmd->id = SI5518_CMDID_NVM_STATUS;
	return RC_OK;
}

int class_cmdmgr::show_NVM_STATUS_ack(void)
{
	cmd_NVM_STATUS_ack* ack = (cmd_NVM_STATUS_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("cmd_NVM_STATUS_ack\n");
	PRN_INFO("   status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO(" err2_cnt: 0x%x (%d)\n", ack->err2_cnt, ack->err2_cnt);
	PRN_INFO(" err1_cnt: 0x%x (%d)\n", ack->err1_cnt, ack->err1_cnt);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_RESTART(void)
{
	m_InStreamSize = sizeof(cmd_RESTART);
	m_OutStreamSize = sizeof(cmd_RESTART_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_RESTART* cmd = (cmd_RESTART*)m_pInStream;
	cmd->id = SI5518_CMDID_RESTART;
	return RC_OK;
}

int class_cmdmgr::show_RESTART_ack(void)
{
	cmd_RESTART_ack* ack = (cmd_RESTART_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("cmd_RESTART_ack\n");
	PRN_INFO("   status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_APP_INFO(void)
{
	m_InStreamSize = sizeof(cmd_APP_INFO);
	m_OutStreamSize = sizeof(cmd_APP_INFO_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_APP_INFO* cmd = (cmd_APP_INFO*)m_pInStream;
	cmd->id = SI5518_CMDID_APP_INFO;
	return RC_OK;
}

int class_cmdmgr::show_APP_INFO_ack(void)
{
	cmd_APP_INFO_ack* ack = (cmd_APP_INFO_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("cmd_APP_INFO_ack\n");
	PRN_INFO("          status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("       app_major: 0x%x (%d)\n", ack->app_major, ack->app_major);
	PRN_INFO("       app_minor: 0x%x (%d)\n", ack->app_minor, ack->app_minor);
	PRN_INFO("      app_branch: 0x%x (%d)\n", ack->app_branch, ack->app_branch);
	PRN_INFO("       app_build: 0x%x (%d)\n\n", ack->app_build, ack->app_build);

	PRN_INFO("   planner_major: 0x%x (%d)\n", ack->planner_major, ack->planner_major);
	PRN_INFO("   planner_minor: 0x%x (%d)\n", ack->planner_minor, ack->planner_minor);
	PRN_INFO("  planner_branch: 0x%x (%d)\n", ack->planner_branch, ack->planner_branch);
	PRN_INFO("   planner_build: 0x%x (%d)\n\n", ack->planner_build, ack->planner_build);
	PRN_INFO("       design_id: ");
	for (int i = 0; i<8; i++)
	{
		PRN_INFO("%02x ", ack->design_id[i]);
	}
	PRN_INFO("\n");

	PRN_INFO(" cbpro_rev_major: 0x%x (%d)\n", ack->cbpro_rev_major, ack->cbpro_rev_major);
	PRN_INFO(" cbpro_rev_minor: 0x%x (%d)\n", ack->cbpro_rev_minor, ack->cbpro_rev_minor);
	PRN_INFO("   cbpro_rev_rev: 0x%x (%d)\n", ack->cbpro_rev_revision, ack->cbpro_rev_revision);
	PRN_INFO("  cbpro_rev_spec: 0x%x (%d)\n", ack->cbpro_rev_special, ack->cbpro_rev_special);
	
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_PLL_ACTIVE_REFCLOCK(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_PLL_ACTIVE_REFCLOCK);
	m_OutStreamSize = sizeof(cmd_PLL_ACTIVE_REFCLOCK_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_PLL_ACTIVE_REFCLOCK* cmd = (cmd_PLL_ACTIVE_REFCLOCK*)m_pInStream;
	cmd->id = SI5518_CMDID_PLL_ACTIVE_REFCLOCK;
	cmd->pll = (unsigned char)strtoul(params[0].c_str(), (char**)0, 0);

	PRN_INFO("PLL_ACTIVE_REFCLOCK: pll=%d\n", cmd->pll);
	return RC_OK;
}

int class_cmdmgr::show_PLL_ACTIVE_REFCLOCK_ack(void)
{
	cmd_PLL_ACTIVE_REFCLOCK* cmd = (cmd_PLL_ACTIVE_REFCLOCK*)m_pInStream;
	cmd_PLL_ACTIVE_REFCLOCK_ack* ack = (cmd_PLL_ACTIVE_REFCLOCK_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("cmd_PLL_ACTIVE_REFCLOCK_ack\n");
	PRN_INFO("          PLL-ID: 0x%x (%d)\n", cmd->pll, cmd->pll);
	PRN_INFO("          status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO(" ACTIVE_REFCLOCK: 0x%x (%d)\n", ack->active_pll_refclk, ack->active_pll_refclk);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_INPUT_STATUS(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_INPUT_STATUS);
	m_OutStreamSize = sizeof(cmd_INPUT_STATUS_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_INPUT_STATUS* cmd = (cmd_INPUT_STATUS*)m_pInStream;
	cmd->id = SI5518_CMDID_INPUT_STATUS;
	cmd->input = (unsigned char)strtoul(params[0].c_str(), (char**)0, 0);

	PRN_INFO("INPUT STATUS: INPUT_SELECT=%d\n", cmd->input);
	return RC_OK;
}

int class_cmdmgr::show_INPUT_STATUS_ack(void)
{
	cmd_INPUT_STATUS* cmd = (cmd_INPUT_STATUS*)m_pInStream;
	cmd_INPUT_STATUS_ack* ack = (cmd_INPUT_STATUS_ack*)m_pOutStream;
	PRN_INFO("---------------\n");
	PRN_INFO("cmd_INPUT_STATUS_ack\n");
	PRN_INFO("       INPUT_SELECT: 0x%x (%d)\n", cmd->input, cmd->input);
	PRN_INFO("             status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO(" inp_clk_validation: %d\n", ack->inp_clk_validation&3);
	PRN_INFO("     loss_of_signal: %d\n", ack->loss_of_signal&1);
	PRN_INFO("        out_of_freq: %d\n", ack->out_of_freq&1);
	PRN_INFO("      phase_monitor\n");
	PRN_INFO("         * PHASE_MONITOR_SIGNAL_EARLY: %d\n", (ack->phase_monitor >> 0)&1);
	PRN_INFO("         * PHASE_MONITOR_SIGNAL_LATE : %d\n", (ack->phase_monitor >> 1)&1);
	PRN_INFO("         * PHASE_MONITOR_PHASE_ERROR : %d\n", (ack->phase_monitor >> 2)&1);
	PRN_INFO("---------------\n");
	return RC_OK;
}

int class_cmdmgr::create_PLL_STATUS(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_PLL_STATUS);
	m_OutStreamSize = sizeof(cmd_PLL_STATUS_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_PLL_STATUS* cmd = (cmd_PLL_STATUS*)m_pInStream;
	cmd->id = SI5518_CMDID_PLL_STATUS;
	cmd->pll = (unsigned char)strtoul(params[0].c_str(), (char**)0, 0);
	PRN_INFO("PLL STATUS: PLL=%d\n", cmd->pll);
	return RC_OK;
}

int class_cmdmgr::show_PLL_STATUS_ack(void)
{
	cmd_PLL_STATUS* cmd = (cmd_PLL_STATUS*)m_pInStream;
	cmd_PLL_STATUS_ack* ack = (cmd_PLL_STATUS_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_PLL_STATUS_ack\n");
	PRN_INFO("                                PLL: 0x%x (%d)\n", cmd->pll, cmd->pll);
	PRN_INFO("                             status: 0x%x (%s)\n\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");

	PRN_INFO("   PLL_LOSS_OF_LOCK_MISC:\n");
	PRN_INFO("             * PLL_OUT_OF_FREQUENCY: %d\n", (ack->PLL_LOSS_OF_LOCK_MISC>>1)&1);
	PRN_INFO("                 * PLL_OUT_OF_PHASE: %d\n", (ack->PLL_LOSS_OF_LOCK_MISC>>2)&1);
	PRN_INFO("                 * PLL_LOSS_OF_LOCK: %d\n", (ack->PLL_LOSS_OF_LOCK_MISC>>4)&1);
	PRN_INFO("                 * PLL_INITIAL_LOCK: %d\n\n", (ack->PLL_LOSS_OF_LOCK_MISC>>5)&1);

	PRN_INFO("                          PLL_STATUS: 0x%x (%d)\n\n", ack->PLL_STATUS, ack->PLL_STATUS);

	PRN_INFO("   PLL_SLIP_COUNT:\n");
	PRN_INFO("                * PLL_SLIP_COUNT_POS: %d\n", (ack->PLL_SLIP_COUNT>>0)&7);
	PRN_INFO("                * PLL_SLIP_COUNT_NEG: %d\n\n", (ack->PLL_SLIP_COUNT>>4)&0xf);

	PRN_INFO("                  PLL_SLIP_COUNT_NET: 0x%x (%d)\n", ack->PLL_SLIP_COUNT_NET&0x1F, ack->PLL_SLIP_COUNT_NET&0x1F);
	PRN_INFO("                  PLL_HOLDOVER_VALID: %d\n", ack->PLL_HOLDOVER_VALID&1);
	PRN_INFO("                        PLL_HOLDOVER: %d\n", ack->PLL_HOLDOVER&1);
	PRN_INFO("             PLL_SHORT_TERM_HOLDOVER: %d\n", ack->PLL_SHORT_TERM_HOLDOVER&1);
	PRN_INFO("                  PLL_PHASE_BLEEDOUT: %d\n\n", ack->PLL_PHASE_BLEEDOUT&1);

	PRN_INFO("  PLL_LOOP_FILTER_STATUS:\n");
	PRN_INFO("      * LOOP_FILTER_INITIAL_LOCK    : %d\n", (ack->PLL_LOOP_FILTER_STATUS>>4)&1);
	PRN_INFO("      * LOOP_FILTER_FASTLOCK        : %d\n", (ack->PLL_LOOP_FILTER_STATUS>>5)&1);
	PRN_INFO("      * LOOP_FILTER_RAMP_IN_PROGRESS: %d\n", (ack->PLL_LOOP_FILTER_STATUS>>6)&1);
	PRN_INFO("--------------------------------------------\n");
	
	return RC_OK;
}

int class_cmdmgr::create_INTERRUPT_STATUS(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_INTERRUPT_STATUS);
	m_OutStreamSize = sizeof(cmd_INTERRUPT_STATUS_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_INTERRUPT_STATUS* cmd = (cmd_INTERRUPT_STATUS*)m_pInStream;
	cmd->id = SI5518_CMDID_INTERRUPT_STATUS;
	return RC_OK;
}

int class_cmdmgr::show_INTERRUPT_STATUS_ack(void)
{
	cmd_INTERRUPT_STATUS_ack* ack = (cmd_INTERRUPT_STATUS_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_INTERRUPT_STATUS_ack\n");
	PRN_INFO("                  status: 0x%x (%s)\n\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("  INPUT_CLOCK_INVALID\n");
	PRN_INFO("          *  IN0_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>0)&1);
	PRN_INFO("          *  IN1_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>2)&1);
	PRN_INFO("          *  IN2_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>4)&1);
	PRN_INFO("          * IN2B_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>5)&1);
	PRN_INFO("          *  IN3_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>6)&1);
	PRN_INFO("          * IN3B_INVALID: %d\n", (ack->INPUT_CLOCK_INVALID>>7)&1);
	PRN_INFO("\n");

	PRN_INFO("  INPUT_CLOCK_VALID\n");
	PRN_INFO("          * IN0_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>0)&1);
	PRN_INFO("          * IN1_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>2)&1);
	PRN_INFO("          * IN2_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>4)&1);
	PRN_INFO("          *IN2B_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>5)&1);
	PRN_INFO("          * IN3_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>6)&1);
	PRN_INFO("          *IN3B_VALID: %d\n", (ack->INPUT_CLOCK_VALID>>7)&1);
	PRN_INFO("\n");

	PRN_INFO("  PLLR\n");
	PRN_INFO("   *   PLLR_LOSS_OF_LOCK: %d\n", (ack->PLLR>>0)&1);
	PRN_INFO("   *         PLLR_LOCKED: %d\n", (ack->PLLR>>1)&1);
	PRN_INFO("   *     PLLR_CYCLE_SLIP: %d\n", (ack->PLLR>>2)&1);
	PRN_INFO("   *       PLLR_HOLDOVER: %d\n", (ack->PLLR>>3)&1);
	PRN_INFO("   * PLLR_HITLESS_SWITCH: %d\n", (ack->PLLR>>4)&1);
	PRN_INFO("\n");

	PRN_INFO("  PLLA\n");
	PRN_INFO("   *   PLLA_LOSS_OF_LOCK: %d\n", (ack->PLLA>>0)&1);
	PRN_INFO("   *         PLLA_LOCKED: %d\n", (ack->PLLA>>1)&1);
	PRN_INFO("   *     PLLA_CYCLE_SLIP: %d\n", (ack->PLLA>>2)&1);
	PRN_INFO("   *       PLLA_HOLDOVER: %d\n", (ack->PLLA>>3)&1);
	PRN_INFO("   * PLLA_HITLESS_SWITCH: %d\n", (ack->PLLA>>4)&1);
	PRN_INFO("\n");

	PRN_INFO("  PLLB\n");
	PRN_INFO("   *   PLLB_LOSS_OF_LOCK: %d\n", (ack->PLLB>>0)&1);
	PRN_INFO("   *         PLLB_LOCKED: %d\n", (ack->PLLB>>1)&1);
	PRN_INFO("   *     PLLB_CYCLE_SLIP: %d\n", (ack->PLLB>>2)&1);
	PRN_INFO("   *       PLLB_HOLDOVER: %d\n", (ack->PLLB>>3)&1);
	PRN_INFO("   * PLLB_HITLESS_SWITCH: %d\n", (ack->PLLB>>4)&1);
	PRN_INFO("\n");

	PRN_INFO("  GENERAL\n");
	PRN_INFO("   *          SYSTEM_CALIBRATION: %d\n", (ack->GENERAL>>0)&1);
	PRN_INFO("   *      INNERLOOP_LOSS_OF_LOCK: %d\n", (ack->GENERAL>>1)&1);
	PRN_INFO("   * INNERLOOP_LOSS_OF_REFERENCE: %d\n", (ack->GENERAL>>2)&1);
	PRN_INFO("   *                SYSTEM_FAULT: %d\n", (ack->GENERAL>>3)&1);
	PRN_INFO("   *              WATCHDOG_TIMER: %d\n", (ack->GENERAL>>4)&1);
	PRN_INFO("   *                   I2C_SMBUS: %d\n", (ack->GENERAL>>5)&1);
	PRN_INFO("\n");

	PRN_INFO("  SOFTWARE\n");
	PRN_INFO("   * PPS_CLOCK_REARRANGEMENT_THRESHOLD: %d\n", (ack->SOFTWARE>>0)&1);
	PRN_INFO("   *                    INTERRUPT[6:0]: %d\n", (ack->SOFTWARE>>3)&0x3f);
	PRN_INFO("\n");

	PRN_INFO("  PHASE_CONTROL\n");
	PRN_INFO("   *          SYNC_COMPLETE: %d\n", (ack->PHASE_CONTROL>>0)&1);
	PRN_INFO("   *  PHASE_ADJUST_COMPLETE: %d\n", (ack->PHASE_CONTROL>>1)&1);
	PRN_INFO("   *          PIN_TOO_SHORT: %d\n", (ack->PHASE_CONTROL>>2)&1);
	PRN_INFO("   *      COMMAND_TOO_CLOSE: %d\n", (ack->PHASE_CONTROL>>3)&1);
	PRN_INFO("--------------------------------------------\n");

	return RC_OK;
}

int class_cmdmgr::create_METADATA(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_METADATA);
	m_OutStreamSize = sizeof(cmd_METADATA_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_METADATA* cmd = (cmd_METADATA*)m_pInStream;
	cmd->id = SI5518_CMDID_METADATA;
	return RC_OK;
}

int class_cmdmgr::show_METADATA_ack(void)
{
	cmd_METADATA_ack* ack = (cmd_METADATA_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_METADATA_ack\n");
	PRN_INFO("                         status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("               DCO_MR_STEP_SIZE: 0x%x (%u)\n", ack->DCO_MR_STEP_SIZE, ack->DCO_MR_STEP_SIZE);
	PRN_INFO("               DCO_NA_STEP_SIZE: 0x%x (%u)\n", ack->DCO_NA_STEP_SIZE, ack->DCO_NA_STEP_SIZE);
	PRN_INFO("               DCO_MA_STEP_SIZE: 0x%x (%u)\n", ack->DCO_MA_STEP_SIZE, ack->DCO_MA_STEP_SIZE);
	PRN_INFO("               DCO_NB_STEP_SIZE: 0x%x (%u)\n", ack->DCO_NB_STEP_SIZE, ack->DCO_NB_STEP_SIZE);
	PRN_INFO("               DCO_MB_STEP_SIZE: 0x%x (%u)\n\n", ack->DCO_MB_STEP_SIZE, ack->DCO_MB_STEP_SIZE);
	PRN_INFO("           PLAN_OPTIONS\n");
	PRN_INFO("               * PTP_STEERED_RF: %d\n", (ack->PLAN_OPTIONS>>0)&1);
	PRN_INFO("               *        PPS_PLL: %d\n\n", (ack->PLAN_OPTIONS>>2)&1);
	PRN_INFO("   PHASE_JAM_PPS_OUT_RANGE_HIGH: 0x%x (%u)\n", ack->PHASE_JAM_PPS_OUT_RANGE_HIGH, ack->PHASE_JAM_PPS_OUT_RANGE_HIGH);
	PRN_INFO("    PHASE_JAM_PPS_OUT_RANGE_LOW: 0x%x (%u)\n", ack->PHASE_JAM_PPS_OUT_RANGE_LOW, ack->PHASE_JAM_PPS_OUT_RANGE_LOW);
	PRN_INFO("    PHASE_JAM_PPS_OUT_STEP_SIZE: 0x%llx (%llu)\n", ack->PHASE_JAM_PPS_OUT_STEP_SIZE, ack->PHASE_JAM_PPS_OUT_STEP_SIZE);
	PRN_INFO("--------------------------------------------\n");
	return RC_OK;
}

int class_cmdmgr::create_REFERENCE_STATUS(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_REFERENCE_STATUS);
	m_OutStreamSize = sizeof(cmd_REFERENCE_STATUS_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_REFERENCE_STATUS* cmd = (cmd_REFERENCE_STATUS*)m_pInStream;
	cmd->id = SI5518_CMDID_REFERENCE_STATUS;
	return RC_OK;
}

int class_cmdmgr::show_REFERENCE_STATUS_ack(void)
{
	cmd_REFERENCE_STATUS_ack* ack = (cmd_REFERENCE_STATUS_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_REFERENCE_STATUS_ack\n");
	PRN_INFO("                     status: 0x%x (%s)\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("     REFERENCE_CLOCK_STATUS: %d\n", ack->REFERENCE_CLOCK_VALIDATION&0x3);
	PRN_INFO("        LOSS_OF_SIGNAL_FLAG: %d\n", ack->LOSS_OF_SIGNAL&0x1);
	PRN_INFO("      OUT_OF_FREQUENCY_FLAG: %d\n\n", ack->OUT_OF_FREQUENCY&0x1);
	PRN_INFO("  PHASE_MONITOR\n");
	PRN_INFO("    * PHASE_MONITOR_SIGNAL_EARLY: %d\n", (ack->PHASE_MONITOR>>0)&1);
	PRN_INFO("    *  PHASE_MONITOR_SIGNAL_LATE: %d\n", (ack->PHASE_MONITOR>>1)&1);
	PRN_INFO("    *  PHASE_MONITOR_PHASE_ERROR: %d\n", (ack->PHASE_MONITOR>>2)&1);
	PRN_INFO("--------------------------------------------\n");
	return RC_OK;
}

int class_cmdmgr::create_PHASE_READOUT(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_PHASE_READOUT);
	m_OutStreamSize = sizeof(cmd_PHASE_READOUT_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_PHASE_READOUT* cmd = (cmd_PHASE_READOUT*)m_pInStream;
	cmd->id = SI5518_CMDID_PHASE_READOUT;
	cmd->PHASE_GROUP = (unsigned char)strtoul(params[0].c_str(), (char**)0, 0) & 0xf;
	PRN_INFO("PHASE READOUT: PHASE_GROUP=%d\n", cmd->PHASE_GROUP);
	return RC_OK;
}

int class_cmdmgr::show_PHASE_READOUT_ack(void)
{
	cmd_PHASE_READOUT_ack* ack = (cmd_PHASE_READOUT_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_PHASE_READOUT_ack\n");
	PRN_INFO("                status: 0x%x (%s)\n\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");

	PRN_INFO("     ERRORS\n");
	PRN_INFO("       * ERR_LOS: %d\n", (ack->ERRORS>>0)&1);
	PRN_INFO("       * ERR_TSP: %d\n", (ack->ERRORS>>1)&1);
	PRN_INFO("       * ERR_CYC: %d\n", (ack->ERRORS>>2)&1);
	PRN_INFO("\n");

	PRN_INFO("  PHASE_VALID\n");
	PRN_INFO("   * PHRDG_VALID: %d\n", (ack->PHASE_VALID>>0)&1);
	PRN_INFO("\n");

	PRN_INFO("         PHASE_READOUT: 0x%llx (%llu)\n", ack->PHASE_READOUT, ack->PHASE_READOUT);
	PRN_INFO("--------------------------------------------\n");
	return RC_OK;
}

int class_cmdmgr::create_INPUT_PERIOD_READOUT(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_INPUT_PERIOD_READOUT);
	m_OutStreamSize = sizeof(cmd_INPUT_PERIOD_READOUT_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_INPUT_PERIOD_READOUT* cmd = (cmd_INPUT_PERIOD_READOUT*)m_pInStream;
	cmd->id = SI5518_CMDID_INPUT_PERIOD_READOUT;
	cmd->ref_a = (unsigned char)strtoul(params[0].c_str(), (char**)0, 0);
	cmd->ref_b = (unsigned char)strtoul(params[1].c_str(), (char**)0, 0);
	PRN_INFO("INPUT_PERIOD_READOUT: REF-A=%d, REF-B=%d\n", cmd->ref_a, cmd->ref_b);
	return RC_OK;
}

int class_cmdmgr::show_INPUT_PERIOD_READOUT_ack(void)
{
	cmd_INPUT_PERIOD_READOUT_ack* ack = (cmd_INPUT_PERIOD_READOUT_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_INPUT_PERIOD_READOUT_ack\n");
	PRN_INFO("               status: 0x%x (%s)\n\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("     PERIOD_READOUT_A: 0x%llx (%llu)\n", ack->PERIOD_READOUT_A, ack->PERIOD_READOUT_A);
	PRN_INFO("     PERIOD_READOUT_B: 0x%llx (%llu)\n", ack->PERIOD_READOUT_B, ack->PERIOD_READOUT_B);
	PRN_INFO("--------------------------------------------\n");
	return RC_OK;
}

int class_cmdmgr::create_TEMPERATURE_READOUT(std::vector<std::string>& params)
{
	m_InStreamSize = sizeof(cmd_TEMPERATURE_READOUT);
	m_OutStreamSize = sizeof(cmd_TEMPERATURE_READOUT_ack);
	if (alloc_buffers() < RC_OK)
		return RC_CMDMGR_ALLOC_ERROR;

	cmd_TEMPERATURE_READOUT* cmd = (cmd_TEMPERATURE_READOUT*)m_pInStream;
	cmd->id = SI5518_CMDID_TEMPERATURE_READOUT;
	return RC_OK;
}

int class_cmdmgr::show_TEMPERATURE_READOUT_ack(void)
{
	cmd_TEMPERATURE_READOUT_ack* ack = (cmd_TEMPERATURE_READOUT_ack*)m_pOutStream;
	PRN_INFO("--------------------------------------------\n");
	PRN_INFO("   cmd_TEMPERATURE_READOUT_ack\n");
	PRN_INFO("               status: 0x%x (%s)\n\n", ack->status, is_error(ack->status) ? "ERROR" : "OK");
	PRN_INFO("     PERIOD_READOUT_A: 0x%x (%u C)\n", ack->TEMPERATURE_READOUT, ack->TEMPERATURE_READOUT / (1 << 23));
	PRN_INFO("--------------------------------------------\n");
	return RC_OK;
}

