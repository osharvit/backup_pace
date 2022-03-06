/*=============================================================
 * File Name:         @file wdt_ctrl.h
 *
 * Last committed:    $Revision: 25691 $
 * Last changed by:   $Author: jhkim $
 * Last changed date: $Date: 2015-08-28 14:50:37 -0700 (Fri, 28 Aug 2015) $
 * ID:                $Id: wdt_ctrl.h 25691 2015-08-28 21:50:37Z jhkim $
 *=============================================================*/
#ifndef _WDT_CTRL_H_
#define _WDT_CTRL_H_

#include "num_def.h"
#include <sys/types.h>
#include <stdint.h>
#include "hal.h"

/*---------------------------------------------------------------------
 * DEFINES
 *---------------------------------------------------------------------*/
#define WATCHDOG_IOCTL_BASE    'W'

struct watchdog_info
{
    uint32_t  options;            /* Options the card/driver supports */
    uint32_t  firmware_version;   /* Firmware version of the card */
    uint8_t   identity[32];       /* Identity of the board */
};

#define WDIOC_GETSUPPORT        _IOR(WATCHDOG_IOCTL_BASE, 0, struct watchdog_info)
#define WDIOC_GETSTATUS         _IOR(WATCHDOG_IOCTL_BASE, 1, int)
#define WDIOC_GETBOOTSTATUS     _IOR(WATCHDOG_IOCTL_BASE, 2, int)
#define WDIOC_GETTEMP           _IOR(WATCHDOG_IOCTL_BASE, 3, int)
#define WDIOC_SETOPTIONS        _IOR(WATCHDOG_IOCTL_BASE, 4, int)
#define WDIOC_KEEPALIVE         _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT        _IOR(WATCHDOG_IOCTL_BASE, 7, int)
#define WDIOC_SETPRETIMEOUT     _IOWR(WATCHDOG_IOCTL_BASE, 8, int)
#define WDIOC_GETPRETIMEOUT     _IOR(WATCHDOG_IOCTL_BASE, 9, int)
#define WDIOC_GETTIMELEFT       _IOR(WATCHDOG_IOCTL_BASE, 10, int)

#define WDIOF_UNKNOWN           -1      /* Unknown flag error */
#define WDIOS_UNKNOWN           -1      /* Unknown status error */

#define WDIOF_OVERHEAT          0x0001  /* Reset due to CPU overheat */
#define WDIOF_FANFAULT          0x0002  /* Fan failed */
#define WDIOF_EXTERN1           0x0004  /* External relay 1 */
#define WDIOF_EXTERN2           0x0008  /* External relay 2 */
#define WDIOF_POWERUNDER        0x0010  /* Power bad/power fault */
#define WDIOF_CARDRESET         0x0020  /* Card previously reset the CPU */
#define WDIOF_POWEROVER         0x0040  /* Power over voltage */
#define WDIOF_SETTIMEOUT        0x0080  /* Set timeout (in seconds) */
#define WDIOF_MAGICCLOSE        0x0100  /* Supports magic close char */
#define WDIOF_PRETIMEOUT        0x0200  /* Pretimeout (in seconds), get/set */
#define WDIOF_KEEPALIVEPING     0x8000  /* Keep alive ping reply */

#define WDIOS_DISABLECARD       0x0001  /* Turn off the watchdog timer */
#define WDIOS_ENABLECARD        0x0002  /* Turn on the watchdog timer */
#define WDIOS_TEMPPANIC         0x0004  /* Kernel panic on temperature trip */

/*--------------------------------------------------------------------
 * TYPES
 *--------------------------------------------------------------------*/

//#------------------------------------------------------------
//! @class  wdt_ctrl
//! @brief  todo_brief_description
//! todo_more_detailed_class_description
//! @see    todo_reference
//#------------------------------------------------------------
class wdt_ctrl
{
    public:
        wdt_ctrl(const std::string &dev_path);
        virtual ~wdt_ctrl() {}

        void installMutex(void *p1arg);

        void initialize();

        void allow_close();
        void keep_alive();
        int set_time_out(int time);
        int get_time_out();
        void setDesc(int desc);

    protected:
        std::string devpath;
        int fd;
        pid_t pid;
        hal::u32le_t pl_version;

    private:
        // block to use the default constructor
        wdt_ctrl();
};

/*--------------------------------------------------------------------
 * EXTERNS
 *--------------------------------------------------------------------*/


#endif  //_FPGA_AHB_H_
/*  vim:  set ts=4 sts=4 sw=4 et cin   : */
