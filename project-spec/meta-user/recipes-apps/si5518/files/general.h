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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <stdio.h>
#include <limits.h>
#include <string>
#include <vector>

#define VERSION_H                                   0
#define VERSION_L                                   1

#define MACRO_STR(x)                                #x
#define xMACRO_STR(x)                               MACRO_STR(x)
#define VERSION                                     xMACRO_STR(VERSION_H) "." xMACRO_STR(VERSION_L)

#define DEF_SPI_DEV                                 "/dev/spidev1.0"
#define DEF_SPI_BITS                                8
#define DEF_SPI_SPEED                               500000
#define DEF_SPI_MODE                                SPI_MODE_0

#define CLF_HELP                                    (1<<0)
#define CLF_CMD_LIST                                (1<<1)
#define CLF_SPI_DEV                                 (1<<2)
#define CLF_SPI_BITS                                (1<<3)
#define CLF_SPI_SPEED                               (1<<4)
#define CLF_SPI_MODE                                (1<<5)
#define CLF_CMD                                     (1<<6)
#define CLF_DUMP_COM                                (1<<7)      // To dump the communication
#define CLF_VERBOSE                                 (1<<8)
#define CLF_FILE                                    (1<<9)

#define SI5518_CMDID_READ_REPLY                     0x0
#define SI5518_CMDID_SIO_TEST                       0x1
#define SI5518_CMDID_SIO_INFO                       0x2
#define SI5518_CMDID_HOST_LOAD                      0x5
#define SI5518_CMDID_BOOT                           0x7
#define SI5518_CMDID_DEVICE_INFO                    0x8
#define SI5518_CMDID_NVM_STATUS                     0xA
#define SI5518_CMDID_RESTART                        0xF0
#define SI5518_CMDID_APP_INFO                       0x10
#define SI5518_CMDID_PLL_ACTIVE_REFCLOCK            0x11
#define SI5518_CMDID_INPUT_STATUS                   0x12
#define SI5518_CMDID_PLL_STATUS                     0x13
#define SI5518_CMDID_INTERRUPT_STATUS               0x14
#define SI5518_CMDID_METADATA                       0x15
#define SI5518_CMDID_REFERENCE_STATUS               0x16
#define SI5518_CMDID_PHASE_READOUT                  0x17
#define SI5518_CMDID_INPUT_PERIOD_READOUT           0x18
#define SI5518_CMDID_TEMPERATURE_READOUT            0x19

struct si5518_command
{
    int                         id;
    std::vector<std::string>    params;
};

struct CLF_PARAMS_TYPE
{
    unsigned int                    flags;          // The CLF_xxxx

    unsigned int                    spi_bits;
    unsigned char                   spi_mode;
    unsigned int                    spi_speed;
    char                            spi_dev[128];

    std::vector<si5518_command>     cmd_list;
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


#define PRN_VER(...)                            \
do {                                            \
extern int g_verbose;                           \
if (PRN_IS_DEV())                               \
{                                               \
        static char outbuf[PATH_MAX*2];         \
        if (g_verbose)                          \
        {                                       \
            snprintf(outbuf, sizeof(outbuf),__VA_ARGS__);           \
            PRN_SEND_TO_DEV(outbuf);            \
        }                                       \
}                                               \
else                                            \
{                                               \
    if (g_verbose)                              \
    {                                           \
        printf(__VA_ARGS__);                    \
        fflush(stdout);                         \
    }                                           \
}                                               \
}while(0)

#ifndef MIN
#define MIN(x,y)        (((x)<(y))?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)        (((x)>(y))?(x):(y))
#endif

#endif // _GENERAL_H_

