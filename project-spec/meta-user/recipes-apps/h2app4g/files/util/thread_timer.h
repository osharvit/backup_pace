#ifndef _THREAD_TIMER_H_
#define _THREAD_TIMER_H_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>   // timer_t, timer_create()

class thread_timer
{
    public:
        timer_t timerid;
};  // end of class thread_timer

#endif // _THREAD_TIMER_H_
