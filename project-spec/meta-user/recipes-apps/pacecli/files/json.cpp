/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class implements JSON parser, it represents the JSON text
 *  as the C++ object(s)
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
#include "gen-types.h"
#include "json.h"
#include "lexer.h"

class_json::class_json(void)
{
	m_objtype = JSON_NONE;
	m_objname = "";
}

class_json::~class_json(void)
{
	std::vector<class_json*>::iterator i;
	for (i = m_subobjs.begin(); i < m_subobjs.end(); i++)
	{
		delete (*i);
	}

	m_subobjs.clear();
}

int class_json::is_ctrl_symb(std::string lex)
{
	if (lex=="{"		||
		lex=="}"		||
		lex=="["		||
		lex=="]"		||
		lex==":"		||
		lex==",")
	{
		return 1;
	}
	return 0;
}

int class_json::create(const char* json)
{
	if (json == NULL)
		return RC_JSON_PARAM_ERROR;

	class_lexer lexer("{}:[],", " \r\n\t");
	std::string lex;
	lexer.lex_init(json);

	std::string	objname;
	std::string	objdata;

	lex = lexer.lex_next();
	if (lex != "{")
		return RC_JSON_FORMAT_ERROR;

	int res = parse_object(NULL, "", lexer);
	if (res < RC_OK)
		return res;

	//lex = lexer.lex_next();
	//if (lex != "}")
	//	return RC_JSON_FORMAT_ERROR;
	//printf("The parsing is done!\n");
	return RC_OK;
}

json_obj_type class_json::getobjtype(void)
{
	return m_objtype;
}

const char* class_json::getobjname(void)
{
	return m_objname.c_str();
}

class_json* class_json::findelmbyname(const char* objname)
{
	std::vector<class_json*>::iterator i;
	for (i = m_subobjs.begin(); i < m_subobjs.end(); i++)
	{
		if (objname[0] == '\0')
			return (*i);

		if ((*i)->m_objname == objname)
			return (*i);
	}

	return NULL;
}

class_json* class_json::findelmbyindex(unsigned int idx)
{
	if (idx >= m_subobjs.size())
		return NULL;

	return m_subobjs[idx];
}

std::string class_json::getobjdata(const char* name, class_lexer* plexer)
{
	class_json* pelm;

	if (name == NULL)
		return "";

	class_lexer lexer("[]", " \t\n\r.");
	std::string lex;

	if (plexer == NULL)
	{
		lexer.lex_init(name);
		plexer = &lexer;
	}

	lex = plexer->lex_next();

	if (lex == "")
		return print_obj();

	if (lex == "[")
	{
		lex = plexer->lex_next();
		unsigned int idx = (unsigned int)atoi(lex.c_str());
		lex = plexer->lex_next();
		if (lex != "]")
			return "";

		pelm = findelmbyindex(idx);
	}
	else
	{
		pelm = findelmbyname(lex.c_str());
	}
	if (pelm == NULL)
		return "";

	if (pelm->m_objtype == JSON_PARAM)
		return pelm->m_objdata;

	if (plexer->is_end())
	{
		return pelm->print_obj(1);
	}

	return pelm->getobjdata(name, plexer);
}

std::string class_json::print_obj(int content_only)
{
	std::string text;

	switch ((int)m_objtype)
	{
		case JSON_OBJECT:
		{
			if (m_objname != "" && content_only == 0)
			{
				text += "\"" + m_objname + "\":{";
			}
			else
			{
				text += "{";
			}
		}
		break;

		case JSON_ARRAY:
		{
			if (m_objname != "" && content_only == 0)
			{
				text += "\"" + m_objname + "\":[";
			}
			else
			{
				text += "[";
			}
		}
		break;
	}

	std::vector<class_json*>::iterator i;
	for (i = m_subobjs.begin(); i < m_subobjs.end(); i++)
	{
		if (i != m_subobjs.begin())
		{
			text += ", ";
		}

		switch ((int)(*i)->m_objtype)
		{
			case JSON_PARAM:
			{
				if ((*i)->m_objname != "")
				{
					text += "\"" + (*i)->m_objname + "\"";
					text += ":";
				}
				text += "\"" + (*i)->m_objdata + "\"";
			}
			break;

			case JSON_OBJECT:
			case JSON_ARRAY:
			{
				text += (*i)->print_obj();
			}
			break;
		}
	}

	switch ((int)m_objtype)
	{
		case JSON_OBJECT:
		{
			text += "}";
		}
		break;

		case JSON_ARRAY:
		{
			text += "]";
		}
		break;
	}

	return text;
}

