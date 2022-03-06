#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // get_pid()
#include <signal.h>     // SIGUSR1, SIGUSR2
#include <memory.h>     // memset()
#include <sys/socket.h> // socket()
#include <netinet/in.h> // struct sockaddr_in
#include <netinet/tcp.h>// TCP_NODELAY
#include <fcntl.h>      // fcntl()
#include <arpa/inet.h>
#include <netdb.h>      // gethostbyaddr()
#include <sys/time.h>   // struct timeval
#include <string>
#include <sstream>      // std::stringstream
#include <vector>
#include <list>
#include "tcpserv_thread.h"

tcpserv_thread::tcpserv_thread(const std::string &name,
                               size_t thread_id,
                               int listen_port,
                               size_t connsock_thread_base,
                               const std::list<size_t> &connsockid_list):
    thread_base(name, thread_id),
    stm(NUM_STATE)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    this->connect_socket        = -1;
    this->listenport            = listen_port;
    this->role                  = tcpserv_thread::ROLE_LISTEN;
    this->connsock_thread_base  = connsock_thread_base;
    this->connidlist            = connsockid_list;
    this->p1sockmutex           = NULL;

    this->install_state_handler(DISCONNECTED_S,
                                &tcpserv_thread::tcpserv_disconnected_entry,
                                &tcpserv_thread::tcpserv_disconnected_state,
                                &tcpserv_thread::tcpserv_disconnected_exit);
    this->install_state_handler(CONNECTED_S,
                                &tcpserv_thread::tcpserv_connected_entry,
                                &tcpserv_thread::tcpserv_connected_state,
                                &tcpserv_thread::tcpserv_connected_exit);
} // end of tcpserv_thread::tcpserv_thread(const std::string &name, size_t thread_id)

tcpserv_thread::tcpserv_thread(const std::string &name,
                               size_t thread_id,
                               size_t connect_socket_id,
                               size_t listen_thread_id):
    thread_base(name, thread_id),
    stm(NUM_STATE)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    this->listenport           = -1;
    this->role                  = tcpserv_thread::ROLE_CONNECT;
    this->connsockid            = connect_socket_id;
    this->listenthreadid        = listen_thread_id;

    this->install_state_handler(DISCONNECTED_S,
                                &tcpserv_thread::tcpserv_disconnected_entry,
                                &tcpserv_thread::tcpserv_disconnected_state,
                                &tcpserv_thread::tcpserv_disconnected_exit);
    this->install_state_handler(CONNECTED_S,
                                &tcpserv_thread::tcpserv_connected_entry,
                                &tcpserv_thread::tcpserv_connected_state,
                                &tcpserv_thread::tcpserv_connected_exit);
} // end of tcpserv_thread::tcpserv_thread(const std::string &name, size_t thread_id)

tcpserv_thread::~tcpserv_thread() {}

void tcpserv_thread::install_sockmutex(void *p1arg)
{
    this->p1sockmutex = p1arg;
}

bool tcpserv_thread::send_msg(const std::string &msg)
{
    bool status = true;

    if((this->currstate != CONNECTED_S) || (this->connect_socket < 0)) return(false);

    if(send(this->connect_socket, msg.c_str(), msg.size(), /* int flags= */ 0) < 0){
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in send socket" << std::endl;
        perror(sstrm.str().c_str());
    }

    return(status);
} // end of size_t tcpserv_thread::send_msg(...)

bool tcpserv_thread::send_msg(const char *p1buf, size_t bufsize)
{
    bool status = true;

    if((this->currstate != CONNECTED_S) || (this->connect_socket < 0)) return(false);

    if(send(this->connect_socket, p1buf, bufsize, /* int flags= */ 0) < 0){
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in send socket" << std::endl;
        perror(sstrm.str().c_str());
    }

    return(status);
} // end of size_t tcpserv_thread::send_msg(...)

