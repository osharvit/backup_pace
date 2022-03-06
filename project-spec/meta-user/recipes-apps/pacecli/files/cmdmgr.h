/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This manager is designed to control native & extended CLI commands
 *     to pars the input stream, to keep the commands, to provide 
 *     all the commands parameters, etc.
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

#ifndef _CMDMGR_H_
#define _CMDMGR_H_

#include "retcodes.h"
#include <vector>
#include <string>
#include <pthread.h>
#include "cmd.h"
#include "extcmdmgr.h"
#include "hwmgr.h"
#include "clicpmgr.h"
#include "luamgr.h"
#include "h2mgr.h"
#include "rc_cmdmgr.h"
#include "ipcmgr.h"
#include "path.h"

class class_rccmdmgr;

class class_cmdmgr
{
    friend int custom_func(int id, std::string proc, void* mgr, long long& func_arg_func_res);
    friend class_rccmdmgr;

public:
                        class_cmdmgr();
                        ~class_cmdmgr();


            int         init(CLF_PARAMS_TYPE* p);
            int         run_commands(const char* cmds);
            int         internal_run_commands(const char* cmds);
            int         exec_lua_script(const char* plua_scr_text);
            int         sched_time_event(void);
            int         run_sched_tasks(void);
            int         cmd_name2id(const char* name);

            int         set_scr_cur_path(const char* path);
            std::string get_scr_cur_path(void);

            int         redirect_console_to_file(void);
            int         restore_console(std::string * pstr);

            int         set_var(const char* name, const char* value);
            const char* get_var(const char* name);
            int         set_reg(int oprid, const char* bits, const char* addr, const char* value, const char* msg = NULL);
            int         set_reg_nosync(int oprid, const char* bits, const char* addr, const char* value, const char* msg);
            int         get_reg(const char* bits, const char* addr, uint64_t& value, const char* msg = NULL);
            int         get_reg_nosync(const char* bits, const char* addr, uint64_t& value, const char* msg = NULL);
            int         download(const char* bits, const char* addr, const char*len, const char* filename);
            int         download_nosync(const char* bits, const char* addr, const char*len, const char* filename);
            int         upload(const char* bits, const char* filename, const char* addr);
            int         upload_nosync(const char* bits, const char* filename, const char* addr);
            int         h2(std::string& cmd, std::string&resp);
            int         getfile(const char* filename, void*& poutdata);
            int         readdata(int bits, const char* text_addr, unsigned int size, void* poutdata);
            int         readdata(int bits, unsigned long long address, unsigned int size, void* poutdata);
            int         readdata_nosync(int bits, unsigned long long address, unsigned int size, void* poutdata);
            int         writedata(int bits, const char* text_addr, void* pindata, unsigned int size);
            int         writedata(int bits, unsigned long long address, void* pindata, unsigned int size);
            int         writedata_nosync(int bits, unsigned long long address, void* pindata, unsigned int size);

            class_luamgr*   get_luamgr(void);

            int         fifo_ctrl(const char* fifo_name, const char* opr);

            int         load_cur_dir_files(const char* pat, std::vector<std::string> & list);
            int         get_cmd_list(const char* pat, std::vector<std::string> & list);
            int         is_lua_scr(const char* name);
            int         is_lua_folder(const char* path);

protected:
            int     exec_commands(std::vector<class_cmd*>& objcmds);

            int     parse_commands(const char* cmds, std::vector<class_cmd*>& objcmds);
            int     parse_single_command(const char* pcmd, std::vector<class_cmd*>& objcmds);
            int     init_native_cmds(CLF_PARAMS_TYPE* p);
            int     init_extended_cmds(CLF_PARAMS_TYPE* p);
            int     init_vars(CLF_PARAMS_TYPE* p);
            int     init_hw_comp(CLF_PARAMS_TYPE* p);
            int     init_lua(CLF_PARAMS_TYPE* p);
            int     init_cli_serv_conn(CLF_PARAMS_TYPE* p);
            int     init_h2app_conn(CLF_PARAMS_TYPE* p);
            int     init_rc_cmdmgr(CLF_PARAMS_TYPE* p);
            int     init_ipc_mgr(CLF_PARAMS_TYPE* p);
            int     init_autorun(CLF_PARAMS_TYPE* p);

            void    release_cmds(void);

