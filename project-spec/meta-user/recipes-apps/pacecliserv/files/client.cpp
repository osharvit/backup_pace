/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class handles CLI client by using connected class_socket
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/ioctl.h>
#include "client.h"

jmp_buf *sigbus_jmp = NULL;

class_client::class_client(void)
{
	m_thread_created = 0;
	m_map_dev = -1;
	m_psocket = NULL;
	m_stopped = 0;
	memset(&m_thread, 0, sizeof(m_thread));
}

class_client::~class_client(void)
{
	close();

	m_map_dev = -1;
	m_stopped = 0;
	m_psocket = NULL;
}

void* client_thread(void* param)
{
	int res;

	class_client* pclient = (class_client*)param;

	PRN_INFO("The client thread is started\n");

	// to send the version this server supports
	// ----------------------------------------

	res = pclient->send_version();
	if (res < 0)
	{
		PRN_ERROR("Sending version error (%d)\n", res);
	}

	while (!pclient->is_stopped())
	{
		res = pclient->proc_cmd();
		if (res < 0)
			break;
	}

	PRN_INFO("The client thread is stopped\n");

	pclient->set_stop();
	return NULL;
}

int class_client::create(class_socket* psocket)
{
	int r;
	m_stopped = 0;
	m_psocket = psocket;

	// Here we need to open the device to map the registers CLI will ask about
	m_map_dev = open("/dev/mem", O_RDWR | O_SYNC);
    if (m_map_dev < 0)
    {
        PRN_ERROR("Error to open /dev/mem device\n");
        //return RC_CLIENT_DEV_OPEN_ERROR;
    }

	if((r = pthread_create(&m_thread, NULL, client_thread, this)))
    {
        PRN_ERROR("class_client::Thread creation error, rc:%d\n", r);
        return RC_CLIENT_THREAD_CREATE_ERROR;
    }
	m_thread_created = 1;
	return RC_OK;
}

int class_client::close(void)
{
	PRN_INFO("class_client::close() 'To close the client'\n");
	void*ret;
	if (m_thread_created)
	{
		if (m_stopped == 0)
		{
			m_stopped = 1;
			pthread_cancel(m_thread);
		}
	
		m_thread_created = 0;
		pthread_join(m_thread, &ret);
	}

	if (m_psocket != NULL)
	{
		delete m_psocket;
		m_psocket = NULL;
	}

	// To unmap all the mapped regions

	std::vector<map_region>::iterator i;
	for (i = m_regions.begin(); i < m_regions.end(); i++)
	{
		munmap((void*)i->m_pmapped_mem, i->m_mapped_size);
	}

	std::vector<axis_fifo>::iterator j;
	for (j = m_axis_fifo.begin(); j < m_axis_fifo.end(); j++)
	{
		printf("To close AXIS-FIFO driver: 0x%llx\n", (unsigned long long)j->m_addr);
		::close(j->devfile);
	}

	m_regions.clear();
	m_axis_fifo.clear();

	if (m_map_dev != -1)
	{
		::close (m_map_dev);
		m_map_dev = -1;
	}

	m_stopped = 1;
	return RC_OK;
}

int class_client::is_stopped(void)
{
	return m_stopped;
}

void class_client::set_stop(void)
{
	m_stopped = 1;
}

int class_client::send_version(void)
{
	clicp_cmd cmd;
	CMD_INIT(NCID_VERSION, &cmd, clicpi_cmd_version);

	cmd.params.version.version = VERSION_HEX;
	return send_cmd(&cmd);
}

int class_client::send_cmd(clicp_cmd* pcmd)
{
	if(m_psocket == NULL)
		return RC_CLIENT_SEND_CMD_ERROR;

	if (!m_psocket->is_connected())
		return RC_CLIENT_SEND_CMD_ERROR;

	return (m_psocket->send(pcmd, CMD_SIZE(pcmd)) != (int)CMD_SIZE(pcmd)) ? RC_CLIENT_SEND_CMD_ERROR : RC_OK;
}

