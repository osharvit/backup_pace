/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    To caclulate the mathematic operations
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

#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "math.h"
#include "retcodes.h"
#include "lexer.h"
#include "varmgr.h"

class_math::class_math(class_varmgr* pvars, math_user_proc proc, void* proc_param)
{
	m_pvars = pvars;
	m_user_proc = proc;
	m_user_proc_param = proc_param;
}

class_math::~class_math(void)
{
	m_pvars = NULL;
}

int class_math::is_digit(const char* val)
{
	if (val == NULL)
		return 0;

	// The finite state machine
	// ---------------------------
	// 0 by [0]		->	1
	// 0 by [NUM]	->	2
	// 1 by [x]		->  3
	// 2 by [NUM]	->	2
	// 2 by [eof]	->  [done], this is a DEC digit
	// 3|4 by [NUM]	->	4
	// 3|4 by [A...F]	->	4
	// 4 by [eof]	->  [done], this is HEX digit

	int state = 0;

	int i = 0;
	char ch;
	while ((ch = val[i++]) != 0)
	{
		switch (state)
		{
			case 0:
				if (ch == '0')
				{
					state = 1;
				}
				else if(isdigit(ch))
				{
					state = 2;
				}
				else
				{
					return 0;
				}
				break;

			case 1:
				if (ch == 'x')
				{
					state = 3;
				}
				else
				{
					return 0;
				}
				break;

			case 2:
				if (isdigit(ch))
				{
					state = 2;
				}
				else
				{
					return 0;
				}
				break;

			case 3:
			case 4:
				if (isdigit(ch)||(ch>='A'&&ch<='F')||(ch>='a'&&ch<='f'))
				{
					state = 4;
				}
				else
				{
					return 0;
				}
				break;

			default:
				return 0;
		}
	}

	if (state == 1)
		return 1;
	if (state == 2)
		return 1;
	if (state == 4)
		return 2;
	return 0;
}

int class_math::is_var_ref(const char* pname)
{
	if (pname == NULL)
		return 0;

	if (pname[0] != '$')
		return 0;

	if (pname[1] == 0)
		return 0;

	int i = 1;
	char ch;
	while ( (ch=pname [i]) != 0)
	{
		if (isalpha(ch) || ch == '_')
		{
			i++;
			continue;
		}

		if (i > 1)
		{
			if (isdigit(ch))
			{
				i++;
				continue;		
			}
		}
		return 0;
	}

	return 1;
}

int class_math::is_reg_name(const char* pname, long long & addr)
{
	if (m_user_proc == NULL)
		return 0;

	int res = m_user_proc(2, pname, m_user_proc_param, addr);
	return (res == RC_OK) ? 1 : 0;
}

int class_math::is_operation(std::string& lex)
{
	if (lex == "|")
		return 5;

	if (lex == "^")
		return 6;

	if (lex == "&")
		return 7;

	if (lex == "<<" || lex == ">>")
		return 8;

	if (lex == "+"||lex == "-")
		return 9;

	if (lex == "*"||lex == "/")
		return 10;

	if (lex == "~")
		return 11;

	return 0;
}

int class_math::pop2val(long long&v1, long long& v2)
{
	if (m_val.size()< 2)
		return RC_MATH_FORMAT_ERROR;

	v2 = m_val.back(); m_val.pop_back();
	v1 = m_val.back(); m_val.pop_back();
	return RC_OK;
}

int class_math::proc_operation(long long v1, long long v2, std::string opr, long long& res)
{
	if (opr == "+")
	{
		res = v1+v2;
	}
	else if (opr == "-")
	{
		res = v1-v2;
	}
	else if (opr == "*")
	{
		res = v1*v2;
	}
	else if (opr == "/")
	{
		res = v1/v2;
	}
	else if (opr == "|")
	{
		res = v1|v2;
	}
	else if (opr == "&")
	{
		res = v1&v2;
	}
	else if (opr == "^")
	{
		res = v1^v2;
	}
	else if (opr == "<<")
	{
		res = v1<<v2;
	}
	else if (opr == ">>")
	{
		res = v1>>v2;
	}
	else
		return RC_MATH_OPERATION_ERROR;
		
	return RC_OK;
}

int class_math::roll_back(void)
{
	std::string lex, sign;
	long long v1, v2;
	int r;

	while (1)
	{
		if (m_opr.size()==0)
			return RC_MATH_FORMAT_ERROR;

		sign = m_opr.back(); m_opr.pop_back();
		if (sign == "(")
			break;

		if ((r = pop2val(v1, v2)) < 0)
			return r;

		if ((r = proc_operation(v1,v2, sign, v1)) < 0)
			return r;
		m_val.push_back(v1);
	}

	return RC_OK;
}

int class_math::proc_unar_opr(void)
{
	long long val;
	std::string opr;
	while (m_opr.size())
	{
		opr =  m_opr.back();
		if (opr == "-U")
		{
			val = m_val.back(); m_val.pop_back();
			val = val*-1;
			m_val.push_back(val);
		}
		else if (opr == "+U")
		{

		}
		else if (opr == "~U")
		{
			val = m_val.back(); m_val.pop_back();
			val = ~val;
			m_val.push_back(val);
		}
		else
		{
			break;
		}
		m_opr.pop_back();
	}
	return RC_OK;
}

