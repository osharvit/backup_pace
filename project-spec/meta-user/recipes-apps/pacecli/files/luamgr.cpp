/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class is a manager of the lua scripts, listing, running,
 *  exporting some C++ API into the LUA scripts, etc
 *  This class is used by the command manager class
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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "luamgr.h"
#include "file.h"
#include "lexer.h"
#include "path.h"

class_luamgr::class_luamgr(void)
{
	m_LuaH = NULL;
	m_lua_api = NULL;
}

class_luamgr::~class_luamgr(void)
{
	if (m_LuaH != NULL)
	{
		lua_close(m_LuaH);
		m_LuaH = NULL;
	}
}

int class_luamgr::init(CLF_PARAMS_TYPE* p)
{
	char cur_path[PATH_MAX];
	if (getcwd(cur_path,sizeof(cur_path)) == NULL)
		return RC_LUAMGR_CUR_DIR_ERROR;

	m_scr_root_folder = cur_path;
	m_scr_root_folder += CLI_LUA_SCRIPTS_FOLDER_NAME;

	m_LuaH = luaL_newstate();
	if (m_LuaH == NULL)
		return RC_LUAMGR_INIT_ERROR;
	luaL_openlibs(m_LuaH);

	m_lua_api = p->lua_api;

	int i = 0;
	while(p->lua_api[i].name != NULL)
	{
		lua_register(m_LuaH, p->lua_api[i].name, (int(*)(lua_State*))p->lua_api[i].proc);
		i++;
	}

	return RC_OK;
}

int class_luamgr::load_scripts(const char* scr_folder)
{
	DIR *d;
	struct dirent *dir_elm;
	int r = RC_OK;

	class_path dir_path = m_scr_root_folder;
	dir_path += scr_folder;

	// To scan optional folder with the set of files
	// ---------------------------------------------
	if ((d = opendir((const char*)dir_path)) != NULL)
	{
	    while ((dir_elm = readdir(d)) != NULL)
	    {
		std::string name;
		name = dir_elm->d_name;

		// -------------------------------------------------------------
		//if (dir_elm->d_type != DT_DIR && name != "." && name != "..")
		// -------------------------------------------------------------
		if (!class_path::is_dir((const char*)(dir_path+name)))
		{
				r = add_script((const char*)(dir_path + dir_elm->d_name));
				if (r < 0)
				{
					PRN_ERROR("An error(%d) to add LUA script :%s\n", r, (const char*)(dir_path + dir_elm->d_name));
					break;
				}
		}
		// if this is a directory
		else if (name != "." && name != "..")
		{
			class_path sub_path(scr_folder);
			sub_path+= name;
			r = load_scripts(sub_path.c());
			if (r < RC_OK)
				break;
		}
	    }
	    closedir(d);
	}
	return r;
}

std::string class_luamgr::scr_opt2text(lua_scr_opt opt)
{
	std::string str = "";

	if (opt.m_options & lua_scr_opt::LUA_SCR_OPT_CONTINUED)
	{
		str += "[cont]";
	}

	if (opt.m_options & lua_scr_opt::LUA_SCR_OPT_AUTO)
	{
		str += "[auto]";
	}

	return str;
}

std::string class_luamgr::find_rel_path(const char* scr_path)
{
	std::string rel_path = scr_path;

	// to exclude the root path to the scripts
	if (rel_path.find((const char*)m_scr_root_folder) == 0)
	{
		rel_path = scr_path + strlen((const char*)m_scr_root_folder);
	}
	return rel_path;
}

/** @brief This function adds information about new script to the script list
	@param filename[in]	- the full path to the script

	@return [int] error code*/
int class_luamgr::add_script(const char* filename)
{
	lua_script_info scr;
	lua_scr_opt opt;

	class_path path(filename);
	std::string file_path;
	std::string file_name;
	path.split(file_path, file_name);

	scr.m_path    = file_path;
	scr.m_rel_path= find_rel_path(file_path.c_str());
	scr.m_name 	  = file_name;
	scr.m_descr   = load_script_description(filename, opt);
	scr.m_textopt = scr_opt2text(opt);
	scr.m_options = opt.m_options;

	if (opt.m_options & lua_scr_opt::LUA_SCR_OPT_CONTINUED)
	{
		scr.m_LuaState = luaL_newstate();
		if (scr.m_LuaState == NULL)
			return RC_LUAMGR_ALLOC_STATE_ERROR;
		luaL_openlibs(scr.m_LuaState);
		int i = 0;
		if (m_lua_api != NULL)
		{
			while(m_lua_api[i].name != NULL)
			{
				lua_register(scr.m_LuaState, m_lua_api[i].name, (int(*)(lua_State*))m_lua_api[i].proc);
				i++;
			}
		}
	}
	m_scripts.push_back(scr);
	return RC_OK;
}