int class_client::proc_cmd(void)
{
	if (m_psocket == NULL || !m_psocket->is_connected())
		return RC_CLIENT_SEND_CMD_ERROR;

	clicp_cmd* pcmd;
	clicp_cmd  header;

	if (!m_psocket->is_connected())
		return RC_CLIENT_RECV_CMD_ERROR;

	int res = m_psocket->recv(&header, CMD_MAIN_HDR_SIZE());
	if (res < 0)
		return RC_CLIENT_RECV_CMD_ERROR;

	pcmd = (clicp_cmd*)malloc(header.cmd_param_len + CMD_MAIN_HDR_SIZE());
	if (pcmd == NULL)
		return RC_CLIENT_RECV_CMD_ERROR;

	memcpy(pcmd, &header, CMD_MAIN_HDR_SIZE());

	res = m_psocket->recv((char*)pcmd + CMD_MAIN_HDR_SIZE(), header.cmd_param_len);
	if (res != (int)header.cmd_param_len)
	{
		free(pcmd);
		return RC_CLIENT_RECV_CMD_ERROR;
	}

	// The table of commands handlers
	// -------------------------------------------
	struct
	{
		uint32_t		cmd_id;
		uint32_t		drv_id;
		int 			(class_client::*proc)(clicp_cmd* pcmd);
	}
	handlers[] = 
	{
			{NCID_REG_SET,				0,				&class_client::proc_reg_set},
			{NCID_REG_GET,				0,				&class_client::proc_reg_get},
			{NCID_REG_AND,				0,				&class_client::proc_reg_and},
			{NCID_REG_XOR,				0,				&class_client::proc_reg_xor},
			{NCID_REG_OR,				0,				&class_client::proc_reg_or},
			{NCID_READ_DATA,			0,				&class_client::proc_read_data},
			{NCID_READ_DATA,			1,				&class_client::proc_read_data},
			{NCID_WRITE_DATA,			0,				&class_client::proc_write_data},
			{NCID_WRITE_DATA,			1,				&class_client::proc_write_data},
			{NCID_MAP,					0,				&class_client::proc_map},
			{NCID_MAP,					1,				&class_client::proc_map_axi_fifo},
			{NCID_GET_FILE,				0,				&class_client::proc_get_file},
			{NCID_FIFO_CTRL,			1,				&class_client::proc_fifo_ctrl},

			{0,							0,				NULL}
	};

	int i = 0;
	res = RC_CLIENT_UNKNOWN_CMD;
	while (handlers[i].proc != NULL)
	{
		if (handlers[i].cmd_id == pcmd->cmd_id && handlers[i].drv_id == pcmd->cmd_drv_id)
		{
			res = (this->*handlers[i].proc)(pcmd);
			break;
		}
	
		i++;
	}

	if (res == RC_CLIENT_UNKNOWN_CMD)
	{
		clicp_cmd cmd_ack;
		memset(&cmd_ack, 0, sizeof(cmd_ack));

		cmd_ack.cmd_id = pcmd->cmd_id;
		cmd_ack.res_code = RC_CLIENT_UNKNOWN_CMD;
		send_cmd(&cmd_ack);
		PRN_ERROR("Unsupported command-id: %d\n", pcmd->cmd_id);
		res = RC_OK;
	}

	free(pcmd);
	return res;
}

int class_client::proc_reg_set(clicp_cmd* pcmd)
{
	// Here we need to write some value to the specific address
	// and to return ACK
	int res = RC_OK;
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_REG_SET_ACK, &cmd_ack, clicpi_cmd_reg_set_ack);

	switch(pcmd->params.reg_set.bits)
	{
		case 64:
		case 32:
		case 16:
		case 8:
			PRN_INFO("[reg.set] REG%d[0x%llx]=0x%llx(%llu)\n", pcmd->params.reg_set.bits, (unsigned long long)pcmd->params.reg_set.address, (unsigned long long)pcmd->params.reg_set.value, (unsigned long long)pcmd->params.reg_set.value);
			break;

		default:
			PRN_ERROR("[reg.set]: Unsupported number of bits:%d\n", pcmd->params.reg_set.bits);
			res = RC_CLIENT_NUM_BITS_ERROR;
			break;
	}

	if (res == RC_OK)
	{
		res = hw_reg_write(pcmd->params.reg_set.address, pcmd->params.reg_set.value, pcmd->params.reg_set.bits);
	}

	cmd_ack.res_code = res;
	cmd_ack.params.reg_set_ack.res = 0;
	return send_cmd(&cmd_ack);
}

int class_client::proc_reg_get(clicp_cmd* pcmd)
{
	// Here we need to write some value to the specific address
	// and to return ACK
	int res = RC_OK;
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_REG_GET_ACK, &cmd_ack, clicpi_cmd_reg_get_ack);

	switch(pcmd->params.reg_get.bits)
	{
		case 64:
		case 32:
		case 16:
		case 8:
			PRN_INFO("[reg.get] REG%d[0x%llx]==", pcmd->params.reg_get.bits, (unsigned long long)pcmd->params.reg_get.address);
			break;

		default:
			PRN_ERROR("[reg.get]: Unsupported number of bits:%d\n", pcmd->params.reg_get.bits);
			res = RC_CLIENT_NUM_BITS_ERROR;
			break;
	}

	if (res == RC_OK)
	{
		res = hw_reg_read(pcmd->params.reg_get.address, pcmd->params.reg_get.bits, cmd_ack.params.reg_get_ack.value);
		if (res == RC_OK)
			PRN_INFO("0x%llx(%llu)", (unsigned long long)cmd_ack.params.reg_get_ack.value, (unsigned long long)cmd_ack.params.reg_get_ack.value);
		else
			PRN_INFO("Error(%d)", res);
	}

	PRN_INFO("\n");

	cmd_ack.res_code = res;
	return send_cmd(&cmd_ack);
}

