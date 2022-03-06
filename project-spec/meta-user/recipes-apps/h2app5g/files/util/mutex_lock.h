#ifndef __MUTEX_LOCK_H__
#define __MUTEX_LOCK_H__

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>    // pthread_mutex_t
#include <errno.h>      // errno
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

class mutex_lock
{
    public:
        mutex_lock(void *p1arg);
        virtual ~mutex_lock();

    protected:
        pthread_mutex_t    *p1mutex;
}; // end of class mutex_lock

inline
mutex_lock::mutex_lock(void *p1arg):
    p1mutex((pthread_mutex_t *)p1arg)
{
    if(p1mutex != NULL){
        if(pthread_mutex_lock(this->p1mutex) < 0) {
            TRACE0();
        }
#if 0
        else {
            TRACE3(std::cerr, mutex_lock, 0) << "lock" << std::endl;
        }
#endif
    }
}

inline
mutex_lock::~mutex_lock()
{
    if(this->p1mutex != NULL) {
        pthread_mutex_unlock(this->p1mutex);
#if 0
        TRACE3(std::cerr, mutex_lock, 0) << "unlock" << std::endl;
#endif
    }
}


#endif // __MUTEX_LOCK_H__
