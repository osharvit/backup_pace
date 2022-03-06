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
#include "lexer.h"

class_lexer::class_lexer()
{
	m_delims = "";
	m_lexems = "";

	m_text = NULL;
	m_text_len = 0;

	m_possition = 0;
}

class_lexer::class_lexer(const char* plexems, const char* delim, const char* multiple_lexs)
{
	m_text = NULL;
	m_text_len = 0;
	m_possition = 0;

	m_lexems = (plexems != NULL) ? plexems : "";
	m_delims = (delim != NULL) ? delim : "";

	// to split the text from multiple_lexs
	// into the array of the lexems

	if (multiple_lexs != NULL)
	{
		char ch;
		int i = 0;
		std::string lex;
		while ((ch = multiple_lexs[i]) != 0)
		{
			if (is_in(ch, delim))
			{
				if (lex.size())
				{
					m_mul_lex.push_back(lex);
					lex = "";
				}
			}
			else
			{
				lex += ch;
			}
			i++;
		}

		if (lex.size())
		{
			m_mul_lex.push_back(lex);
			lex = "";
		}
	}
}

class_lexer::~class_lexer()
{
	free_symbols();

	m_text = NULL;
	m_text_len = 0;
}

int class_lexer::is_in(char symb, const char* txt)
{
	if (txt == NULL)
		return 0;

	while (*txt != 0)
	{
		if (*txt == symb)
			return 1;

		txt++;
	}

	return 0;
}

int class_lexer::is_mul_lex(int pos, std::string &mlex)
{
	int mlex_pos = 0;
	int offs = pos;

	//printf("is_mul_lex (offs:%d)\n", offs, m_mul_lex.size());

	for (unsigned int i = 0; i < m_mul_lex.size();i++)
	{
		const char* lextxt = m_mul_lex[i].c_str();
		offs = pos;
		mlex_pos = 0;

		while (offs < m_text_len && lextxt[mlex_pos] != 0)
		{
			if (m_text[offs] == lextxt[mlex_pos])
			{
				offs ++;
				mlex_pos++;
				continue;
			}
			break;
		}

		if (lextxt[mlex_pos] == '\0')
		{
			mlex = lextxt;
			return 1;
		}
	}

	return 0;
}

int class_lexer::init_text(const char* text)
{
	m_text = NULL;
	m_text_len = 0;
	m_possition = 0;

	if (text == NULL)
		return RC_LEXER_INIT_TEXT_ERROR;

	m_text = text;
	m_text_len = strlen(text);

	return RC_OK;
}

int class_lexer::init_symbols(const char* plexems, const char* delim)
{
	free_symbols();

	m_lexems = (plexems != NULL) ? plexems : "";
	m_delims = (delim != NULL) ? delim : "";

	return RC_OK;
}

void class_lexer::free_symbols(void)
{
}

int class_lexer::lex_init(const char* ptext)
{
	if (ptext == NULL)
		return RC_LEXER_PARAM_ERROR;

	m_text = ptext;
	m_text_len = strlen(ptext);
	m_possition = 0;

	return 0;
}

int class_lexer::lex_init(const char* ptext, const char* lex, const char* spaces)
{
	int r;
	if ((r = init_text(ptext) < 0))
		return r;

	if ((r = init_symbols(lex, spaces)) < 0)
		return r;

	return RC_OK;
}

int class_lexer::lex_load_str(char end_sym)
{
    char sym;
    int string_len = 0;
    while (m_text != NULL)
    {
        sym = m_text[m_possition];

        if (sym == '\0')
            break;

        if (sym == end_sym)
        {
            m_possition++;
            break;
        }

        string_len++;
        m_possition++;
    }
    return string_len;
}

int class_lexer::set_delims(const char* symb)
{
	m_delims = symb;
	return RC_OK;
}

int class_lexer::set_lexems(const char* symb)
{
	m_lexems = symb;
	return RC_OK;
}

std::string class_lexer::lex_next(int separate_str)
{
	int begin_index = -1;
	int lex_size = 0;
	char sym;
	int str_ctx = 0;
	std::string lex = "";
	std::string mlex = "";

	if (m_text == NULL || m_text_len == 0 || m_possition >= m_text_len)
		return lex;

	while (m_possition < m_text_len)
	{
	    sym = m_text[m_possition];

	    if (is_in(sym, m_delims.c_str()) && !str_ctx)
	    {
		if (lex_size == 0)
		{
		    m_possition ++;
		    continue;
		}

		lex.insert(0, m_text, begin_index, lex_size);
			lex_size = 0;
		break;
	    }

	    if (is_mul_lex(m_possition, mlex))
	    {
		if (lex_size == 0)
		{
		    lex = mlex;
		    lex_size = 0;
		    m_possition += mlex.size();
		    break;
		}

		lex.insert(0, m_text, begin_index, lex_size);
			lex_size = 0;
			break;
	    }

	    if (is_in(sym, m_lexems.c_str()) && !str_ctx)
	    {
		if (lex_size == 0)
		{
		    lex.insert(0, m_text, m_possition, 1);
		    lex_size = 0;
		    m_possition++;
		    break;
		}

		lex.insert(0, m_text, begin_index, lex_size);
			lex_size = 0;
			break;
	    }

	    // if this is string of type:  " ... "
	    // -----------------------------------
	    if (sym == '"')
	    {
		if (separate_str)
		{
		    if (lex_size == 0)
	            {
			begin_index = ++m_possition;
			lex_size = lex_load_str('"');
			lex.insert(0, m_text, begin_index, lex_size);
			lex_size = 0;
	            }
	            else
	            {
	                lex.insert(0, m_text, begin_index, lex_size);
					lex_size = 0;
	            }
	            break;
		}
		else
		{
		    str_ctx ^= 1;
		}
	    }

	    if (begin_index == -1)
	    {
		begin_index = m_possition;
		lex_size = 0;
	    }
	    m_possition++;
	    lex_size++;
	}

	if (lex_size != 0)
	{
		lex.insert(0, m_text, begin_index, lex_size);
	}
	return lex;
}

std::string class_lexer::lex_rest_text(const char* b_delim, const char* e_delim)
{
	int size_limit = 0, i;
	char sym;
	std::string lex = "";

	while (m_possition < m_text_len)
	{
	    sym = m_text[m_possition];

	    if (is_in(sym, b_delim))
	    {
		m_possition++;
		continue;
	    }
	    break;
	}

	i = m_text_len - 1;
	while (i >= 0)
	{
	    sym = m_text[i];

	    if (is_in(sym, e_delim))
	    {
		i--;
		size_limit++;
		continue;
	    }
	    break;
	}

	while (m_possition < (m_text_len - size_limit))
	{
	    sym = m_text[m_possition++];
	    lex+= sym;
	}

	return lex;
}

int class_lexer::is_end(void)
{
	return m_possition >= m_text_len;
}