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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "cmdeditor.h"

static class_cmd_editor stream;

int tab_handler(const char* p, void* data, char** p_ins_text)
{
	return 0;
}

int cmd_editor_init(const char* logo, int cmdlen, int history_size)
{
	stream.setlogo(logo);
	stream.init(cmdlen, history_size);
	stream.set_tab_handler(tab_handler, NULL);
	return RC_OK;
}

int cmd_editor_set_input(const char* input)
{
	return stream.set_input(input);
}

char* cmd_editor_read_cmd(void)
{
	return stream.getcmd();
}

void cmd_editor_window_resize(void)
{
	stream.resize_window();
}

class_cmd_editor::class_cmd_editor()
{
	m_ptext = NULL;
	m_text_len = 0;
	m_text_max_len = 0;
	m_text_pos = 0;
	m_logo = "";
	m_term_col = 0;
	m_term_row = 0;
	m_cur_x = 0;
	m_cur_y = 0;
	m_init_x = 0;
	m_init_y = 0;

	m_tab_handler = NULL;
	m_tab_handler_data = NULL;
	memset(&m_tab_ticks, 0, sizeof(m_tab_ticks));

	m_tab_delimiters = " ";

	m_stop = 0;
}

class_cmd_editor::~class_cmd_editor()
{
	deinit();
}

int class_cmd_editor::init(unsigned int textmaxsize, unsigned int history_len)
{
	if (textmaxsize == 0)
		return RC_CMD_EDITOR_PARAM_ERROR;

	deinit();

	m_ptext = (char*)malloc(textmaxsize+1);	// +1 - zero termination
	if (m_ptext == NULL)
		return RC_CMD_EDITOR_ALLOW_ERROR;

	memset(m_ptext, 0, sizeof(textmaxsize));
	m_text_max_len = textmaxsize;
	m_history_max_len = history_len;

	m_text_pos = 0;
	m_text_len = 0;

    if (tcgetattr(0, &term_current_state) < 0)
		return RC_CMD_EDITOR_TERM_CFG_ERROR;

	struct termios newone = term_current_state;
    newone.c_lflag &= ~ICANON;
    newone.c_lflag &= ~ECHO;
    newone.c_cc[VMIN] = 1;
    newone.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newone) < 0)
		return RC_CMD_EDITOR_TERM_CFG_ERROR;


	struct winsize w;
    int res = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	if (res < 0)
		return RC_CMD_EDITOR_TERM_PARAM_ERROR;

	m_term_col = w.ws_col;
	m_term_row = w.ws_row;

	return RC_OK;
}

int class_cmd_editor::deinit(void)
{
	if (m_ptext != NULL)
	{
		free(m_ptext);
		tcsetattr(STDIN_FILENO, TCSANOW, &term_current_state);
	}

	m_ptext = NULL;
	m_text_len = 0;
	m_text_max_len = 0;
	m_text_pos = 0;

	m_history.clear();
	m_history_max_len = 0;
	m_history_cur_elm = -1;
	return RC_OK;
}

int class_cmd_editor::stop(void)
{
	m_stop = 1;
	return RC_OK;
}

int class_cmd_editor::setlogo(const char* logo)
{
	if (logo == NULL)
		return RC_CMD_EDITOR_PARAM_ERROR;

	m_logo = logo;
	return RC_OK;
}

int class_cmd_editor::set_tab_handler(cmd_editor_tab_handler tab, void* data)
{
	m_tab_handler = tab;
	m_tab_handler_data = data;
	return RC_OK;
}

