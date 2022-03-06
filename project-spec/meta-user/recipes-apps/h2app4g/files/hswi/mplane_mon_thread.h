#ifndef _MPLANE_MON_THREAD_H_
#define _MPLANE_MON_THREAD_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream
#include <algorithm>    // std::remove_if()
#include "tcpserv_thread.h"
#include "hswi_json.h"          // hswi_json

// Hermes Software Interface
class mplane_mon_thread : public tcpserv_thread, public hswi_json
{
    public:
        mplane_mon_thread(const std::string &name,
                    size_t thread_id,
                    int listen_port,
                    size_t connsock_thread_base,
                    const std::list<size_t> &connsockid);
        mplane_mon_thread(const std::string &name,
                    size_t thread_id,
                    size_t connect_socket_id,
                    size_t listen_thread_id);    // for tcpserv_thread::ROLE_CONNECT only
        virtual ~mplane_mon_thread();

        void start_mplane_mon_thread();
        //static void *start_wrapper(void *p1arg);

        void install_state_handler(size_t state,
                                   void (mplane_mon_thread::*entry_func)(),
                                   size_t (mplane_mon_thread::*state_func)(void *, int),
                                   void (mplane_mon_thread::*exit_func)());
        void install_state_handler(size_t state, size_t (mplane_mon_thread::*state_func)(void *, int));

    protected:
        //void run_mplane_mon_thread();

        void mplane_mon_connected_entry();
        size_t mplane_mon_connected_state(void *p1arg, int event);
        void mplane_mon_connected_exit();


    protected:
        std::string residual;

}; // end of class mplane_mon_thread

#endif // _MPLANE_MON_THREAD_H_
