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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <vector>
#include <string>
#include "general.h"
#include "spimgr.h"
#include "cmdmgr.h"

int 						g_verbose   = 0;
std::string* 				g_putputdev = NULL;


struct si5518_cmd_id
{
	int				id;
	const char*		name;
	int				param_num;
}
command_list [] = 
{
		{SI5518_CMDID_READ_REPLY,			"READ_REPLY",				0},
		{SI5518_CMDID_SIO_TEST,				"SIO_TEST",					0},
		{SI5518_CMDID_SIO_INFO,				"SIO_INFO",					0},
		{SI5518_CMDID_HOST_LOAD,			"HOST_LOAD",				1},
		{SI5518_CMDID_BOOT,					"BOOT",						0},
		{SI5518_CMDID_DEVICE_INFO,			"DEVICE_INFO",				0},
		{SI5518_CMDID_NVM_STATUS,			"NVM_STATUS",				0},
		{SI5518_CMDID_RESTART,				"RESTART",					0},
		{SI5518_CMDID_APP_INFO,				"APP_INFO",					0},
		{SI5518_CMDID_PLL_ACTIVE_REFCLOCK,	"PLL_ACTIVE_REFCLOCK",		1},
		{SI5518_CMDID_INPUT_STATUS,			"INPUT_STATUS",				1},
		{SI5518_CMDID_PLL_STATUS,			"PLL_STATUS",				1},
		{SI5518_CMDID_INTERRUPT_STATUS,		"INTERRUPT_STATUS",			0},
		{SI5518_CMDID_METADATA,				"METADATA",					0},
		{SI5518_CMDID_REFERENCE_STATUS,		"REFERENCE_STATUS",			0},
		{SI5518_CMDID_PHASE_READOUT,		"PHASE_READOUT",			1},
		{SI5518_CMDID_INPUT_PERIOD_READOUT,	"INPUT_PERIOD_READOUT",		2},
		{SI5518_CMDID_TEMPERATURE_READOUT,	"TEMPERATURE_READOUT",		0},

		{0,									NULL},
};

int cmdname2id(const char* name, int& param_num)
{
	int pos = 0;
	while (command_list[pos].name != NULL)
	{
		if (strcasecmp(command_list[pos].name, name) == 0)
		{
			param_num = command_list[pos].param_num;
			return command_list[pos].id;
		}

		pos ++;
	}
	return -1;
}

const char* cmdid2name(int id, int& param_num)
{
	int pos = 0;
	while (command_list[pos].name != NULL)
	{
		if (id == command_list[pos].id)
		{
			param_num = command_list[pos].param_num;
			return command_list[pos].name;
		}

		pos ++;
	}
	return NULL;
}

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

void dump(void* p, int size)
{
    unsigned char* d = (unsigned char*)p;
    printf("To dump the memory:\n");
    for (int i = 0; i < size; i++)
    {
        if (i%32 == 0)
            printf("0x%04x ", i);

        printf("%02x ", d[i]);

        if (i && ((i+1)%32) == 0)
            printf("\n");
    }

    printf("\n");
}

int usage (void)
{
	printf ("si5518 (version: %s) application usage:\n", VERSION);
	printf (" si5518 <options>\n");
	printf ("  where options are:\n");
	printf ("  -h (--help)              - To see this help message\n");
	printf ("  -v(--verbose)            - To be verbose, by default it is turned off\n");
	printf ("  -c <cmdid>               - To specify the command name or command id, please see the list of supported commands (-l)\n");
	printf ("                             Some commands require parameters, as:\n");
	printf ("                              -c HOST_LOAD <filename>\n");
	printf ("                              -c PLL_ACTIVE_REFCLOCK <pllid>\n");
	printf ("                              -c INPUT_STATUS <input>\n");
	printf ("                              -c PLL_STATUS <pllid>\n");
	printf ("                              -c PHASE_READOUT <grp_number>\n");
	printf ("                              -c INPUT_PERIOD_READOUT <refA> <refB>\n");
	printf ("  -l (--list)              - To see the list of supported commands\n");
	printf ("  -d (--dev)               - To defice the SPI device, by default is %s\n", DEF_SPI_DEV);
	printf ("  -m (--mode) <spi_mode>   - To define the SPI mode, by default, this is 0x%x\n", DEF_SPI_MODE);
	printf ("  -s <spispeed>            - To define the SPI bus speed, by default, this is %d Hz\n", DEF_SPI_SPEED);
	printf ("  -b <bits>                - To define the SPI bits per word, by default this is %d bits\n", DEF_SPI_BITS);
	printf ("  --dump                   - To dump the SPI communication (commands)\n");
	return 0;
}

int cmd_list(void)
{
	printf ("The list of supported commands:\n");
	int pos = 0;
	while (command_list[pos].name != NULL)
	{
		printf(" (0x%02x) %s\n", command_list[pos].id, command_list[pos].name);
		pos ++;
	}

	return 0;
}

