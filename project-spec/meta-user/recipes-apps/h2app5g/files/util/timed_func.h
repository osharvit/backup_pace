#ifndef _TIMED_FUNC_H_
#define _TIMED_FUNC_H_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "mutex_lock.h"   // class mutex_lock

class timed_func
{
    public:
        timed_func();
        virtual ~timed_func();

        size_t get_count() const;
        bool is_time(size_t tick, size_t idx) const;
        void exec_item(size_t idx);
        void install_mutex(void *p1arg);
        void install_upper_watermark(size_t threshold, void (*p1func)(void *), void *p1arg);
        int add_item(void (*p1func)(void *), void *p1arg, int offset=0, int interval=1, bool override=false);
        int del_item(void (*p1func)(void *), void *p1arg);

    public:
        static const int LIST_SIZE = 200;

    protected:
        // 
        void   *mp1mutex;
        int     item_count;
        // list: mutex is needed to access the mp1funclist
        struct _timed_func {
            void  (*p1func)(void *);
            void   *p1arg;
            size_t  offset;
            size_t  interval;
            bool    enable;
        } mp1funclist[LIST_SIZE];
        // upper watermark callback
        int     upperwmthreshold;                   // upper watermark threshold
        static void  (*mp1upperwmcallback)(void *);  // upper watermark callback
        static void   *mp1upperwmarg;                // upper watermark argument

        static void defaultfunction(void *p1arg);
};  // end of class timed_func

#endif  // _TIMED_FUNC_H_
