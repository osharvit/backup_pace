#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>          // std::stringstream
#include "il_thread.h"

#if defined(__aarch64__)    // for real platform
il_thread::il_thread(const std::string &name,
                     size_t id,
                     h2top_hal (&h2top_ctrl)[NUM_HERMES]):
    thread_base(name, id),
    stm(NUM_IL_STATE),
    innolinkid((int)id - (int)THREAD_IL),
    p1h2top(h2top_ctrl)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    this->install_state_handler(IL_INIT_S,
                                &il_thread::il_init_entry,
                                &il_thread::il_init_state,
                                &il_thread::il_init_exit);
    this->install_state_handler(IL_BIST_S,
                                &il_thread::il_bist_entry,
                                &il_thread::il_bist_state,
                                &il_thread::il_bist_exit);
    this->install_state_handler(IL_READY_S,
                                &il_thread::il_ready_entry,
                                &il_thread::il_ready_state,
                                &il_thread::il_ready_exit);

    transactioncnt = 0;

    this->nvreg_json.SetObject();

    setup_hswi_handler();
} // end of il_thread::il_thread(const std::string &name, size_t id)
#endif

il_thread::~il_thread() {}

void il_thread::install_state_handler(size_t state,
                                            void (il_thread::*entry_func)(),
                                            size_t (il_thread::*state_func)(void *, int),
                                            void (il_thread::*exit_func)())
{
    if(state < NUM_IL_STATE) {
        stm::install_state_handler(state,
                                   static_cast<void (stm::*)()>(entry_func),
                                   static_cast<size_t (stm::*)(void *, int)>(state_func),
                                   static_cast<void (stm::*)()>(exit_func));
    }
} // end of il_thread::install_state_handler(...)

void il_thread::il_init_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ()
            << "version=" << p1h2top[0].get_pl_version()
            << std::endl;
    }
#if 0
    transit_state(IL_READY_S, true);
#endif
} // end of void il_thread::il_init_entry()

size_t il_thread::il_init_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    (void)(event);  // to avoid of unused warning

    if(p1arg != NULL){
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid) {
            case ITC_KICKOFF_ID: {
                process_itc_kickoff();
                break;
            }
            case ITC_VERSION_REQ_ID: {
                process_version_req();
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_IL_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                    transit_state(this->nextstate, true);
                }
                break;
            }
            case ITC_NVDATA_ID: {
                rapidjson::Document reqdoc;;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& nil_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);
#if 0
                TRACE0OBJ() << strsdu << std::endl;
#endif

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    this->process_hswi_messages("",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                nil_at);
                }
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
} // end of il_thread::il_init_state(...)

void il_thread::il_init_exit()
{
} // end of il_thread::il_init_exit()

void il_thread::il_bist_entry()
{
    TRACE0OBJ()
        << "bist=" << p1h2top[0].p1innolink[P_IL].bist()
        << std::endl;

#if 0
    transit_state(IL_READY_S, true);
#endif
} // end of void il_thread::il_bist_entry()

size_t il_thread::il_bist_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    (void)(event);  // to avoid of unused warning

    if(p1arg != NULL){
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid) {
            case ITC_KICKOFF_ID: {
                process_itc_kickoff();
                break;
            }
            case ITC_VERSION_REQ_ID: {
                process_version_req();
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_IL_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                    transit_state(this->nextstate, true);
                }
                break;
            }
            case ITC_NVDATA_ID: {
                rapidjson::Document reqdoc;;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& nil_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);
#if 1
                TRACE0OBJ() << strsdu << std::endl;
#endif

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                nil_at);
                }
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
} // end of il_thread::il_bist_state(...)

void il_thread::il_bist_exit()
{
} // end of il_thread::il_bist_exit()

void il_thread::il_ready_entry()
{
    if(1 || (g_debug_mask & (1UL << MASKBIT_CALL))){
        TRACE0OBJ();
    }

    int num_innolink = (this->p1h2top[0].get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int ilidx = S_IL; ilidx < num_innolink; ilidx++){
        p1h2top[0].p1innolink[ilidx].reset_serdes();
    }
} // end of il_thread::il_ready_entry()