void tcpserv_thread::process_connsock_event()
{
    if(g_debug_mask & (1UL << MASKBIT_EVENT)){
        TRACE0OBJ();
    }

    switch(this->currstate){
        case DISCONNECTED_S: {
            break;
        }
        case CONNECTED_S: {
            struct rdbufstruct rdbuf;
            while((rdbuf.bytecnt = recv(this->connect_socket, rdbuf.buf, MAX_SOCKBUFF_BYTE, 0)) > 0)
            {
                // append a null string
                if(rdbuf.bytecnt < MAX_SOCKBUFF_BYTE) rdbuf.buf[rdbuf.bytecnt] = '\0';
                this->dispatch_event((void *)&rdbuf, EVENT_SOCKET);
            }
            // If no messages are available at the socket and O_NONBLOCK is set on the socket's file descriptor,
            // recv() shall fail and set errno to [EAGAIN] or [EWOULDBLOCK].
            if((rdbuf.bytecnt == 0) || ((rdbuf.bytecnt < 0) && (errno != EWOULDBLOCK))) {
                std::stringstream sstrm;
                TRACE1OBJ(sstrm) << " socket is closed by remote peer" << std::endl;
                perror(sstrm.str().c_str());
                if(this->connect_socket >= 0) {
                    close(this->connect_socket);
                    this->connect_socket = -1;
                }
                this->transit_state(DISCONNECTED_S);
            }

            break;
        }
        default:;
    };
} // end of void tcpserv_thread::process_connsock_event()


void tcpserv_thread::process_itc_event()
{
    itc_queue::elem *p1elem;

    while(thread_base::p1itc_q[this->thread_id].count() > 0) {
        if((p1elem = thread_base::p1itc_q[this->thread_id].get()) != NULL){
            this->transit_state(this->nextstate, /*bool reentry*/false);

            // TODO: add more code to handle p1elem->p1comp
            this->dispatch_event((void *)p1elem, EVENT_ITC);

#if 1
            p1elem->p1pool->put(p1elem);
#else
            // loop#0)
            //       p1elem              p1next_comp
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|------>|     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //
            // loop#1)
            //       in a pool           p1elem           p1next_comp
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|->NULL |     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //
            //
            for( ; p1elem != NULL; ) {
                itc_queue::elem *p1next_comp = p1elem->p1comp;  // for next
                p1elem->p1comp = NULL;                          // clear current p1elem
                p1elem->p1pool->put(p1elem);                    // push back the current p1elem to pool
                p1elem = p1next_comp;                           // swap
            }
#endif
        }
    } // end of while(...)

    this->transit_state(this->nextstate, /*bool reentry*/false);
} // end of void tcpserv_thread::process_itc_event()

void tcpserv_thread::install_state_handler(size_t state,
                                           void (tcpserv_thread::*entry_func)(),
                                           size_t (tcpserv_thread::*state_func)(void *, int),
                                           void (tcpserv_thread::*exit_func)())
{
    stm::install_state_handler(state,
                               static_cast<void (stm::*)()>(entry_func),
                               static_cast<size_t (stm::*)(void *, int)>(state_func),
                               static_cast<void (stm::*)()>(exit_func));

} // end of bool tcpserv_thread::install_state_handler(...)

void tcpserv_thread::install_state_handler(size_t state, size_t (tcpserv_thread::*state_func)(void *, int))
{
    stm::install_state_handler(state, static_cast<size_t (stm::*)(void *, int)>(state_func));
} // end of bool tcpserv_thread::install_state_handler(...)

void tcpserv_thread::tcpserv_disconnected_entry()
{
    mutex_lock  lock(this->p1sockmutex);

    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT) | (1UL << MASKBIT_SOCKET))){
        TRACE0OBJ();
    }

    if(this->connect_socket >= 0) {
        close(this->connect_socket);
        this->connect_socket = -1;
    }
}

size_t tcpserv_thread::tcpserv_disconnected_state(void *p1arg, int event)
{
    (void)(p1arg);  // to avoid of unused warning
    (void)(event);  // to avoid of unused warning
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    if(event == EVENT_SOCKET) {
    }
    else if(event == EVENT_ITC) {
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid){
            case ITC_NEW_CONNSOCK_ID: {
                if(p1msg->new_connsock.sockfd >= 0) {
                    // NOTE: do NOT unblock below codes, or it makes multiple tcp connections troubled.
#if 0
                    if(this->connect_socket >= 0) {
                        close(this->connect_socket);
                        this->connect_socket = -1;
                    }
#endif
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

                    this->transit_state(CONNECTED_S);
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
            default:;
        }
    }
    return(this->currstate);
} // end of size_t tcpserv_thread::tcpserv_disconnected_state()

void tcpserv_thread::tcpserv_disconnected_exit()
{
}

void tcpserv_thread::tcpserv_connected_entry()
{
    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT) | (1UL << MASKBIT_SOCKET))){
        TRACE0OBJ();
    }

    mutex_lock  lock(this->p1sockmutex);

    if(this->role == ROLE_CONNECT){
        timed_list.add_item(poll_timer_wrapper,
                            (void *)this,
                            /*int offset=*/ 0,
                            /*int interval=*/ 100,
                            /*bool override=*/true);
    }
}

