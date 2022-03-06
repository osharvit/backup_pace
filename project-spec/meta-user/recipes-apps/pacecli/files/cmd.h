/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *    This file defines 3 classes to implement 3 types of the commands
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

#ifndef _CMD_H_
#define _CMD_H_

#include "retcodes.h"
#include <string>
#include <vector>
#include "varmgr.h"

enum cmd_type
{
    CMD_NATIVE      =   1,
    CMD_EXTENDED    =   2,
    CMD_LUA         =   3,
};

class class_cmd
{
public:

    class_cmd();
    virtual ~class_cmd();

    virtual cmd_type    get_cmd_type(void) = 0;
    virtual void*       packcmd(void) = 0;
    virtual int         unpack(void* pdata) = 0;

    int                 set_name(const char* name);
    const char*         get_name(void);

    int                 set_id(int id);
    int                 get_id(void);

    int                 set_descr(const char* descr);
    const char*         get_descr(void);

protected:

    virtual const char* id_to_name(int id) = 0;


    int             m_id;           // The command ID, to define specific native command or to identify extended command
    std::string     m_name;         // The command name , this should be unique in the list of the commands and CLI 'variables'
    std::string     m_descr;        // The command description
};

/**
    @brief This class defines the native CLI command

*/

enum NATIVE_CMD_ID
{
    NCID_REG_SET         =   1,
    NCID_REG_SET_ACK     =   101,
    NCID_REG_GET         =   2,
    NCID_REG_GET_ACK     =   102,
    NCID_REG_OR          =   3,
    NCID_REG_OR_ACK      =   103,
    NCID_REG_XOR         =   4,
    NCID_REG_XOR_ACK     =   104,
    NCID_REG_AND         =   5,
    NCID_REG_AND_ACK     =   105,
    NCID_SLEEP           =   6,
    NCID_PRINT           =   7,
    NCID_DUMP            =   8,
    NCID_DUMP_ACK        =   108,
    NCID_UPLOAD          =   9,
    NCID_UPLOAD_ACK      =   109,
    NCID_DOWNLOAD        =   10,
    NCID_DOWNLOAD_ACK    =   110,
    NCID_SET_VAR         =   11,
    NCID_KILL_VAR        =   12,        // This command is the special case of SET command with var name only
    NCID_VAR_LIST        =   14,        // This command is the special case of SET command without parameters
    NCID_HELP            =   15,
    NCID_LS              =   16,
    NCID_CD              =   17,
    NCID_PWD             =   18,
    NCID_MATH            =   19,
    NCID_VERSION         =   20,
    NCID_MAP             =   21,
    NCID_MAP_ACK         =   22,
    NCID_SCHED           =   23,
    NCID_READ_DATA       =   24,
    NCID_READ_DATA_ACK   =   124,
    NCID_WRITE_DATA      =   25,
    NCID_WRITE_DATA_ACK  =   125,
    NCID_WRITE           =   26,
    NCID_CALC            =   27,
    NCID_GET_FILE        =   28,
    NCID_GET_FILE_ACK    =   128,
    NCID_FIFO_CTRL       =   29,
    NCID_FIFO_CTRL_ACK   =   129,
    NCID_OS_SYS_CMD      =   30,
    NCID_CD_SCR_FOLDER   =   31,

    NCID_EXIT            =   1000,

    NCID_EXT_CMD         =   10000,
    NCID_LUA_CMD         =   20000,

};

class class_native_cmd : public class_cmd
{
public:
                            class_native_cmd();
                            class_native_cmd(int id);
        virtual             ~class_native_cmd();

        virtual cmd_type    get_cmd_type(void);

        virtual void*       packcmd(void);
        virtual int         unpack(void* pdata);

        int                 set_address(const char* addr);
        int                 set_compreg_name(const char* comp, const char* reg);
        int                 set_message(const char* msg);
        const char*         get_message(void);
        int                 enable_message(int ena);
        int                 is_message(void);
        int                 set_value(const char* val, int this_is_variable = 0);

        const char*         get_address(void)
        {
                            return m_address.c_str();
        }

        const char*         get_value(void)
        {
                            return m_value.c_str();
        }

        int                 set_bits(int bits)
        {
                            if (bits == 8 || bits == 16 || bits == 32 || bits == 64)
                            {
                                m_bits = bits;
                                return RC_OK;
                            }
                            return RC_CMD_BITS_ERROR;
        }

        int                 get_bits(void)
        {
                            return m_bits;
        }

        int                 get_addr_type(void)
        {
                            return m_addr_type;
        }


        void                set_file_name(const char* name);
        const char*         get_file_name(void);

protected:
        virtual const char* id_to_name(int id);

private:

        int                 m_bits;

        int                 m_addr_type;        //  1 - address(or var name), 2 - component.reg name

        std::string         m_filename;           // The name of the file for the upload/download commands
        std::string         m_address;
        std::string         m_hwname;
        std::string         m_regname;
        std::string         m_message;
        int                 m_is_message;
        std::string         m_value;            // The direct value or the name of the variable (depends on the m_val_type)
        int                 m_value_type;       // 0 - a direct value, 1 - the variable name
};


/**
    @brief This class defines the extended CLI command and some extra types
            related to the extended commands handling
*/

struct cmd_param
{
    std::string m_name;
    std::string m_defval;
};

enum extcmdinterface
{
    EXT_CMD_CLI     =   1,
    EXT_CMD_H2      =   2
};

class class_ext_cmd : public class_cmd
{
public:
                        class_ext_cmd();
    virtual             ~class_ext_cmd();
    virtual cmd_type    get_cmd_type(void);
    virtual void*       packcmd(void);
    virtual int         unpack(void* pdata);

    void                set_name(const char* name);
    const char*         get_name(void);

    void                set_text(const char* text);
    const char*         get_text(void);

    void                set_interface(extcmdinterface inf);
    extcmdinterface     get_interface(void);

    void                add_param(cmd_param& param);
    const cmd_param&    get_param(unsigned int idx);
    int                 find_param(const char* param_name);
    int                 get_param_num(void){ return m_params.size(); }
    int                 get_req_param_num(void);

    const char*         get_params_info(void);
    int                 set_json_resp_format(const char* pformat);
    const char*         get_json_resp_format(void);

protected:

    virtual const char* id_to_name(int id);

private:

    std::string                 m_name;
    std::string                 m_text;
    extcmdinterface             m_interface;            // see: extcmdinterface
    std::string                 m_json_resp_format;

    std::vector<cmd_param>      m_params;
};





class class_lua_cmd : public class_cmd
{
public:
                        class_lua_cmd(void);
    virtual             ~class_lua_cmd(void);
    virtual cmd_type    get_cmd_type(void);
    virtual void*       packcmd(void);
    virtual int         unpack(void* pdata);

    const std::vector<std::string>& get_params(void);
    int                 add_param(const char* p);
    int                 set_proc_name(const char* p);
    const char*         get_proc_name(void);

protected:
    virtual const char* id_to_name(int id);

    std::vector<std::string>    m_params;
    std::string                 m_proc_name;
};

#endif // _CMD_H_
