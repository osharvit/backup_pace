/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager is designed to keeps the information about some FPGA HW blocks
 *      and the registers of these blocks
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
#include "hwmgr.h"
#include "lexer.h"
#include "file.h"
#include "gen-types.h"

class_hwmgr::class_hwmgr(void)
{
	m_line = 0;
}

class_hwmgr::~class_hwmgr(void)
{
}

int class_hwmgr::load(const char* input, const char* filename)
{
	class_lexer lexer(":,\n"," \r\t");
	std::string lex;

	m_line = 1;

	lexer.lex_init(input);

	// The finite state machine:
	//
	// 0	by	[name]			->		100 	[to parse hw name]
	// 0	by  [driver]		->		200		[to parse driver id]
	// 0	by	[version]		->		300		[to parse HW comp version]
	// 0	by 	[info]			->		400		[to parse HW component version]
	// 0	by  [reg]			->		500		[to parse the new reg]
	// 0	by  [other]			->		error

	int state = 0;

	std::string name;
	hw_block hwblock;
	int first = 1;
	int idx;

	hw_reg reg;
	hwblock.reset();
	while (1)
	{
		lex = lexer.lex_next();
		if (lexer.is_end())
			break;

		//printf("state:%d, lex:%s\n", state, lex.c_str());
	
		if (lex == "\n")
		{
			m_line++;
			continue;
		}
		switch (state)
		{
			case 0:
				if (lex == "name")
				{
					if (!first)
					{
						idx = find_by_name(hwblock.m_name.c_str());
						if (idx > -1)
						{
							PRN_ERROR("HW name('%s') is duplicated, originally in:'%s', duplicated in:'%s'\n", hwblock.m_name.c_str(), m_hwblocks[idx].m_filename.c_str(), filename);
							return RC_HWMGR_PARSER_ERROR;
						}
						hwblock.m_filename = filename;
						m_hwblocks.push_back(hwblock);
						hwblock.reset();
					}
					first = 0;
					state = 100;
				}
				else if (lex == "driver")
				{
					state = 200;
				}
				else if (lex == "version")
				{
					state = 300;
				}
				else if (lex == "info")
				{
					state = 400;
				}
				else if (lex == "reg")
				{
					state = 500;
				}
				else
				{
					PRN_ERROR("HW comp description: unknown key-word:%s\n", lex.c_str());
					return RC_HWMGR_PARSER_ERROR;
				}
				break;

			/*****************************************************************/
			/*                        NAME, base, size                       */
			/*****************************************************************/
			case 100:
				if (lex != ":")
				{
					PRN_ERROR("HW comp description: it's expected to see ':' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 102;
				break;

			case 102:
				hwblock.m_name = lex;
				state = 103;
			break;

			case 103:
				if (lex != ",")
				{
					PRN_ERROR("HW comp description: it's expected to see ',' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 104;
			break;

			case 104:
				hwblock.m_base = strtoull(lex.c_str(), (char**)0, 0);
				hwblock.m_base &= ~7;		// At least to be aligned on 8 bytes
				state = 105;
			break;

			case 105:
				if (lex != ",")
				{
					PRN_ERROR("HW comp description: it's expected to see ',' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 106;
			break;

			case 106:
				hwblock.m_size = strtoull(lex.c_str(), (char**)0, 0);

				// to align the size on 8 bytes (at least)
				// ---------------------------------------
				hwblock.m_size = (hwblock.m_size + 7)&~7;
				state = 0;
			break;

			/*****************************************************************/
			/*                        DRIVER-ID                              */
			/*****************************************************************/
			case 200:
				if (lex != ":")
				{
					PRN_ERROR("HW comp description: it's expected to see ':' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 201;
			break;

			case 201:
				hwblock.m_drvid = strtoull(lex.c_str(), (char**)0, 0);
				state = 0;
			break;

			/*****************************************************************/
			/*                        VERSION 	                             */
			/*****************************************************************/
			case 300:
				if (lex != ":")
				{
					PRN_ERROR("HW comp description: it's expected to see ':' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 301;
			break;

			case 301:
				//hwblock.m_version = strtoull(lex.c_str(), (char**)0, 0);
				hwblock.m_version = lex;
				state = 0;
			break;

			/*****************************************************************/
			/*                        INFO 	                                 */
			/*****************************************************************/
			case 400:
				if (lex != ":")
				{
					PRN_ERROR("HW comp description: it's expected to see ':' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 401;
			break;

			case 401:
				hwblock.m_descr = lex;
				state = 0;
			break;

			/*****************************************************************/
			/*                        REGs 	                                 */
			/*****************************************************************/
			case 500:
				reg.reset();
				if (lex != ":")
				{
					PRN_ERROR("HW comp description: it's expected to see ':' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 501;
			break;

			case 501:
				if (hwblock.find_reg_by_name(lex.c_str()) > -1)
				{
					PRN_ERROR("HW comp description: The register name '%s' is duplicated, line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				
				reg.m_regname = lex;
				state = 502;
			break;

			case 502:
				if (lex != ",")
				{
					PRN_ERROR("HW comp description: it's expected to see ',' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 503;
			break;

			case 503:
				reg.m_regoffs = strtoull(lex.c_str(), (char**)0, 0);
				state = 504;
			break;

			case 504:
				if (lex != ",")
				{
					PRN_ERROR("HW comp description: it's expected to see ',' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 505;
			break;

			case 505:
				reg.m_regbits = strtoull(lex.c_str(), (char**)0, 0);

				switch (reg.m_regbits)
				{
					case 8:
						break;
					case 16:
						reg.m_regoffs &= ~1;
						break;
					case 32:
						reg.m_regoffs &= ~3;
						break;
					case 64:
						reg.m_regoffs &= ~7;
						break;
					default:
						PRN_ERROR("HW comp description: unsupported number of bits (%d), line:%d\n", reg.m_regbits, m_line);
						return RC_HWMGR_PARSER_ERROR;
				}

				if (reg.m_regoffs >= hwblock.m_size)
				{
					PRN_ERROR("HW comp description: the register (%s) outs of the component memory space, line:%d\n", reg.m_regname.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 506;
			break;

			case 506:
				if (lex != ",")
				{
					PRN_ERROR("HW comp description: it's expected to see ',' instead of '%s', line:%d\n", lex.c_str(), m_line);
					return RC_HWMGR_PARSER_ERROR;
				}
				state = 507;
			break;

			case 507:
				reg.m_desc = lex;
				hwblock.m_regs.push_back(reg);
				reg.reset();
				state = 0;
			break;
		}
	}

	if (!first)
	{
		idx = find_by_name(hwblock.m_name.c_str());
		if (idx > -1)
		{
			PRN_ERROR("HW name('%s') is duplicated, originally in:'%s', duplicated in:'%s'\n", hwblock.m_name.c_str(), m_hwblocks[idx].m_filename.c_str(), filename);
			return RC_HWMGR_PARSER_ERROR;
		}
		hwblock.m_filename = filename;
		m_hwblocks.push_back(hwblock);
		hwblock.reset();
	}

	return RC_OK;
}

int class_hwmgr::load_from_file(const char* filename)
{
	int res = RC_OK;
	class class_file file(filename);
	if (!file.is_open())
		return RC_HWMGR_NO_FILE;

	void* pdata = NULL;

	if (file.readfile(pdata) < 0)
		return RC_HWMGR_READFILE_ERROR;

	if (pdata)
	{
		res = load((const char*)pdata, filename);
		free(pdata);
		pdata = NULL;
	}
	return res;
}

int class_hwmgr::find_by_name(const char* hwname)
{
	std::vector<hw_block>::iterator i;
	int idx;

	if (hwname == NULL)
		return -1;

	for (idx=0, i=m_hwblocks.begin(); i < m_hwblocks.end(); ++i, ++idx)
	{
		if (i->m_name == hwname)
			return i - m_hwblocks.begin();
	}
	return -1;
}

int class_hwmgr::find_by_addr(unsigned long long addr)
{
	std::vector<hw_block>::iterator i;
	int idx;

	for (idx=0, i=m_hwblocks.begin(); i < m_hwblocks.end(); ++i, ++idx)
	{
		if (i->m_base == addr)
			return i - m_hwblocks.begin();
	}
	return -1;
}

const hw_block& class_hwmgr::get(unsigned int idx)
{
	static hw_block fake;
	if (idx >= m_hwblocks.size())
		return fake;

	return m_hwblocks[idx];
}

const hw_block& class_hwmgr::operator[](unsigned int idx)
{
	return get(idx);
}

unsigned long long class_hwmgr::get_addr_by_name(const char* fullregname)
{
	class_lexer lexer("",".");
	std::string hwname, regname;

	lexer.lex_init(fullregname);
	hwname  = lexer.lex_next();
	regname = lexer.lex_next();

	int idx = find_by_name(hwname.c_str());
	if (idx < 0)
		return 0;

	return m_hwblocks[idx].get_reg_addr_by_name(regname.c_str());
}

int class_hwmgr::split_hw_name(const char* fullregname, std::string& hwname, std::string& regname)
{
	class_lexer lexer("",".");

	lexer.lex_init(fullregname);
	hwname  = lexer.lex_next();
	regname = lexer.lex_next();

	if (hwname == "" || regname == "")
		return RC_CMDMGR_NAME_ERROR;

	return RC_OK;
}

int class_hwmgr::is_reg_name(const char* fullregname)
{
	class_lexer lexer("",".");
	std::string hwname, regname;

	lexer.lex_init(fullregname);
	hwname  = lexer.lex_next();
	regname = lexer.lex_next();

	int idx = find_by_name(hwname.c_str());
	if (idx < 0)
		return -1;

	return m_hwblocks[idx].find_reg_by_name(regname.c_str());
}

unsigned int class_hwmgr::get_drvid_by_name(const char* fullregname)
{
	class_lexer lexer("",".");
	std::string hwname, regname;

	lexer.lex_init(fullregname);
	hwname  = lexer.lex_next();
	regname = lexer.lex_next();

	int idx = find_by_name(hwname.c_str());
	if (idx < 0)
		return 0;

	return m_hwblocks[idx].m_drvid;
}

unsigned int class_hwmgr::get_drvid_by_addr(unsigned long long addr)
{
	std::vector<hw_block>::iterator i;
	for (i=m_hwblocks.begin(); i < m_hwblocks.end(); ++i)
	{
		if (i->m_base <= addr && addr <= i->m_base+i->m_size)
			return i->m_drvid;
	}

	return DEF_DRVID;
}