unsigned int class_luamgr::get_scripts_num(void)
{
	return m_scripts.size();
}

const lua_script_info& class_luamgr::operator[](unsigned int idx)
{
	static lua_script_info fake;
	if (idx >= get_scripts_num())
		return fake;

	return m_scripts[idx];
}

std::string class_luamgr::find_proc_name(const char* scrname)
{
	// scrname - is the full path to the LUA script
	class_path fullname;
	fullname = m_scr_root_folder;
	fullname+= get_scr_rel_path(scrname);
	fullname+= scrname;

	std::string file_path;
	std::string file_name;
	std::string scr_name;
	std::string proc_name;

	if (fullname.split(file_path, file_name) < 0)
		return "<path error>";

	split_file_name(file_name.c_str(), scr_name, proc_name);
	if (proc_name == "")
		proc_name = LUA_SCR_ENTRY_POINT_NAME;
	return proc_name;
}

/**  @brief Let's go through all the scripts and to run the scripts marked with option init
*/
int class_luamgr::autorun(void)
{
	std::vector<std::string> params;
	std::vector<lua_script_info>::iterator i;

	for (i = m_scripts.begin(); i < m_scripts.end(); ++i)
	{
		if ((*i).m_options & lua_scr_opt::LUA_SCR_OPT_AUTO)
		{
			class_path path;

			path = (*i).m_rel_path;
			path += (*i).m_name;

			PRN_INFO("Script: [%s]:\n", (const char*)path);
			run_script((const char*)path, params, LUA_SCR_AUTORUN_NAME);
			PRN_INFO("\n");
		}
	}
	return RC_OK;
}

/**  @brief Let's go through all the scripts and to calculate how many autorun scripts we have
*/
int class_luamgr::autorun_get_num(void)
{
	int num = 0;
	std::vector<lua_script_info>::iterator i;

	for (i = m_scripts.begin(); i < m_scripts.end(); ++i)
	{
		if ((*i).m_options & lua_scr_opt::LUA_SCR_OPT_AUTO)
		{
			num++;
		}
	}
	return num;
}

/** @brief This function checks if the script defined by index
			is located in the currently selected folder (defined with m_scr_rel_path)
*/
int class_luamgr::is_scr_path(int idx)
{
	if ((unsigned int)idx >= m_scripts.size())
		return 0;

	std::string work_dir = (const char*)(m_scr_root_folder + get_scr_rel_path() + "/");
	lua_script_info& info = m_scripts[idx];
	//printf("\n(NAME:%s) work_dir:%s\n", info.m_name.c_str(), work_dir.c_str());
	//printf("          info.m_path:%s, rel:%s\n", info.m_path.c_str(), info.m_rel_path.c_str());
	return work_dir == info.m_path;
}

int class_luamgr::is_folder(const char* rel_path)
{
	class_path dir_path(m_scr_root_folder);
	dir_path += get_scr_rel_path(rel_path);
	dir_path+= rel_path;
	return class_path::is_dir((const char*)dir_path);
}

int class_luamgr::load_folders(int set, std::vector<std::string>& list)
{
	// to scan the folder defined by <script path + relative path> and to find all
	// folders located there
	DIR *d;
	struct dirent *dir_elm;

	class_path dir_path;
	dir_path = m_scr_root_folder;
	dir_path+= get_scr_rel_path();

	if ((d = opendir((const char*)dir_path)) != NULL)
	{
	    while ((dir_elm = readdir(d)) != NULL)
	    {
		std::string name;
		name = dir_elm->d_name;
		if (name == "." || name == "..")
			continue;

		// -------------------------------------------------------------
		//if (dir_elm->d_type != DT_DIR && name != "." && name != "..")
		// -------------------------------------------------------------
		if (class_path::is_dir((const char*)(dir_path+name)))
		{
			if (set & FOLDER_SET_FOLDERS)
				list.push_back(name+"/");
		}
		else if ((set & FOLDER_SET_FILES))
		{
			list.push_back(name);
		}
	    }
	    closedir(d);
	}
	std::sort(list.begin(), list.end());
	return RC_OK;
}