char class_cmd_editor::read_ch(int wait_ms, int use_queue)
{
	int ch = 0;
	int res;

	//printf("read_ch: size:%d  %d %d\n", m_queue.size(), wait_ms, use_queue);
	if (use_queue && m_queue.size() != 0)
	{
		ch = m_queue.front();
		m_queue.pop();
		return ch;
	}

	if (wait_ms == 0)
	{
		res = read(STDIN_FILENO, &ch, 1);
		if (res < 0)
			ch = 0;
	}
	else
	{
		fd_set set;
		struct timeval timeout;
		FD_ZERO(&set);
		FD_SET(STDIN_FILENO, &set);
		timeout.tv_sec = wait_ms/1000;
		timeout.tv_usec = (wait_ms%1000)*1000;
		int res = select(STDIN_FILENO+1, &set, NULL, NULL, &timeout);

		if (res > 0)
			res = read(STDIN_FILENO, &ch, 1);
		else
			ch = 0;
	}
	return ch;
}

int class_cmd_editor::get_cursor(int &x, int &y)
{
	char ch_set;
	printf("\033[6n");
	fflush(stdout);

	// ----------------------------------------
	// The responce is
	// ESC[#;#R
	// where # - the rows, and # - the columns
	// ----------------------------------------
	while ((ch_set=read_ch(0, 0)) != 0x1b)
	{
		m_queue.push(ch_set);
	}

	while ((ch_set=read_ch(0,0)) != '[')
	{
	}

	// to load the fist value, the current ROW
	char buf[16];
	char ch;
	int i = 0;
	while ((ch = read_ch(0,0)) != ';')
	{
		if (ch < '0' && ch > '9')
			continue;

		buf[i++] = ch;
	}
	buf[i] = 0;
	y = atoi(buf);

	i = 0;
	while ((ch = read_ch(0,0)) != 'R')
	{
		if (ch < '0' && ch > '9')
			continue;

		buf[i++] = ch;
	}
	buf[i] = 0;
	x = atoi(buf);
	return RC_OK;
}

int class_cmd_editor::set_cursor(int x, int y)
{
	char buf[32];
	if (x < 1)
	{
		while (x < 1)
		{
			x += m_term_col;
			y --;
		}
	}
	else if (x > m_term_col)
	{
		int dy = x/m_term_col;
		y += dy;
		x = (x % m_term_col);

		if (y > m_term_row)
		{
			dy = y/m_term_row;
			while (dy > 0)
			{
				putchar('\n');
				m_init_y --;
				dy --;
				y --;
			}
		}
	}
	m_cur_x = x;
	m_cur_y = y;

	// The command is:
	// ESC[{line};{column}H
	sprintf(buf, "\033[%d;%dH", y, x);
	printf("%s", buf);
	fflush(stdout);
	return RC_OK;
}

int class_cmd_editor::move_cursor(int dx)
{
	// Current cursor possition is defined by
	// m_cur_x & m_cur_y
	return set_cursor(m_cur_x+dx, m_cur_y);
}

int class_cmd_editor::update_cursor_pos(int dx)
{
	int x = m_cur_x+dx;
	int y = m_cur_y;

	if (x < 1)
	{
		while (x < 1)
		{
			x += m_term_col;
			y --;
		}
	}
	else if (x > m_term_col)
	{
		int dy = x/m_term_col;
		y += dy;
		x = (x % m_term_col);

		if (y > m_term_row)
		{
			dy = y/m_term_row;
			while (dy > 0)
			{
				putchar('\n');
				m_init_y --;
				dy --;
			}
		}
	}
	m_cur_x = x;
	m_cur_y = y;
	return RC_OK;
}

void class_cmd_editor::save_cursor(void)
{
	get_cursor(m_cur_x, m_cur_y);
}

void class_cmd_editor::load_cursor(int dx)
{
	set_cursor(m_cur_x+dx, m_cur_y);
}

void class_cmd_editor::shift_cursor(int dx)
{
	int x,y;
	get_cursor(x, y);
	set_cursor(x+dx, y);
}

int class_cmd_editor::get_text_rows_num(void)
{
	int scroll_rows = (m_text_len+m_logo.length())/m_term_col;
	// The case when all the symbols can be located in one line
	// but the cursor is located on the new line(at the end of the text)
	if ((m_text_len+(int)m_logo.length()) == m_term_col)
	{
		scroll_rows++;
	}
	return scroll_rows;
}

