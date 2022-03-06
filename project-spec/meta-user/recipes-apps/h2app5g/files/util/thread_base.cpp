#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.

#include <stdio.h>
#include <stdlib.h>
#include <iostream>     // std::stringstream
#include <sstream>      // std::stringstream
#include <pthread.h>

#include "thread_base.h"

// static instantiation
timed_func      thread_base::timed_list;
pthread_t       thread_base::p1thread[NUM_THREAD];
itc_queue       thread_base::p1itc_q[NUM_THREAD];

itc_queue::elem thread_base::p1itcelem[MAX_ITC_ELEM];
itc_msg         thread_base::p1itcmsgbuf[MAX_ITC_ELEM];
itc_queue       thread_base::pool_q(thread_base::p1itcelem, (void *)thread_base::p1itcmsgbuf, MAX_ITC_ELEM, sizeof(itc_msg));


thread_base::thread_base(const std::string &name, size_t id) :
    thread(thread_base::p1thread[id]),
    thread_name(name),
    thread_id(id)

{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }
}

thread_base::~thread_base() {}

void thread_base::initialize_statically(void *p1mutex)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }

    // install mutex
    thread_base::pool_q.install_mutex(p1mutex);
    for(size_t kk = 0; kk < NUM_THREAD; kk++) {
        thread_base::p1itc_q[kk].install_mutex(p1mutex);
    }
}

std::string thread_base::objname() const
{
    return(thread_name);
}

void thread_base::join()
{
    pthread_join(this->thread, NULL);   // wait for thread termination 
} // end of void thread_base::join()

void thread_base::send_event(size_t thread_id, int event)
{
    if(thread_id < NUM_THREAD){
        pthread_kill(thread_base::p1thread[thread_id],event); // send event signal to the thread
    }
}

void thread_base::send_event(int event)
{
    // send event to itself
    send_event(thread_id, event);
}

void thread_base::send_itc_message(int src_thread_id,
                                   int dst_thread_id,
                                   uint16_t msg_id,
                                   uint16_t body_len,
                                   const uint8_t *sdu)
{
    itc_queue::elem *p1elem = NULL;

    if(body_len > MAX_ITC_MSG_BYTE){
        TRACE0OBJ()
            << "------------------------------" << std::endl
            << "warning: body_lenth for id(" << msg_id << ") is too large;" << body_len << " > " << MAX_ITC_MSG_BYTE << std::endl
            << "------------------------------" << std::endl;

        body_len = std::min<uint16_t>(body_len, MAX_ITC_MSG_BYTE);
    }

    if((p1elem = pool_q.get()) != NULL)
    {
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        p1msg->hdr.msgid = msg_id;
        p1msg->hdr.msglen = sizeof(itc_hdr) + body_len;
        p1msg->hdr.msgsrc = src_thread_id;
        p1msg->hdr.msgdst = dst_thread_id;

        // TODO: use p1comp for longer message
        if(!((p1msg->hdr.msglen >= sizeof(itc_hdr)) && ((p1msg->hdr.msglen + 1) <= (uint16_t)sizeof(itc_msg)))){
            TRACE0() << "give-up due to oversized msglen=" << p1msg->hdr.msglen << std::endl;
            p1elem->p1pool->put(p1elem);
            return;
        }
        if(body_len > 0) {
            char *p1buf = (char *)&p1msg->startofbody;
            memcpy(p1buf, sdu, body_len);
            p1buf[body_len] = '\0'; // end of character
        }
        p1itc_q[dst_thread_id].put(p1elem);

#if defined(UNITTEST)  // do NOT use for unittest
#else
        // send signal to wakeup the thread
        pthread_kill(p1thread[dst_thread_id], EVENT_ITC);
#endif
    }
}   // end of void thread_base::send_itc_message()

void thread_base::send_itc_message(int dst_thread_id,
                                   uint16_t msg_id,
                                   uint16_t body_len,
                                   const uint8_t *sdu)
{
    send_itc_message(this->thread_id, dst_thread_id, msg_id, body_len, sdu);
}

void thread_base::dump_itc_q()
{
    for(size_t kk = 0; kk < NUM_THREAD; kk++) {
        int cnt = p1itc_q[kk].count();
        if(cnt > 0) {
            std::cout << "thread_id=" << kk << ", count=" << cnt;
            p1itc_q[kk].dump();
        }
    }
    std::cout << "pool count=" << pool_q.count() << std::endl;
}


void thread_base::error(const std::string &errmsg)
{
    perror(errmsg.c_str());
    exit(1);
}
