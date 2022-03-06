/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class implements command line editor
 *  it handles history, editing of the text ... etc
 *
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

#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <string>
#include <vector>
#include <queue>
#include <termios.h>

#define RC_OK                                                       0

#define RC_CMD_EDITOR_PARAM_ERROR                                   (-20001)
#define RC_CMD_EDITOR_ALLOW_ERROR                                   (-20002)
#define RC_CMD_EDITOR_GETCMD_ERROR                                  (-20003)
#define RC_CMD_EDITOR_INIT_ERROR                                    (-20004)
#define RC_CMD_EDITOR_MAXLEN_ERROR                                  (-20005)
#define RC_CMD_EDITOR_TERM_CFG_ERROR                                (-20006)
#define RC_CMD_EDITOR_TERM_PARAM_ERROR                              (-20007)

#define CMD_EDITOR_TAB_TAB_TIMEOUT                                  500

extern "C"
{
    int   cmd_editor_init(const char* logo, int cmdlen, int history_size);
    int   cmd_editor_set_input(const char* input);
    char* cmd_editor_read_cmd(void);
    void  cmd_editor_window_resize(void);
};

enum
{
        CMD_KEY_LEFT                =   1,
        CMD_KEY_RIGHT               =   2,
        CMD_KEY_UP                  =   3,
        CMD_KEY_DOWN                =   4,
        CMD_KEY_HOME                =   5,
        CMD_KEY_END                 =   6,
        CMD_KEY_PAGEUP              =   7,
        CMD_KEY_PAGEDOWN            =   8,
        CMD_KEY_DEL                 =   9,
};

#define CHID_CTRL_KEY_BIT                   (1<<8)
#define CHID_IS_CTRL_KEY(ch)                ((ch)&CHID_CTRL_KEY_BIT)
#define CHID_GET_CTRL_KEY(ch)               (((ch)>>9)&0xFF)
#define CHID_CTRL_KEY(key)                  (((key)<<9) | CHID_CTRL_KEY_BIT)
#define CHID_GET_CHAR(ch)                   ((ch)&0xFF)

typedef int (*cmd_editor_tab_handler)(const char* cmd, void* param, char** pins_text);

class class_cmd_editor
{
public:
            class_cmd_editor();
            ~class_cmd_editor();


            int         init(unsigned int textmaxsize, unsigned int history_len = 0);
            int         deinit(void);
            int         stop(void);
            int         setlogo(const char* logo);
            int         set_tab_handler(cmd_editor_tab_handler tab, void* data);
            char*       getcmd(void);
            int         getcmd(std::string& cmd);

            void        clear_input(int update_history_idx = 1);
            int         set_input(const char* pcmd, int update_history_idx = 1);
            int         update_input(void);

            int         resize_window(void);

            int         get_cols(void) {return m_term_col;}
            int         get_rows(void) {return m_term_row;}

            void        set_tab_delim(const char* delim, const char* possible_at_start=""){ m_tab_delimiters = delim; m_possible_at_start=possible_at_start;}

protected:
            char        read_ch(int wait_ms = 0, int use_queue = 1);
            int         read_char(void);
            int         proc_add_history(void);
            int         proc_ch(char ch);
            int         proc_backspace(void);
            int         proc_tab(void);
            int         proc_esc(void);
            int         proc_left(void);
            int         proc_right(void);
            int         proc_del(void);
            int         proc_home(void);
            int         proc_end(void);
            void        proc_history(int key);
            void        proc_enter(void);

            int         get_cursor(int &x, int &y);
            int         set_cursor(int x, int y);

            void        save_cursor(void);
            void        load_cursor(int dx = 0);
            void        shift_cursor(int dx);
            int         move_cursor(int dx);
            int         update_cursor_pos(int dx);
            int         get_text_rows_num(void);

private:
            int         is_equal(char* array, char* ref, int size = 4);
            void        del_char_in_buf(void);
            void        ins_char_in_buf(char ch);
            void        update_text(int pos, int remove = 0);

            char*               m_ptext;            // The buffer with the text
            int                 m_text_pos;         // The cursor possition(possition in the text)
            int                 m_text_len;         // The typed text length in bytes
            int                 m_text_max_len;     // The maximum allowed characters

            std::string         m_logo;
            std::queue<char>    m_queue;
            std::vector<std::string>    m_history;
            unsigned int        m_history_max_len;
            int                 m_history_cur_elm;
            int                 m_init_x;
            int                 m_init_y;

            int                 m_cur_x;
            int                 m_cur_y;

            int                 m_term_col;
            int                 m_term_row;

            struct termios      term_current_state;

            cmd_editor_tab_handler  m_tab_handler;
            void*                   m_tab_handler_data;
            struct timeval          m_tab_ticks;
            int                     m_stop;
            std::string             m_tab_delimiters;
            std::string             m_possible_at_start;
};

#endif // _EDITOR_H_

