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

#ifndef _EXT_CMD_MGR_H_
#define _EXT_CMD_MGR_H_

#include <vector>
#include <string>
#include "cmd.h"

class class_extcmdmgr
{
public:
                                            class_extcmdmgr();
                                            ~class_extcmdmgr();


            int                             load(const char* pinp);
            int                             load_from_file(const char* filename);

            int                             get_err_line(void){ return m_line;}

            int                             get_num();
            int                             find_by_name(const char* name);

            class_ext_cmd*                  operator[](int idx);
protected:

            int                             calc_newlines(const char* ptxt);

private:

            int                             is_name_used(const char* name, std::vector<std::string>&names);

            int                             m_line;
            std::vector<class_ext_cmd*>     m_cmds;
};

#endif // _EXT_CMD_MGR_H_
