#ifndef _THREAD_BASE_H_
#define _THREAD_BASE_H_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>     // memcpy()
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <pthread.h>
#include <signal.h>         // SIGUSR1, SIGUSR2
#include "itc_msg.h"
#include "itc_queue.h"    // itc_queue
#include "timed_func.h"     // class timed_func

enum {
    EVENT_ITC       = SIGUSR1,
    EVENT_SOCKET    = SIGUSR2,
};

class thread_base
{
    public:
        thread_base(const std::string &name, size_t id);
        virtual ~thread_base();

        static void initialize_statically(void *p1mutex);

        void join();
        void send_event(size_t thread_id, int event);
        void send_event(int event);
        void error(const std::string &errmsg);
        void send_itc_message(int src_thread_id,
                              int dst_thread_id,
                              uint16_t msg_id,
                              uint16_t body_len,
                              const uint8_t *sdu);
        void send_itc_message(int dst_thread_id,
                              uint16_t msg_id,
                              uint16_t body_len,
                              const uint8_t *sdu);
        void dump_itc_q();
        std::string objname() const;

    public:
        static timed_func   timed_list;

    protected:
        pthread_t      &thread;
        pthread_attr_t  threadattr;
        sigset_t        sigmask;
        std::string     thread_name;
        size_t          thread_id;


        static pthread_t    p1thread[NUM_THREAD];
        static itc_queue::elem  p1itcelem[MAX_ITC_ELEM];
        static itc_msg      p1itcmsgbuf[MAX_ITC_ELEM];

        static itc_queue    p1itc_q[NUM_THREAD];
        static itc_queue    pool_q;
}; // end of class thread_base

#endif // _THREAD_BASE_H_