int class_client::proc_reg_xor(clicp_cmd* pcmd)
{
	// Here we need to write some value to the specific address
	// and to return ACK
	int res = RC_OK;
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_REG_XOR_ACK, &cmd_ack, clicpi_cmd_reg_xor_ack);

	switch(pcmd->params.reg_xor.bits)
	{
		case 64:
		case 32:
		case 16:
		case 8:
			PRN_INFO("[reg.xor] REG%d[0x%llx]^=0x%llx(%llu)", pcmd->params.reg_xor.bits, (unsigned long long)pcmd->params.reg_xor.address, (unsigned long long)pcmd->params.reg_xor.value, (unsigned long long)pcmd->params.reg_xor.value);
			break;

		default:
			PRN_ERROR("[reg.xor]: Unsupported number of bits:%d\n", pcmd->params.reg_xor.bits);
			res = RC_CLIENT_NUM_BITS_ERROR;
			break;
	}

	if (res == RC_OK)
	{
		res = hw_reg_read(pcmd->params.reg_xor.address, pcmd->params.reg_or.bits, cmd_ack.params.reg_xor_ack.read_val);
		if (res == RC_OK)
		{
			cmd_ack.params.reg_xor_ack.write_val = cmd_ack.params.reg_xor_ack.read_val ^ pcmd->params.reg_xor.value;
			res = hw_reg_write(pcmd->params.reg_xor.address, cmd_ack.params.reg_xor_ack.write_val, pcmd->params.reg_xor.bits);
			PRN_INFO("read:0x%llx, write:0x%llx", (unsigned long long)cmd_ack.params.reg_or_ack.read_val, (unsigned long long)cmd_ack.params.reg_or_ack.write_val);
		}
		PRN_INFO("\n");
	}

	cmd_ack.res_code = res;
	return send_cmd(&cmd_ack);
}

int class_client::proc_reg_or(clicp_cmd* pcmd)
{
	// Here we need to write some value to the specific address
	// and to return ACK
	int res = RC_OK;
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_REG_OR_ACK, &cmd_ack, clicpi_cmd_reg_or_ack);

	switch(pcmd->params.reg_or.bits)
	{
		case 64:
		case 32:
		case 16:
		case 8:
			PRN_INFO("[reg.or] REG%d[0x%llx]|=0x%llx(%llu)", pcmd->params.reg_or.bits, (unsigned long long)pcmd->params.reg_or.address, (unsigned long long)pcmd->params.reg_or.value, (unsigned long long)pcmd->params.reg_or.value);
			break;

		default:
			PRN_ERROR("[reg.or]: Unsupported number of bits:%d\n", pcmd->params.reg_or.bits);
			res = RC_CLIENT_NUM_BITS_ERROR;
			break;
	}

	if (res == RC_OK)
	{
		res = hw_reg_read(pcmd->params.reg_or.address, pcmd->params.reg_or.bits, cmd_ack.params.reg_or_ack.read_val);
		if (res == RC_OK)
		{
			cmd_ack.params.reg_or_ack.write_val = cmd_ack.params.reg_or_ack.read_val | pcmd->params.reg_or.value;
			res = hw_reg_write(pcmd->params.reg_or.address, cmd_ack.params.reg_or_ack.write_val, pcmd->params.reg_or.bits);
			PRN_INFO("read:0x%llx, write:0x%llx", (unsigned long long)cmd_ack.params.reg_or_ack.read_val, (unsigned long long)cmd_ack.params.reg_or_ack.write_val);
		}
		PRN_INFO("\n");
	}

	cmd_ack.res_code = res;
	return send_cmd(&cmd_ack);
}

int class_client::proc_reg_and(clicp_cmd* pcmd)
{
	// Here we need to write some value to the specific address
	// and to return ACK
	int res = RC_OK;
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_REG_AND_ACK, &cmd_ack, clicpi_cmd_reg_and_ack);

	switch(pcmd->params.reg_and.bits)
	{
		case 64:
		case 32:
		case 16:
		case 8:
			PRN_INFO("[reg.and] REG%d[0x%llx]&=0x%llx(%llu)", pcmd->params.reg_and.bits, (unsigned long long)pcmd->params.reg_and.address, (unsigned long long)pcmd->params.reg_and.value, (unsigned long long)pcmd->params.reg_and.value);
			break;

		default:
			PRN_ERROR("[reg.and]: Unsupported number of bits:%d\n", pcmd->params.reg_and.bits);
			res = RC_CLIENT_NUM_BITS_ERROR;
			break;
	}

	if (res == RC_OK)
	{
		res = hw_reg_read(pcmd->params.reg_and.address, pcmd->params.reg_and.bits, cmd_ack.params.reg_and_ack.read_val);
		if (res == RC_OK)
		{
			cmd_ack.params.reg_and_ack.write_val = cmd_ack.params.reg_and_ack.read_val & pcmd->params.reg_and.value;
			res = hw_reg_write(pcmd->params.reg_and.address, cmd_ack.params.reg_and_ack.write_val, pcmd->params.reg_and.bits);
			PRN_INFO("read:0x%llx, write:0x%llx", (unsigned long long)cmd_ack.params.reg_and_ack.read_val, (unsigned long long)cmd_ack.params.reg_and_ack.write_val);
		}
		PRN_INFO("\n");
	}

	cmd_ack.res_code = res;
	return send_cmd(&cmd_ack);
}

