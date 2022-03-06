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

#ifndef _LUAMGR_H_
#define _LUAMGR_H_

#include <vector>
#include <string>
#include "gen-types.h"
#include "retcodes.h"
#include "lua.hpp"
#include "luasockets.h"
#include "luaipcmgr.h"
#include "path.h"

struct lua_scr_opt
{
    enum
    {
        LUA_SCR_OPT_CONTINUED       = (1<<0),
        LUA_SCR_OPT_AUTO            = (1<<1)
    };

    lua_scr_opt()
    {
        m_options = 0;
    }

    int m_options;         // The script context lives all the time (all the global variables are saved between run to run)
};

struct lua_script_info
{
    lua_script_info()
    {
        m_LuaState = NULL;
        m_LuaLoaded = 0;
    }

    lua_State*      m_LuaState;
    int             m_LuaLoaded;
    std::string     m_path;         // The full script path, it must be terminated with '/' symbol at the end
    std::string     m_rel_path;     // The relative path, the between scripts parent folder and script name
    std::string     m_name;         // The script name
    std::string     m_descr;
    std::string     m_textopt;
    int             m_options;      // lua_scr_opt::enum_xxx
};

enum FOLDER_SET
{
    FOLDER_SET_FOLDERS  =   (1<<0),
    FOLDER_SET_FILES    =   (1<<1),
};

class class_luamgr
{
public:
                                    class_luamgr(void);
                                    ~class_luamgr(void);

        int                         init(CLF_PARAMS_TYPE* p);
        int                         load_scripts(const char* scr_folder = "");
        int                         add_script(const char* filename);
        unsigned int                get_scripts_num(void);

        const lua_script_info&      operator[](unsigned int idx);

        int                         find_by_name(const char* filename);
        int                         run_script(const char* filename, const std::vector<std::string>&params, const char* proc_name = "main");
        int                         run_buffer(const char* filedata);

        int                         socket(int type=0);
        int                         bind(int socket, const char* ip, int port);
        int                         accept(int socket);
        int                         connect(int socket, const char* ip, int port);
        int                         send(int socket, void* pdata, int size);
        int                         recv(int socket, void* poutdata, int size, int wait_ms = 0, int exact_size = 0);
        int                         close(int h);

        int                         ipc_open(const char* ip, unsigned long port);
        int                         ipc_close(int hIPC);
        int                         ipc_call(unsigned int hIPC, unsigned int api_id, lua_ipc_params & list, lua_ipc_parameter& ret);
        int                         ipc_call_done(unsigned int hIPC);

        int                         show_scr_help(const char* scrname);
        std::string                 find_proc_name(const char* scrname);

        int                         autorun(void);
        int                         autorun_get_num(void);

        int                         is_scr_path(int idx);
        int                         is_folder(const char* rel_path);
        int                         load_folders(int set, std::vector<std::string>& list);
        int                         load_folder_content(std::string abs_path, std::vector<std::string>& list);
        int                         load_content(std::string pattern, std::vector<std::string>& list);

        std::string                 get_scr_root(void);

        int                         set_rel_path(const char* path);
        std::string                 get_rel_path(void);

protected:
        int                         split_file_name(const char* filename, std::string & scrname, std::string & procname);
        const char*                 load_script_description(const char* filename, lua_scr_opt& opt);
        std::string                 find_rel_path(const char* scr_path);
        std::string                 scr_opt2text(lua_scr_opt opt);
        int                         proc_scr_options(const char* line, lua_scr_opt& opt);

        class_path                  get_scr_rel_path(std::string pattern = "");
private:

    lua_export_api*                 m_lua_api;

    class_lua_sockets               m_luasockets;
    class_lua_ipc_mgr               m_luaipc;

    std::vector<lua_script_info>    m_scripts;
    lua_State*                      m_LuaH;
    class_path                      m_scr_root_folder;      // The full path to the scripts folder
    class_path                      m_scr_rel_path;         // The LUA script relative path( the current path for the scripts )
};

#endif //_LUAMGR_H_