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
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>
#include "gen-types.h"
#include "connmgr.h"
#include "clientmgr.h"
#include "h2emul.h"

volatile unsigned int	g_Stop	= 0;

int usage (void)
{
	printf ("<paceserv> application usage:\n");
	printf (" pacesev [options]\n");
	printf ("  where options are:\n");
	printf ("  -h (--help)               - To see the help\n");
	printf ("  --console                 - To run the service in the console mode (to see the prints, etc)\n");
	printf ("  --port <port>             - To set CLI server port, by def: %d\n", DEF_SERV_PORT);
	printf (" --h2						 - To open H2 socket and to receive/send JSON commands\n");
	return 0;
}

void print_app_header(void)
{
	PRN_INFO("-----------------------------------------------------\n");
	PRN_INFO("          PACE SERVER application, version:%s\n", VERSION);
	PRN_INFO("-----------------------------------------------------\n");
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

int param_pars (int argc, char ** argv, CLF_PARAMS_TYPE * params)
{
    int c = 0;
    memset (params, 0, sizeof (*params));

	params->serv_port = DEF_SERV_PORT;

    while (1)
    {
        //int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        static struct option long_options[] = 
        {
            {"help",            no_argument,			0, 'h'},
			{"console",         no_argument,			0, CLF_CONSOLE_MODE},
			{"port",            required_argument,		0, CLF_SERV_PORT},
			{"h2",              no_argument,			0, CLF_H2_EMUL},

			{0,                 0,						0,  0}
		};

		c = getopt_long(argc, argv, "f:c:h", long_options, &option_index);

		if (c == -1)
			break;

		switch(c)
		{
			case 'h':
				params->flags |= CLF_HELP;
				break;

			case CLF_CONSOLE_MODE:
				params->flags |= CLF_CONSOLE_MODE;
				break;

			case CLF_SERV_PORT:
				params->flags |= CLF_SERV_PORT;
				params->serv_port = param_get_num(optarg);
				break;

			case CLF_H2_EMUL:
				params->flags |= CLF_H2_EMUL;
				break;

			default:
				PRN_ERROR("Unknown parameter: %s", long_options[option_index].name);
				if (optarg)
                	PRN_ERROR(" with arg %s", optarg);
            	PRN_ERROR("\n");
				return RC_UNKNOWN_PARAMETER;
		}
    }

	return 0;
}

void sig_handler(int signum)
{
	if (g_Stop != 0)
		return;

	g_Stop = 1;
	PRN_INFO("\n[CTRL+C signal is received]\n");
}

void sig_bus_error_handler(int signum)
{
	PRN_INFO("[Bus error is detected!]\n");

	extern jmp_buf *sigbus_jmp;

	if (sigbus_jmp != NULL)
		siglongjmp(*sigbus_jmp, 1);

	abort();
}

int main(int argc, char** argv)
{
	struct sigaction sa;
	CLF_PARAMS_TYPE params;

	class_client_mgr 		clients;
	class_connection_mgr 	g_con;
	class_h2_emulator 		h2emul;
	
	int r = param_pars(argc, argv, &params);
	if (r < 0)
	{
		PRN_ERROR("command line parameter error, rc:%d\n", r);
		return r; 
	}

	if (params.flags & CLF_HELP)
	{
		usage();
		return 0;
	}

	if (params.flags&CLF_CONSOLE_MODE)
	{
		print_app_header();
	}
	else
	{
		pid_t pid, sid;

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0)
		{
			exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0)
		{
			exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0)
		{
			/* Log any failure here */
			exit(EXIT_FAILURE);
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
			/* Log any failure here */
			exit(EXIT_FAILURE);
        }
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
	}

	memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0; //SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    //sigaction(SIGTERM, &sa, NULL);

	memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sig_bus_error_handler;
    sa.sa_flags = 0; //SA_RESTART;
    sigaction(SIGBUS, &sa, NULL);

	if ((r = g_con.init (&params)) < 0)
	{
		PRN_ERROR("Error to initialize connection manager, rc:%d\n", r);
		return r;
	}

	if ((r=clients.init(&params))<0)
	{
		PRN_ERROR("Error to initialize client manager, rc:%d\n", r);
		return r;
	}

	if ((r=h2emul.init(&params))<0)
	{
		PRN_ERROR("Error to initialize ð2 emulator, rc:%d\n", r);
		return r;
	}

	class_socket* sock;
	while (g_Stop == 0)
	{
		sock = g_con.listen ();
		if (sock == NULL)
			break;

		if (params.flags & CLF_H2_EMUL)
		{
			PRN_INFO("h2 client is connected: %s:%d\n", sock->get_ipv4_address(), sock->get_port());

			if ((r = h2emul.run_client(sock)) < 0)
			{
				PRN_ERROR("Error to run the h2 emulator, CLI request from %s:%d, rc:%d\n", sock->get_ipv4_address(), sock->get_port(), r);
				continue;
			}
		}
		else
		{
			// Here we received the connection from the client,
			// let's create the thread and to handle the client requests
			PRN_INFO("cli client is connected: %s:%d\n", sock->get_ipv4_address(), sock->get_port());

			if ((r = clients.run_client(sock)) < 0)
			{
				PRN_ERROR("Error to run the client, CLI request from %s:%d, rc:%d\n", sock->get_ipv4_address(), sock->get_port(), r);
				continue;
			}
		}
	}

	clients.close_clients();
	g_con.close();
	printf("The application is stopped\n");
    return 0;
}
