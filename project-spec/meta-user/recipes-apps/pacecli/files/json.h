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

#ifndef _JSON_H_
#define _JSON_H_

#include "retcodes.h"
#include <string>
#include <vector>
#include "lexer.h"

enum json_obj_type
{
    JSON_NONE           =   0,
    JSON_OBJECT         =   1,
    JSON_ARRAY          =   2,
    JSON_PARAM          =   3,
};

class class_json
{
public:
                                class_json(void);
                                ~class_json(void);

                int             create(const char* json);

                json_obj_type   getobjtype(void);
                const char*     getobjname(void);

                std::string     print_obj(int content_only=0);
                std::string     getobjdata(const char* name, class_lexer* plexer = NULL);
                class_json*     findelmbyname(const char* objname);
                class_json*     findelmbyindex(unsigned int idx);
protected:

                int             is_ctrl_symb(std::string lex);

                int             parse_object(class_json* parent, std::string objname, class_lexer&lexer);
                int             parse_array(class_json* parent, std::string arrayname, class_lexer&lexer);
                int             parse_param(class_json* parent, std::string paramname, std::string paramval, class_lexer&lexer);
private:
    json_obj_type               m_objtype;

    std::string                 m_objname;
    std::string                 m_objdata;

    std::vector<class_json*>    m_subobjs;
};

#endif //_JSON_H_