int class_client::proc_read_data_direct_map(clicp_cmd* pcmd)
{
	// to map the region and to add +2 4K pages
	// ----------------------------------------
	int res = map_hw_region(pcmd->params.readdata.address, pcmd->params.readdata.len + sysconf(_SC_PAGE_SIZE)*2, 1);
	if (res < RC_OK)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_READ_DATA_ACK, &cmd_ack, clicpi_cmd_read_data_ack);
		PRN_ERROR("read_data, mapping memory region error:%d\n", res);
		cmd_ack.res_code = res;
		return (m_psocket->send(&cmd_ack, CMD_SIZE(&cmd_ack)) != (int)CMD_SIZE(&cmd_ack)) ? RC_CLIENT_SEND_CMD_ERROR : RC_OK;
	}

	// So, the region was mapped, let's access it
	// ------------------------------------------
	clicp_cmd* pcmd_ack;
	pcmd_ack = (clicp_cmd*)malloc(CMD_MAIN_HDR_SIZE() + pcmd->params.readdata.len);
	if (pcmd_ack == NULL)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_READ_DATA_ACK, &cmd_ack, clicpi_cmd_read_data_ack);
		PRN_ERROR("read_data, allocation ACK command error\n");
		cmd_ack.res_code = RC_CLIENT_ALLOC_ERROR;
		return (m_psocket->send(&cmd_ack, CMD_SIZE(&cmd_ack)) != (int)CMD_SIZE(&cmd_ack)) ? RC_CLIENT_SEND_CMD_ERROR : RC_OK;
	}

	memset(pcmd_ack, 0, CMD_MAIN_HDR_SIZE());

	pcmd_ack->cmd_id = NCID_READ_DATA_ACK;
	pcmd_ack->cmd_param_len = pcmd->params.readdata.len;

	void* psrc = phys2virt(pcmd->params.readdata.address, pcmd->params.readdata.len);
	void* pdst = (char*)pcmd_ack+CMD_MAIN_HDR_SIZE();

	jmp_buf sigbus_jmpbuf;
    sigbus_jmp = &sigbus_jmpbuf;
	if (sigsetjmp(sigbus_jmpbuf, 1) == 0)
	{
		res = RC_OK;
		switch (pcmd->params.readdata.bits)
		{
			case 8:
				memcpy (pdst, psrc, pcmd->params.readdata.len);
				break;

			case 16:
				for (unsigned int i = 0; i < pcmd->params.readdata.len / 2; i++)
				{
					((uint16_t*)pdst)[i] = ((uint16_t*)psrc)[i];
				}
				break;

			case 32:
				for (unsigned int i = 0; i < pcmd->params.readdata.len / 4; i++)
				{
					((uint32_t*)pdst)[i] = ((uint32_t*)psrc)[i];
				}
				break;

			case 64:
				for (unsigned int i = 0; i < pcmd->params.readdata.len / 8; i++)
				{
					((uint64_t*)pdst)[i] = ((uint64_t*)psrc)[i];
				}
				break;

			default:
				memset(pdst, 0xCC, pcmd->params.readdata.len);
		}
	}
	else
	{
		res = RC_BUS_ERROR;
	}

	if (res < 0)
	{
		PRN_INFO("read_data[0x%llx ... 0x%llx] - Error(%d)\n", (unsigned long long)pcmd->params.readdata.address, (unsigned long long)pcmd->params.readdata.address+pcmd->params.readdata.len, res);
	}
	else
	{
		PRN_INFO("read_data[0x%llx ... 0x%llx] - OK\n", (unsigned long long)pcmd->params.readdata.address, (unsigned long long)pcmd->params.readdata.address+pcmd->params.readdata.len);
	}
	pcmd_ack->res_code = res;
	res = send_cmd(pcmd_ack);
	free(pcmd_ack);
	pcmd_ack = NULL;
	return res;
}

int class_client::proc_read_data_axis_fifo(clicp_cmd* pcmd)
{
	int res = open_axis_fifo(pcmd->params.readdata.address);

	if (res < RC_OK)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_READ_DATA_ACK, &cmd_ack, clicpi_cmd_read_data_ack);
		cmd_ack.res_code = res;
		return send_cmd(&cmd_ack);
	}

	const axis_fifo& fifo = find_axis_fifo_info(pcmd->params.readdata.address);

	clicp_cmd* pcmd_ack;
	pcmd_ack = (clicp_cmd*)malloc(CMD_MAIN_HDR_SIZE() + pcmd->params.readdata.len);
	if (pcmd_ack == NULL)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_READ_DATA_ACK, &cmd_ack, clicpi_cmd_read_data_ack);
		PRN_ERROR("read_data, allocation ACK command error\n");
		cmd_ack.res_code = RC_CLIENT_ALLOC_ERROR;
		return send_cmd(&cmd_ack);
	}

	void* pdst = (char*)pcmd_ack->params.readdata_ack.opt;
	memset(pcmd_ack, 0, CMD_MAIN_HDR_SIZE()+pcmd->params.readdata.len);

	pcmd_ack->cmd_id = NCID_READ_DATA_ACK;
	pcmd_ack->cmd_param_len = pcmd->params.readdata.len;
	pcmd_ack->res_code = read(fifo.devfile, pdst, pcmd->params.readdata.len);

	if (pcmd_ack->res_code < 0)
	{
		pcmd_ack->cmd_param_len = 0;
	}
	else
	{
		pcmd_ack->cmd_param_len = pcmd_ack->res_code;
	}

	PRN_INFO("AXIS-FIFO read [0x%llx], %lu bytes, res:%d\n", (unsigned long long)pcmd->params.readdata.address, (unsigned long)pcmd->params.readdata.len, pcmd_ack->cmd_param_len);

	res = send_cmd(pcmd_ack);
	free(pcmd_ack);
	pcmd_ack = NULL;
	return res;
}