void class_cmd_editor::clear_input(int update_history_idx)
{
	if (m_text_len == 0)
		return;

	set_cursor(m_init_x, m_init_y);

	char* p = strdup(m_ptext);
	if (p != NULL)
	{
		memset(p, ' ', m_text_len);
		printf("%s", p);
		free(p);
		set_cursor(m_init_x, m_init_y);
	}

	m_text_pos = 0;
	m_text_len = 0;
	m_ptext[m_text_pos] = 0;

	if (update_history_idx)
	{
		m_history_cur_elm = -1;
	}
}

int class_cmd_editor::set_input(const char* pcmd, int update_history_idx)
{
	clear_input(update_history_idx);

	if (pcmd == NULL)
		return RC_CMD_EDITOR_PARAM_ERROR;

	int len = strlen(pcmd);
	if (len > m_text_max_len)
	{
		free(m_ptext);
		m_ptext = (char*)malloc(len + 1);
		if (m_ptext == NULL)
		{
			m_text_pos = 0;
			m_text_len = 0;
			m_text_max_len = 0;
			return RC_CMD_EDITOR_ALLOW_ERROR;
		}
		m_text_max_len = len;
	}
	m_ptext[0] = 0;
	m_text_pos = 0;
	m_text_len = 0;

	int pos = 0;
	while (pcmd[pos] != 0)
	{
		proc_ch(pcmd[pos]);
		pos++;
	}
	return RC_OK;
}

int class_cmd_editor::update_input(void)
{
	int logo_len = m_logo.length();
	printf("\n%s", m_logo.c_str());
	fflush(stdout);

	// The logic of this code is to take into account the situation
	// when the screen gets scrolled and <m_init_y> of original logo
	// needs to be adjusted
	int scroll_rows = (m_text_len+logo_len)/m_term_col;
	get_cursor(m_init_x, m_init_y);
	m_cur_x = m_init_x;
	m_cur_y = m_init_y;
	if (m_init_y+scroll_rows <= m_term_row)
	{
		scroll_rows = 0;
	}

	printf("%s", m_ptext);
	// The case when all the symbols can be located in one line
	// but the cursor is located on the new line(at the end of the text)
	if ((m_text_len+logo_len) == m_term_col)
	{
		printf("\n");
	}
	fflush(stdout);
	m_init_y -=scroll_rows;
	set_cursor(m_init_x, m_init_y);
	move_cursor(m_text_pos);
	return RC_OK;
}

int class_cmd_editor::resize_window(void)
{
	struct winsize w;
    int res = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	if (res < 0)
		return RC_CMD_EDITOR_TERM_PARAM_ERROR;

	int current_rows_num = get_text_rows_num();
	m_term_col = w.ws_col;
	m_term_row = w.ws_row;
	int new_rows_num = get_text_rows_num();

	if (new_rows_num != current_rows_num)
	{
		update_input();
	}
	return RC_OK;
}

int class_cmd_editor::is_equal(char* array, char* ref, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (ref[i+1] == 0)
			break;

		if (array[i] != ref[i+1])
			return 0;
	}
	return 1;
}