int class_luamgr::load_folder_content(std::string abs_path, std::vector<std::string>& list)
{
	// to scan the folder defined by <script path + relative path> and to find all
	// folders located there
	DIR *d;
	struct dirent *dir_elm;
	class_path dir_path(abs_path);
	if ((d = opendir((const char*)dir_path)) != NULL)
	{
	    while ((dir_elm = readdir(d)) != NULL)
	    {
		std::string name;
		name = dir_elm->d_name;
		// -------------------------------------------------------------
		//if (dir_elm->d_type != DT_DIR && name != "." && name != "..")
		// -------------------------------------------------------------
		if (class_path::is_dir((const char*)(dir_path+name)) && name != "." && name != "..")
		{
			list.push_back(name+"/");
		}
		else if (name != "." && name != "..")
		{
			list.push_back(name);
		}
	    }
	    closedir(d);
	}
	std::sort(list.begin(), list.end());
	return RC_OK;
}

int class_luamgr::load_content(std::string pattern, std::vector<std::string>& list)
{
	class_path path(m_scr_root_folder);
	path += get_scr_rel_path(pattern);
	path += pattern;

	std::string abs_path;
	std::string abs_objname;
	path.split(abs_path, abs_objname);

	// Let's check, if this is just a directory
	if (path.is_dir())
	{
		if (!path.is_slash_ended())
		{
			list.push_back((get_scr_rel_path(pattern)+pattern+"/").c());
			return RC_OK;
		}
		// to load the folder content
		return load_folder_content((const char*)path, list);
	}
	// OK, this is not a completed directory path
	// so, it can be a set of LUA files, folders, etc ...
	std::string rel_path;
	std::string rel_objname;
	class_path::split(pattern, rel_path, rel_objname, 0);

	int is_root = pattern[0] == '/';

	int rc;
	std::vector<std::string> content;
	if ((rc = load_folder_content(abs_path.c_str(), content)) < 0)
		return rc;

	//printf("\npattern:%s\n", pattern.c_str());
	//printf("\nrel_path:%s\n", rel_path.c_str());
	//printf("\nrel_objname:%s\n", rel_objname.c_str());

	for(unsigned int i = 0; i < content.size(); i++)
	{
		const char* name = content[i].c_str();
		if (strstr(name, rel_objname.c_str()) == name)
		{
			// if the path is started from LUA ROOT
			// we need to use this path defined in the pattern
			// and to create the string by using :  pattern + name (with an offset to skip the part of text from the pattern)
			if (is_root)
			{
				list.push_back(pattern + (name + rel_objname.length()));
			}
			// 127.0.0.1:50005:fld1/>../f
			// typed&expected path:  fld1/../
			// we have rel_path == ""
			// we need to add:  ../ + name
			else
			{
				list.push_back(rel_path + name);
			}
		}
	}
	return RC_OK;
}

std::string class_luamgr::get_scr_root(void)
{
	return m_scr_root_folder.str();
}

int class_luamgr::set_rel_path(const char* path)
{
	if (strcmp(path,"/") == 0)
	{
		m_scr_rel_path.reset();
		return RC_OK;
	}

	if (path[0] == '/')
	{
		m_scr_rel_path.reset();
		m_scr_rel_path = path+1;
	}
	else
	{
		m_scr_rel_path += path;
	}
	m_scr_rel_path.set_folder();
	return RC_OK;
}

std::string class_luamgr::get_rel_path(void)
{
	return m_scr_rel_path.c();
}

int class_luamgr::split_file_name(const char* filename, std::string & scrname, std::string & procname)
{

	// Actually the name of LUA script may be:
	//   1. scrname              - in this case "main" proc will be used
	//   2. scrname.procname     - in this case "procname" will be used

	class class_lexer lex("", ". ");
	lex.lex_init(filename);
	scrname = lex.lex_next();
	procname = lex.lex_next();
	return 0;
}

int class_luamgr::find_by_name(const char* filename)
{
	class_path path;

	path = m_scr_root_folder;
	path += get_scr_rel_path(filename);
	path += filename;

	std::string scr_path;		// The relative path in <filename> if any
	std::string scr_name;		// The script name

	// In case of error, just to return -1
	if (path.split(scr_path, scr_name) < 0)
		return -1;

	//printf("\nfind_by_name: %s\n", filename);
	//printf("  full_cmd:%s\n", path.c());
	//printf("  scr_path:%s\n", scr_path.c_str());
	//printf("  scr_name:%s\n", scr_name.c_str());

	std::string fname, procname;
	split_file_name(scr_name.c_str(), fname, procname);

	std::vector<lua_script_info>::iterator i;
	for (i = m_scripts.begin(); i < m_scripts.end(); ++i)
	{
		//printf("i->m_path:%s    i->m_name:%s  >>> fname:%s\n", i->m_path.c_str(), i->m_name.c_str(), fname.c_str());
		if (scr_path == i->m_path && i->m_name == fname)
		{
			//printf("FOUND:%d\n", i - m_scripts.begin());
			return i - m_scripts.begin();
	}
	}
	return -1;
}