int class_client::proc_read_data(clicp_cmd* pcmd)
{
	switch(pcmd->cmd_drv_id)
	{
		case 0:
			return proc_read_data_direct_map(pcmd);

		case 1:
			return proc_read_data_axis_fifo(pcmd);

		default:
			break;
	}

	clicp_cmd cmd_ack;
	CMD_INIT(NCID_READ_DATA_ACK, &cmd_ack, clicpi_cmd_read_data_ack);
	cmd_ack.res_code = RC_CLIENT_UNSUPPORTED_DRVID;
	return send_cmd(&cmd_ack);
}

int class_client::proc_write_data_direct_map(clicp_cmd* pcmd)
{
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_WRITE_DATA_ACK, &cmd_ack, clicpi_cmd_write_data_ack);

	// to map the region and to add +2 4K pages
	// ----------------------------------------
	int res = map_hw_region(pcmd->params.writedata.address, pcmd->params.writedata.len + sysconf(_SC_PAGE_SIZE)*2, 1);
	if (res < RC_OK)
	{
		PRN_ERROR("writedata, mapping memory region error:%d\n", res);
		cmd_ack.res_code = res;
		return (m_psocket->send(&cmd_ack, CMD_SIZE(&cmd_ack)) != (int)CMD_SIZE(&cmd_ack)) ? RC_CLIENT_SEND_CMD_ERROR : RC_OK;
	}

	void* pdst = phys2virt(pcmd->params.writedata.address, pcmd->params.writedata.len);
	void* psrc = (char*)pcmd->params.writedata.data;

	jmp_buf sigbus_jmpbuf;
    sigbus_jmp = &sigbus_jmpbuf;
	if (sigsetjmp(sigbus_jmpbuf, 1) == 0)
	{
		switch (pcmd->params.writedata.bits)
		{
			case 8:
				memcpy (pdst, psrc, pcmd->params.writedata.len);
				break;

			case 16:
				for (unsigned int i = 0; i < pcmd->params.writedata.len / 2; i++)
				{
					((uint16_t*)pdst)[i] = ((uint16_t*)psrc)[i];
				}
				break;

			case 32:
				for (unsigned int i = 0; i < pcmd->params.writedata.len / 4; i++)
				{
					((uint32_t*)pdst)[i] = ((uint32_t*)psrc)[i];
				}
				break;

			case 64:
				for (unsigned int i = 0; i < pcmd->params.writedata.len / 8; i++)
				{
					((uint64_t*)pdst)[i] = ((uint64_t*)psrc)[i];
				}
				break;

			default:
				res = RC_CLIENT_NUM_BITS_ERROR;
				break;
		}
	}
	else
	{
		res = RC_BUS_ERROR;
	}

	PRN_INFO("writedata [0x%llx ... 0x%llx] - ", (unsigned long long)pcmd->params.writedata.address, (unsigned long long)pcmd->params.writedata.address+pcmd->params.writedata.len);
	if (res == RC_OK)
	{
		PRN_INFO("OK\n");
	}
	else
	{
		PRN_INFO("Error(%d)\n", res);
	}

	cmd_ack.res_code = pcmd->params.writedata.len;
	return send_cmd(&cmd_ack);
}

int class_client::proc_write_data_axis_fifo(clicp_cmd* pcmd)
{
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_WRITE_DATA_ACK, &cmd_ack, clicpi_cmd_write_data_ack);

	int res = open_axis_fifo(pcmd->params.writedata.address);

	if (res < RC_OK)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_WRITE_DATA_ACK, &cmd_ack, clicpi_cmd_write_data_ack);
		cmd_ack.res_code = res;
		return send_cmd(&cmd_ack);
	}

	const axis_fifo& fifo = find_axis_fifo_info(pcmd->params.writedata.address);
	void* psrc = (char*)pcmd->params.writedata.data;

	PRN_INFO("axis-fifo writedata 0x%llx - ", (unsigned long long)pcmd->params.writedata.address);

	res = write(fifo.devfile, psrc, pcmd->params.writedata.len);
	if (res >= 0)
	{
		PRN_INFO("OK (written %d bytes)\n", res);
	}
	else
	{
		PRN_INFO("Error(%d)\n", res);
	}

	cmd_ack.res_code = res;
	return send_cmd(&cmd_ack);
}

