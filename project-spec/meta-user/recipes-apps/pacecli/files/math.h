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

#ifndef _MATH_H_
#define _MATH_H_

#include <vector>
#include <string>

class class_varmgr;

typedef int (*math_user_proc)(int id, std::string proc, void* mgr, long long& val_res);

class class_math
{
public:
                                    class_math(class_varmgr* pvars=NULL, math_user_proc proc = NULL, void* proc_param = NULL);
                                    ~class_math(void);

                int                 calculate(const char* math, unsigned long long& result);
                
protected:

                int                 is_digit(const char* val);
                int                 is_var_ref(const char* pname);
                int                 is_reg_name(const char* pname, long long & addr);
                int                 is_operation(std::string& lex);
                int                 pop2val(long long&v1, long long& v2);
                int                 proc_operation(long long v1, long long v2, std::string opr, long long & res);
                int                 roll_back(void);
                int                 proc_unar_opr(void);
                int                 is_func(std::string&name);
                int                 proc_func(void);
private:

        class_varmgr*               m_pvars;
        math_user_proc              m_user_proc;
        void*                       m_user_proc_param;
    
        std::vector<long long>      m_val;      // The values
        std::vector<std::string>    m_opr;      // The operations: '+' '-' '/' '*' '<<' '>>'
};

#endif //_MATH_H_