int class_cmd_editor::read_char(void)
{
	char ch_set[5] = {0};

	// Here to process the consoles control sequences
	// ----------------------------------------------
	if ((ch_set[0] = read_ch()) != 0x1b)
		return ch_set[0];

	if ((ch_set[1] = read_ch(150)) != 0x5b)
	{
		m_queue.push(ch_set[1]);
		return ch_set[0];
	}

	// Here is a control key combination
	// ---------------------------------
	for (int i = 2; i < 5; i++)
	{
		ch_set[i] = read_ch(10, 0);
		if (ch_set[i] == 0)
			break;
	}

	//printf("{0x%x  0x%x  0x%x  0x%x, 0x%x}\n", ch_set[0], ch_set[1], ch_set[2], ch_set[3], ch_set[4]);

	// Here we have some the control sequence
	// let's analize it and return the value
	char ctrl_keys [][1+5] =
	{
		{CMD_KEY_UP,       0x1b, 0x5b, 0x41, 0x00, 0x00},
		{CMD_KEY_DOWN,     0x1b, 0x5b, 0x42, 0x00, 0x00},
		{CMD_KEY_RIGHT,    0x1b, 0x5b, 0x43, 0x00, 0x00},
		{CMD_KEY_LEFT,     0x1b, 0x5b, 0x44, 0x00, 0x00},
		{CMD_KEY_HOME,     0x1b, 0x5b, 0x31, 0x7e, 0x00},
		{CMD_KEY_HOME,     0x1b, 0x5b, 0x48, 0x00, 0x00},
		{CMD_KEY_DEL,      0x1b, 0x5b, 0x33, 0x7e, 0x00},
		{CMD_KEY_END,      0x1b, 0x5b, 0x34, 0x7e, 0x00},
		{CMD_KEY_END,      0x1b, 0x5b, 0x46, 0x00, 0x00},
		{CMD_KEY_PAGEUP,   0x1b, 0x5b, 0x35, 0x7e, 0x00},
		{CMD_KEY_PAGEDOWN, 0x1b, 0x5b, 0x36, 0x7e, 0x00},

		{0,                0,    0,    0,    0,    0}
	};

	int key = 0;
	while(ctrl_keys[key][0] != 0)
	{
		if (is_equal(ch_set, ctrl_keys[key]))
			return CHID_CTRL_KEY(ctrl_keys[key][0]);
		key ++;
	}
	return 0;
}

int class_cmd_editor::proc_add_history(void)
{
	if (m_history_max_len == 0)
		return RC_OK;

	if (m_ptext == NULL)
		return RC_OK;

	// to check if this is just an empty string
	int i = 0;
	while (m_ptext[i] != 0)
	{
		if (m_ptext[i] != ' ')
			break;
		i++;
	}

	if (m_ptext[i] == 0)
		return RC_OK;

	if (m_history.size())
	{
		if (m_history[0] == m_ptext)
			return RC_OK;
	}

	if (m_history.size()+1 > m_history_max_len)
	{
		m_history.erase(m_history.end());
	}

	m_history_cur_elm = -1;
	m_history.insert(m_history.begin(), m_ptext);
	return RC_OK;
}

int class_cmd_editor::proc_ch(char ch)
{
	if (m_text_len + 1 > m_text_max_len)
		return RC_CMD_EDITOR_MAXLEN_ERROR;

	// if the cursor at the end of line
	// to add new one char to the end of the text
	if (m_text_pos == m_text_len)
	{
		m_ptext[m_text_pos+0] = ch;
		m_ptext[m_text_pos+1] = 0;

		putchar(ch);
		fflush(stdout);
		//update_cursor_pos(1);
		move_cursor(1);
		m_text_len++;
		m_text_pos++;
	}
	else
	{
		ins_char_in_buf(ch);
		m_text_len++;
		update_text(m_text_pos, 0);
		move_cursor(1);
		m_text_pos++;
	}
	return RC_OK;
}

int class_cmd_editor::proc_left(void)
{
	if (m_text_pos == 0)
		return RC_OK;

	m_text_pos--;
    move_cursor(-1);
	return RC_OK;
}

int class_cmd_editor::proc_right(void)
{
	if (m_text_pos == m_text_len)
		return RC_OK;

	m_text_pos++;
    move_cursor(1);
	return RC_OK;
}

int class_cmd_editor::proc_home(void)
{
	if (m_text_len == 0 || m_text_pos == 0)
		return RC_OK;

	m_text_pos = 0;
	set_cursor(m_init_x, m_init_y);
	return RC_OK;
}