int class_client::proc_write_data(clicp_cmd* pcmd)
{
	switch(pcmd->cmd_drv_id)
	{
		case 0:
			return proc_write_data_direct_map(pcmd);

		case 1:
			return proc_write_data_axis_fifo(pcmd);

		default:
			break;
	}

	clicp_cmd cmd_ack;
	CMD_INIT(NCID_WRITE_DATA_ACK, &cmd_ack, clicpi_cmd_write_data_ack);
	cmd_ack.res_code = RC_CLIENT_UNSUPPORTED_DRVID;
	return send_cmd(&cmd_ack);
}

int class_client::proc_map(clicp_cmd* pcmd)
{
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_MAP_ACK, &cmd_ack, clicpi_cmd_map_ack);

	PRN_INFO("Mapping region [0x%llx ... 0x%llx] = ", (unsigned long long)pcmd->params.map.address, (unsigned long long)pcmd->params.map.address+pcmd->params.map.size);

	cmd_ack.res_code = map_hw_region(pcmd->params.map.address, pcmd->params.map.size);

	if (cmd_ack.res_code == RC_OK)
		PRN_INFO("OK\n");
	else
		PRN_INFO("Error(%d)\n", cmd_ack.res_code);

	int len = m_psocket->send(&cmd_ack, CMD_SIZE(&cmd_ack));
	if (len != (int)CMD_SIZE(&cmd_ack))
		return RC_CLIENT_SEND_CMD_ERROR;
	return RC_OK;
}

int class_client::proc_map_axi_fifo(clicp_cmd* pcmd)
{
	clicp_cmd cmd_ack;
	CMD_INIT(NCID_MAP_ACK, &cmd_ack, clicpi_cmd_map_ack);

	PRN_INFO("AXIS-FIFO driver opening for address 0x%016llx - ", (unsigned long long)pcmd->params.map.address);

	cmd_ack.res_code = open_axis_fifo(pcmd->params.map.address);

	if (cmd_ack.res_code == RC_OK)
		PRN_INFO("OK\n");
	else
		PRN_INFO("Error(%d)\n", cmd_ack.res_code);

	return send_cmd(&cmd_ack);
}

int class_client::proc_get_file(clicp_cmd* pcmd)
{
	int file_len = 0;
	int hfile = 0;
	int res = 0;

	PRN_INFO("reading file %s ... \n", (const char*)pcmd->params.get_file.filename);

	// ---------------------------------------------------
	// to try to read the file
	// ---------------------------------------------------
	hfile = open((const char*)pcmd->params.get_file.filename, O_RDONLY);
	if (hfile < 0)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_GET_FILE_ACK, &cmd_ack, clicpi_cmd_get_file_ack);
		PRN_ERROR("get_file, open file(%s) error(%d)\n", pcmd->params.get_file.filename, hfile);
		cmd_ack.res_code = RC_CLIENT_OPEN_ERROR;
		return send_cmd(&cmd_ack);
	}

	struct stat st;
	res = stat((const char*)pcmd->params.get_file.filename, &st);
	if (res < 0)
	{
		::close(hfile);
	
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_GET_FILE_ACK, &cmd_ack, clicpi_cmd_get_file_ack);
		PRN_ERROR("get_file, get file file(%s) stat error(%d)\n", pcmd->params.get_file.filename, res);
		cmd_ack.res_code = RC_CLIENT_OPEN_ERROR;
		return send_cmd(&cmd_ack);
	}
	
	file_len = st.st_size;

	clicp_cmd* pcmd_ack;
	pcmd_ack = (clicp_cmd*)malloc(CMD_MAIN_HDR_SIZE() + file_len);
	if (pcmd_ack == NULL)
	{
		clicp_cmd cmd_ack;
		CMD_INIT(NCID_GET_FILE_ACK, &cmd_ack, clicpi_cmd_get_file_ack);
		PRN_ERROR("get_file, allocation ACK command error\n");
		cmd_ack.res_code = RC_CLIENT_ALLOC_ERROR;
		return send_cmd(&cmd_ack);
	}

	void* pdst = (char*)pcmd_ack->params.get_file_ack.data;
	memset(pcmd_ack, 0, CMD_MAIN_HDR_SIZE()+file_len);

	pcmd_ack->cmd_id = NCID_GET_FILE_ACK;
	pcmd_ack->cmd_param_len = file_len;

	// Here we need to read the file
	// ----------------------------------------------

	unsigned int offs = 0;
	int len = file_len;
	while (len > 0)
	{
		int rx_len = ::read(hfile, (char*)pdst+offs, len);
		if (rx_len <= 0)
		{
			pcmd_ack->res_code = rx_len;
			break;
		}
		offs += rx_len;
		len -= rx_len;
	}

	if (pcmd_ack->res_code < 0)
	{
		pcmd_ack->cmd_param_len = 0;
	}
	else
	{
		pcmd_ack->cmd_param_len = file_len;
	}

	res = send_cmd(pcmd_ack);
	free(pcmd_ack);
	pcmd_ack = NULL;
	::close(hfile);

	PRN_INFO("reading file:%s, data size:%d bytes\n", pcmd->params.get_file.filename, file_len);
	return res;
}

