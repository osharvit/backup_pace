#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream
#include <algorithm>    // std::remove_if()
#include "mplane_mon_thread.h"

mplane_mon_thread::mplane_mon_thread(const std::string &name,
                         size_t thread_id,
                         int listen_port,
                         size_t connsock_thread_base,
                         const std::list<size_t> &connsockid):
    tcpserv_thread(name, thread_id, listen_port, connsock_thread_base, connsockid)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
} // end of mplane_mon_thread::mplane_mon_thread(...)

mplane_mon_thread::mplane_mon_thread(const std::string &name,
                         size_t thread_id,
                         size_t connect_socket_id,
                         size_t listen_thread_id):
    tcpserv_thread(name, thread_id, connect_socket_id, listen_thread_id)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    this->install_state_handler(CONNECTED_S,
                                &mplane_mon_thread::mplane_mon_connected_entry,
                                &mplane_mon_thread::mplane_mon_connected_state,
                                &mplane_mon_thread::mplane_mon_connected_exit);
} // end of mplane_mon_thread::mplane_mon_thread(...)

mplane_mon_thread::~mplane_mon_thread() {}

void mplane_mon_thread::install_state_handler(size_t state,
                                        void (mplane_mon_thread::*entry_func)(),
                                        size_t (mplane_mon_thread::*state_func)(void *, int),
                                        void (mplane_mon_thread::*exit_func)())
{
    tcpserv_thread::install_state_handler(state,
                                          static_cast<void (tcpserv_thread::*)()>(entry_func),
                                          static_cast<size_t (tcpserv_thread::*)(void *, int)>(state_func),
                                          static_cast<void (tcpserv_thread::*)()>(exit_func));
} // end of bool tcpserv_thread::install_state_handler(...)

void mplane_mon_thread::install_state_handler(size_t state, size_t (mplane_mon_thread::*state_func)(void *, int))
{
    tcpserv_thread::install_state_handler(state, static_cast<size_t (tcpserv_thread::*)(void *, int)>(state_func));
} // end of bool tcpserv_thread::install_state_handler(...)

void mplane_mon_thread::mplane_mon_connected_entry()
{
    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT) | (1UL << MASKBIT_SOCKET))){
        TRACE0OBJ();
    }

    tcpserv_thread::tcpserv_connected_entry();
    //
    this->residual.clear();
}

/* call flow:--------------------------------------------------------------------
 *  <start_mplane_mon_thread class="mplane_mon_thread">
 *      <start_wrapper class="tcpserv_thread">
 *          <run_tcpserv_thread class="tcpserv_thread">
 *               <process_connsock_event class="tcpserv_thread">
 *                   <dispatch_event class="stm">
 *                       <mplane_mon_connected_state class="mplane_mon_thread">
 *                          <send_itc_message class="thread_base"/>
 *                       </mplane_mon_connected_state>
 *                   </dispatch_event>
 *               </process_connsock_event>
 *               <process_itc_event class="mplane_mon_thread">
 *                   <dispatch_event class="stm">
 *                       <mplane_mon_connected_state class="mplane_mon_thread">
 *                          <send_msg class="mplane_mon_thread"/>
 *                       </mplane_mon_connected_state>
 *                   </dispatch_event>
 *               </process_itc_event>
 *          </run_tcpserv_thread>
 *      </start_wrapper>
 *  </start_mplane_mon_thread>
 */