#if defined(__aarch64__)    // for real platform
size_t il_thread::il_ready_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    int num_innolink = (this->p1h2top[0].get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    if(event == EVENT_SOCKET) {
        rapidjson::Document rspdoc;

        for(int ilidx = 0; ilidx < num_innolink; ilidx++){
            bool parse_result = this->load(rspdoc, p1residual[ilidx]);
            if(parse_result){
                // NOTE: do not call the "process_hswi_messages()" here.
                std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
                if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_MPLANE_JSON))){
                    TRACE0OBJ() << "rspdoc=" << rspdocstr << std::endl;
                }

                send_itc_message(THREAD_RE,
                                 ITC_PAYLOAD_FROM_IL_ID,
                                 rspdocstr.size(),
                                 (const uint8_t *)rspdocstr.c_str());
                for(size_t kk = 0; kk < NUM_MPMON_CONNSOCK; kk++){
                    send_itc_message(THREAD_MPMON_CONN + kk,
                                     ITC_MPMON_ID,
                                     rspdocstr.size(),
                                     (const uint8_t *)rspdocstr.c_str());
                }
            }
        }
    }
    else if(event == EVENT_ITC){
        if(p1arg != NULL){
            itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
            itc_msg *p1msg = (itc_msg *)p1elem->p1body;

            if(p1msg == NULL) return(this->currstate);

            switch(p1msg->hdr.msgid) {
                case ITC_KICKOFF_ID: {
                    process_itc_kickoff();
                    break;
                }
                case ITC_VERSION_REQ_ID: {
                    process_version_req();
                    break;
                }
                case ITC_STATE_REQ_ID: {
                    if(p1msg->state_req.nextstate < NUM_IL_STATE){
                        this->nextstate = p1msg->state_req.nextstate;
                        transit_state(this->nextstate, true);
                    }
                    break;
                }
                case ITC_PAYLOAD_ID: {
                    if((g_debug_mask & (1UL << MASKBIT_MPLANE_PATTERN))) {    // CP test mode (fpga -> h2)
                        std::string sdustr("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
                        p1h2top[0].p1innolink[P_IL].send_mplane(sdustr);
                    }
                    else {
                        std::string sdustr(p1msg->payload.sdu);
                        p1h2top[0].p1innolink[P_IL].send_mplane(sdustr);

                        for(size_t kk = 0; kk < NUM_MPMON_CONNSOCK; kk++){
                            if ( p1msg->hdr.msgsrc == THREAD_MPMON_CONN + kk ) continue;
                            if((g_debug_mask & (1UL << MASKBIT_EVENT)) && 0){
                                TRACE0OBJ() << "ITC_MPMON_ID: il -> mpmon" << std::endl;
                            }
                            send_itc_message(THREAD_MPMON_CONN + kk,
                                             ITC_MPMON_ID,
                                             sdustr.size(),
                                             (const uint8_t *)sdustr.c_str());
                        }
                    }
                    break;
                }

                case ITC_NVDATA_ID: {
                    rapidjson::Document reqdoc;;
                    rapidjson::Document nildoc;
                    rapidjson::Document::AllocatorType& nil_at = nildoc.GetAllocator();

                    std::string strsdu(p1msg->payload.sdu);
#if 0
                    TRACE0OBJ() << strsdu << std::endl;
#endif

                    bool parse_result = this->load(reqdoc, strsdu);
                    if(parse_result){
                        nildoc.SetObject();
                        //---------------------------------------------------------------
                        // call hswi message handler
                        //---------------------------------------------------------------
                        this->process_hswi_messages("",
                                                    static_cast<const rapidjson::Value &>(reqdoc),
                                                    static_cast<rapidjson::Value &>(nildoc),
                                                    nil_at);
                        // NOTE: do nothing for the nildoc
                    }
                    break;
                }

                default:{
                    TRACE0OBJ()
                        << "ignore the msgid=" << p1msg->hdr.msgid
                        << std::endl;
                }
            }
        }
    }

    return(this->currstate);
} // end of il_thread::il_ready_state(...)
#endif

void il_thread::il_ready_exit()
{
} // end of il_thread::il_ready_exit()

void il_thread::process_itc_event()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    itc_queue::elem *p1elem;

    while(p1itc_q[this->thread_id].count() > 0) {
#if 0
        TRACE0OBJ();
        dump_itc_q();
#endif
        if((p1elem = p1itc_q[this->thread_id].get()) != NULL){
            this->transit_state(this->nextstate, /*bool reentry=*/false);

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
} // end of void h2app_thread::process_itc_event()

void il_thread::process_itc_kickoff()
{
    // not yet defined
}

void il_thread::process_version_req()
{
    uint32_t fpga_version = p1h2top[0].get_pl_version();

    itc_version_rsp version_rsp;
    version_rsp.major_version = (fpga_version & 0xFFFF0000) >> 16;
    version_rsp.minor_version = (fpga_version & 0x0000FFFF) >> 0;

#if 0
    TRACE0OBJ() << "fpga_version=0x" << std::hex << std::setw(8) << std::setfill('0') << fpga_version
        << ", major_version=0x" << std::hex << std::setw(8) << std::setfill('0') << version_rsp.major_version
        << ", minor_version=0x" << std::hex << std::setw(8) << std::setfill('0') << version_rsp.minor_version
        << std::endl;
#endif

    send_itc_message(THREAD_RE,
                     ITC_VERSION_RSP_ID,
                     sizeof(itc_version_rsp),
                     (const uint8_t *)&version_rsp);
}


void il_thread::process_il_mplane_event()
{
    int num_innolink = (this->p1h2top[0].get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int ilidx = 0; ilidx < num_innolink; ilidx++){
        size_t loopcnt = 0;

        size_t  rxlevel = p1h2top[0].p1innolink[ilidx].poll_rxfifo_level();
        hal::nsleep(10);

        if(rxlevel <= 0) return;

        do{
            if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_MPLANE_HEX))){
                TRACE0OBJ()
                    << "rxlevel=" << rxlevel << std::endl;
            }

            for(size_t kk = 0; kk < rxlevel; kk++){
                uint32_t recvdata = p1h2top[0].p1innolink[ilidx].recv_mplane();

                size_t state = p1deframer[ilidx].dispatch_event(&p1residual[ilidx], (int)recvdata);

                if(state == il_mplane_deframer::INNOMP_EOP_S) {
                    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_MPLANE_HEX))){
                        TRACE0OBJ()
                            << "transactioncnt=" << transactioncnt++ << std::endl
                            << p1residual[ilidx] << std::endl;
                    }
                    this->dispatch_event(NULL, EVENT_SOCKET);

                    // clear buffer
                    p1residual[ilidx].clear();
                }
            }

            rxlevel = p1h2top[0].p1innolink[ilidx].poll_rxfifo_level();
            hal::nsleep(10);
        }while((rxlevel > 0) && (loopcnt++ < 10));
    }
} // end of void il_thread::process_il_mplane_event()

