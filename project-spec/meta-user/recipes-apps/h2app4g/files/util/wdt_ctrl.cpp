/*=============================================================
 * File Name:         @file wdt_ctrl.cpp
 *
 * Last committed:    $Revision: 25765 $
 * Last changed by:   $Author: bybae $
 * Last changed date: $Date: 2015-10-22 10:06:03 -0700 (Thu, 22 Oct 2015) $
 * ID:                $Id: wdt_ctrl.cpp 25765 2015-10-22 17:06:03Z bybae $
 *=============================================================*/
#include <stdio.h>
#include <stdlib.h>     // NULL
#include <string.h>     // strcmp()
#include <assert.h>     // assert()
#include <fcntl.h>      // open(), O_RDWR, O_NOCTTY, O_NONBLOCK
#include <unistd.h>     // close()
#include <sys/ioctl.h>  // ioctl()
#include <time.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <sstream>      // std::stringstream
#include <signal.h>

#include "wdt_ctrl.h"

/*---------------------------------------------------------------------
 * DEFINES
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------
 * TYPES
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------
 * VARIABLES
 *---------------------------------------------------------------------*/

/*--------------------------------------------------------------------
 * EXTERNS
 *--------------------------------------------------------------------*/

/*---------------------------------------------------------------------
 * Functions
 *---------------------------------------------------------------------*/
wdt_ctrl::wdt_ctrl(const std::string &dev_path):
    devpath(dev_path)
{
    this->fd = -1;
    this->pid = -1;
}

void wdt_ctrl::initialize()
{
    FILE      *fp;
    char  buf[32];

    if( (fp = popen("pidof watchdog", "r")) != NULL)
    {
        if(fgets(buf, sizeof(buf), fp) )
        {
          this->pid = strtol(buf, NULL, 0);
          TRACE0() << "the watchdog process[" << this->pid << "] is running already." << std::endl;

        }
        pclose(fp);
    }

#if 0		// chaega 08/26/2021
    if ( this->pid < 0 )
    {
        if((this->fd = open(this->devpath.c_str(), O_WRONLY)) < 0) {
            std::stringstream sstrm;
            TRACE3(sstrm, blocking, 0) << "open device " << this->devpath << std::endl;
            perror(sstrm.str().c_str());
        }
        else {
            TRACE0() << "the " << this->devpath << " device is opened." << std::endl;
        }
    }
#endif
}

void wdt_ctrl::allow_close()
{
    if(fd >= 0){
        (void)write(fd, "V", 1);  // stop when close
    }
}

void wdt_ctrl::keep_alive()
{
    if(pid >= 0){
        kill(pid, SIGIO);
    }
    if(fd >= 0){
        ioctl(fd, WDIOC_KEEPALIVE, 0);
    }
}

int wdt_ctrl::set_time_out(int time)
{
    int ret = 0;

    if(fd >= 0){
        ret = ioctl(fd, WDIOC_SETTIMEOUT, &time);
    }

    return ret;
}

int wdt_ctrl::get_time_out()
{
    int time = 0;

    if(fd >= 0){
        ioctl(fd, WDIOC_GETTIMEOUT, &time);
    }

    return(time);
}

/*  vim:  set ts=4 sts=4 sw=4 et cin   : */
