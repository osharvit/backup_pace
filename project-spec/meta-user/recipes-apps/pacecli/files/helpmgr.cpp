
/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This class provides the information about system command
 *      command parameters, etc
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

#include <stdlib.h>
#include "helpmgr.h"
#include "gen-types.h"
#include "retcodes.h"
#include "cmd.h"

class_helpmgr::class_helpmgr(void)
{

}

class_helpmgr::~class_helpmgr(void)
{

}

std::string class_helpmgr::to_string(int val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%x", val);
	}
	else
	{
		sprintf(buf, "%d", val);
	}
	return buf;
}
std::string class_helpmgr::to_string(unsigned int val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%x", val);
	}
	else
	{
		sprintf(buf, "%u", val);
	}
	return buf;
}

std::string class_helpmgr::to_string(long val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%lx", val);
	}
	else
	{
		sprintf(buf, "%ld", val);
	}
	return buf;
}

std::string class_helpmgr::to_string(unsigned long val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%lx", val);
	}
	else
	{
		sprintf(buf, "%lu", val);
	}
	return buf;
}

std::string class_helpmgr::to_string(long long val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%llx", val);
	}
	else
	{
		sprintf(buf, "%lld", val);
	}
	return buf;
}

std::string class_helpmgr::to_string(unsigned long long val, int in_hex)
{
	char buf[128];
	if (in_hex)
	{
		sprintf(buf, "0x%llx", val);
	}
	else
	{
		sprintf(buf, "%llu", val);
	}
	return buf;
}

long long class_helpmgr::to_num(const char* data)
{
	return strtoul(data, NULL, 0);
}