#if defined(__aarch64__)    // for real platform
void il_thread::setup_hswi_handler()
{
    //---------------------------------------------------------------------
    // register:1
    //---------------------------------------------------------------------
    this->objid_map["/load_param_req/register"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::load_param_req__register);

}   // end of void il_thread::setup_hswi_handler()
#endif

//---------------------------------------------------------------------
// register:1
//---------------------------------------------------------------------
bool il_thread::load_param_req__register(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx,
                                         size_t num_item)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    bool  status =  true;

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                load_param_req__register(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            if(inst_id == 1){
                // the object "register:1" for fpga
                rapidjson::Document::AllocatorType& nvreg_at = this->nvreg_json.GetAllocator();

                this->nvreg_json.CopyFrom(req_node, nvreg_at);
            }

            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

//-------------------------------------------------------------------------------
// il_thread
//-------------------------------------------------------------------------------
void il_thread::start_il_thread()
{
    // Initialize and set thread joinable
    pthread_attr_init(&this->threadattr);
    //pthread_attr_setdetachstate(&this->threadattr, PTHREAD_CREATE_JOINABLE);
    sigemptyset(&this->sigmask);   // clear signals
    sigaddset(&this->sigmask, EVENT_ITC);
    sigaddset(&this->sigmask, EVENT_SOCKET);
    sigaddset(&this->sigmask, SIGPIPE);    // NOTE: double-protection against crash due to SIGPIPE
    if(pthread_sigmask(SIG_BLOCK, &this->sigmask, NULL) != 0){
        perror("pthread_sigmask()");
    }

    pthread_create(&this->thread, &this->threadattr, il_thread::start_wrapper, (void *)this);
    pthread_detach(this->thread); // mark the thread to return its reources automatically in terminating
} // end of il_thread::start_il_thread()

void *il_thread::start_wrapper(void *p1arg)
{
    il_thread *p1thread = (il_thread *)p1arg;

    p1thread->run_il_thread();

    return((void *)p1thread);
} // end of il_thread::start_wrapper(void *p1arg)

void il_thread::poll_timer_wrapper(void *p1arg)
{
#if 0
    fprintf(stderr, ".");
#endif
    // send EVENT_SOCKET to itself
    ((il_thread *)p1arg)->send_event(EVENT_SOCKET);
} // end of void il_thread::poll_timer_wrapper(..)

void il_thread::run_il_thread()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    // update FPGA version
    g_fpga_ver = p1h2top[0].p1innolink[P_IL].get_verstr();

    timed_list.add_item(il_thread::poll_timer_wrapper,
                        (void *)this,
                        /*int offset=*/ 0,
                        /*int interval=*/ 10,
                        /*bool override=*/false);

    // kick statemachine
    transit_state(IL_INIT_S, true);

    for( ; ; ) {    // infinite loop
        int recvsig;    // for sigwait()

        if(sigwait(&sigmask, &recvsig) != 0){
            std::stringstream sstrm;
            TRACE3(sstrm, il_thread, thread_id);
            perror(sstrm.str().c_str());
        }

        switch(recvsig) {
            case EVENT_SOCKET: {    // event from time_thread
                process_il_mplane_event();
                break;
            }

            case EVENT_ITC: {
                process_itc_event();
                break;
            }


            case SIGINT: { // usually cntl+c
                std::stringstream sstrm;
                TRACE3(sstrm, il_thread, thread_id) << "received SIGINT" << std::endl;
                perror(sstrm.str().c_str());
                break;
            }

            case SIGTERM:
            case SIGALRM:
            default:;
        }
    }

    TRACE0OBJ();

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); //
} // end of void run_il_thread()

