/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This class handles regular Linux files/directories PATH
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

#ifndef _CLASS_PATH_H_
#define _CLASS_PATH_H_

#include <string>
#include <vector>

class class_path
{
public:
        class_path();
        class_path(const char* path);
        class_path(std::string path);
        class_path(const class_path& obj);

        ~class_path(void);

        operator const char*();
        class_path& operator =(const char* p);
        class_path& operator =(const class_path& obj);
        class_path& operator =(std::string str);

        class_path& operator +=(const char* p);
        class_path& operator +=(std::string& str);
        class_path& operator +=(class_path& obj);

        class_path operator + (const char* p);
        class_path operator +(std::string str);
        class_path operator + (class_path& obj);

        int operator == (const char* p);
        int operator == (std::string & str);
        int operator == (class_path& obj);

        std::string get_path();
        std::string get_path(const char* p);
        std::string get_path(std::string p);

        int set_folder_path(const char* p);
        int set_folder_path(std::string p);
        int set_folder(void);

        std::string str(void) {return m_path;};
        const char* c(void) {return m_path.c_str();};
        const char* c_str(void) {return m_path.c_str();};

        int         split(std::string& path, std::string&name);
        static int  split(std::string& full_path, std::string& path, std::string& name, int proc_par_folder = 1);

        int         is_slash_started(void);
        int         is_slash_ended(void);
        int         is_dir(void);
        static  int is_dir(const char* path);

        int         reset(void);

protected:

        int is_last_slash();
        static int path2list(const char*p, std::vector<std::string>&list, int proc_par_folder = 1);

private:

    std::string         m_path;
};


#endif // _CLASS_PATH_H_