int class_client::proc_fifo_ctrl(clicp_cmd* pcmd)
{
	PRN_INFO("AXIS FIFO CTRL addr:0x%016llx, operation:%d ... \n", (long long unsigned int)pcmd->params.fifo_ctrl.address, pcmd->params.fifo_ctrl.operation);

	clicp_cmd cmd_ack;
	CMD_INIT(NCID_FIFO_CTRL_ACK, &cmd_ack, clicpi_cmd_fifo_ctrl_ack);

	// to find the opened AXIS-FIFO driver
	// and to run proper ioctrl command

	cmd_ack.res_code = open_axis_fifo(pcmd->params.fifo_ctrl.address);

	if (cmd_ack.res_code == RC_OK)
	{
		const axis_fifo& fifo = find_axis_fifo_info(pcmd->params.fifo_ctrl.address);

		// ------------------------------------------------------------------------------
		//      Such macros are taken from axis-fifo.h (axis driver file)
		// ------------------------------------------------------------------------------

		#define AXIS_FIFO_IOCTL_MAGIC 'Q'
		#define AXIS_FIFO_NUM_IOCTLS 8

		#define AXIS_FIFO_GET_REG         _IOR(AXIS_FIFO_IOCTL_MAGIC, 0, struct axis_fifo_kern_regInfo)
		#define AXIS_FIFO_SET_REG         _IOW(AXIS_FIFO_IOCTL_MAGIC, 1, struct axis_fifo_kern_regInfo)
		#define AXIS_FIFO_GET_TX_MAX_PKT  _IOR(AXIS_FIFO_IOCTL_MAGIC, 2, uint32_t)
		#define AXIS_FIFO_SET_TX_MAX_PKT  _IOW(AXIS_FIFO_IOCTL_MAGIC, 3,  uint32_t)
		#define AXIS_FIFO_GET_RX_MIN_PKT  _IOR(AXIS_FIFO_IOCTL_MAGIC, 4,  uint32_t)
		#define AXIS_FIFO_SET_RX_MIN_PKT  _IOW(AXIS_FIFO_IOCTL_MAGIC, 5,  uint32_t)
		#define AXIS_FIFO_RESET_IP        _IO(AXIS_FIFO_IOCTL_MAGIC,6)
		#define AXIS_FIFO_GET_FPGA_ADDR   _IOR(AXIS_FIFO_IOCTL_MAGIC,7, uint32_t)

		// -------------------------------------------------------------------------------

		switch (pcmd->params.fifo_ctrl.operation)
		{
			case AXIS_FIFO_CTRL_RESET:
				cmd_ack.res_code = ioctl(fifo.devfile, AXIS_FIFO_RESET_IP, NULL);
				break;

			default:
				cmd_ack.res_code = RC_CLIENT_FIFO_OPERATION_ERROR;
				break;
		}

		if (cmd_ack.res_code < RC_OK)
		{
			PRN_INFO("Error(%d)\n", cmd_ack.res_code);
		}
		else
		{
			PRN_INFO("OK\n");
		}
	}
	else
	{
		PRN_INFO("Error(%d)\n", cmd_ack.res_code);
	}

	return send_cmd(&cmd_ack);
}

int class_client::is_hw_region_mapped(uint64_t addr, uint64_t len)
{
	uint64_t begin, end;

	unsigned int page_size = sysconf(_SC_PAGE_SIZE);

	// to align the length on 4K page size
	len = (len + (page_size-1)) & ~(page_size-1);

	begin = addr & ~(page_size-1);
	end   = begin + len;

	std::vector<map_region>::iterator i;
	for (i = m_regions.begin(); i < m_regions.end(); i++)
	{
		// if this region is inside of another region,
		// this means it's already mapped
		// --------------------------------------------------------
		if ((i->m_addr_begin <= begin && begin <= i->m_addr_end) &&
			(i->m_addr_begin <= end && end <= i->m_addr_end))
			return 1;
	}

	return 0;
}

const map_region* class_client::find_hw_mapped_region(uint64_t addr, uint64_t len)
{
	std::vector<map_region>::iterator i;
	for (i = m_regions.begin(); i < m_regions.end(); i++)
	{
		if ((i->m_addr_begin <= addr && addr <= i->m_addr_end)&&
			(i->m_addr_begin <= (addr+len) && (addr+len) <= i->m_addr_end))
			return &(*i);
	}

	return NULL;
}

const axis_fifo & class_client::find_axis_fifo_info(uint64_t addr)
{
	std::vector<axis_fifo>::iterator i;
	for (i = m_axis_fifo.begin(); i < m_axis_fifo.end(); i++)
	{
		if (i->m_addr == addr)
			return *i;
	}

	static axis_fifo fifo = {0};
	return fifo;
}

