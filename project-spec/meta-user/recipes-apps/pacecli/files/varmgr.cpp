/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *   This class implements CLI variables (CLI variables manager)
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
#include <sys/stat.h>
#include <stdlib.h>
#include "varmgr.h"
#include "lexer.h"
#include "file.h"

class_varmgr::class_varmgr(void)
{
	m_updated = 0;
	m_errorline = 0;
}

class_varmgr::~class_varmgr(void)
{
	save_to_file(m_filename.c_str());
}

/** @brief To load the variables from the text (actually from the file)

		[The current list of the variables gets cleaned]

	@param pdata [in]	- the pointer to the data

	@return [int] the error code */
int class_varmgr::load(const char* pdata)
{
	int line = 1;
	int state = 0;

			// STATE is the state of the finite state machine
			// ----------------------------------------------
			//  0 - the initial state (ready to get var name):  0->1 or 0->0
			//  1 - ready to get '='
			//  2 - ready to get var value
			//  3 - ready to get '\n' or EOF
			//  3 -> 0

	class_lexer lexer ("=\n", " \t\r");
	std::string lex;
	variable var;

	int res = lexer.lex_init(pdata);
	if (res < 0)
		return res;

	// to pars the input stream and to create the list of variables
	// ------------------------------------------------------------
	m_vars.clear();
	m_updated = 0;  // The variables are loaded and do not require to be saved (in case of error, the save is also not needed)

	// till we have something in the input text
	while (!(lex = lexer.lex_next()).empty())
	{
		switch (state)
		{
			case 0:
				if (lex == "\n")
				{
					state = 0;
					line ++;
					break;
				}

				var.init();
				var.setname(lex.c_str());
				state = 1;
				break;

			case 1:
				if (lex != "=")
				{
					m_errorline = line;
					return RC_VARMGR_SYNTAX_ERROR;
				}

				lexer.set_lexems("\n");
				lexer.set_delims("");
				state = 2;
				break;

			case 2:
				var.setval(lex.c_str());
				if ((res = set(var))< 0)
				{
					m_errorline = line;
					return res;
				}
				lexer.set_lexems("=\n");
				lexer.set_delims(" \t\r");
				state = 3;
				break;

			case 3:
				if (lex != "\n")
				{
					m_errorline = line;
					return RC_VARMGR_SYNTAX_ERROR;
				}

				line ++;
				state = 0;
				break;

			default:
				printf("VARMGR:: parser state error\n");
				return RC_VARMGR_STATE_ERROR;
		}
	}

	if (state == 3)
	{
		if ((res = set(var))< 0)
		{
			m_errorline = line;
			return res;
		}
		state = 0;
	}
	return RC_OK;
}

int class_varmgr::load_from_file(const char* filename)
{
	int res = RC_OK;
	class_file file(filename);

	m_filename = filename;

	if (!file.is_open())
		return RC_VARMGR_OPENFILE_ERROR;

	void* pdata = NULL;

	if (file.readfile(pdata) < 0)
		return RC_VARMGR_READFILE_ERROR;

	if (pdata)
	{
		res = load((const char*)pdata);
		free(pdata);
		pdata = NULL;
	}
	return res;
}

int class_varmgr::save_to_file(const char* filename)
{
	FILE* file = fopen(filename, "w+");
	if (file == NULL)
		return RC_VARMGR_OPENFILE_ERROR;

	std::string data;
	std::vector<variable>::iterator iter;
	int idx;

	for (idx = 0, iter = m_vars.begin(); iter != m_vars.end(); ++iter, ++idx)
	{
		if (!(*iter).m_system)
		{
			data += (*iter).m_name;
			data += "=";
			data += (*iter).m_value;
			data += "\n";
		}
	}

	int wrsize = data.size();
	int wrlen = 0, wr_offs = 0;
	while (wrsize > 0)
	{
		wrlen = fwrite(data.c_str() + wr_offs, 1, wrsize, file);
		if (wrlen < 0)
			return RC_VARMGR_WRITEFILE_ERROR;

		wrsize -= wrlen;
		wr_offs += wrlen;
	}
	fclose(file);
	return RC_OK;
}

