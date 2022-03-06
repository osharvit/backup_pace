#ifndef _TIME_THREAD_H_
#define _TIME_THREAD_H_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <sstream>      // std::stringstream
#include <time.h>       // struct timespec, clock_gettime(), timer_create(), timer_settime(), timer_
#include <signal.h>     // sigaction()
#include <memory.h>     // memset()
#include "thread_base.h"

#undef  USE_SIGACTION_TIMER

class time_thread: public thread_base
{
    public:
        virtual ~time_thread();

        static time_thread *singleton(const std::string &name, size_t id);  // for singleton pattern
        time_thread(const std::string &name, size_t id);
        void start_time_thread();
#if defined(USE_SIGACTION_TIMER)
        static void  timer_wrapper(int arg);
#else
        static void *start_wrapper(void *p1arg);
#endif

        size_t  get_interval_ms() const;
        size_t  get_tick() const;
        void    inc_tick();

    protected:
        void run_time_thread();

    protected:
        volatile size_t     time_tick;
        static const size_t interval_in_msec = 10;    // 10 ms is recommended
        static time_thread *sp1instance;        // for singleton pattern

#if defined(USE_SIGACTION_TIMER)
        static struct sigaction    sa;
        static timer_t timer_id;
#endif
}; // end of class time_thread

#endif // _TIME_THREAD_H_