int class_cmd_editor::proc_end(void)
{
	if (m_text_pos == m_text_len)
		return RC_OK;

	int dx = m_text_len - m_text_pos;
	m_text_pos = m_text_len;
	//shift_cursor(dx);
	move_cursor(dx);
	return RC_OK;
}

int class_cmd_editor::proc_del(void)
{
	if (m_text_pos == m_text_len)
		return RC_OK;

	del_char_in_buf();
	update_text(m_text_pos, 1);
	move_cursor(0);
	return RC_OK;
}

int class_cmd_editor::proc_tab(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	unsigned long long us_first = m_tab_ticks.tv_sec*1000000 + m_tab_ticks.tv_usec;
	unsigned long long us_second= tv.tv_sec*1000000 + tv.tv_usec;
	unsigned long long diff_ms;

	m_tab_ticks = tv;

	diff_ms = (us_second - us_first) / 1000;
	if (diff_ms < CMD_EDITOR_TAB_TAB_TIMEOUT)
	{
		// Here we are ready to process [TAB][TAB] command
		if (m_tab_handler && m_text_len != 0)
		{
			// Let's parse the text at the current cursor possition
			// ----------------------------------------------------
			char* p = strdup(m_ptext);
			if (p != NULL)
			{
				// to find the begining of the lexem at the cursor
				int pos = m_text_pos - 1;
				int len = 0;
				int exclude_ch = 0;
				while (pos >= 0)
				{
					if (m_tab_delimiters.find(m_ptext[pos]) != std::string::npos && exclude_ch != m_ptext[pos])
					{
						if (len == 0 && (m_possible_at_start.find(m_ptext[pos]) != std::string::npos))
						{
							exclude_ch = m_ptext[pos];
						}
						else
						{
							pos ++;
							break;
						}
					}
					len ++;
					if (pos == 0)
						break;

					pos --;
				}

				p[m_text_pos] = 0;
				if (len != 0)
				{
					char* p_ins_text = NULL;
					int res = m_tab_handler(p + pos, m_tab_handler_data, &p_ins_text);

					if(res == 2)
					{
						// we need to update the command line as
						// the handler printed the list of possible variants
						update_input();
						if (p_ins_text != NULL)
						{
							int pos = 0;
							while (p_ins_text[pos] != 0)
							{
								proc_ch(p_ins_text[pos++]);
							}
							free(p_ins_text);
						}
					}
					else if (res == 1)
					{
						// to add the text to the current possition of the cursor
						// ------------------------------------------------------
						if (p_ins_text != NULL)
						{
							int pos = 0;
							while (p_ins_text[pos] != 0)
							{
								proc_ch(p_ins_text[pos++]);
							}
							free(p_ins_text);
						}
					}
				}
				free(p);
			}
		}
	}
	return RC_OK;
}

int class_cmd_editor::proc_esc(void)
{
	clear_input();
	return RC_OK;
}

int class_cmd_editor::proc_backspace(void)
{
	if (m_text_len == 0 || m_text_pos == 0)
		return RC_OK;

	m_text_pos--;
	del_char_in_buf();
	move_cursor(-1);
	update_text(m_text_pos, 1);
	move_cursor(0);
	return RC_OK;
}

void class_cmd_editor::proc_history(int key)
{
	if (m_history.size() == 0)
		return;

	if (key == CMD_KEY_DOWN)
	{
		if (m_history_cur_elm >= (int)m_history.size())
		{
			m_history_cur_elm--;
		}
		else if (m_history_cur_elm < 0)
		{
			return;
		}

		m_history_cur_elm--;
		if (m_history_cur_elm >= 0)
		{
			std::string str = m_history[m_history_cur_elm];
			set_input(str.c_str(), 0);
		}
		else
		{
			clear_input();
		}
	}
	else // CMD_KEY_UP
	{
		if (m_history_cur_elm >= (int)m_history.size())
			return;

		if (m_history_cur_elm < 0)
			m_history_cur_elm = 0;

		std::string str = m_history[m_history_cur_elm++];
		set_input(str.c_str(), 0);
	}
}

