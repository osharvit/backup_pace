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

#include <string.h>
#include <stdlib.h>
#include "gen-types.h"
#include "extcmdmgr.h"
#include "lexer.h"
#include "retcodes.h"
#include "file.h"

class_extcmdmgr::class_extcmdmgr(void)
{
	m_line = 1;
}

class_extcmdmgr::~class_extcmdmgr(void)
{

}

int class_extcmdmgr::calc_newlines(const char* ptxt)
{
	int num = 0;

	class_lexer lexer("", "\n");
	lexer.lex_init(ptxt);
	std::string lex;

	while (!(lex=lexer.lex_next(0)).empty())
	{
		if (lex == "\n")
			num++;
	}
	return num;
}

int class_extcmdmgr::load(const char* pinp)
{
	int state = 0;
	std::string lex;

	std::string name, inf, par_name, par_val, cmd_text, cmd_desc, json_resp;

	std::vector<std::string> param_name;
	std::vector<std::string> param_val;

	if (pinp == NULL)
		return RC_EXTCMDMGR_PARAM_ERROR;

	m_line = 1;

	class_lexer lexer("():,\n=", " \t\r", "end format //");
	lexer.lex_init(pinp);

	// The finite state machine
	// -------------------------------------------
	//
	//  0 by [command] 				->		1
	//  0 by [other]				->		error
	//	
	//  1 by (						->		2
	//  1 by other					->		error
	//
	//  2 by name					->		3
	//	
	//  3 by :						->		4
	//  3 by other					->		error
	//	
	//  4 by CLI|H2					->		5
	//  4 by other					->		error
	//	
	//  5 by :						->		6 // The options
	//  5 by ,						->		7 // The list variables
	//  5 by ')'					->		1000 (done, the header is parsed)
	//  5 by other					->		error
	//	
	//  7 by name					->		8
	//  7 by other					->		error
	//	
	//  8 by ','					->		7
	//  8 by '='					->		9
	//  8 by ')'					->		1000 (done, the header is parsed)
	//  8 by other					->		error
	//	
	//  9 by 'value'				->		10
	//  9 by other					->		error
	//
	//  10 by ','					->		11
	//  10 by ')'					->		1000 (done, the header is parsed)
	//  10 by other					->		error
	//
	//  11 by '='					->		12
	//  11 by other					->		error
	//
	//  12 by 'value'				->		10
	//  12 by other					->		error
	//
	//  1000 by [text]				->		1001
	//	1001 by [json_resp]			->		1002
	//  1001 by [end]				->		0 [done]
	//	1002 by [format]			->		1001

	while (!(lex = lexer.lex_next(0)).empty())
	{
		//printf("state:%d, lex:%s\n", state, lex.c_str());
		if (lex == "\n")
		{
			m_line ++;
			continue;
		}

		if (lex == "//")
		{
			lexer.set_delims((const char*)"\n");
			lexer.set_lexems((const char*)"");
			
			cmd_desc += lexer.lex_next(0);

			lexer.set_delims((const char*)" \t\r");
			lexer.set_lexems((const char*)"():,=\n");
			continue;
		}
		
		switch (state)
		{
			case 0:
				if (lex == "command")
				{
					state = 1;
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected 'command' word\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 1:
				if (lex == "(")
				{
					state = 2;
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected '(' word\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 2:
				if (find_by_name(lex.c_str()) >= 0)
				{
					PRN_ERROR("Ext command error: line:%d, there is the name duplication:%s\n", m_line, lex.c_str());
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
				
				name = lex;
				state = 3;
			break;

			case 3:
				if (lex == ":")
				{
					state = 4;
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected ':' word\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 4:
				if (lex == "CLI" || lex == "cli")
				{
					inf = "cli";
					state = 5;
				}
				else if (lex == "H2" || lex == "h2")
				{
					inf = "h2";
					state = 5;
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected 'cli or h2' word\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 5:
				if (lex == ":")
				{
					state = 6;		// To load the options
				}
				else if (lex == ",")
				{
					state = 7;		// To load the variables or end of the command header
				}
				else if (lex == ")")
				{
					state = 1000;
					lexer.set_delims("");
					lexer.set_lexems("");
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected ':' or ',' word\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 6:
				if (lex == ":")
				{
					state = 7;		// To load the options
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected ':' word as options separator\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
					
				}
			break;

			case 7:
				par_name = lex;
				state = 8;
			break;

			case 8:
				if (lex == ",")
				{
					state = 7;
					par_val = "";
					if (is_name_used(par_name.c_str(), param_name))
					{
						PRN_ERROR("Ext command format error: line:%d, the parameter[%s] is duplicated\n", m_line, par_name.c_str());
						return RC_EXTCMDMGR_PARSER_ERROR;
					}
					param_name.push_back(par_name);
					param_val.push_back(par_val);

					par_name = "";
					par_val  = "";
				}
				else if (lex == "=")
				{
					state = 9;
				}
				else if (lex == ")")
				{
					state = 1000;
					par_val = "";
					if (is_name_used(par_name.c_str(), param_name))
					{
						PRN_ERROR("Ext command format error: line:%d, the parameter[%s] is duplicated\n", m_line, par_name.c_str());
						return RC_EXTCMDMGR_PARSER_ERROR;
					}
					param_name.push_back(par_name);
					param_val.push_back(par_val);

					par_name = "";
					par_val  = "";

					lexer.set_delims("");
					lexer.set_lexems("");
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected ',' or '=' word for the parameter definition\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 9:
				par_val = lex;
				if (is_name_used(par_name.c_str(), param_name))
				{
					PRN_ERROR("Ext command format error: line:%d, the parameter[%s] is duplicated\n", m_line, par_name.c_str());
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
				param_name.push_back(par_name);
				param_val.push_back(par_val);

				par_name = "";
				par_val  = "";
				state = 10;
			break;

			case 10:
				if (lex == ",")
				{
					state = 11;
				}
				else if (lex == ")")
				{
					state = 1000;
					lexer.set_delims("");
					lexer.set_lexems("");
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected ',' or ')' word for the parameters definition\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 11:
				par_name = lex;
				state = 12;
			break;

			case 12:
				if (lex == "=")
				{
					state = 14;
				}
				else
				{
					PRN_ERROR("Ext command format error: line:%d, expected '=' word for the parameters definition\n", m_line);
					return RC_EXTCMDMGR_PARSER_ERROR;
				}
			break;

			case 14:
				par_val = lex;
				state = 10;

				if (is_name_used(par_name.c_str(), param_name))
				{
					PRN_ERROR("Ext command format error: line:%d, the parameter[%s] is duplicated\n", m_line, par_name.c_str());
					return RC_EXTCMDMGR_PARSER_ERROR;
				}

				param_name.push_back(par_name);
				param_val.push_back(par_val);

				par_name = "";
				par_val  = "";
			break;

			case 1000:
				cmd_text = lex;
				lexer.set_delims((const char*)" \t\r");
				lexer.set_lexems((const char*)"():,=\n");
				state = 1001;
				m_line += calc_newlines(cmd_text.c_str());
			break;

			case 1001:
			{
				if (lex == "format")
				{
					lexer.set_delims((const char*)"");
					lexer.set_lexems((const char*)"");
					state = 1002;
					break;
				}
				else if (lex != "end")
				{
					PRN_ERROR("Ext command format error: line:%d, expected 'end' word, but have:%s\n", m_line, lex.c_str());
					return RC_EXTCMDMGR_PARSER_ERROR;
				}

				class_ext_cmd* pcmd = new class_ext_cmd();
				if (pcmd == NULL)
				{
					PRN_ERROR("Ext command: command object creation error\n");
					return RC_EXTCMDMGR_ALLOC_ERROR;
				}

				pcmd->set_name(name.c_str());
				pcmd->set_interface(inf == "cli" ? EXT_CMD_CLI : EXT_CMD_H2);
				pcmd->set_text(cmd_text.c_str());
				pcmd->set_descr(cmd_desc.c_str());
				pcmd->set_json_resp_format(json_resp.c_str());

				std::vector<std::string>::iterator iter;
				int idx;
				for (idx = 0,iter=param_name.begin(); iter < param_name.end(); ++iter, ++idx)
				{
					cmd_param param;

					param.m_name 	= *iter;
					param.m_defval 	= param_val[idx];
					pcmd->add_param(param);
				}

				m_cmds.push_back(pcmd);

				param_name.clear();
				param_val.clear();

				cmd_desc = "";
				
				state = 0;
			}
			break;

			case 1002:
				lexer.set_delims((const char*)" \t\r");
				lexer.set_lexems((const char*)"():,=\n");
				json_resp = lex;
				state = 1001;
			break;
		}
	}

	if (state != 0)
	{
		PRN_ERROR("Ext command format error: line:%d, unexpected text termination\n", m_line);
		return RC_EXTCMDMGR_PARSER_ERROR;
	}

	return RC_OK;
}

int class_extcmdmgr::load_from_file(const char* filename)
{
	int res = RC_OK;
	class class_file file(filename);
	if (!file.is_open())
		return RC_EXTCMDMGR_NO_FILE;

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

int class_extcmdmgr::get_num()
{
	return m_cmds.size();
}

int class_extcmdmgr::find_by_name(const char* name)
{
	std::vector<class_ext_cmd*>::iterator iter;
	int idx;
	for (idx = 0, iter=m_cmds.begin(); iter<m_cmds.end(); ++iter, ++idx)
	{
		if (strcmp((*iter)->get_name(), name) == 0)
			return idx;
	}

	return -1;
}

class_ext_cmd* class_extcmdmgr::operator[](int idx)
{
	if ((unsigned int)idx >= m_cmds.size())
		return NULL;

	return m_cmds[idx];
}

int class_extcmdmgr::is_name_used(const char* name, std::vector<std::string>&names)
{
	std::vector<std::string>::iterator iter;

	for (iter=names.begin(); iter < names.end(); ++iter)
	{
		if (*iter == name)
			return 1;
	}

	return 0;
}