int class_json::parse_object(class_json* parent, std::string objname, class_lexer&lexer)
{
	std::string lex, name;
	int state = 1;
	int res = RC_OK;
	int stop = 0;

	// 1	by 	name		->		2   // to set the object/array/param name
	// 2	by	:			->		3	// to prepare to read the type of the object
	// 2	by  {			->		100	// to parse the object
	// 2	by	[			->		100	// to parse the array
	// 2	by 	value		->		100	// to add parameter
	// 100	by  ,			->		1	// ready to parse the new element
	// 100  by  }			->		1	// STOP parsing this OBJECT

	while (!(lex=lexer.lex_next()).empty())
	{
		//PRN_INFO("OBJ-state:%d, lex: %s\n", state, lex.c_str());
		switch (state)
		{
			case 1:
			{
				if (lex == "{")
				{
					class_json* obj = new class_json();
					if (obj == NULL)
						return RC_JSON_ALLOC_OBJ_ERROR;
					if ((res = obj->parse_object(this, "", lexer)) < RC_OK)
					{
						delete obj;
						return res;
					}
					state = 100;
					m_subobjs.push_back(obj);
				}
				else if (lex == "[")
				{
					class_json* obj = new class_json();
					if (obj == NULL)
						return RC_JSON_ALLOC_OBJ_ERROR;
					if ((res = obj->parse_array(this, "", lexer)) < RC_OK)
					{
						delete obj;
						return res;
					}
					state = 100;
					m_subobjs.push_back(obj);
				}
				else if (lex == "}")
				{
					stop = 1;
					state = 101;
					break;
				}
				else
				{
					name = lex;
					state = 2;
				}
			}
			break;

			case 2:
			{
				if (lex != ":")
					return RC_JSON_FORMAT_ERROR;
				state = 3;
			}
			break;

			case 3:
			{
				class_json* obj = new class_json();
				if (obj == NULL)
					return RC_JSON_ALLOC_OBJ_ERROR;
				if (lex == "{")
				{
					res = obj->parse_object(this, name, lexer);
				}
				else if (lex == "[")
				{
					res = obj->parse_array(this, name, lexer);
				}
				else
				{
					res = RC_OK;
					obj->parse_param(this, name, lex, lexer);
				}

				if (res < RC_OK)
				{
					delete obj;
					return res;
				}
				state = 100;
				m_subobjs.push_back(obj);
			}
			break;

			case 100:
			{
				if (lex == "}")
				{
					state = 101;
					stop = 1;
				}
				else if (lex == ",")
					state = 1;
				else
					return RC_JSON_FORMAT_ERROR;
			}
			break;

			default:
				return RC_JSON_FORMAT_ERROR;
		}

		if (stop)
			break;
	}

	if (state != 101)
		return RC_JSON_FORMAT_ERROR;

	m_objtype = JSON_OBJECT;
	m_objname = objname;
	return RC_OK;
}

int class_json::parse_array(class_json* parent, std::string arrayname, class_lexer&lexer)
{
	std::string lex, name;
	int state = 1;
	int res = RC_OK;
	int stop = 0;

	// Example:
	// [ {name:value, name:value} , objname:{name:value}, [ val1, val2, arraname:[val1, val2] ], val1, val2 ]

	// 1	by 	name		->		2   	// to set the object/array/param name
	// 1	by  {			->		2		// to parse the unnamed object
	// 1	by	[			->		2		// to parse the unnamed array
	// 2	by  ]			->		100		// END of parsing
	// 2	by  ,			->		1		// to add the value to the array
	// 2    by  :			->		3		//
	// 3	by  {			->		4		// to parse the NAMED object
	// 3	by  [			->		4		// to parse the NAMED array

	while (!(lex=lexer.lex_next()).empty())
	{
		//PRN_INFO("ARRAY state:%d, lex:  %s\n", state, lex.c_str());
		switch (state)
		{
			case 1:
			{
				if (lex == "{" || lex == "[")
				{
					class_json* obj = new class_json();
					if (obj == NULL)
						return RC_JSON_ALLOC_OBJ_ERROR;
					if (lex == "{")
					{
						res = obj->parse_object(this, name, lexer);
					}
					else if (lex == "[")
					{
						res = obj->parse_array(this, name, lexer);
					}
					else
					{
						delete obj;
						return RC_JSON_FORMAT_ERROR;
					}

					if (res < RC_OK)
					{
						delete obj;
						return res;
					}
					m_subobjs.push_back(obj);
					state = 4;
				}
				else if (lex == "]")
				{
					state = 100;
					stop = 1;
				}
				else
				{
					name = lex;
					state = 2;
				}
			}
			break;

			case 2:
			{
				if (lex == "]")
				{
					if (name != "")
					{
						class_json* obj = new class_json();
						if (obj == NULL)
							return RC_JSON_ALLOC_OBJ_ERROR;
						int res = obj->parse_param(this, "", name, lexer);
						if (res < RC_OK)
						{
							delete obj;
							return res;
						}
						m_subobjs.push_back(obj);
					}
					stop = 1;
					state = 100;
				}
				else if (lex == ",")
				{
					class_json* obj = new class_json();
					if (obj == NULL)
						return RC_JSON_ALLOC_OBJ_ERROR;
					int res = obj->parse_param(this, "", name, lexer);
					if (res < RC_OK)
					{
						delete obj;
						return res;
					}
					m_subobjs.push_back(obj);
					state = 1;
					name = "";
				}
				else if (lex == ":")
				{
					state = 3;
				}
			}
			break;

			case 3:
			{
				class_json* obj = new class_json();
				if (obj == NULL)
					return RC_JSON_ALLOC_OBJ_ERROR;
				if (lex == "{")
				{
					res = obj->parse_object(this, name, lexer);
				}
				else if (lex == "[")
				{
					res = obj->parse_array(this, name, lexer);
				}
				else
				{
					delete obj;
					return RC_JSON_FORMAT_ERROR;
				}

				if (res < RC_OK)
				{
					delete obj;
					return res;
				}
				state = 4;
				name = "";
				m_subobjs.push_back(obj);
			}
			break;

			case 4:
			{
				if (lex == ",")
					state = 1;
				else if (lex == "]")
				{
					state = 100;
					stop = 1;
				}
				else
				{
					return RC_JSON_FORMAT_ERROR;
				}
			}
			break;

			default:
				return RC_JSON_FORMAT_ERROR;
		}

		if (stop)
			break;
	}

	if (state != 100)
		return RC_JSON_FORMAT_ERROR;

	m_objtype = JSON_ARRAY;
	m_objname = arrayname;
	return RC_OK;
}

int class_json::parse_param(class_json* parent, std::string paramname, std::string paramval, class_lexer&lexer)
{
	m_objtype = JSON_PARAM;
	m_objname = paramname;
	m_objdata = paramval;
	return RC_OK;
}
