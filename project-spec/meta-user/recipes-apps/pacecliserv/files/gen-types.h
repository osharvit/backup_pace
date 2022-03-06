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

#ifndef _GEN_TYPES_H_
#define _GEN_TYPES_H_

#include <limits.h>
#include <stdio.h>

#define VERSION                                     "0.01"
#define VERSION_HEX                                 (0<<8 | 1)

#define DEF_SERV_PORT                               50005

/*******************************************************************************
        The list if the Command Line IDs 
*******************************************************************************/
#define CLF_HELP                                    (1<<0)
#define CLF_CONSOLE_MODE                            (1<<1)
#define CLF_SERV_PORT                               (1<<2)
#define CLF_H2_EMUL                                 (1<<3)

struct CLF_PARAMS_TYPE
{
    unsigned int    flags;
    unsigned int    serv_port;
};

template <class T> class class_ptr
{
public:
    class_ptr(T pobj)
    {
        m_obj = pobj;
    }
    ~class_ptr(void)
    {
        if (m_obj != NULL)
            delete (m_obj);

        m_obj = NULL;
    }

    T operator= (const T & obj)
    {
        m_obj = obj;
        return m_obj;
    }

protected:
    T m_obj;
};

template <class T> class data_ptr
{
public:
    data_ptr(void)
    {
        m_obj = NULL;
    }
    data_ptr(const T pobj)
    {
        m_obj = pobj;
    }
    ~data_ptr(void)
    {
        if (m_obj != NULL)
        {
            //free (m_obj);
        }

        m_obj = NULL;
    }

    T operator= (const T & obj)
    {
        m_obj = obj;
        return m_obj;
    }

protected:
    T m_obj;
};

#define PRN_ERROR(...)    printf(__VA_ARGS__)
#define PRN_INFO(...)     printf(__VA_ARGS__) 

#endif // _GEN_TYPES_H_

