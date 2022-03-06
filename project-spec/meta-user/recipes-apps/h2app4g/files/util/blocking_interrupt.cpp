
#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>     // NULL
#include <unistd.h>     // close()
#include <string.h>     // memset()
#include <fcntl.h>      // open(), O_RDWR, O_NOCTTY, O_NONBLOCK
#include <sys/ioctl.h>  // ioctl()
#include <iostream>
#include <string>
#include <sstream>      // std::stringstream
#include "blocking_interrupt.h"

blocking_interrupt::blocking_interrupt(const std::string &dev_path):
    devpath(dev_path)
{
    fd = -1;
}

blocking_interrupt::~blocking_interrupt()
{
    if(this->fd >= 0){
        close(this->fd);
    }
}

void blocking_interrupt::initialize()
{
    if((this->fd = open(this->devpath.c_str(), O_RDONLY)) < 0) {
        std::stringstream sstrm;
        TRACE3(sstrm, blocking, 0) << "open device " << this->devpath << std::endl;
        perror(sstrm.str().c_str());
    }
    else {
        TRACE0() << "the " << this->devpath << " device is opened." << std::endl;
    }
}

int blocking_interrupt::wait_for_interrupt()
{
    int status = 0;
    const int bufsize = 16;
    char buffer[bufsize + 1];

    memset(buffer, '\0', bufsize + 1);

    if(this->fd >= 0){
        // this read() function is connected to a blocking kernel driver which returns
        // 0 if the @timeout elapes,
        // -ERESTARTSYS if it was interrupted by a signal, or
        // remaing jiffies otherwise.
        if(read(this->fd, buffer, bufsize) > 0){
            status = atoi(buffer);
            //TRACE0() << "buffer=" << buffer << ", status=" << status << std::endl;
        }
    }

    return(status);
}

/*  vim:  set ts=4 sts=4 sw=4 et cin   : */