int class_luamgr::run_script(const char* filename, const std::vector<std::string>&params, const char* proc_name)
{
	static int recursion = 0;
	lua_State* lua_state = m_LuaH;

	int idx = find_by_name(filename);
	if (idx < 0)
		return RC_LUAMGR_NAME_ERROR;

	lua_script_info&lua_info = m_scripts[idx];
	std::string path = lua_info.m_path + lua_info.m_name;

	if (lua_info.m_LuaState != NULL)
	{
		lua_state = lua_info.m_LuaState;

		if (lua_info.m_LuaLoaded == 0)
		{
			if (luaL_loadfile(lua_state, path.c_str()))
			{
				PRN_ERROR("Error to load LUA script from file: %s\n", path.c_str());
				PRN_ERROR("---------------------------------------------------\n");
				PRN_ERROR("%s\n", lua_tostring(lua_state, -1));
				PRN_ERROR("---------------------------------------------------\n");
				lua_pop(lua_state,1);
				return RC_LUAMGR_SCR_LOAD_ERROR;
			}
			lua_info.m_LuaLoaded = 1;
		}
	}
	else
	{
		if (luaL_loadfile(lua_state, path.c_str()))
		{
			PRN_ERROR("Error to load LUA script from file: %s\n", path.c_str());
			PRN_ERROR("---------------------------------------------------\n");
			PRN_ERROR("%s\n", lua_tostring(lua_state, -1));
			PRN_ERROR("---------------------------------------------------\n");
			lua_pop(lua_state,1);
			return RC_LUAMGR_SCR_LOAD_ERROR;
		}
	}

	// this is needed like an initial call before using scripts functions
	// ------------------------------------------------------------------
	lua_pcall(lua_state, 0, 0, 0);

	lua_getglobal(lua_state, (proc_name != NULL) ? proc_name : "main");
	for (unsigned int i=0; i < params.size();i++)
	{
		//lua_pushnumber(lua_state, params[i].c_str());
		lua_pushstring(lua_state, params[i].c_str());
	}

	recursion ++;
	int r = lua_pcall(lua_state, params.size(), 0, 0);
	recursion --;

#if 0
	{
		lua_pushglobaltable(m_LuaH);       // Get global table
		lua_pushnil(m_LuaH);               // put a nil key on stack
		while (lua_next(m_LuaH,-2) != 0){ // key(-1) is replaced by the next key(-1) in table(-2)
		    const char* pname = lua_tostring(m_LuaH,-2);  // Get key(-2) name
		    printf("panme = %s\n", pname);
		    lua_pop(m_LuaH,1);               // remove value(-1), now key on top at(-1)
		}
		lua_pop(m_LuaH,1);
	}
#endif

#if 0
	// It's expected the scripts MUST close opened sockets!!!
	// -----------------------------------------------------
	if (recursion  == 0)
	{
		m_luasockets.init();
	}
#endif

	if (r)
	{
		PRN_ERROR("---------------------------------------------------\n");
		PRN_ERROR("Error to run LUA script %s() function\n", proc_name);
		PRN_ERROR("file: %s\n", path.c_str());
		PRN_ERROR("---------------------------------------------------\n");
		PRN_ERROR("%s\n", lua_tostring(lua_state, -1));
		PRN_ERROR("---------------------------------------------------\n");
		lua_pop(lua_state,1);
		return RC_LUAMGR_SCR_LOAD_ERROR;
	}

	return RC_OK;
}