size_t tcpserv_thread::tcpserv_connected_state(void *p1arg, int event)
{
    (void)(p1arg);  // to avoid of unused warning
    (void)(event);  // to avoid of unused warning
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    if(event == EVENT_SOCKET) {
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
} // end of tcpserv_thread::tcpserv_connected_state()

void tcpserv_thread::tcpserv_connected_exit()
{
    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT) | (1UL << MASKBIT_SOCKET))){
        TRACE0OBJ();
    }

    if(this->connect_socket >= 0) {
        close(this->connect_socket);
        this->connect_socket = -1;
    }

    if(this->role == ROLE_CONNECT){
        timed_list.del_item(poll_timer_wrapper, (void *)this);
    }

    itc_dis_connsock itcdisconnsock;
    itcdisconnsock.connid = this->connsockid;
    send_itc_message(this->listenthreadid,
                     ITC_DIS_CONNSOCK_ID,
                     sizeof(itc_dis_connsock),
                     (const uint8_t *)&itcdisconnsock);
}


//-------------------------------------------------------------------------------
// tcpserv_thread
//-------------------------------------------------------------------------------
/* call flow
 *  <start_tcpserv_thread class="tcpserv_thread">
 *      <start_wrapper class="tcpserv_thread">
 *          <run_tcp_listen_thread class="tcpserv_thread">
 *               <process_connsock_event class="tcpserv_thread">
 *                   <dispatch_event class="tcpserve_thread">
 *                       <tcpserv_connected_state class="hswi_thread">
 *                       </tcpserv_connected_state>
 *                   </dispatch_event>
 *               </process_connsock_event>
 *               <process_itc_event>
 *               </process_itc_event>
 *          </run_tcp_listen_thread>
 *      </start_wrapper>
 *  </start_hswi_thread>
 */
void tcpserv_thread::start_tcpserv_thread()
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
} // end of tcpserv_thread::start_tcpserv_thread()

/* call flow
 *  <start_tcpserv_thread class="tcpserv_thread">
 *      <start_wrapper class="tcpserv_thread">
 *          <run_tcp_listen_thread class="tcpserv_thread">
 *               <process_connsock_event class="tcpserv_thread">
 *                   <dispatch_event class="tcpserve_thread">
 *                       <tcpserv_connected_state class="hswi_thread">
 *                       </tcpserv_connected_state>
 *                   </dispatch_event>
 *               </process_connsock_event>
 *               <process_itc_event>
 *               </process_itc_event>
 *          </run_tcp_listen_thread>
 *      </start_wrapper>
 *  </start_hswi_thread>
 */
void *tcpserv_thread::start_wrapper(void *p1arg)
{
    tcpserv_thread *p1thread = (tcpserv_thread *)p1arg;

    if(p1thread->role == ROLE_LISTEN){
        p1thread->run_tcp_listen_thread();
    }
    else{
        p1thread->run_tcp_connect_thread();
    }

    return((void *)p1thread);
} // end of tcpserv_thread::start_wrapper(void *p1arg)

void tcpserv_thread::poll_timer_wrapper(void *p1arg)
{
#if 0
    fprintf(stderr, ".");
#endif
    // send EVENT_SOCKET to itself
    ((tcpserv_thread *)p1arg)->send_event(EVENT_SOCKET);
} // end of void il_thread::poll_timer_wrapper(..)


/* call flow
 *  <start_tcpserv_thread class="tcpserv_thread">
 *      <start_wrapper class="tcpserv_thread">
 *          <run_tcp_listen_thread class="tcpserv_thread">
 *               <process_connsock_event class="tcpserv_thread">
 *                   <dispatch_event class="tcpserve_thread">
 *                       <tcpserv_connected_state class="hswi_thread">
 *                       </tcpserv_connected_state>
 *                   </dispatch_event>
 *               </process_connsock_event>
 *          </run_tcp_listen_thread>
 *      </start_wrapper>
 *  </start_hswi_thread>
 */
