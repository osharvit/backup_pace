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
#include <string>

#define VERSION_H                                   0
#define VERSION_L                                   8

#define MACRO_STR(x)                                #x
#define xMACRO_STR(x)                               MACRO_STR(x)
#define VERSION                                     xMACRO_STR(VERSION_H) "." xMACRO_STR(VERSION_L)

#define CLI_VARS_FILENAME                           "cli-vars.dat"
#define CLI_EXT_CMD_FILENAME                        "cli-extcmd.dat"

#define CLI_AUTOVAR_NAME                            "autorun"

#define LUA_SCR_OPT_NAME_CONTINUED                  "cli:continued"
#define LUA_SCR_OPT_NAME_AUTORUN                    "cli:autorun"

#define LUA_SCR_ENTRY_POINT_NAME                    "main"
#define LUA_SCR_AUTORUN_NAME                        "autorun"
#define LUA_SCR_HELP_NAME                           "help"

#define CLI_HW_DESCR_FOLDER_NAME                    "hw"
#define CLI_EXT_CMD_FOLDER_NAME                     "extcmd"
#define CLI_LUA_SCRIPTS_FOLDER_NAME                 "scripts"

#define CLI_MAX_CMD_LEN                             512
#define CLI_MAX_CMD_HISTORY                         64

#define DEF_H2_PORT                                 1004
#define DEF_SERV_PORT                               50005
#define DEF_REMOTE_CTRL_PORT                        50006
#define DEF_IPC_PORT                                50007
#define DEF_SERV_IP                                 "127.0.0.1"
#define DEF_LOG_CTRL                                0
#define DEF_REMOTE_CTRL                             1
#define DEF_TAB_FILE_LIST                           1

#define DEF_DRVID                                   0

#define CLF_MAX_NUM_OF_CMD                          32

/*******************************************************************************
        The list if the Command Line IDs 
*******************************************************************************/
#define CLF_HELP                                    (1<<0)
#define CLF_SERV_IP                                 (1<<1)
#define CLF_SERV_PORT                               (1<<2)
#define CLF_CMD_FILE_NAME                           (1<<3)
#define CLF_CMD                                     (1<<4)
#define CLF_LOG_FILE                                (1<<5)
#define CLF_LOG_CTRL                                (1<<6)
#define CLF_H2_PORT                                 (1<<7)
#define CLF_REMOTE_CTRL                             (1<<8)
#define CLF_IPC_PORT                                (1<<9)
#define CLF_TAB_FILE_LIST                           (1<<10)
#define CLF_INPUT_0_MODE                            (1<<11)

struct lua_export_api
{
        const char* name;
        int (*proc)(void *L);
};

struct CLF_PARAMS_TYPE
{
    unsigned int    flags;

    unsigned int    serv_port;
    unsigned int    h2_port;
    unsigned int    rc_port;
    unsigned int    ipc_port;
    unsigned int    tab_files;
    char            serv_ip[PATH_MAX];
    char            cmd_file[PATH_MAX];

    unsigned int    cmd_num;
    char            cmd[CLF_MAX_NUM_OF_CMD][PATH_MAX];

    char            log_file[PATH_MAX];
    int             log_ctrl;

    lua_export_api* lua_api;
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
            free (m_obj);
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


void PRN_SET_DEV(std::string* ptr);
int PRN_IS_DEV(void);
int PRN_SEND_TO_DEV(const char* pdata);

#define PRN_ERROR(...)                          \
do { if (PRN_IS_DEV())                          \
{                                               \
        static char outbuf[PATH_MAX*2];               \
        snprintf(outbuf, sizeof(outbuf),__VA_ARGS__);           \
        PRN_SEND_TO_DEV(outbuf);                \
}                                               \
else                                            \
{                                               \
    printf(__VA_ARGS__);                        \
    fflush(stdout);                             \
}                                               \
}while(0)


#define PRN_INFO(...)                           \
do {                                            \
if (PRN_IS_DEV())                               \
{                                               \
        static char outbuf[PATH_MAX*2];               \
        snprintf(outbuf, sizeof(outbuf),__VA_ARGS__);           \
        PRN_SEND_TO_DEV(outbuf);                \
}                                               \
else                                            \
{                                               \
    printf(__VA_ARGS__);                        \
    fflush(stdout);                             \
}                                               \
}while(0)

#endif // _GEN_TYPES_H_