int class_math::is_func(std::string&name)
{
	if (name == "u64")
		return 1;
	else if (name == "u32")
		return 1;
	else if (name == "u16")
		return 1;
	else if (name == "u8")
		return 1;

	if (m_user_proc != NULL)
	{
		long long val = 0;
		return m_user_proc(0, name, NULL, val);
	}

	return 0;
}

int class_math::proc_func(void)
{
	long long val;
	std::string func;
	while (m_opr.size())
	{
		func =  m_opr.back();
		if (func == "u64")
		{
			val = m_val.back(); m_val.pop_back();
			val = (unsigned long long)val;
			m_val.push_back(val);
		}
		else if (func == "u32")
		{
			val = m_val.back(); m_val.pop_back();
			val = (unsigned int)val;
			m_val.push_back(val);
		}
		else if (func == "u16")
		{
			val = m_val.back(); m_val.pop_back();
			val = (unsigned short)val;
			m_val.push_back(val);
		}
		else if (func == "u8")
		{
			val = m_val.back(); m_val.pop_back();
			val = (unsigned char)val;
			m_val.push_back(val);
		}
		else if (m_user_proc!=NULL)
		{
			if (!m_user_proc(0, func, m_user_proc_param, val))
				break;
		
			val = m_val.back();
			int res = m_user_proc(1, func, m_user_proc_param, val);
			if (res != RC_OK)
				return res;

			m_val.pop_back();
			m_val.push_back(val);
		}
		else
		{
			break;
		}
		m_opr.pop_back();
	}
	return RC_OK;
}

int class_math::calculate(const char* math, unsigned long long & result)
{
	int r;
	int priority;
	std::string lex, sign;
	int last_opr_prior = 0;
	long long v1, v2, addr;
	int unar_opr = 1;
	int exp_func = 0;

	class_lexer lexer("+-/*()|&^~", " \r\n\t", "<< >>");
	lexer.lex_init(math);
	while ( !(lex=lexer.lex_next()).empty() )
	{
		//printf("MATH-LEX:%s\n", lex.c_str());
		if (exp_func && lex != "(")
			return RC_MATH_FORMAT_ERROR;
	
		if (is_digit(lex.c_str())!= 0)
		{
			long long v = strtoull(lex.c_str(), (char**)0, 0);
			m_val.push_back(v);
			proc_unar_opr();
			unar_opr = 0;
		}
		else if (is_var_ref(lex.c_str()))
		{
			if (m_pvars == NULL)
				return RC_MATH_VARS_ACCESS_ERROR;

			int idx = m_pvars->find_var(lex.c_str()+1);
			if (idx < 0)
				return RC_MATH_VARS_ACCESS_ERROR;

			long long v = strtoull(m_pvars->get((unsigned int)idx).c_str(), (char**)0, 0);
			m_val.push_back(v);
			proc_unar_opr();
			unar_opr = 0;
		}
		else if (is_reg_name(lex.c_str(), addr))
		{
			m_val.push_back(addr);
			proc_unar_opr();
			unar_opr = 0;
		}
		else if (is_func(lex))
		{
			exp_func = 1;
			m_opr.push_back(lex);
		}
		else if (lex == "(")
		{
			exp_func = 0;
			last_opr_prior = 0;
			m_opr.push_back(lex);
		}
		else if (lex == ")")
		{
			// to roll back everything till the '('
			// ------------------------------------
			if ((r = roll_back()) < 0)
				return r;

			if ((r = proc_func()) < 0)
				return r;

			proc_unar_opr();
			if (m_opr.size())
				last_opr_prior = is_operation(m_opr.back());
			else
				last_opr_prior = 0;
		}
		else if ((priority = is_operation(lex)) != 0)
		{
			unar_opr++;

			if (unar_opr > 2)
				return RC_MATH_FORMAT_ERROR;

			if (unar_opr == 2)
			{
				m_opr.push_back(lex+"U");
			}
			else if (last_opr_prior < priority)
			{
				m_opr.push_back(lex);
			}
			else
			{
				if ((r = pop2val(v1, v2)) < 0)
					return r;

				if (m_opr.size () == 0)
					return RC_MATH_FORMAT_ERROR;
				
				proc_operation(v1,v2, m_opr.back(), v1);
				m_opr.pop_back();

				m_val.push_back(v1);
				m_opr.push_back(lex);
			}

			if (unar_opr<2)
				last_opr_prior = priority;
		}
		else
		{
			return -1;
		}
	}

	// no more data, let's complete processing stacks
	// ----------------------------------------------
	while (1)
	{
		if (m_val.size() == 1 && m_opr.size()==0)
		{
			result = m_val.back();
			m_val.pop_back();
			return RC_OK;
		}

		if (m_opr.size() == 0)
			return RC_MATH_FORMAT_ERROR;

		lex = m_opr.back();
		m_opr.pop_back();
		if ((r = pop2val(v1, v2)) < 0)
			return r;
		if ((r = proc_operation(v1,v2, lex, v1)) < 0)
			return r;
		m_val.push_back(v1);
	}
	return RC_OK;
}
