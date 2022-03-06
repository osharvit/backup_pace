#ifndef _TCPSERV_THREAD_H_
#define _TCPSERV_THREAD_H_

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
#include "thread_base.h"
#include "stm.h"
#include "hswi_json.h"          // hswi_json

# if defined(__x86_64__)    // for simulation only
#  if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>  // gettid()
#define gettid() syscall(SYS_gettid)
#  endif
# endif


// Innophase Hermes Radio Protocol
class tcpserv_thread : public thread_base, public stm
{
    public:
        enum _state{
            DISCONNECTED_S,
            CONNECTED_S,
            NUM_STATE
        };

        enum _role{
            ROLE_LISTEN,
            ROLE_CONNECT,
            NUM_ROLE
        };

        struct rdbufstruct{
            int     bytecnt;
            char    buf[ALIGN4BYTE(MAX_SOCKBUFF_BYTE + 1)];
        };

        tcpserv_thread(const std::string &name,
                       size_t thread_id,
                       int listen_port,
                       size_t connsock_thread_base,
                       const std::list<size_t> &connsockid_list);
        tcpserv_thread(const std::string &name,
                       size_t thread_id,
                       size_t connect_socket_id,
                       size_t listen_thread_id);    // for tcpserv_thread::ROLE_CONNECT only
        virtual ~tcpserv_thread();

        void install_sockmutex(void *p1arg);
        bool send_msg(const std::string &msg);
        bool send_msg(const char *p1buf, size_t bufsize);

        void start_tcpserv_thread();
        static void *start_wrapper(void *p1arg);

    protected:
        static void poll_timer_wrapper(void *p1arg);

        void run_tcp_listen_thread();
        void run_tcp_connect_thread();
        void process_connsock_event();
        void process_itc_event();

        void install_state_handler(size_t state,
                                   void (tcpserv_thread::*entry_func)(),
                                   size_t (tcpserv_thread::*state_func)(void *, int),
                                   void (tcpserv_thread::*exit_func)());
        void install_state_handler(size_t state, size_t (tcpserv_thread::*state_func)(void *, int));

        void tcpserv_disconnected_entry();
        size_t tcpserv_disconnected_state(void *p1arg, int event);
        void tcpserv_disconnected_exit();

        void tcpserv_connected_entry();
        size_t tcpserv_connected_state(void *p1arg, int event);
        void tcpserv_connected_exit();

    protected:
        int             listen_socket;      // file descriptors for listen socket
        int             connect_socket;
        unsigned short  listenport;
        size_t          role;
        size_t          connsockid;
        size_t          connsock_thread_base;
        size_t          listenthreadid;
        std::list<size_t> connidlist;       // for ROLE_LISTEN
        void           *p1sockmutex;

}; // end of class tcpserv_thread

#endif // _TCPSERV_THREAD_H_
