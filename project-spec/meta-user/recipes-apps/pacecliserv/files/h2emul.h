/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class emulates H2 APP connection, it can receive the JSON commands
 *  to print them and to return some responces, also it may generate some indications
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef _H2_EMUL_H_
#define _H2_EMUL_H_

#include <pthread.h>
#include "socket.h"
#include "retcodes.h"
#include "gen-types.h"
#include <vector>

class class_h2_emulator
{
public:
                class_h2_emulator(void);
                ~class_h2_emulator(void);

            int init(CLF_PARAMS_TYPE* p);
            int run_client(class_socket* psocket);
            void close_clients(void);
            void close_stopped(void);

protected:

private:

    class_socket*   m_psocket;
};

#endif // _H2_EMUL_H_