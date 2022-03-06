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

#ifndef _LEXER_H_
#define _LEXER_H_

#include "retcodes.h"
#include <string>
#include <vector>

class class_lexer
{
public:
    class_lexer();
    class_lexer(const char* plexems, const char* spaces, const char* multiple_lexs = "");
    ~class_lexer();

    int             lex_init(const char* ptext);
    int             lex_init(const char* ptext, const char* lex, const char* spaces);
    int             set_delims(const char* symb);
    int             set_lexems(const char* symb);
    std::string     lex_next(int separate_str = 1);
    std::string     lex_rest_text(const char* b_delim="", const char* e_delim="");
    int             is_end(void);
   
private:
    public:
    int             lex_load_str(char end_sym);
    int             is_in(char symb, const char* txt);
    int             is_mul_lex(int pos, std::string &mlex);
    int             init_text(const char* text);
    int             init_symbols(const char* lex, const char* spaces);
    void            free_symbols(void);

    std::string     m_delims;       // The array of the symbols that are treated like spacebars
    std::string     m_lexems;       // The array of the symbols that are lexem by themself
    std::vector<std::string> m_mul_lex;   // The array of the multiple lexems delimited with any symbol from the delimiters

    int             m_possition;    // The index in the text pointed by the m_text;
    const char*     m_text;         // The text that needs to be parsed
    int             m_text_len;     // The text length in bytes
};

#endif //_LEXER_H_