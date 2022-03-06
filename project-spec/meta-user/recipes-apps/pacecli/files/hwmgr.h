/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager is designed to keeps the information about some FPGA HW blocks
 *      and the registers of these blocks
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

#ifndef _HWMGR_H_
#define _HWMGR_H_

#include "retcodes.h"
#include <vector>
#include <string>

enum HW_DRV_ID
{
    HW_DRV_ID_REG_MEMORY        =   0,
    HW_DRV_ID_REG_AXIS_FIFO     =   1
};

struct hw_reg
{
    void reset(void)
    {
        m_regname = "";
        m_regoffs = 0;
        m_regbits = 32;
    }

    std::string             m_regname;
    unsigned int            m_regoffs;
    unsigned int            m_regbits;          // 8,16,32,64 bits
    std::string             m_desc;
};

struct hw_block
{
    void  reset(void)
    {
        m_base = 0;
        m_size = 0;
        m_version = "0.0";
        m_name = "";
        m_descr = "";
        m_drvid = 0;
        m_regs.clear();
        m_filename = "";
    }

    int find_reg_by_name(const char* reg_name)
    {
        int idx;
        std::vector<hw_reg>::iterator i;
        for (idx = 0, i = m_regs.begin(); i < m_regs.end(); ++i, ++idx)
        {
            if (i->m_regname == reg_name)
                return idx;
        }

        return -1;
    }

    unsigned long long get_reg_addr_by_name(const char* reg_name)
    {
        int idx;
        std::vector<hw_reg>::iterator i;
        for (idx = 0, i = m_regs.begin(); i < m_regs.end(); ++i, ++idx)
        {
            if (i->m_regname == reg_name)
                return m_base + i->m_regoffs;
        }

        return 0;
    }

    std::string get_base_as_text(void)
    {
        char buf[128];
        sprintf(buf, "0x%llx", m_base);
        return std::string(buf);
    }

    std::string get_reg_name(unsigned int reg_idx)
    {
        if (reg_idx < m_regs.size())
            return m_name + "." + m_regs[reg_idx].m_regname;

        return "";
    }

    std::string get_reg_addr_as_text(unsigned int reg_idx)
    {
        char buf[128];

        if (reg_idx >= m_regs.size())
            return "";
        
        sprintf(buf, "0x%llx", m_base+m_regs[reg_idx].m_regoffs);
        return std::string(buf);
    }

    unsigned long long      m_base;                 // The base address of the block (64bits address)
    unsigned int            m_size;                 // The memory size in bytes assigned to this block

    std::string             m_version;

    std::string             m_name;
    std::string             m_descr;
    unsigned int            m_drvid;                // Some driver ID, CLI-service should know this id

    mutable std::vector<hw_reg>     m_regs;

    std::string             m_filename;             // The name of file from which this component was loaded
};

class class_hwmgr
{
public:
                                class_hwmgr(void);
                                ~class_hwmgr(void);

        int                     load(const char* input, const char* filename = "");
        int                     load_from_file(const char* filename);

        int                     get_err_line(void){return m_line;}
        int                     get_num(void){return m_hwblocks.size();}


        int                     find_by_name(const char* hwname);
        int                     find_by_addr(unsigned long long addr);
        const hw_block&         get(unsigned int idx);
        const hw_block&         operator[](unsigned int idx);
        unsigned long long      get_addr_by_name(const char* fullregname);
        int                     split_hw_name(const char* fullregname, std::string& hwname, std::string& regname);
        int                     is_reg_name(const char* fullregname);
        unsigned int            get_drvid_by_name(const char* fullregname);
        unsigned int            get_drvid_by_addr(unsigned long long addr);

protected:

private:
        int                     m_line;         // The like with the syntax error
        std::vector<hw_block>   m_hwblocks;
};

#endif // _HWMGR_H_