            int     pars_reg_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_set_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_math_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_print_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_help_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_dump_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_write_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_sleep_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_upload_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_download_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_exit_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_ls_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_pwd_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_cd_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_extended_cmd(int cmdidx, std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_lua_cmd(int cmdidx, std::string& fullname, std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_lua_folder(std::string& fullname, std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_sched_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_calc_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_getfile_cmd(std::string& cmd_body, std::vector<class_cmd*>& objcmds);
            int     pars_os_system_cmd(std::string& name, std::string& cmd_body, std::vector<class_cmd*>& objcmds);

            int     exec_cmd_set_var(class_native_cmd*pcmd);
            int     exec_cmd_remove_var(class_native_cmd*pcmd);
            int     exec_cmd_varlist(class_native_cmd*pcmd);
            int     exec_cmd_print(class_native_cmd*pcmd);
            int     exec_cmd_help(class_native_cmd*pcmd);
            int     exec_cmd_dump(class_native_cmd*pcmd);
            int     exec_cmd_write(class_native_cmd*pcmd);
            int     exec_cmd_sleep(class_native_cmd*pcmd);
            int     exec_cmd_reg(class_native_cmd*pcmd);
            int     exec_cmd_upload(class_native_cmd*pcmd);
            int     exec_cmd_download(class_native_cmd*pcmd);
            int     exec_cmd_sched(class_native_cmd*pcmd);
            int     exec_cmd_exit(class_native_cmd*pcmd);
            int     exec_cmd_ls(class_native_cmd*pcmd);
            int     exec_cmd_pwd(class_native_cmd*pcmd);
            int     exec_cmd_cd(class_native_cmd*pcmd);
            int     exec_cmd_math(class_native_cmd*pnative_cmd);
            int     exec_cmd_calc(class_native_cmd*pcmd);
            int     exec_cmd_getfile(class_native_cmd*pcmd);
            int     exec_cmd_os_sys_cmd(class_native_cmd*pcmd);
            int     exec_cmd_ext(class_ext_cmd*pcmd);
            int     exec_cmd_lua(class_lua_cmd*pcmd);
            int     exec_cd_lua_scr_folder(class_native_cmd*pcmd);


            int     sync_cmd_begin(void);
            int     sync_cmd_end(void);

private:
            int                     is_digit(const char* val);
            int                     is_alpha(const char* txt);
            int                     is_name(const char* txt);
            int                     is_bits_ok(int val);
            int                     is_var_ref(const char* pname);
            int                     is_math_lex(std::string& lex);
            int                     get_lex_num(const char* p, const char* br_sym, const char* sp_sym);
            unsigned long long      var2val(const char* pvariable_or_value);
            std::string             expand_vars(const char*pvalue);
            std::string             expand_ctrl_symb(const char*p);
            std::string             expand_hwname2addr(const char*p);
            int                     calc_value(const char*p, unsigned long long& result);
            std::string             get_str_content(const char*p);
            int                     calc_equation(const char* eq, unsigned int bits, unsigned long long & nval);
            unsigned long long      get_hwreg_addr(const char* regname);
            unsigned int            addr_to_drvid(unsigned long long addr);
            int                     is_addr_valid(unsigned long long addr);
            unsigned long long      addr_bits_align(unsigned long long addr, int bits);

        std::vector<class_cmd*>     m_cmds;
        class_varmgr                m_vars;             // The CLI variables
        class_extcmdmgr             m_extcmdmgr;        // The CLI extended commands
        class_hwmgr                 m_hwmgr;            // The HW manager that keeps information about FPGA HW blocks
        class_luamgr                m_luamgr;

        class_clicpimgr             m_connection;
        class_h2mgr                 m_h2mgr;

        pthread_mutex_t             m_runcmd_mutex;     // This one is used in run_command API & exec_lua_script from Remote Control
        pthread_mutex_t             m_cmd_mutex;        // This one is used with a set of API (remote control/lua/IPC)

        std::string                 m_sched_cmd;
        unsigned int                m_sched_time;
        unsigned int                m_sched_time_cur;


        class_rccmdmgr              m_rccmdmgr;
        class_ipc_mgr               m_ipcmgr;

        int                         m_stdout_fd;
        fpos_t                      m_stdout_pos;

        int                         m_tab_files;
};

#endif //_CMDMGR_H_