#ifndef __ITC_QUEUE_H__
#define __ITC_QUEUE_H__

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "itc_msg.h"
#include "mutex_lock.h"   // mutex_lock

class itc_queue
{
    public:
        typedef struct _elem {
            struct _elem   *p1next; // a pointer to the next link
            void           *p1body; // a pointer to service data unit
            struct _elem   *p1comp; // a pointer to the companion
            itc_queue      *p1pool; // a pointer to the queue of pool
        }elem;

    protected:
        // NOTE: The "p1deq" should be located at the 1st place
        //       the "p1enq" should be at the 2nd place of "clas itc_queue".
        elem    *p1deq;
        elem    *p1enq;

    public:
        itc_queue();
        itc_queue(itc_queue::elem *p1elem, void *p1arg, int num, int size);
        virtual ~itc_queue();

        void put(itc_queue::elem *p1elem);
        elem * get();
        int count() const;
        void link(itc_queue::elem *p1elem, void *p1arg);
        void flush();
        void set_name(const std::string &name);
        void install_mutex(void *p1arg);
        void dump();

    protected:
        int             num_elements;
        std::string name;

        void           *p1mutex;
};

#endif // __ITC_QUEUE_H__
