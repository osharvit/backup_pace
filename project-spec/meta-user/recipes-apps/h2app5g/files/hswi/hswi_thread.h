#ifndef _HSWI_THREAD_H_
#define _HSWI_THREAD_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream
#include <algorithm>    // std::remove_if()
#include "tcpserv_thread.h"
#include "hswi_json.h"          // hswi_json

// Hermes Software Interface
class hswi_thread : public tcpserv_thread, public hswi_json
{
    public:
        hswi_thread(const std::string &name,
                    size_t thread_id,
                    int listen_port,
                    size_t connsock_thread_base,
                    const std::list<size_t> &connsockid);
        hswi_thread(const std::string &name,
                    size_t thread_id,
                    size_t connect_socket_id,
                    size_t listen_thread_id);    // for tcpserv_thread::ROLE_CONNECT only
        virtual ~hswi_thread();

        void start_hswi_thread();
        //static void *start_wrapper(void *p1arg);

        void install_state_handler(size_t state,
                                   void (hswi_thread::*entry_func)(),
                                   size_t (hswi_thread::*state_func)(void *, int),
                                   void (hswi_thread::*exit_func)());
        void install_state_handler(size_t state, size_t (hswi_thread::*state_func)(void *, int));

    protected:
        //void run_hswi_thread();

        void hswi_connected_entry();
        size_t hswi_connected_state(void *p1arg, int event);
        void hswi_connected_exit();


    protected:
        std::string residual;

}; // end of class hswi_thread

#endif // _HSWI_THREAD_H_