void tcpserv_thread::run_tcp_listen_thread()
{
    struct sockaddr_in  server_addr;
    int sock_opt = 1;

    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    // create a TCP socket to listen
    this->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(this->listen_socket < 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << " cannot create listen socket" << std::endl;
        perror(sstrm.str().c_str());
        exit(1);
    }

    // prevent bind error
    setsockopt(this->listen_socket,
               SOL_SOCKET,
               SO_REUSEADDR,
               &sock_opt,
               sizeof(sock_opt));
#if 0
    // set TCP keepalive
    setsockopt(this->listen_socket,
               SOL_SOCKET,
               SO_KEEPALIVE,
               &sock_opt,
               sizeof(sock_opt));
#endif
    // send segment as soon as possible even if there is only a small amount of data
    setsockopt(this->listen_socket,
               IPPROTO_TCP,
               TCP_NODELAY,
               &sock_opt,
               sizeof(sock_opt));

    // set address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(this->listenport);
    //bzeros(&(server_addr.sin_zero), 8); // zero the rest of the struct

    //TRACE0OBJ();
    // bind listen socket to the listen port.
    if(bind(this->listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        std::stringstream sstrm;
        TRACE1OBJ(sstrm)
            << "error in bind the listen socket." << std::endl
            << "reboot is required!" << std::endl;
        perror(sstrm.str().c_str());
        close(this->listen_socket);

        exit(1);
    }

    // it registers this program with the system as expecing connections on this sock,
    // and then continue because of non-blocking
    // the backlog argument of the listen() limits the number of outstanding connections in the socket's listen queue.
    TRACE0OBJ()
        << "start to listen inbound socket" << std::endl;
    if(listen(this->listen_socket, /*int backlog*/ 3) < 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in bind()" << std::endl;
        perror(sstrm.str().c_str());

        return;
    }

    for( ; ; ) {    // infinite loop
        struct sockaddr_in  client_addr;
        socklen_t           client_addr_size = sizeof(client_addr);
        int new_connsock = -1;

        // if no pending connections are present on the queue, and the socket is not marked as nonblocking,
        // accopt() blocks the caller until a connection is present.
        new_connsock = accept(this->listen_socket, (struct sockaddr *)&client_addr, &client_addr_size);

        // process pending itc events just after accept()
        this->process_itc_event();

        if(new_connsock < 0) {
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "error in accept()" << std::endl;
            perror(sstrm.str().c_str());
        }
        else {
            if(g_debug_mask & (1UL << MASKBIT_SOCKET)){
                TRACE0OBJ()
                    << "new_connsock=" << new_connsock << std::endl;
            }

            // round-robin
            size_t cand_connsock_id = this->connidlist.front();
            this->connidlist.pop_front();
            this->connidlist.push_back(cand_connsock_id);

            itc_new_connsock itcnewconnsock;
            itcnewconnsock.sockfd = new_connsock;
            send_itc_message(this->connsock_thread_base + cand_connsock_id,
                             ITC_NEW_CONNSOCK_ID,
                             sizeof(itc_new_connsock),
                             (const uint8_t *)&itcnewconnsock);
        }
    } // end of for( ; ; )

    TRACE1OBJ(std::cerr) << "thread exit" << std::endl;

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); //
} // end of void run_tcp_listen_thread()

void tcpserv_thread::run_tcp_connect_thread()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    for( ; ; ) {    // infinite loop
        int recvsig;    // for sigwait()

        if(sigwait(&sigmask, &recvsig) != 0){
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "error in sigwait()" << std::endl;
            perror(sstrm.str().c_str());
        }

        switch(recvsig) {
            case EVENT_SOCKET: {
                if(g_debug_mask & (1UL << MASKBIT_EVENT)){
                    TRACE0OBJ() << "EVENT_SOCKET" << std::endl;
                }
                process_connsock_event();
                break;
            }

            case EVENT_ITC: {
                process_itc_event();
                break;
            }

            case SIGINT: { // usually cntl+c
                std::stringstream sstrm;
                TRACE1OBJ(sstrm) << "received SIGINT" << std::endl;
                perror(sstrm.str().c_str());
                break;
            }

            case SIGTERM:
            case SIGALRM:
            default:;   // do nothing
        } // end of switch(recvsig)
    } // end of for( ; ; )

    TRACE1OBJ(std::cerr) << "thread exit" << std::endl;

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); //
} // end of void run_tcp_connect_thread()
