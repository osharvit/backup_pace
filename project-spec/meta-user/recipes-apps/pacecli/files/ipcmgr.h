/*
 * Copyright(c) 2021 Parallel Wireless. All rights reserved.
 *
 *  This class implements IPC server interface
 *  and provides access to CLI API over this IPC mechanism
 *    set/get/or/xor registers
 *    save/load data to/from file
 *    print message to CLI console
 *    etc (please see the complete IPC API interface)
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

#ifndef _IPC_MGR_H_
#define _IPC_MGR_H_

#include "gen-types.h"
#include "retcodes.h"
#include "cli_ipc_ctrl_inf.h"

class class_cmdmgr;

class class_ipc_mgr
{
public:
                        class_ipc_mgr(void);
                        ~class_ipc_mgr(void);

            int         init(CLF_PARAMS_TYPE* p, class_cmdmgr* p_cmdmgr);

protected:


private:

        class_cmdmgr*       m_pcmdmgr;
        void*               m_h_ipc_serv;
};


#endif // _IPC_MGR_H_
