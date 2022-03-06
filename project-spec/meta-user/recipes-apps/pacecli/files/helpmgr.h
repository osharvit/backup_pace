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

#ifndef _HELP_MGR_H_
#define _HELP_MGR_H_

#include <string>

class class_helpmgr
{
public:
                class_helpmgr(void);
                ~class_helpmgr(void);

                static void show_commad_info(int cmd_id);
                static void show_commad_list(void);

                static std::string to_string(int val, int in_hex = 0);
                static std::string to_string(unsigned int val, int in_hex = 0);
                static std::string to_string(long val, int in_hex = 0);
                static std::string to_string(unsigned long val, int in_hex = 0);
                static std::string to_string(long long val, int in_hex = 0);
                static std::string to_string(unsigned long long val, int in_hex = 0);
                static long long   to_num(const char* data);
protected:

private:
    
};


#endif //_HELP_MGR_H_