void class_cmd_editor::proc_enter(void)
{
	// to move the cursor to the end of text
	if (m_text_pos != m_text_len)
	{
		move_cursor(m_text_len - m_text_pos);
		m_text_pos = m_text_len;
	}
	printf("\n");
	fflush(stdout);
	m_history_cur_elm = -1;
}

void class_cmd_editor::del_char_in_buf(void)
{
	if (m_text_len == 0 || m_ptext == NULL)
		return;

	if (m_text_pos < m_text_len)
	{
		memmove(m_ptext+m_text_pos, m_ptext+m_text_pos+1, m_text_len - (m_text_pos+1));
	}
	m_text_len--;
	m_ptext[m_text_len] = 0;
}

void class_cmd_editor::ins_char_in_buf(char ch)
{
	int move_size = m_text_len - m_text_pos;
	if (m_text_len == 0 || move_size == 0)
		return;

	// move_size+1 = to proc ZERO terminator
	memmove(m_ptext+m_text_pos+1, m_ptext+m_text_pos, move_size+1);
	m_ptext[m_text_pos] = ch;
}

void class_cmd_editor::update_text(int pos, int remove)
{
	if (!remove)
		printf("%s", m_ptext+pos);
	else
		printf("%s ", m_ptext+pos);
	//if (remove != 0)
	//	printf("\033[%dP", remove);
	fflush(stdout);
}

char* class_cmd_editor::getcmd(void)
{
	int ch = 0;
	if (m_ptext == NULL)
		return NULL;

	printf("%s", m_logo.c_str());
	fflush(stdout);

	m_text_pos = 0;
	m_ptext[m_text_pos] = 0;
	m_text_len = 0;

	// --------------------------------------
	// Load cursor position
	// --------------------------------------
	get_cursor(m_init_x, m_init_y);
	m_cur_x = m_init_x;
	m_cur_y = m_init_y;
	// --------------------------------------

	while (ch != '\r' && ch != '\n')
	{
		ch = read_char();
		switch(ch)
		{
			case '\r':		// Carriage return (pressed enter)
			case '\n':		// New line (pressed enter)
				proc_enter();
				proc_add_history();
				break;

			// to handle unregistered control sequence
			case 0:
				if (m_stop)
					return NULL;
				break;

			case '\t':
				proc_tab();
				break;

			case 0x1b:		// Simple ESC
				proc_esc();
				break;

			case 0x7F:		// Backspace
			case '\b':
				proc_backspace();
				break;

			case CHID_CTRL_KEY(CMD_KEY_LEFT):
				proc_left();
				break;

			case CHID_CTRL_KEY(CMD_KEY_RIGHT):
				proc_right();
				break;

			case CHID_CTRL_KEY(CMD_KEY_HOME):
				proc_home();
				break;

			case CHID_CTRL_KEY(CMD_KEY_END):
				proc_end();
				break;

			case CHID_CTRL_KEY(CMD_KEY_DEL):
				proc_del();
				break;

			case CHID_CTRL_KEY(CMD_KEY_UP):
				proc_history(CMD_KEY_UP);
				break;

			case CHID_CTRL_KEY(CMD_KEY_DOWN):
				proc_history(CMD_KEY_DOWN);
				break;

			case CHID_CTRL_KEY(CMD_KEY_PAGEUP):
				break;

			case CHID_CTRL_KEY(CMD_KEY_PAGEDOWN):
				break;

			default:
				proc_ch(ch);
				break;
		}
	}

	return strdup(m_ptext);
}

int class_cmd_editor::getcmd(std::string& cmd)
{
	char* pcmd = getcmd();
	if (pcmd == NULL)
		return RC_CMD_EDITOR_GETCMD_ERROR;

	cmd = pcmd;
	free(pcmd);
	pcmd = NULL;
	return RC_OK;
}