void class_helpmgr::show_commad_info(int cmd_id)
{
	switch (cmd_id)
	{
		case NCID_REG_SET:

			PRN_INFO("set register:\n");
			PRN_INFO("reg <address|reg_name>[:bits] = <value>\n\n");
			PRN_INFO("  this command writes the value into the register\n");
			PRN_INFO("  <address>  - the address (like: 1234 or 0x1234...), the variable name: $myvar, the register name: compname.regname\n");
			PRN_INFO("  [:bits]    - the optional parameter: 8,16,32,64 bits, by default 32 bits operation\n");
			PRN_INFO("  <value>    - the value: 1234,0x1234, the variable name:$myvar, the math eq: 1<<4+$myoffs...\n");
			PRN_INFO("\n");
			break;

		case NCID_REG_GET:

			PRN_INFO("read register:\n");
			PRN_INFO("reg <address|reg_name>[:bits] [, message]\n\n");
			PRN_INFO("  this command reads specified register value\n");
			PRN_INFO("  <address>  - the address (like: 1234 or 0x1234...), the variable name: $myvar, the register name: compname.regname\n");
			PRN_INFO("  [:bits]    - the optional parameter: 8,16,32,64 bits, by default 32 bits operation\n");
			PRN_INFO("  [message]  - the optional parameter, the message to write within received register value:\n");
			PRN_INFO("               \"Received val is $retd - the value in decimal, $retx - the value in hex\"\n");
			PRN_INFO("\n");
			break;

		case NCID_REG_OR:

			PRN_INFO("set register with OR operation:\n");
			PRN_INFO("reg <address|reg_name>[:bits] |= <value>\n\n");
			PRN_INFO("  this command reads, ORs and writes the value into the register\n");
			PRN_INFO("  <address>  - the address (like: 1234 or 0x1234...), the variable name: $myvar, the register name: compname.regname\n");
			PRN_INFO("  [:bits]    - the optional parameter: 8,16,32,64 bits, by default 32 bits operation\n");
			PRN_INFO("  <value>    - the value: 1234,0x1234, the variable name:$myvar, the math eq: 1<<4+$myoffs...\n");
			PRN_INFO("\n");
			break;

		case NCID_REG_XOR:

			PRN_INFO("set register with XOR operation:\n");
			PRN_INFO("reg <address|reg_name>[:bits] ^= <value>\n\n");
			PRN_INFO("  this command reads, XORs and writes the value into the register\n");
			PRN_INFO("  <address>  - the address (like: 1234 or 0x1234...), the variable name: $myvar, the register name: compname.regname\n");
			PRN_INFO("  [:bits]    - the optional parameter: 8,16,32,64 bits, by default 32 bits operation\n");
			PRN_INFO("  <value>    - the value: 1234,0x1234, the variable name:$myvar, the math eq: 1<<4+$myoffs...\n");
			PRN_INFO("\n");
			break;

		case NCID_REG_AND:

			PRN_INFO("set register with AND operation:\n");
			PRN_INFO("reg <address|reg_name>[:bits] &= <value>\n\n");
			PRN_INFO("  this command reads, ANDs and writes the value into the register\n");
			PRN_INFO("  <address>  - the address (like: 1234 or 0x1234...), the variable name: $myvar, the register name: compname.regname\n");
			PRN_INFO("  [:bits]    - the optional parameter: 8,16,32,64 bits, by default 32 bits operation\n");
			PRN_INFO("  <value>    - the value: 1234,0x1234, the variable name:$myvar, the math eq: 1<<4+$myoffs...\n");
			PRN_INFO("\n");
			break;

		case NCID_UPLOAD:

			PRN_INFO("to upload the file data into the memory\n");
			PRN_INFO("upload[:bits] <filename> <address>\n\n");
			PRN_INFO("  this command writes the file data to the specific address, the data size is defined by the file length\n");
			PRN_INFO("  [:bits]    - 8,16,32,64 bits operations\n");
			PRN_INFO("  <filename> - the name of file, this may include the complete file path\n");
			PRN_INFO("  <address>  - the address or the variable name, or math equation\n");
			PRN_INFO("\n");
			break;

		case NCID_DOWNLOAD:

			PRN_INFO("to save a memory region or AXIS-FIFO data into the file\n");
			PRN_INFO("download[:bits] <address> <len> <filename>\n\n");
			PRN_INFO("  this command reads the data and writes the read data into the file\n");
			PRN_INFO("  [:bits]    - 8,16,32,64 bits operations\n");
			PRN_INFO("  <address>  - the address or the variable name, or math equation in \"\"\n");
			PRN_INFO("  <len>      - the length of data in bytes\n");
			PRN_INFO("  <filename> - the name of file, it may include directories also\n");
			PRN_INFO("\n");
			break;

		case NCID_SET_VAR:

			PRN_INFO("to set, create, update or remove the variable\n");
			PRN_INFO("set <varname> <value>\n\n");
			PRN_INFO("  this command creates, updates or removes the CLI variable\n");
			PRN_INFO("  <varname>  - the variable name\n");
			PRN_INFO("  <value>    - some variable value\n");
			PRN_INFO("\n");
			PRN_INFO("set\n");
			PRN_INFO("  this command shows the list of CLI variables with values\n");
			PRN_INFO("\n");
			break;

		case NCID_MATH:

			PRN_INFO("it sets or creates, or updates the variable with the result of calculation (mathematic expression)\n");
			PRN_INFO("math <varname> <math-expression>\n\n");
			PRN_INFO("  this command creates, updates CLI variable with the result of math-expression calculation\n");
			PRN_INFO("  <varname>  - the variable name\n");
			PRN_INFO("  <value>    - some math expressions like: 1 or 1+2 or 1+2<<4, (~1<<1)|10*20  etc\n");
			PRN_INFO("\n");
			break;

		case NCID_CALC:

			PRN_INFO("to calculate math expression\n");
			PRN_INFO("= <math-expression>\n\n");
			PRN_INFO("  <math-expression>    - some math expression like: 1 or 1+2 or 1+2<<4, etc\n");
			PRN_INFO("\n");
			break;

		case NCID_PRINT:

			PRN_INFO("to print the message to the console and to the log if the log is enabled\n");
			PRN_INFO("print <message>\n\n");
			PRN_INFO("  this command printes the [message], the message may include variables\n");
			PRN_INFO("  <message>  - the text message with the set of variables: Hello, the var A is equal to $A\n");
			PRN_INFO("  example: print Hello, the var A is equal to $A\n");
			PRN_INFO("\n");
			break;

		case NCID_DUMP:

			PRN_INFO("to dump the memory region or AXIS-FIFO data to the console\n");
			PRN_INFO("dump[:bits] <address> <len>\n\n");
			PRN_INFO("  this command dumps the memory content and outputs the value in 8,16,32,64 bits format\n");
			PRN_INFO("  [:bits]    - how to dump the memory content, as 8, 16, 32 or 64 bits\n");
			PRN_INFO("  <address>  - the memory address or the variable name\n");
			PRN_INFO("  <len>      - the size of block in bytes: as the value or as the variable name\n");
			PRN_INFO("\n");
			break;

		case NCID_WRITE:

			PRN_INFO("to write the data to the memory or to the axis-fifo\n");
			PRN_INFO("write[:bits] <address> , data {, data}\n\n");
			PRN_INFO("  this command writes the values at the specific memory address or to the AXIS-FIFO interface (by default as 8bits units)\n");
			PRN_INFO("  [:bits]    - the unit size: 8, 16, 32 or 64 bits, 8bits by default\n");
			PRN_INFO("  <address>  - the memory address or the variable name or HW block name\n");
			PRN_INFO("  <data>     - the value\n");
			PRN_INFO("  for example:  write name, 1, 2, 3, 4, 5, 6\n");
			PRN_INFO("\n");
			break;

		case NCID_SCHED:

			PRN_INFO("to execute periodicaly the command(s)\n");
			PRN_INFO("- sched <time> <cmd1 [params]; cmd2 ...>\n\n");
			PRN_INFO("    the 'sched' command runs periodically the command or set of commands\n");
			PRN_INFO("       <time> - the time is seconds\n");
			PRN_INFO("       <cmd1 [params]; cmd2 ...> - the set of commands with the parameters\n");
			PRN_INFO("\n");
			PRN_INFO("- sched\n\n");
			PRN_INFO("    to see the currently scheduled commands\n");
			PRN_INFO("\n");
			PRN_INFO("- sched 0\n\n");
			PRN_INFO("    to disable running of commands\n");
			break;

		case NCID_HELP:

			PRN_INFO("to show the system help message\n");
			PRN_INFO("help [cmd_name]\n\n");
			PRN_INFO("  this command shows information about a command or shows the list of commands if no parameter provided\n");
			PRN_INFO("  [cmd_name] - the optional parameter: command name\n");
			PRN_INFO("\n");
			PRN_INFO("help\n");
			PRN_INFO("  to show the list of the supported commands\n");
			PRN_INFO("\n");
			break;

		case NCID_LS:

			PRN_INFO("to list a directory content:\n");
			PRN_INFO("ls\n\n");
			PRN_INFO("  this command prints current folder list of files\n");
			PRN_INFO("\n");
			break;

		case NCID_CD:

			PRN_INFO("to change/set directory\n");
			PRN_INFO("cd <path>\n\n");
			PRN_INFO("  this command changes a current directory\n");
			PRN_INFO("  <path>     - the new path\n");
			PRN_INFO("\n");
			break;

		case NCID_PWD:

			PRN_INFO("to get the current directory\n");
			PRN_INFO("pwd\n");
			PRN_INFO("  it prints the current directory path\n");
			PRN_INFO("\n");
			break;

		case NCID_GET_FILE:
			PRN_INFO("to copy a file to the current directory\n");
			PRN_INFO("getfile <filename>\n");
			PRN_INFO("\n");
			break;

		case NCID_EXIT:

			PRN_INFO("to stop and exit the application\n");
			PRN_INFO("exit [ret_code]\n\n");
			PRN_INFO("  [ret_code] - an optional parameter, the return code, by default this is 0\n");
			PRN_INFO("\n");
			break;
	}
}