int param_get_num(char * pVal)
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

int param_pars (int argc, char ** argv, CLF_PARAMS_TYPE * p)
{
    int c = 0;
    memset (p, 0, sizeof (*p));

	p->spi_bits  = DEF_SPI_BITS;
	p->spi_speed = DEF_SPI_SPEED;
	p->spi_mode  = DEF_SPI_MODE;
	strcpy(p->spi_dev, DEF_SPI_DEV);

	std::string file_name = "";

    while (1)
    {
        //int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        static struct option long_options[] = 
        {
            {"help",            no_argument,			0, 'h'},
			{"dump",            no_argument,			0, CLF_DUMP_COM},
			{"list",            no_argument,			0, 'l'},
			{"dev",             required_argument,		0, 'd'},
			{"speed",           required_argument,		0, 's'},
			{"bits",           	required_argument,		0, 'b'},
			{"verbose",         no_argument,			0, 'v'},
			{"cmd",      		required_argument,		0, 'c'},
			{"file",      		required_argument,		0, 'f'},

			{0,                 0,						0,  0}
		};

		c = getopt_long(argc, argv, "hd:ls:b:vc:f:", long_options, &option_index);
		if (c == -1)
			break;

		switch(c)
		{
			case 'h':
				p->flags |= CLF_HELP;
				break;

			case 'l':
				p->flags |= CLF_CMD_LIST;
				break;

			case 'd':
				strncpy(p->spi_dev, optarg, sizeof(p->spi_dev));
				p->flags |= CLF_SPI_DEV;
				break;

			case 's':
				p->spi_speed = param_get_num(optarg);
				p->flags |= CLF_SPI_SPEED;
				break;

			case 'b':
				p->spi_bits = param_get_num(optarg);
				p->flags |= CLF_SPI_BITS;
				break;

			case CLF_DUMP_COM:
 				p->flags |= CLF_DUMP_COM;
				break;

			case 'v':
 				p->flags |= CLF_VERBOSE;
				g_verbose = 1;
				break;

			case 'm':
 				p->flags |= CLF_SPI_MODE;
				p->spi_mode = param_get_num(optarg);
				break;

			case 'c':
				{
					p->flags |= CLF_CMD;
					si5518_command cmd;
					int param_num = 0;
					cmd.id = cmdname2id(optarg, param_num);
					if (cmd.id < 0)
					{
						cmd.id = param_get_num(optarg);
						if (cmdid2name(cmd.id, param_num) == NULL)
						{
							printf("Unknown command: %s\n", optarg);
							return -1;
						}
					}

					for (int j=0; j < param_num; j++)
					{
						if (optind >= argc)
						{
							printf("-c %s requires %d parameter(s)\n", optarg, param_num);
							return -2;
						}
						cmd.params.push_back(argv[optind]);
						optind ++;
					}
					p->cmd_list.push_back(cmd);
				}
				break;

			case 'f':
				file_name = optarg;
				break;

			default:
				printf("Unknown parameter: %s %d\n", argv[optind], optind);
				break;
 		}
    }

	return 0;
}

int main(int argc, char** argv)
{
	CLF_PARAMS_TYPE params;
	int res = param_pars(argc, argv, &params);
	if (res < 0)
	{
		printf("Parameters parsing error, rc:%d\n", res);
		return res;
	}

	if (params.flags & CLF_HELP)
		return usage();

	if (params.flags & CLF_CMD_LIST)
		return cmd_list();

	PRN_VER("SPI initialization\n");
	PRN_VER("SPI device: %s\n",  params.spi_dev);
	PRN_VER("  SPI mode: 0x%x\n", params.spi_mode);
	PRN_VER("  SPI bits: %d\n",  params.spi_bits);
	PRN_VER(" SPI speed: %d\n",  params.spi_speed);

	class_spimgr spimgr;
	res = spimgr.init(params.spi_dev, params.spi_mode, params.spi_speed, params.spi_bits);
	if (res < RC_OK)
	{
		PRN_ERROR("SPI initialization error, rc: %d\n", res);
		return res;
	}
	spimgr.dumpena(g_verbose);
	PRN_VER("SPI initialization is done successfully\n\n");

	// to process the list of commands specified as
	// the command line parameters
	for(unsigned int i = 0; i < params.cmd_list.size(); i++)
	{
		int data = 0;
		PRN_INFO("Processing command id[0x%02x], name[%s]:\n", params.cmd_list[i].id, cmdid2name(params.cmd_list[i].id, data));

		class_cmdmgr mgr(&spimgr, params.cmd_list[i].id);
		if ((res = mgr.proc(params.cmd_list[i].params)) < 0)
			PRN_ERROR("ERROR to process the command, rc:%d\n", res);
	}
	
    return 0;
}