/** @brief To create (or to update) the variable and to add it to the list of the variables if the variable was created

	@Note:
		if the value is equal to the empty string, this variable will be removed

	@param varname [in] - the variable name
	@param value   [in] - the variable value
	@param system  [in] - the flag, if TRUE - here is a system variable (tracked by the CLI automatically)

	@return [int] an error code*/
int class_varmgr::set(const char* varname, const char* value, int system, int set_f)
{
	if (varname == NULL || value == NULL)
		return RC_VARMGR_PARAM_ERROR;

	int idx = find_var(varname);

	// to check if we want to remove the variable
	// ------------------------------------------
	if (strlen(value) == 0)
	{
		if (idx < 0)
			return RC_OK;

		m_updated = 1;
		m_vars.erase(m_vars.begin() + idx);
		return RC_OK;
	}

	if (idx >= 0 && set_f == 0)
	{
		if (m_vars[idx].m_system && system == 0)
			return RC_VARMGR_SYSVAR_PROTECTED;
	}

	m_updated = 1;

	if (idx >= 0)
	{
		m_vars[idx].setval(value);
		m_vars[idx].m_system = system;
	}
	else
	{
		variable v(varname, value, system);
		m_vars.push_back(v);
	}

	return RC_OK;
}

/** @brief To create (or to update) the variable and to add it to the list of the variables if the variable was created
	@param var - the ref. to the variable object
	@return [int] an error code*/
int class_varmgr::set(const variable & var, int uniqueue)
{
	return set(var.m_name.c_str(), var.m_value.c_str(), uniqueue);
}

std::string class_varmgr::get(const char* varname)
{
	int idx = find_var(varname);
	if (idx < 0)
		return "";

	return m_vars[idx].m_value;
}

std::string class_varmgr::get(unsigned int idx)
{
	if (idx >= m_vars.size())
		return "";

	return m_vars[idx].m_value;
}

unsigned int class_varmgr::get_var_num(void)
{
	return m_vars.size();
}
const variable& class_varmgr::get_var(unsigned int idx)
{
	static variable err_var;

	if (idx >= m_vars.size())
	{
		err_var.setname("<index error>");
		err_var.setval("<index error>");
		return err_var;
	}

	return m_vars[idx];
}

int class_varmgr::find_var(const char* name)
{
	std::vector<variable>::iterator iter;
	int idx;

	if (name == NULL)
		return -1;

	for (idx = 0, iter = m_vars.begin(); iter != m_vars.end(); ++iter, ++idx)
	{
        if ((*iter).m_name == name)
			return idx;
	}

	return -1;
}

int class_varmgr::get_error_line(void)
{
	return m_errorline;
}

int class_varmgr::is_system(const char* name)
{
	std::vector<variable>::iterator iter;

	if (name == NULL)
		return -1;

	for (iter = m_vars.begin(); iter != m_vars.end(); ++iter)
	{
        if ((*iter).m_name == name)
        {
			return (*iter).m_system;
        }
	}
	return 0;
}

uint8_t class_varmgr::u8(const char* name)
{
	std::string v = get(name);
	return (uint8_t)strtoull(v.c_str(), (char**)0, 0);
}

uint16_t class_varmgr::u16(const char* name)
{
	std::string v = get(name);
	return (uint16_t)strtoull(v.c_str(), (char**)0, 0);
}

uint32_t class_varmgr::u32(const char* name)
{
	std::string v = get(name);
	return (uint32_t)strtoull(v.c_str(), (char**)0, 0);
}

uint64_t class_varmgr::u64(const char* name)
{
	std::string v = get(name);
	return (uint64_t)strtoull(v.c_str(), (char**)0, 0);
}

variable& class_varmgr::operator[](int idx)
{
	static variable fake_var;
	if ((unsigned int)idx >= m_vars.size())
		return fake_var;

	return m_vars[idx];
}

int class_varmgr::is_save_needed(void)
{
	return m_updated;
}