int class_luamgr::run_buffer(const char* filedata)
{
	static int recursion = 0;

	if (luaL_loadbuffer(m_LuaH, filedata, strlen(filedata), "__REMOTE_SCRIPT__"))
	{
		PRN_ERROR("Error to load LUA remote script\n");
		PRN_ERROR("---------------------------------------------------\n");
		PRN_ERROR("%s\n", lua_tostring(m_LuaH, -1));
		PRN_ERROR("---------------------------------------------------\n");
		lua_pop(m_LuaH,1);
		return RC_LUAMGR_SCR_LOAD_ERROR;
	}

	// this is needed like an initial call before using scripts functions
	// ------------------------------------------------------------------
	lua_pcall(m_LuaH, 0, 0, 0);
	lua_getglobal(m_LuaH, "main");

	recursion ++;
	int r = lua_pcall(m_LuaH, 0, 0, 0);
	recursion --;
	if (recursion  == 0)
	{
		m_luasockets.init();
	}

	if (r)
	{
		PRN_ERROR("---------------------------------------------------\n");
		PRN_ERROR("Error to run LUA remote script main() function\n");
		PRN_ERROR("---------------------------------------------------\n");
		PRN_ERROR("%s\n", lua_tostring(m_LuaH, -1));
		PRN_ERROR("---------------------------------------------------\n");
		lua_pop(m_LuaH,1);
		return RC_LUAMGR_SCR_LOAD_ERROR;
	}

	return RC_OK;
}

int class_luamgr::socket(int type)
{
	return m_luasockets.socket(type);
}

int class_luamgr::bind(int socket, const char* ip, int port)
{
	return m_luasockets.bind(socket, ip, port);
}
int class_luamgr::accept(int socket)
{
	int res = m_luasockets.accept(socket);
	return res;
}

int class_luamgr::connect(int socket, const char* ip, int port)
{
	return m_luasockets.connect(socket, ip, port);
}

int class_luamgr::send(int socket, void* pdata, int size)
{
	return m_luasockets.send(socket, pdata, size);
}

int class_luamgr::recv(int socket, void* poutdata, int size, int wait_ms, int exact_size)
{
	return m_luasockets.recv(socket, poutdata, size, wait_ms, exact_size);
}

int class_luamgr::close(int h)
{
	return m_luasockets.close(h);
}

int class_luamgr::ipc_open(const char* ip, unsigned long port)
{
	int res = m_luaipc.ipc_open(ip, port);
	return res;
}

int class_luamgr::ipc_close(int hIPC)
{
	int res = m_luaipc.ipc_close(hIPC);
	return res;
}

int class_luamgr::ipc_call(unsigned int hIPC, unsigned int api_id, lua_ipc_params & list, lua_ipc_parameter& ret)
{
	return m_luaipc.ipc_call(hIPC, api_id, list, ret);
}

int class_luamgr::ipc_call_done(unsigned int hIPC)
{
	return m_luaipc.ipc_call_done(hIPC);
}

int class_luamgr::show_scr_help(const char* scrname)
{
	std::vector<std::string> params;

	int res = run_script(scrname, params, LUA_SCR_HELP_NAME);
	return res;
}

int class_luamgr::proc_scr_options(const char* line, lua_scr_opt& opt)
{
	int res = 0;
	if (strcasestr(line, LUA_SCR_OPT_NAME_CONTINUED) != NULL)
	{
		opt.m_options |= lua_scr_opt::LUA_SCR_OPT_CONTINUED;
		res = 1;
	}

	if (strcasestr(line, LUA_SCR_OPT_NAME_AUTORUN) != NULL)
	{
		opt.m_options |= lua_scr_opt::LUA_SCR_OPT_AUTO;
		res = 1;
	}

	return res;
}

class_path  class_luamgr::get_scr_rel_path(std::string pattern)
{
	if (pattern == "")
		return m_scr_rel_path;

	if (pattern.length() && pattern[0]=='/')
		return "";

	return m_scr_rel_path;
}

const char* class_luamgr::load_script_description(const char* filename, lua_scr_opt& opt)
{
	class class_file file(filename);
	if (!file.is_open())
		return "";

	void*data=NULL;
	if (file.readfile(data) < 0)
		return "";

	class class_lexer lexer("\n", "", "--");
	lexer.lex_init((char*)data);

	int state = 0, stop = 0;
	std::string lex;
	static std::string descr;
	descr = "";
	while (stop == 0 && !(lex = lexer.lex_next()).empty())
	{
		switch(state)
		{
			case 0:
				if (lex == "--")
				{
					state = 1;
				}
				else if (lex == "\n")
				{
				}
				else
				{
					stop = 1;
				}
			break;

			case 1:
				if (lex != "\n")
				{
					// If this line does not contain any script options
					// this means this is just a script comment
					if (!proc_scr_options(lex.c_str(), opt))
					{
						descr += lex;
					}
					state = 2;
				}
				else
				{
					stop = 1;
				}
			break;

			case 2:
				if (lex == "\n")
					state = 0;
			break;
		}
	}

	free(data);
	return descr.c_str();
}
