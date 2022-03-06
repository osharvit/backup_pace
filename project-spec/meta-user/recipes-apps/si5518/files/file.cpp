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

#include <stdlib.h>
#include "file.h"
#include "rc.h"

class_file::class_file(void)
{
	m_file = NULL;
}

class_file::class_file(const char* filename, const char* mode)
{
	m_file = NULL;
	open(filename, mode);
}

class_file::~class_file(void)
{
	close();
}

int class_file::open(const char* filename, const char* mode)
{
	close();

	m_file = fopen(filename, mode);
	if (m_file == NULL)
		return RC_FILE_OPEN_ERROR;

	return RC_OK;
}

void class_file::close(void)
{
	if (m_file != NULL)
	{
		fclose(m_file);
		m_file = NULL;
	}
}

int class_file::is_open(void)
{
	return m_file != NULL;
}

int class_file::readfile(void* &ptr)
{
	int read_len = 0, read_offs = 0;
	int size = length();

	ptr = malloc(size + 1);
	if (ptr == NULL)
		return RC_FILE_ALLOC_ERROR;

	((char*)ptr)[size] = 0;

	while (size != 0)
	{
		read_len = fread((char*)ptr+read_offs, 1, size, m_file);
		if (read_len <= 0)
		{
			free(ptr);
			return RC_FILE_READ_ERROR;
		}
		size -= read_len;
		read_offs += read_len;
	}

	return RC_OK;
}

int class_file::length(void)
{
	if (!is_open())
		return 0;

	int cur_offs = ftell(m_file);
	fseek(m_file, 0, SEEK_END);
	int len = ftell(m_file);

	fseek(m_file, cur_offs, SEEK_SET);
	return len;
}

int class_file::write(void*p, int len)
{
	int wr_offs = 0;

	if (m_file == NULL)
		return RC_FILE_OPEN_ERROR;

	while (len > 0)
	{
		int wr_len = fwrite((char*)p + wr_offs, 1, len, m_file);
		if (wr_len <= 0)
			return RC_FILE_WRITE_ERROR;

		len -= wr_len;
		wr_offs += wr_len;
	}

	return RC_OK;
}

int class_file::read(void*p, int len)
{
	int rd_offs = 0;

	if (m_file == NULL)
		return RC_FILE_OPEN_ERROR;

	while (len > 0)
	{
		int rd_len = fread((char*)p + rd_offs, 1, len, m_file);
		if (rd_len <= 0)
			return RC_FILE_WRITE_ERROR;

		len -= rd_len;
		rd_offs += rd_len;
	}
	return RC_OK;
}

std::string class_file::read_to_str(void)
{
	void* pstr = NULL;
	int res = readfile(pstr);
	if (res < 0)
		return "";

	std::string ret = (char*)pstr;
	free(pstr);
	return ret;
}