void class_helpmgr::show_commad_list(void)
{
	PRN_INFO("\n");
	PRN_INFO("-----------------------------------------------------\n");
	PRN_INFO(" CLI help, the list of the supported commands\n");
	PRN_INFO(" use help <cmd_name> to find details about the command\n");
	PRN_INFO("-----------------------------------------------------\n");

	PRN_INFO(" !          - to set/get/or/xor/and register value\n");
	PRN_INFO(" reg        - to set/get/or/xor/and register value\n");
	PRN_INFO(" =          - to calculate math expresion\n");
	PRN_INFO(" set        - to set/update/remove CLI variable\n");
	PRN_INFO(" math       - to set/update CLI variable with the result of math-expression\n");
	PRN_INFO(" upload     - to read the data from file and to write it into the memory\n");
	PRN_INFO(" download   - to read memory region and to save it into the file\n");
	PRN_INFO(" getfile    - to copy remote file to current directory\n");
	PRN_INFO(" dump       - to dump the memory region or to read AXIS-FIFO\n");
	PRN_INFO(" write      - to write the data to the memory or into AXIS-FIFO\n");
	PRN_INFO(" sleep      - to sleep the required number of milliseconds\n");
	PRN_INFO(" print      - to print the message to the console and to the log\n");
	PRN_INFO(" help       - to get this help message\n");
	PRN_INFO(" ?          - to get this help message\n");
	PRN_INFO(" ls         - to print the current folder list of files\n");
	PRN_INFO(" pwd        - to print the current folder\n");
	PRN_INFO(" cd         - to change the folder\n");
	PRN_INFO(" sched      - to schedule the command(s) for periodic execution\n");
	PRN_INFO(" exit       - to stop and exit from the application \n");
	PRN_INFO(" lnx cmd    - to run Linux command\n");
	PRN_INFO("\n");
	PRN_INFO("\n");
}