void* class_client::phys2virt(uint64_t addr, uint64_t len)
{
	const map_region* pregion = find_hw_mapped_region(addr, len);
	if (pregion == NULL)
		return NULL;
	return (void*)((char*)pregion->m_pmapped_mem + (addr - pregion->m_addr_begin));
}

int class_client::map_hw_region(uint64_t addr, uint64_t len, int prn)
{
	uint64_t len_in_4k;
	uint64_t begin, end;

	unsigned int page_size = sysconf(_SC_PAGE_SIZE);

	len_in_4k = (len + (page_size-1)) & ~(page_size-1);

	begin = addr & ~(page_size-1);
	end = begin + len_in_4k;

	if (is_hw_region_mapped(begin, len_in_4k))
		return RC_OK;

	if (m_map_dev == -1)
		return RC_CLIENT_DEV_OPEN_ERROR;

	map_region region;

	region.m_addr_begin = begin;
	region.m_addr_end = end;
	region.m_mapped_size = len_in_4k;
	region.m_pmapped_mem = mmap(NULL, len_in_4k, PROT_READ|PROT_WRITE, MAP_SHARED, m_map_dev, begin);

	if (region.m_pmapped_mem == MAP_FAILED)
		return RC_CLIENT_MAP_ERROR;

	if (prn)
		PRN_INFO("Mapped [0x%llx ... 0x%llx], virt:0x%llx\n", (unsigned long long)begin, (unsigned long long)begin+len_in_4k, (unsigned long long)region.m_pmapped_mem);

	m_regions.push_back(region);
	return RC_OK;
}

int class_client::open_axis_fifo(uint64_t addr)
{
	std::vector<axis_fifo>::iterator i;
	for (i = m_axis_fifo.begin(); i < m_axis_fifo.end(); i++)
	{
		if (i->m_addr == addr)
			return RC_OK;
	}

	char name[128];
	sprintf(name, "/dev/axis_fifo_0x%016llx", (unsigned long long)addr);
	int f = open(name, O_RDWR | O_NONBLOCK);

	if (f < 0)
	{
		PRN_ERROR("Error to open device: %s\n", name);
		return RC_CLIENT_DEV_OPEN_ERROR;
	}

	axis_fifo fifo;
	fifo.devfile = f;
	fifo.m_addr = addr;
	m_axis_fifo.push_back(fifo);
	return RC_OK;
}

int class_client::hw_reg_write(uint64_t addr, uint64_t val, uint32_t bits)
{
	int res = RC_OK;
	const map_region* pregion = find_hw_mapped_region(addr);

	if (pregion == NULL)
	{
		int r = map_hw_region(addr, 2*sysconf(_SC_PAGE_SIZE), 1);
		if (r < RC_OK)
			return r;
	}

	pregion = find_hw_mapped_region(addr);
	if (pregion == NULL)
		return RC_CLIENT_MAP_ERROR;

	uint64_t offs = addr - pregion->m_addr_begin;
	void* paddr = (void*)(((char*)pregion->m_pmapped_mem) + offs);

	jmp_buf sigbus_jmpbuf;
    sigbus_jmp = &sigbus_jmpbuf;
	if (sigsetjmp(sigbus_jmpbuf, 1) == 0)
	{
		switch(bits)
		{
			case 64:
				*((uint64_t*)paddr) = val;
				break;

			case 32:
				*((uint32_t*)paddr) = (uint32_t)val;
				break;

			case 16:
				*((uint16_t*)paddr) = (uint16_t)val;
				break;

			case 8:
				*((uint8_t*)paddr) = (uint8_t)val;
				break;
		}
	}
	else
	{
		res = RC_BUS_ERROR;
	}

	return res;
}

int class_client::hw_reg_read(uint64_t addr, uint32_t bits, uint64_t& val)
{
	int res = RC_OK;
	const map_region* pregion = find_hw_mapped_region(addr);

	if (pregion == NULL)
	{
		int r = map_hw_region(addr, 2*sysconf(_SC_PAGE_SIZE), 1);
		if (r < RC_OK)
			return r;
	}

	pregion = find_hw_mapped_region(addr);
	if (pregion == NULL)
		return RC_CLIENT_MAP_ERROR;

	uint64_t offs = addr - pregion->m_addr_begin;
	void* paddr = (void*)(((char*)pregion->m_pmapped_mem) + offs);

	jmp_buf sigbus_jmpbuf;
    sigbus_jmp = &sigbus_jmpbuf;
	if (sigsetjmp(sigbus_jmpbuf, 1) == 0)
	{
		switch(bits)
		{
			case 64:
				val = *((uint64_t*)paddr);
				break;

			case 32:
				val = *((uint32_t*)paddr);
				break;

			case 16:
				val = *((uint16_t*)paddr);
				break;

			case 8:
				val = *((uint8_t*)paddr);
				break;
		}
	}
	else
	{
		res = RC_BUS_ERROR;
	}

	sigbus_jmp = NULL;
	return res;
}

