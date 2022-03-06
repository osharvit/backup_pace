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

#ifndef _VARIABLES_H_
#define _VARIABLES_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "retcodes.h"

struct variable
{
    variable()
    {
        m_name = "";
        m_value = "";
    }

    variable(const char* name, const char*val, int system)
    {
        m_name = name;
        m_value = val;
        m_system = system;
    }

    void init(void)
    {
        m_name = "";
        m_value = "";
    }

    int setname(const char*name)
    {
        m_name = name;
        return RC_OK;
    }

    int setval(const char*val)
    {
        m_value = val;
        return RC_OK;
    }

    const char* get_val(void)
    {
        return m_name.c_str();
    }

    std::string m_name;
    std::string m_value;
    int         m_system;
};


class class_varmgr
{
public:
    class_varmgr(void);
    ~class_varmgr(void);

    int                 load(const char* pdata);
    int                 load_from_file(const char* filename);
    int                 save_to_file(const char* filename);
    int                 set(const char* varname, const char* value, int system = 0, int set_f = 0);
    int                 set(const variable & var, int uniqueue = 0);
    std::string         get(const char* varname);
    std::string         get(unsigned int idx);

    unsigned int        get_var_num();
    const variable&     get_var(unsigned int idx);

    int                 find_var(const char* name);

    int                 is_save_needed();
    int                 get_error_line();
    int                 is_system(const char* name);

    uint8_t             u8(const char* name);
    uint16_t            u16(const char* name);
    uint32_t            u32(const char* name);
    uint64_t            u64(const char* name);

    variable&           operator[](int idx);

protected:


private:

    int                         m_errorline;    // The number of the line where the error was detected
    std::vector<variable>       m_vars;         // The array of the variables
    int                         m_updated;      // The flag which tracks if the variables need to be saved
    std::string                 m_filename;
};


#endif //_VARIABLES_H_