size_t mplane_mon_thread::mplane_mon_connected_state(void *p1arg, int event)
{
    (void)(p1arg);  // to avoid of unused warning
    (void)(event);  // to avoid of unused warning
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    if(event == EVENT_SOCKET) {
        struct tcpserv_thread::rdbufstruct *inbuf = (struct tcpserv_thread::rdbufstruct *)p1arg;

        std::string in = std::string(inbuf->buf);
#if defined(UNITTEST_TRACE)
        TRACE0OBJ() << "in=" << std::endl << in << std::endl;
#endif
        {
            std::vector<std::string> vsdu;

            this->segment_reassemble(vsdu, residual, in);

#if 1
            for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
                // NOTE: send received hswi message to the h2app_thread.
                send_itc_message(THREAD_IL,
                                 ITC_PAYLOAD_ID,
                                 it->size(),
                                 (const uint8_t *)it->c_str());
            }
#else
            for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
                // NOTE: send received hswi message to the h2app_thread.
                send_itc_message(THREAD_RE,
                                 ITC_PAYLOAD_FROM_HSWI_ID,
                                 it->size(),
                                 (const uint8_t *)it->c_str());
            }
#endif
        }
    }
    else if(event == EVENT_ITC) {
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid){
            case ITC_NEW_CONNSOCK_ID: {
                if(p1msg->new_connsock.sockfd >= 0) {
                    if(this->connect_socket >= 0) {
                        close(this->connect_socket);
                    }
                    this->connect_socket = p1msg->new_connsock.sockfd;

                    if(g_debug_mask & ((1UL << MASKBIT_EVENT) | (1UL << MASKBIT_SOCKET))){
                        TRACE0OBJ()
                            << "new_connsock=" << this->connect_socket << std::endl;
                    }

                    // set flag of asynchronous and nonblock; usually it is not recommended because of its complexity
                    fcntl(this->connect_socket, F_SETFL, O_ASYNC | O_NONBLOCK);
                    // NOTE: it may be necessary to pass F_SETOWN the result of gettid() instead of getpid()
                    // to get sensible result when F_SETSIG is used
                    fcntl(this->connect_socket, F_SETOWN, gettid());
                    // set the signal sent
                    fcntl(this->connect_socket, F_SETSIG, EVENT_SOCKET);;

                    int         optval = 1;
                    socklen_t   optlen = sizeof(optval);

                    // set TCP keepalive
                    setsockopt(this->connect_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
                    // send segment as soon as possible even if there is only a small amount of data
                    setsockopt(this->connect_socket, IPPROTO_TCP, TCP_NODELAY, &optval, optlen);
                }
                break;
            }
            case ITC_DIS_CONNSOCK_ID: {
                if(p1msg->dis_connsock.connid < (int)this->connidlist.size()){
                    // find and push_front
                    this->connidlist.remove(p1msg->dis_connsock.connid);
                    this->connidlist.push_front(p1msg->dis_connsock.connid);
                }
                break;
            }
            case ITC_MPMON_ID: {
                std::string strsdu(p1msg->payload.sdu);

                // NOTE: send received ITC message to the HSWI socket
                this->send_msg(strsdu);
                break;
            }
            default:{
                TRACE0OBJ()
                    << "ignore the msgid=" << p1msg->hdr.msgid
                    << std::endl;
            }
        }
    }

    return(this->currstate);
}

void mplane_mon_thread::mplane_mon_connected_exit()
{
    tcpserv_thread::tcpserv_connected_exit();
}


//-------------------------------------------------------------------------------
// mplane_mon_thread
//-------------------------------------------------------------------------------
void mplane_mon_thread::start_mplane_mon_thread()
{
    // Initialize and set thread joinable
    pthread_attr_init(&this->threadattr);
    //pthread_attr_setdetachstate(&tthis->threadattr, PTHREAD_CREATE_JOINABLE);
    sigemptyset(&this->sigmask);   // clear signals
    sigaddset(&this->sigmask, EVENT_ITC);
    sigaddset(&this->sigmask, EVENT_SOCKET);
    sigaddset(&this->sigmask, SIGPIPE);    // NOTE: double-protection against crash due to SIGPIPE
    if(pthread_sigmask(SIG_BLOCK, &this->sigmask, NULL) != 0){
        perror("pthread_sigmask()");
    }

    pthread_create(&this->thread, &this->threadattr, tcpserv_thread::start_wrapper, (void *)this);
    pthread_detach(this->thread); // mark the thread to return its reources automatically in terminating
} // end of mplane_mon_thread::start_mplane_mon_thread()

