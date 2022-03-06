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

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "path.h"
#include "lexer.h"

class_path::class_path()
{
	m_path = "";
}

class_path::class_path(const char* path)
{
	m_path = get_path(path);
}

class_path::class_path(std::string path)
{
	m_path = get_path(path.c_str());
}

class_path::class_path(const class_path& obj)
{
	m_path = obj.m_path;
}

class_path::~class_path(void)
{

}

class_path::operator const char*()
{
	return m_path.c_str();
}

class_path& class_path::operator =(const char* p)
{
	m_path = get_path(p);
	return *this;
}

class_path& class_path::operator =(const class_path& obj)
{
	m_path = obj.m_path;
	return *this;
}

class_path& class_path::operator =(std::string str)
{
	m_path = get_path(str);
	return *this;
}

class_path& class_path::operator +=(const char* p)
{
	if (strlen(p) == 0)
		return *this;

	if (is_last_slash())
	{
		m_path += p;
	}
	else
	{
		if (m_path.length())
			m_path += "/";
		m_path += p;
	}

	m_path = get_path(m_path);
	return *this;
}

class_path& class_path::operator +=(std::string& str)
{
	return (*this += str.c_str());
}

class_path& class_path::operator +=(class_path& obj)
{
	return (*this += obj.m_path);
}

class_path class_path::operator + (const char* p)
{
	class_path path;

	path = m_path;
	path += p;
	return path;
}

class_path class_path::operator +(std::string str)
{
	class_path path(*this);
	path += str;
	return path;
}

class_path class_path::operator + (class_path& obj)
{
	class_path path(*this);
	path += obj;
	return path;
}

int class_path::operator == (const char* p)
{
	std::string _p = get_path(p);
	return m_path == _p;
}

int class_path::operator == (std::string & str)
{
	return *this == str.c_str();
}

int class_path::operator == (class_path& obj)
{
	return *this == obj.m_path;
}

std::string class_path::get_path(void)
{
	return get_path(m_path.c_str());
}

/** @brief This function takes the path: name1/name2//name3/../name4/./name5
				and converts it to the:  name1/name2/name4/name5

	@param p [in] - the pointer to the ASCIIZ string

	@return [std::string] result*/

std::string class_path::get_path(const char* p)
{
	std::vector<std::string>list;
	int rc = path2list(p, list);

	// error in the text
	if (rc < 0)
		return "";

	if (list.size() == 0)
		return "";

	// here is to connect all the elements
	std::string res;
	for (unsigned int i = 0; i < list.size(); i++)
	{
		if (i)
		{
			res += "/";
		}
		res += list[i];
	}
	return res;
}

std::string class_path::get_path(std::string p)
{
	return get_path(p.c_str());
}

int class_path::set_folder_path(const char* p)
{
	*this = p;
	if (m_path.length() != 0)
	{
		if (m_path[m_path.length() - 1] != '/')
			m_path += "/";
	}
	return 0;
}

int class_path::set_folder_path(std::string p)
{
	return set_folder_path(p.c_str());
}

int class_path::set_folder(void)
{
	if (m_path.length() != 0)
	{
		if (m_path[m_path.length() - 1] != '/')
			m_path += "/";
	}
	return 0;
}

int class_path::split(std::string& path, std::string& name)
{
	// to find the last one element
	if (m_path.length() == 0)
	{
		path = "";
		name = "";
		return 0;
	}

	std::vector<std::string>list;
	int rc = path2list(m_path.c_str(), list);

	// error in the text
	if (rc < 0)
		return rc;

	if (list.size() == 0)
		return -2;

	path = "";
	name = list[list.size() - 1];
	for (unsigned int i = 0; i < list.size()-1; i++)
	{
		if (i)
			path += "/";
		path += list[i];
	}

	if (path != "")
		path += "/";

	return 0;
}

int class_path::split(std::string& full_path, std::string& path, std::string& name, int proc_par_folder)
{
	// to find the last one element
	if (full_path.length() == 0)
	{
		path = "";
		name = "";
		return 0;
	}

	std::vector<std::string>list;
	int rc = path2list(full_path.c_str(), list, proc_par_folder);

	// error in the text
	if (rc < 0)
		return rc;

	if (list.size() == 0)
		return -2;

	path = "";
	name = list[list.size() - 1];
	for (unsigned int i = 0; i < list.size()-1; i++)
	{
		if (i)
			path += "/";
		path += list[i];
	}

	if (path != "")
		path += "/";

	return 0;
}

int class_path::is_slash_started(void)
{
	if (m_path.length() == 0)
		return 0;

	return m_path[0] == '/';
}

int class_path::is_slash_ended(void)
{
	if (m_path.length() == 0)
		return 0;

	return m_path[m_path.length()-1] == '/';
}

int class_path::is_dir(void)
{
	return class_path::is_dir(m_path.c_str());
}

/** @brief The NFS does not provide us with the information about folders
			(dir_elm->d_type == 0 always!!!)
			So, we need to have such workaround
*/
int class_path::is_dir(const char* path)
{
	char cur_pwd[PATH_MAX];
	if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
		return 1;

	if (getcwd(cur_pwd, sizeof(cur_pwd)) == NULL)
		return 0;

	// to try to change the directory
	if (chdir(path) < 0)
		return 0;

	chdir(cur_pwd);
	return 1;
}

int class_path::reset(void)
{
	m_path = "";
	return RC_OK;
}

int class_path::is_last_slash(void)
{
	if (m_path.length() == 0)
		return 0;

	return m_path[m_path.length() - 1] == '/';
}

int class_path::path2list(const char*p, std::vector<std::string>&list, int proc_par_folder)
{
	list.clear();

	if (p == NULL)
		return 0;

	if (p[0] == 0)
		return 0;

	std::string lex;
	std::string name;
	class class_lexer lexer("/", "", "..");
	lexer.lex_init(p);

	int slash;
	int first_slash = 1;
	while (!(lex = lexer.lex_next()).empty())
	{
		slash = 0;
		if (lex == "/")
		{
			slash = 1;
			if (list.size() == 0 && first_slash)
			{
				list.push_back("");
			}
			continue;
		}

		first_slash = 0;

		if (proc_par_folder)
		{
			if (lex == ".")
				continue;

			if (lex == "..")
			{
				if (list.size())
				{
					list.erase(list.end() - 1);
					continue;
				}
				else
				{
					// error in the path
					return -1;
				}
				continue;
			}
		}
		list.push_back(lex);
	}

	if (slash == 1)
	{
		if (list.size())
			list[list.size()-1] += "/";
	}
	return 1;
}
