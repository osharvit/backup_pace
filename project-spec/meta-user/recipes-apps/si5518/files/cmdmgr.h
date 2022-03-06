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
#ifndef _CMD_MGR_H_
#define _CMD_MGR_H_

#include "general.h"
#include "spimgr.h"
#include "rc.h"

// ----------------------------------------------------------------------------
struct cmd_READ_REPLY
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_READ_REPLY_ack
{
    unsigned char       status;
    union 
    {
        struct 
        {
            unsigned char val[2];
        }bytes;
        unsigned short      value;
    }err;
}__attribute__((packed));

// ----------------------------------------------------------------------------

#define cmd_SIO_TEST_data_size      16
struct cmd_SIO_TEST
{
    unsigned char       id;
    unsigned char       data[cmd_SIO_TEST_data_size];
}__attribute__((packed));

struct cmd_SIO_TEST_ack
{
    unsigned char       status;
    unsigned char       cmd_id;
    unsigned char       data[cmd_SIO_TEST_data_size];
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_SIO_INFO
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_SIO_INFO_ack
{
    unsigned char       status;
    unsigned short      cmd_buf_size;
    unsigned short      repl_buf_size;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_HOST_LOAD
{
    unsigned char       id;
    unsigned char       data[1];
}__attribute__((packed));

struct cmd_HOST_ack
{
    unsigned char       status;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_BOOT
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_BOOT_ack
{
    unsigned char       status;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_DEVICE_INFO
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_DEVICE_INFO_ack
{
    unsigned char       status;
    unsigned short      part_num;
    unsigned char       dev_grade;
    unsigned char       dev_revision;
    unsigned int        opn_id;
    unsigned char       opn_rev;
    unsigned char       temp_grade;
    unsigned char       package;
    unsigned char       res0;
    unsigned char       rom_rev;
    unsigned char       res1[6];
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_NVM_STATUS
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_NVM_STATUS_ack
{
    unsigned char       status;
    unsigned char       err2_cnt;
    unsigned char       err1_cnt;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_RESTART
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_RESTART_ack
{
    unsigned char       status;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_APP_INFO
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_APP_INFO_ack
{
    unsigned char       status;
    unsigned char       app_major;
    unsigned char       app_minor;
    unsigned char       app_branch;
    unsigned short      app_build;
    unsigned char       planner_major;
    unsigned char       planner_minor;
    unsigned char       planner_branch;
    unsigned short      planner_build;
    unsigned char       design_id[8];
    unsigned char       cbpro_rev_major;
    unsigned char       cbpro_rev_minor;
    unsigned char       cbpro_rev_revision;
    unsigned char       cbpro_rev_special;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_PLL_ACTIVE_REFCLOCK
{
    unsigned char       id;
    unsigned char       pll;
}__attribute__((packed));

struct cmd_PLL_ACTIVE_REFCLOCK_ack
{
    unsigned char       status;
    unsigned char       active_pll_refclk;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_INPUT_STATUS
{
    unsigned char       id;
    unsigned char       input;
}__attribute__((packed));

struct cmd_INPUT_STATUS_ack
{
    unsigned char       status;
    unsigned char       inp_clk_validation;
    unsigned char       loss_of_signal;
    unsigned char       out_of_freq;
    unsigned char       phase_monitor;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_PLL_STATUS
{
    unsigned char       id;
    unsigned char       pll;
}__attribute__((packed));

struct cmd_PLL_STATUS_ack
{
    unsigned char       status;
    unsigned char       PLL_LOSS_OF_LOCK_MISC;
    unsigned char       PLL_STATUS;
    unsigned char       PLL_SLIP_COUNT;
    unsigned char       PLL_SLIP_COUNT_NET;
    unsigned char       PLL_HOLDOVER_VALID;
    unsigned char       PLL_HOLDOVER;
    unsigned char       PLL_SHORT_TERM_HOLDOVER;
    unsigned char       PLL_PHASE_BLEEDOUT;
    unsigned char       PLL_LOOP_FILTER_STATUS;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_INTERRUPT_STATUS
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_INTERRUPT_STATUS_ack
{
    unsigned char       status;
    unsigned char       INPUT_CLOCK_INVALID;
    unsigned char       INPUT_CLOCK_VALID;
    unsigned char       PLLR;
    unsigned char       PLLA;
    unsigned char       PLLB;
    unsigned char       GENERAL;
    unsigned char       SOFTWARE;
    unsigned char       PHASE_CONTROL;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_METADATA
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_METADATA_ack
{
    unsigned char       status;
    unsigned int        DCO_MR_STEP_SIZE;
    unsigned int        DCO_NA_STEP_SIZE;
    unsigned int        DCO_MA_STEP_SIZE;
    unsigned int        DCO_NB_STEP_SIZE;
    unsigned int        DCO_MB_STEP_SIZE;
    unsigned char       PLAN_OPTIONS;
    unsigned int        PHASE_JAM_PPS_OUT_RANGE_HIGH;
    unsigned int        PHASE_JAM_PPS_OUT_RANGE_LOW;
    unsigned long long  PHASE_JAM_PPS_OUT_STEP_SIZE;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_REFERENCE_STATUS
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_REFERENCE_STATUS_ack
{
    unsigned char       status;
    unsigned char       REFERENCE_CLOCK_VALIDATION;
    unsigned char       LOSS_OF_SIGNAL;
    unsigned char       OUT_OF_FREQUENCY;
    unsigned char       PHASE_MONITOR;
}__attribute__((packed));

// ----------------------------------------------------------------------------

struct cmd_PHASE_READOUT
{
    unsigned char       id;
    unsigned char       PHASE_GROUP;        // bits [3:0]
}__attribute__((packed));

struct cmd_PHASE_READOUT_ack
{
    unsigned char       status;
    unsigned char       ERRORS;
    unsigned char       PHASE_VALID;
    unsigned long long  PHASE_READOUT;
}__attribute__((packed));

// ----------------------------------------------------------------------------
struct cmd_INPUT_PERIOD_READOUT
{
    unsigned char       id;
    unsigned char       ref_a;        // bits [7:0]
    unsigned char       ref_b;        // bits [7:0]
}__attribute__((packed));

struct cmd_INPUT_PERIOD_READOUT_ack
{
    unsigned char       status;
    unsigned long long  PERIOD_READOUT_A;
    unsigned long long  PERIOD_READOUT_B;
}__attribute__((packed));

// ----------------------------------------------------------------------------
struct cmd_TEMPERATURE_READOUT
{
    unsigned char       id;
}__attribute__((packed));

struct cmd_TEMPERATURE_READOUT_ack
{
    unsigned char       status;
    unsigned int        TEMPERATURE_READOUT;
}__attribute__((packed));
// ----------------------------------------------------------------------------

struct host_load_cmd
{
	cmd_HOST_LOAD* 	cmd;
	int 			total_size;
};

class class_cmdmgr
{
public:
            class_cmdmgr(class_spimgr* spi, int cmd_id);
            ~class_cmdmgr(void);
            int proc(std::vector<std::string>& params);
protected:
            static int is_error(unsigned char status);
    
            int create_command(std::vector<std::string>& params);
            int alloc_buffers(void);

            int create_READ_REPLY(void);
            int show_READ_REPLY_ack(void);

            int create_SIO_TEST(void);
            int show_SIO_TEST_ack(void);

            int create_SIO_INFO(void);
            int show_SIO_INFO_ack(void);

            int create_HOST_LOAD(std::vector<std::string>& params);
            int show_HOST_LOAD_ack(void);

            int create_BOOT(void);
            int show_BOOT_ack(void);

            int create_DEVICE_INFO(void);
            int show_DEVICE_INFO_ack(void);

            int create_NVM_STATUS(void);
            int show_NVM_STATUS_ack(void);

            int create_RESTART(void);
            int show_RESTART_ack(void);

            int create_APP_INFO(void);
            int show_APP_INFO_ack(void);

            int create_PLL_ACTIVE_REFCLOCK(std::vector<std::string>& params);
            int show_PLL_ACTIVE_REFCLOCK_ack(void);

            int create_INPUT_STATUS(std::vector<std::string>& params);
            int show_INPUT_STATUS_ack(void);

            int create_PLL_STATUS(std::vector<std::string>& params);
            int show_PLL_STATUS_ack(void);

            int create_INTERRUPT_STATUS(std::vector<std::string>& params);
            int show_INTERRUPT_STATUS_ack(void);

            int create_METADATA(std::vector<std::string>& params);
            int show_METADATA_ack(void);

            int create_REFERENCE_STATUS(std::vector<std::string>& params);
            int show_REFERENCE_STATUS_ack(void);

            int create_PHASE_READOUT(std::vector<std::string>& params);
            int show_PHASE_READOUT_ack(void);

            int create_INPUT_PERIOD_READOUT(std::vector<std::string>& params);
            int show_INPUT_PERIOD_READOUT_ack(void);

            int create_TEMPERATURE_READOUT(std::vector<std::string>& params);
            int show_TEMPERATURE_READOUT_ack(void);

            class_spimgr*       m_spi;
            int                 m_cmdid;

            unsigned char*      m_pInStream;
            unsigned char*      m_pOutStream;
            unsigned int        m_InStreamSize;
            unsigned int        m_OutStreamSize;
};

#endif // _CMD_MGR_H_
