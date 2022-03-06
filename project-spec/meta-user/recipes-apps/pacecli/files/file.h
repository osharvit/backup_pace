/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    The class that implements file API and simplifies file data reading
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

#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <string>

class class_file
{
public:

                class_file(void);
                class_file(const char* filename, const char* mode = "r");
                ~class_file(void);

                int open(const char* filename, const char* mode = "r");
                void close(void); 
                int readfile(void* &ptr);
                int length();
                int is_open(void);

                int write(void*p, int len);
                int read(void*p, int len);

                std::string read_to_str(void);

protected:
                FILE*       m_file;
};

#endif //_FILE_H_

