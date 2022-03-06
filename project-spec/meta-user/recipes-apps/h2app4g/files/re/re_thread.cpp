#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getpid()
#include <dirent.h>     // struct dirent
#include <sys/types.h>
#include <sys/stat.h>   // stat(), struct stat
#include <string.h>
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <exception>    // std::exception
#include "re_thread.h"
#include "il_thread.h"
#include "sp_thread.h"
#include "version.h"    // ::get_verstr()
#include "wdt_ctrl.h"

#define EVENT_TRACE

re_thread::re_thread(const std::string &name,
                     size_t id,
                     wdt_ctrl &watchdog):
    thread_base(name, id),
    stm(NUM_RE_STATE),
    watchdogctrl(watchdog)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    num_sectionobj = 0;
    tmplaybackreq.num_playbackobj = 0;

    fpga_major_version = 0;
    fpga_minor_version = 0;

    this->phase = -1;
    this->antport_on_reset = -1;    // -1: None, 0: antport-A, 1: antport-B
    this->dl_il_sel = -1;           // -1: None, 0: innolink0, 1: innolink1
    this->ul_il_sel = -1;           // -1: None, 0: innolink0, 1: innolink1
    this->n_dpa = 0;                // -1: None, 0: unset, 1~4: set

    this->install_state_handler(RE_INIT_S,
                                &re_thread::re_init_entry,
                                &re_thread::re_init_state,
                                &re_thread::re_init_exit);
    this->install_state_handler(RE_FWLOAD_S,
                                &re_thread::re_fwload_entry,
                                &re_thread::re_fwload_state,
                                &re_thread::re_fwload_exit);
#if 0
    this->install_state_handler(RE_INIT_INNOLINK_S,
                                &re_thread::re_initinnolink_entry,
                                &re_thread::re_initinnolink_state,
                                &re_thread::re_initinnolink_exit);
    this->install_state_handler(RE_INIT_SYNC_S,
                                &re_thread::re_sync_entry,
                                &re_thread::re_sync_state,
                                &re_thread::re_sync_exit);
#endif
    this->install_state_handler(RE_READY_S,
                                &re_thread::re_ready_entry,
                                &re_thread::re_ready_state,
                                &re_thread::re_ready_exit);

    this->install_state_changed_callback(&re_thread::re_state_changed_callback_wrapper, (void *)this);

    setup_hswi_handler();
    setup_hswi_param_handler();

    for(size_t kk = 0; kk < NUM_CARRIER; kk++){
        txcarrierobj[kk].set_stm_id(kk);
        txcarrierobj[kk].install_state_changed_callback(&re_thread::tx_carrier_state_changed_callback_wrapper, (void *)this);

        rxcarrierobj[kk].set_stm_id(kk);
        rxcarrierobj[kk].install_state_changed_callback(&re_thread::rx_carrier_state_changed_callback_wrapper, (void *)this);
    }
} // end of re_thread::re_thread(const std::string &name, size_t id)

re_thread::~re_thread() {}

void re_thread::install_state_handler(size_t state,
                                      void (re_thread::*entry_func)(),
                                      size_t (re_thread::*state_func)(void *, int),
                                      void (re_thread::*exit_func)())
{
    if(state < NUM_RE_STATE) {
        stm::install_state_handler(state,
                                   static_cast<void (stm::*)()>(entry_func),
                                   static_cast<size_t (stm::*)(void *, int)>(state_func),
                                   static_cast<void (stm::*)()>(exit_func));
    }
} // end of re_thread::install_state_handler(...)

void re_thread::read_nvdata(const std::string &file_name)
{
    // read "nvdata/h2nvdata.json" file, but similar to the "hswi_json::process_hswi_messages()"
    FILE *fp = fopen(file_name.c_str(), "r");
    if(fp != NULL) {
        char readbuff[8192];
        rapidjson::FileReadStream is(fp, readbuff, sizeof(readbuff));

        inv_json.ParseStream(is);

        fclose(fp);

        rapidjson::Document nildoc;
        rapidjson::Document::AllocatorType& rsp_at = nildoc.GetAllocator();

        if(inv_json.HasMember("body")){
            //TRACE0OBJ() << file_name << std::endl;
            rapidjson::Value tmprspbody(rapidjson::kObjectType);
            process_hswi_body("", inv_json["body"], /* const size_t uid=*/ 0, tmprspbody, rsp_at);
        }
    }
}

void re_thread::re_init_entry()
{
    std::string nvfilename("nvdata/h2nvdata.json");

    read_nvdata(nvfilename);

    if(watchdogctrl.set_time_out(300) < 0)      // timeout : 1 ~ 300 sec
    {
        TRACE0OBJ() << "WdtCtrl.SetTimeOut Error" << std::endl;
    }

    itc_state_req itcstatereq;
#if 0
    itcstatereq.nextstate = il_thread::IL_READY_S;
    send_itc_message(THREAD_IL,
                     ITC_STATE_REQ_ID,
                     sizeof(itc_state_req),
                     (const uint8_t *)&itcstatereq);

    send_itc_message(THREAD_IL,
                     ITC_VERSION_REQ_ID,
                     0,
                     (const uint8_t *)NULL);
#else
    itcstatereq.nextstate = il_thread::IL_INIT_S;
    send_itc_message(THREAD_IL,
                     ITC_STATE_REQ_ID,
                     sizeof(itc_state_req),
                     (const uint8_t *)&itcstatereq);

    transit_state(RE_FWLOAD_S);
#endif
} // end of void re_thread::re_init_entry()

size_t re_thread::re_init_state(void *p1arg, int event)
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
            case ITC_VERSION_RSP_ID: {
                // update fpga_version info and transit to the next state
                fpga_major_version = p1msg->version_rsp.major_version;
                fpga_minor_version = p1msg->version_rsp.minor_version;
                //TRACE0OBJ() << "ITC_VERSION_RSP_ID, fpga_major_version=" << fpga_major_version << std::endl;


                itc_version_rsp itcversionrsp;
                itcversionrsp.major_version = fpga_major_version;
                itcversionrsp.minor_version = fpga_minor_version;
                send_itc_message(THREAD_SP,
                                 ITC_VERSION_IND_ID,
                                 sizeof(itc_version_rsp),
                                 (const uint8_t *)&itcversionrsp);

                transit_state(RE_FWLOAD_S);
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_RE_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            default:{
                //TRACE0OBJ() << "ignore the msgid=" << p1msg->hdr.msgid << std::endl;
            }
        }
    }

    return(this->currstate);
} // end of re_thread::re_init_state(...)

void re_thread::re_init_exit()
{
} // end of re_thread::re_init_exit()

void re_thread::re_fwload_entry()
{
    if(1 || (g_debug_mask & (1UL << MASKBIT_CALL))){
        TRACE0OBJ();
    }
    TRACE0OBJ() << "=======================" << std::endl;

    std::string nvfilename("nvdata/h2nvdata.json");

    read_nvdata(nvfilename);

    itc_reset_req itcresetreq;
    itcresetreq.nextstate = sp_thread::SP_FWLOAD_S;
    itcresetreq.phase = this->phase;    // -1: for None, 0: for P1, 1: for P2
    itcresetreq.antport_on_reset = this->antport_on_reset;    // -1: for None, 0: for P1, 1: for P2
    itcresetreq.dl_il_sel = this->dl_il_sel;
    itcresetreq.ul_il_sel = this->ul_il_sel;
    itcresetreq.n_dpa = this->n_dpa;

    send_itc_message(THREAD_SP,
                     ITC_RESET_REQ_ID,
                     sizeof(itc_reset_req),
                     (const uint8_t *)&itcresetreq);

#if 0
    // TODO: replace the below line with handshaking
    transit_state(RE_READY_S, true);
#endif
} // end of void re_thread::re_fwload_entry()

size_t re_thread::re_fwload_state(void *p1arg, int event)
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
            case ITC_VERSION_RSP_ID: {
                // update fpga_version info and transit to the next state
                fpga_major_version = p1msg->version_rsp.major_version;
                fpga_minor_version = p1msg->version_rsp.minor_version;
                //TRACE0OBJ() << "ITC_VERSION_RSP_ID, fpga_major_version=" << fpga_major_version << std::endl;
                break;
            }
            case ITC_STATE_IND_ID: {
                TRACE0OBJ() << "recv ITC_STATE_IND_ID"
                    << " msgsrc=" << p1msg->hdr.msgsrc
                    << " nextstate=" << p1msg->state_ind.nextstate
                    << std::endl;
                // if the sp_thread sends the state indication of SP_IDLE_S
                if((p1msg->hdr.msgsrc == THREAD_SP) && (p1msg->state_ind.nextstate == sp_thread::SP_IDLE_S)){
                    TRACE0OBJ() << "recv ITC_STATEIND_ID from THREAD_SP to THREAD_RE" << std::endl;
                    transit_state(RE_READY_S, true);
                }
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_RE_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            default:{
                //TRACE0OBJ() << "ignore the msgid=" << p1msg->hdr.msgid << std::endl;
            }
        }
    }

    return(this->currstate);
} // end of re_thread::re_fwload_state(...)

void re_thread::re_fwload_exit()
{
} // end of re_thread::re_fwload_exit()

void re_thread::re_initinnolink_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    itc_state_req itcstatereq;
    itcstatereq.nextstate = sp_thread::SP_FWLOAD_S;
    send_itc_message(THREAD_SP,
                     ITC_STATE_REQ_ID,
                     sizeof(itc_state_req),
                     (const uint8_t *)&itcstatereq);

    // TODO: replace the below line with handshaking
    transit_state(RE_READY_S, true);
} // end of void re_thread::re_initinnolink_entry()

size_t re_thread::re_initinnolink_state(void *p1arg, int event)
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
            case ITC_VERSION_RSP_ID: {
                // update fpga_version info and transit to the next state
                fpga_major_version = p1msg->version_rsp.major_version;
                fpga_minor_version = p1msg->version_rsp.minor_version;
                //TRACE0OBJ() << "ITC_VERSION_RSP_ID, fpga_major_version=" << fpga_major_version << std::endl;
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_RE_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
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
} // end of re_thread::re_initinnolink_state(...)

void re_thread::re_initinnolink_exit()
{
} // end of re_thread::re_initinnolink_exit()

void re_thread::re_sync_entry()
{
    // TODO: replace the below line with handshaking
    transit_state(RE_READY_S, true);
} // end of void re_thread::re_sync_entry()

size_t re_thread::re_sync_state(void *p1arg, int event)
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
            case ITC_VERSION_RSP_ID: {
                // update fpga_version info and transit to the next state
                fpga_major_version = p1msg->version_rsp.major_version;
                fpga_minor_version = p1msg->version_rsp.minor_version;
                //TRACE0OBJ() << "ITC_VERSION_RSP_ID, fpga_major_version=" << fpga_major_version << std::endl;
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_RE_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            default:{
                //TRACE0OBJ() << "ignore the msgid=" << p1msg->hdr.msgid << std::endl;
            }
        }
    }

    return(this->currstate);
} // end of re_thread::re_sync_state(...)

void re_thread::re_sync_exit()
{
} // end of re_thread::re_sync_exit()

void re_thread::re_ready_entry()
{
    if(1 || (g_debug_mask & (1UL << MASKBIT_CALL))){
        TRACE0OBJ();
    }
    TRACE0OBJ() << "=======================" << std::endl;

#if 1
    itc_state_req itcstatereq;
    itcstatereq.nextstate = il_thread::IL_READY_S;
    send_itc_message(THREAD_IL,
                     ITC_STATE_REQ_ID,
                     sizeof(itc_state_req),
                     (const uint8_t *)&itcstatereq);

    send_itc_message(THREAD_IL,
                     ITC_VERSION_REQ_ID,
                     0,
                     (const uint8_t *)NULL);
#endif
} // end of re_thread::re_ready_entry()

size_t re_thread::re_ready_state(void *p1arg, int event)
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
            case ITC_VERSION_RSP_ID: {
                // update fpga_version info and transit to the next state
                fpga_major_version = p1msg->version_rsp.major_version;
                fpga_minor_version = p1msg->version_rsp.minor_version;
                //TRACE0OBJ() << "ITC_VERSION_RSP_ID, fpga_major_version=" << fpga_major_version << std::endl;
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_RE_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            case ITC_PAYLOAD_FROM_HSWI_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document rspdoc;
                rapidjson::Document::AllocatorType& rsp_at = rspdoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    rspdoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("hswi:",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(rspdoc),
                                                rsp_at);
                    std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
#if 0
                    std::cout << rspdocstr << std::endl;
#endif
                    if ( p1msg->hdr.msgsrc >= THREAD_MPMON_CONN && p1msg->hdr.msgsrc < (THREAD_MPMON_CONN + NUM_MPMON_CONNSOCK) )
                    {
                        // do nothing.
                    }
                    else
                    {
                        send_itc_message(p1msg->hdr.msgsrc,
                                         ITC_PAYLOAD_FROM_HSWI_ID,
                                         rspdocstr.size(),
                                         (const uint8_t *)rspdocstr.c_str());
                    }
                }
                break;
            }
            case ITC_PAYLOAD_FROM_IL_ID: {
#if defined(EVENT_TRACE)
                TRACE0OBJ()
                    << "IT_PAYLOAD_FROM_INNOLINK_ID="
                    << std::string(p1msg->payload.sdu) << std::endl;
#endif
                rapidjson::Document reqdoc;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& nil_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("il:",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                nil_at);
                    // NOTE: do nothing for the nildoc
                }
                break;
            }
            case ITC_DOWNLOAD_TM_FILE_IND_ID:
            case ITC_DOWNLOAD_FILE_IND__TM_FILE_ID:{
                rapidjson::Document inddoc;
                rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();

                inddoc.SetObject();    // NOTE: don't forget this line
                rapidjson::Value indheader(rapidjson::kObjectType);
                {
                    indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
                    indheader.AddMember("uid", rapidjson::Value(p1msg->action_ind.uid).Move(), ind_at);
                }
                inddoc.AddMember("header", indheader, ind_at);

                rapidjson::Value indbody(rapidjson::kObjectType);
                {
                    rapidjson::Value indprim(rapidjson::kObjectType);
                    {
                        rapidjson::Value indobj(rapidjson::kObjectType);
                        {
                            add_member(std::string("result"),
                                       p1msg->action_ind.passfail? std::string("success"): std::string("fail"),
                                       indobj,
                                       ind_at);
                            add_member(std::string("filename"),
                                       std::string(p1msg->action_ind.filename),
                                       indobj,
                                       ind_at);
                        }
                        indprim.AddMember("tm_file:0", indobj, ind_at);
                    }
                    if(p1msg->hdr.msgid == ITC_DOWNLOAD_TM_FILE_IND_ID){
                        indbody.AddMember("tm_file_downloaded_ind", indprim, ind_at);
                    }
                    else{
                        indbody.AddMember("file_transferred_ind", indprim, ind_at);
                    }
                }
                inddoc.AddMember("body", indbody, ind_at);

                std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

#if 0
                std::cout << inddocstr << std::endl;
#endif
                // broadcast to all HSWI connection socket thread
                for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
                    send_itc_message(THREAD_HSWI_CONN + kk,
                                     ITC_PAYLOAD_FROM_HSWI_ID,
                                     inddocstr.size(),
                                     (const uint8_t *)inddocstr.c_str());
                }
                break;
            }
            case ITC_PLAYBACK_IND_ID:{
                rapidjson::Document inddoc;
                rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();

                inddoc.SetObject();    // NOTE: don't forget this line
                rapidjson::Value indheader(rapidjson::kObjectType);
                {
                    indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
                    indheader.AddMember("uid", rapidjson::Value(p1msg->action_ind.uid).Move(), ind_at);
                }
                inddoc.AddMember("header", indheader, ind_at);

                rapidjson::Value indbody(rapidjson::kObjectType);
                {
                    rapidjson::Value indprim(rapidjson::kObjectType);
                    indbody.AddMember("tm_playback_started_ind", indprim, ind_at);
                }
                inddoc.AddMember("body", indbody, ind_at);

                std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

#if 0
                std::cout << inddocstr << std::endl;
#endif
                // broadcast to all HSWI connection socket thread
                for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
                    send_itc_message(THREAD_HSWI_CONN + kk,
                                     ITC_PAYLOAD_FROM_HSWI_ID,
                                     inddocstr.size(),
                                     (const uint8_t *)inddocstr.c_str());
                }
                break;
            }
            case ITC_UPLOAD_FILE_IND__FRC_FILE_ID:{
                rapidjson::Document inddoc;
                rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();

                inddoc.SetObject();    // NOTE: don't forget this line
                rapidjson::Value indheader(rapidjson::kObjectType);
                {
                    indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
                    indheader.AddMember("uid", rapidjson::Value(p1msg->action_ind.uid).Move(), ind_at);
                }
                inddoc.AddMember("header", indheader, ind_at);

                rapidjson::Value indbody(rapidjson::kObjectType);
                {
                    rapidjson::Value indprim(rapidjson::kObjectType);
                    {
                        rapidjson::Value indobj(rapidjson::kObjectType);
                        {
                            add_member(std::string("result"),
                                          p1msg->action_ind.passfail? std::string("success"): std::string("fail"),
                                          indobj,
                                          ind_at);
                            add_member(std::string("filename"),
                                       std::string(p1msg->action_ind.filename),
                                       indobj, ind_at);
                        }
                        indprim.AddMember("frc_file:0", indobj, ind_at);
                    }
                    indbody.AddMember("file_transferred_ind", indprim, ind_at);
                }
                inddoc.AddMember("body", indbody, ind_at);

                std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

#if 0
                std::cout << inddocstr << std::endl;
#endif
                // broadcast to all HSWI connection socket thread
                for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
                    send_itc_message(THREAD_HSWI_CONN + kk,
                                     ITC_PAYLOAD_FROM_HSWI_ID,
                                     inddocstr.size(),
                                     (const uint8_t *)inddocstr.c_str());
                }
                break;
            }
            case ITC_STATE_IND_ID:{
                TRACE0OBJ() << "recv ITC_STATE_IND_ID"
                    << " msgsrc=" << p1msg->hdr.msgsrc
                    << " nextstate=" << p1msg->state_ind.nextstate
                    << std::endl;
                // if the sp_thread sends the state indication of SP_IDLE_S
                if((p1msg->hdr.msgsrc == THREAD_SP) && (p1msg->state_ind.nextstate == sp_thread::SP_IDLE_S)){
                    TRACE0OBJ() << "recv ITC_STATEIND_ID from THREAD_SP to THREAD_RE, but already in RE_READY_S" << std::endl;
                }
                break;
            }
            default:{
                //TRACE0OBJ() << "ignore the msgid=" << p1msg->hdr.msgid << std::endl;
            }
        }
    }

    return(this->currstate);
} // end of re_thread::re_ready_state(...)

void re_thread::re_ready_exit()
{
} // end of re_thread::re_ready_exit()

void re_thread::send_hswi_msg_to_thread(int dst_thread_id,
                                        const size_t uid,
                                        std::string type,
                                        std::string primitivekey,
                                        std::string objidkey,
                                        rapidjson::Value &itc_node,
                                        rapidjson::Document &itcdoc,
                                        rapidjson::Document::AllocatorType& itc_at,
                                        uint16_t msg_id)
{ // itc
    itcdoc.SetObject();

    rapidjson::Value itcheader(rapidjson::kObjectType);
    itcheader.AddMember("type", rapidjson::Value(type.c_str(), type.size(), itc_at).Move(), itc_at);
    itcheader.AddMember("uid", rapidjson::Value(uid).Move(), itc_at);
    itcdoc.AddMember("header", itcheader, itc_at);

    rapidjson::Value itcprimitive(rapidjson::kObjectType);
    itcprimitive.AddMember(rapidjson::Value(objidkey.c_str(), objidkey.size(), itc_at).Move(),
                           itc_node,
                           itc_at);

    rapidjson::Value itcbody(rapidjson::kObjectType);
    itcbody.AddMember(rapidjson::Value(primitivekey.c_str(), primitivekey.size(), itc_at).Move(),
                      itcprimitive,
                      itc_at);
    itcdoc.AddMember("body", itcbody, itc_at);

    std::string itcdocstr = rapidjson_wrapper::str_json(itcdoc);
    send_itc_message(dst_thread_id,
                     msg_id,
                     itcdocstr.size(),
                     (const uint8_t *)itcdocstr.c_str());
} // end of void re_thread::send_hswi_msg_to_thread()

bool re_thread::find_tm_files(std::vector<std::string> &vlist)
{
    bool status = true;

    struct dirent *entry;
    DIR *dirp = opendir(TM_DIR);
    if(dirp == NULL) return(false);

    while((entry = readdir(dirp)) != NULL){
        std::string filename(entry->d_name);
        struct stat sb;
        if(stat((std::string(TM_DIR) + "/" + filename).c_str(), &sb) >= 0){
            if(S_ISREG(sb.st_mode)){
                // find extension
                size_t dot = filename.find_last_of(".");
                if(dot != std::string::npos){
                    std::string ext = filename.substr(dot);
#if 0
                    std::cout << "filename=" << filename << ", ext=" << ext << std::endl;
#endif
                    if(ext == ".bin"){
                        vlist.push_back(filename);
                    }
                }
            }
        }
    }
    closedir(dirp);

    return(status);
}

/* call flow:--------------------------------------------------------------------
 *  <start_re_thread class="re_thread">
 *      <start_wrapper class="re_thread">
 *          <run_re_thread class="re_thread">
 *               <process_itc_event class="re_thread">
 *                   <dispatch_event class="re_thread">
 *                       <re_init_state class="re_thread">
 *                       </re_init_state>
 *                   </dispatch_event>
 *               </process_itc_event>
 *          </run_re_thread>
 *      </start_wrapper>
 *  </start_re_thread>
 */
void re_thread::process_itc_event()
{
    itc_queue::elem *p1elem;

    while(p1itc_q[this->thread_id].count() > 0) {
        if((p1elem = p1itc_q[this->thread_id].get()) != NULL){
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

} // end of void re_thread::process_itc_event()

void re_thread::process_itc_kickoff()
{
    transit_state(RE_INIT_S, /*bool reentry=*/true);
}

//-------------------------------------------------------------------------------
// hswi
//-------------------------------------------------------------------------------
void re_thread::setup_hswi_handler()
{
    //---------------------------------------------------------------------
    // from re_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["/load_param_req/inventory"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::load_param_req__inventory);
    this->objid_map["/load_param_req/register"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::load_param_req__register);

    //---------------------------------------------------------------------
    // re:0 from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/get_param_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__re);
    this->objid_map["hswi:/reset_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::reset_req__re);
    this->objid_map["hswi:/resync_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::resync_req__re);
    this->objid_map["hswi:/get_swver_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_swver_req__re);
    this->objid_map["hswi:/prepare_swupdate_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::prepare_swupdate_req__re);
    this->objid_map["hswi:/activate_sw_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::activate_sw_req__re);

    //---------------------------------------------------------------------
    // inventory:0 from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/get_param_req/inventory"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__inventory);

    //---------------------------------------------------------------------
    // section:n from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/create_obj_req/section"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__section);

    //---------------------------------------------------------------------
    // tm_file:n from hswi_thread to re_thread
    //---------------------------------------------------------------------
    // NOTE: obsolete
    this->objid_map["hswi:/download_tm_file_req/tm_file"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::download_tm_file_req__tm_file);
    this->objid_map["hswi:/download_file_req/tm_file"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::download_file_req__tm_file);

    //---------------------------------------------------------------------
    // tm_playback:n from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/get_param_req/tm_filelist"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__tm_filelist);
    this->objid_map["hswi:/get_param_req/tm_playback"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__tm_playback);
    this->objid_map["hswi:/create_obj_req/tm_playback"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__tm_playback);
    this->objid_map["hswi:/modify_param_req/tm_playback"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__tm_playback);
    this->objid_map["hswi:/delete_obj_req/tm_playback"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::delete_obj_req__tm_playback);

    //---------------------------------------------------------------------
    // frc_capture:0 from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/create_obj_req/frc_capture"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__frc_capture);

    //---------------------------------------------------------------------
    // frc_file:0 from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/capture_file_req/frc_file"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::capture_file_req__frc_file);
    this->objid_map["hswi:/upload_file_req/frc_file"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::upload_file_req__frc_file);

    //---------------------------------------------------------------------
    // tx_carrier_lte:n from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/delete_obj_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::delete_obj_req__tx_carrier_lte);
    this->objid_map["hswi:/create_obj_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__tx_carrier_lte);
    this->objid_map["hswi:/modify_param_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_param_req__tx_carrier_lte);
    this->objid_map["hswi:/get_param_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__tx_carrier_lte);
    this->objid_map["hswi:/modify_state_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_state_req__tx_carrier_lte);

    //---------------------------------------------------------------------
    // rx_carrier_lte:n from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/delete_obj_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::delete_obj_req__rx_carrier_lte);
    this->objid_map["hswi:/create_obj_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_req__rx_carrier_lte);
    this->objid_map["hswi:/modify_param_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_param_req__rx_carrier_lte);
    this->objid_map["hswi:/get_param_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::get_param_req__rx_carrier_lte);
    this->objid_map["hswi:/modify_state_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_state_req__rx_carrier_lte);

    //---------------------------------------------------------------------
    // fpga:0 from hswi_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["hswi:/modify_param_req/fpga"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_param_req__fpga);

    //---------------------------------------------------------------------
    // from il_thread to re_thread
    //---------------------------------------------------------------------
    this->objid_map["il:/restarted_ind/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::restarted_ind__re);
    this->objid_map["il:/resync_rsp/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::resync_rsp__re);
    this->objid_map["il:/resynchronized_ind/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::resynchronized_ind__re);

    this->objid_map["il:/create_obj_rsp/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_rsp__tx_carrier_lte);
    this->objid_map["il:/modify_state_rsp/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_state_rsp__tx_carrier_lte);
    this->objid_map["il:/state_change_ind/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::state_change_ind__tx_carrier_lte);

    this->objid_map["il:/create_obj_rsp/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::create_obj_rsp__rx_carrier_lte);
    this->objid_map["il:/modify_state_rsp/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::modify_state_rsp__rx_carrier_lte);
    this->objid_map["il:/state_change_ind/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&re_thread::state_change_ind__rx_carrier_lte);
}

void re_thread::setup_hswi_param_handler()
{
    this->param_map["hswi:/modify_param_req/re/debug_mask"] =
        static_cast<hswi_json::PARAM_FUNC>(&re_thread::modify_param_req__re_debug_mask);
}

/* call flow:--------------------------------------------------------------------
 *  <start_re_thread class="re_thread">
 *      <start_wrapper class="re_thread">
 *          <run_re_thread class="re_thread">
 *               <process_itc_event class="re_thread">
 *                   <dispatch_event class="re_thread">
 *                       <re_init_state class="re_thread">
 *                           <process_hswi_messages class="hswi_json">
 *                               <process_hswi_body class="hswi_json">
 *                                   <exec_prmtv class="hswi_json">
 *                                       <process_hswi_primitive class="hswi_json">
 *                                           <exec_objid class="hswi_json">
 *                                               <default_obji class="hswi_json">
 *                                                   <exec_para class="hswi_json">
 *                                                       <default_para class="hswi_json">
 *                                                           <exec_para class="hswi_json"/>
 *                                                       </default_param>
 *                                                   </exec_param>
 *                                               </default_objid>
 *                                           </exec_objid>
 *                                       </process_hswi_primitive>
 *                                   </exec_prmtv>
 *                               </process_hswi_body>
 *                           </process_hswi_messages>
 *                           <send_itc_message class="thread_base"/>
 *                       </re_init_state>
 *                   </dispatch_event>
 *               </process_itc_event>
 *          </run_re_thread>
 *      </start_wrapper>
 *  </start_re_thread>
 */

//-------------------------------------------------------------------------------
// re:
//-------------------------------------------------------------------------------
bool re_thread::get_param_req__re(const rapidjson::Value& req_node,
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
                get_param_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kObjectType: {
            rsp_node.SetObject();
            add_member(std::string("max_tx_ant"), 2, rsp_node, rsp_at);
            add_member(std::string("max_rx_ant"), 2, rsp_node, rsp_at);
            switch(this->currstate){
                case RE_INIT_S: {
                    add_member(std::string("fst"), std::string("init"), rsp_node, rsp_at);
                    break;
                }
                case RE_FWLOAD_S: {
                    add_member(std::string("fst"), std::string("fw_load"), rsp_node, rsp_at);
                    break;
                }
                case RE_INIT_SYNC_S: {
                    add_member(std::string("fst"), std::string("init_sync"), rsp_node, rsp_at);
                    break;
                }
                case RE_READY_S: {
                    add_member(std::string("fst"), std::string("ready"), rsp_node, rsp_at);
                    break;
                }
                default:;
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::reset_req__re(const rapidjson::Value& req_node,
                                const std::string& node_key,
                                const int &inst_id,
                                const size_t uid,
                                rapidjson::Value &rsp_node,
                                rapidjson::Document::AllocatorType &rsp_at,
                                size_t item_idx,
                                size_t num_item)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
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
                reset_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            bool hardreset = false;
            this->n_dpa = 0;
            this->dl_il_sel = 0;    // 0: innolink0, 1: innolink1

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin();
                itochild != req_node.MemberEnd();
                ++itochild)
            {
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "reset_type") {
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        if(std::string(itochild->value.GetString()) == "hard") {
                            hardreset = true;
                        }
                        add_member(currkey, itochild->value, rsp_node, rsp_at);
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "phase"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        this->phase = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "antport"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        this->antport_on_reset = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "dl_il_sel"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        this->dl_il_sel = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "ul_il_sel"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        this->ul_il_sel = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx.n_dpa"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        this->n_dpa = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            if(hardreset){
                this->nextstate = RE_FWLOAD_S;
            }
            else {
                send_hswi_msg_to_thread(THREAD_IL,
                                        uid,
                                        std::string("req"),
                                        std::string("reset_req"),
                                        std::string("re:0"),
                                        itcnode,
                                        itcdoc,
                                        itc_at);

                hal::nsleep(1000000000);    // 1000 msec

                this->nextstate = RE_FWLOAD_S;
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::resync_req__re(const rapidjson::Value& req_node,
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
                resync_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = std::string(itochild->name.GetString());

                add_member(currkey, itochild->value, rsp_node, rsp_at);
                add_member(currkey, itochild->value, itcnode, itc_at);
            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("resync_req"),
                                    std::string("re:0"),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::resync_rsp__re(const rapidjson::Value& req_node,
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
                resync_rsp__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            //rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            //rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();

            itc_resync_req  resyncreq;
            // zero-clear
            memset(&resyncreq, 0, sizeof(itc_resync_req));
            resyncreq.enabled = true;

            send_itc_message(THREAD_SP,
                             ITC_RESYNC_REQ_ID,
                             sizeof(itc_resync_req),
                             (const uint8_t *)&resyncreq);

            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::get_swver_req__re(const rapidjson::Value& req_node,
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
                get_swver_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            add_member(std::string("active_sw_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("active_fw_ver"), g_fw_ver, rsp_node, rsp_at);
            //add_member(std::string("active_dsp_ver"), g_dsp_ver, rsp_node, rsp_at);
            add_member(std::string("active_pl_ver"), g_fpga_ver, rsp_node, rsp_at);
            add_member(std::string("active_app_ver"), ::get_verstr(), rsp_node, rsp_at);
            add_member(std::string("passive_sw_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_fw_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_dsp_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_pl_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_app_ver"), std::string(""), rsp_node, rsp_at);
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::prepare_swupdate_req__re(const rapidjson::Value& req_node,
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
                prepare_swupdate_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                // TODO: ftpget
                //add_member(std::string(itochild->name.GetString()), itochild->value, rsp_node, rsp_at);
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool re_thread::activate_sw_req__re(const rapidjson::Value& req_node,
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
                activate_sw_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();

                add_member(currkey, itochild->value, rsp_node, rsp_at);
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

bool re_thread::restarted_ind__re(const rapidjson::Value& req_node,
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
                restarted_ind__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                add_member(currkey, itochild->value, itcnode, itc_at);

                if(currkey == "reset_reason"){
                    // do nothing
                }
                else if(currkey == "app_version" || currkey == "fw_ver"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        g_fw_ver  = g_dsp_ver = itochild->value.GetString();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
                send_hswi_msg_to_thread(THREAD_HSWI_CONN + kk,
                                        uid,
                                        std::string("ind"),
                                        std::string("restarted_ind"),
                                        std::string("re:0"),
                                        itcnode,
                                        itcdoc,
                                        itc_at);
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
} // end of bool re_thread::restarted_ind__re()

bool re_thread::resynchronized_ind__re(const rapidjson::Value& req_node,
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
                resynchronized_ind__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                add_member(currkey, itochild->value, itcnode, itc_at);
            }

            for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
                send_hswi_msg_to_thread(THREAD_HSWI_CONN + kk,
                                        uid,
                                        std::string("ind"),
                                        std::string("resynchronized_ind"),
                                        std::string("re:0"),
                                        itcnode,
                                        itcdoc,
                                        itc_at);
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
} // end of bool re_thread::resynchronized_ind__re()

void re_thread::re_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT))){
        TRACE0OBJ()
            << "stm_id=" << stm_id
            << ", curr_state=" << curr_state
            << ", next_state=" << next_state
            << std::endl;
    }

    rapidjson::Document inddoc;
    rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();
    inddoc.SetObject();
    { // header
        rapidjson::Value indheader(rapidjson::kObjectType);
        {
            indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
            indheader.AddMember("uid", rapidjson::Value(0).Move(), ind_at);
        }
        inddoc.AddMember("header", indheader, ind_at);
    }

    { // body
        rapidjson::Value indbody(rapidjson::kObjectType);
        { // primitive
            rapidjson::Value indprimitive(rapidjson::kObjectType);
            { // object
                rapidjson::Value indobj(rapidjson::kObjectType);
                {
                    std::string fst;
                    switch(next_state){
                        case    RE_INIT_S: {
                            fst = std::string("init");
                            break;
                        }
                        case    RE_FWLOAD_S: {
                            fst = std::string("fw_load");
                            break;
                        }
                        case    RE_INIT_INNOLINK_S: {
                            fst = std::string("init_innolink");
                            break;
                        }
                        case    RE_INIT_SYNC_S: {
                            fst = std::string("init_sync");
                            break;
                        }
                        case    RE_READY_S: {
                            fst = std::string("ready");
                            break;
                        }
                        default:;
                    }
                    indobj.AddMember("fst", rapidjson::Value(fst.c_str(), fst.size(), ind_at).Move(), ind_at);
                }
                std::string objid("re:0");
                indprimitive.AddMember(rapidjson::Value(objid.c_str(), objid.size(), ind_at).Move(), indobj, ind_at);
            }
            indbody.AddMember("state_change_ind", indprimitive, ind_at);
        }
        inddoc.AddMember("body", indbody, ind_at);
    }

    std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

    // broadcast to all HSWI connection socket thread
    for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
        send_itc_message(THREAD_HSWI_CONN + kk,
                         ITC_PAYLOAD_FROM_HSWI_ID,
                         inddocstr.size(),
                         (const uint8_t *)inddocstr.c_str());
    }
} // end of void re_thread::re_state_changed_callback()

void re_thread::re_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((re_thread *)arg)->re_state_changed_callback(stm_id, curr_state, next_state);
    }
}

//-------------------------------------------------------------------------------
// inventory:n
//-------------------------------------------------------------------------------
bool re_thread::load_param_req__inventory(const rapidjson::Value& req_node,
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
                load_param_req__inventory(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "serial_number"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        this->serial_number = itochild->value.GetString();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "product_id"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        this->product_id = itochild->value.GetString();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "hw_version"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        this->hw_version = itochild->value.GetString();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else{
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, rspparam, rsp_at);
                }
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

bool re_thread::get_param_req__inventory(const rapidjson::Value& req_node,
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
                get_param_req__inventory(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            if(inst_id == 2){   // for a chip vendor
                add_member(std::string("product_id"), this->product_id, rsp_node, rsp_at);
                add_member(std::string("serial_number"), this->serial_number, rsp_node, rsp_at);
                add_member(std::string("hw_version"), this->hw_version, rsp_node, rsp_at);
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

//-------------------------------------------------------------------------------
// register:n
//-------------------------------------------------------------------------------
bool re_thread::load_param_req__register(const rapidjson::Value& req_node,
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
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            itcnode.CopyFrom(req_node, itc_at);

#if 0
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, rspparam, rsp_at);
                }
            }
#endif

            if(inst_id == 0){
                // the object "register:0" for hermes2 fw
                send_hswi_msg_to_thread(THREAD_SP,
                                        uid,
                                        std::string("req"),
                                        std::string("load_param_req"),
                                        std::string("register:" + std::to_string(inst_id)),
                                        itcnode,
                                        itcdoc,
                                        itc_at,
                                        ITC_NVDATA_ID);
            }
            else if(inst_id == 1){
                // the object "register:1" for fpga
                send_hswi_msg_to_thread(THREAD_IL,
                                        uid,
                                        std::string("req"),
                                        std::string("load_param_req"),
                                        std::string("register:" + std::to_string(inst_id)),
                                        itcnode,
                                        itcdoc,
                                        itc_at,
                                        ITC_NVDATA_ID);
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
// section:n
//-------------------------------------------------------------------------------
bool re_thread::create_obj_req__section(const rapidjson::Value& req_node,
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
                create_obj_req__section(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "start_symbol_id"){
                    std::vector<int>    data;
                    size_t idx = 0;
                    sectionobj[inst_id].start_symbol_bitmask = 0;
                    for(rapidjson::Value::ConstValueIterator ita = itochild->value.Begin(); ita != itochild->value.End(); ++ita){
                        sectionobj[inst_id].start_symbol_bitmask |= (1UL << ita->GetInt());
                        data.push_back(ita->GetInt());
                        idx++;
                    }
                    sectionobj[inst_id].numsymbol = idx;
                    add_member(currkey, data, itcnode, itc_at);
                }
                else if(currkey == "frame_structure"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        sectionobj[inst_id].frame_structure = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "scs"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        sectionobj[inst_id].scs = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "cp_length"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        sectionobj[inst_id].scs = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "fs"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        sectionobj[inst_id].fs = itochild->value.GetDouble();
                        long fs_x = (long)(sectionobj[inst_id].fs*1000);
                        add_member(std::string("fs_x"), (int)fs_x, itcnode, itc_at);
                        sectionobj[inst_id].fs_x = fs_x;
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "osr"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        sectionobj[inst_id].osr = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);

                    // NOTE: bypass any type of member
                    add_member(currkey, itochild->value, itcnode, itc_at);
                }
            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("create_obj_req"),
                                    std::string("section:") + std::to_string(inst_id),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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
} // end of bool re_thread::create_obj_req__section()

//-------------------------------------------------------------------------------
// tm_file:n
//-------------------------------------------------------------------------------
bool re_thread::download_tm_file_req__tm_file(const rapidjson::Value& req_node,
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

    //TODO: implement this
    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                download_tm_file_req__tm_file(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Value itcnode(rapidjson::kObjectType);

            itc_download_tm_file_req download_tm_file_req;

            download_tm_file_req.uid = uid;

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                int meminstid;
                std::string memstrkey = itochild->name.GetString();
                std::string instkey = this->split_instance_id(memstrkey, meminstid);
                std::string fullkey = node_key + "/" + instkey;

                if(instkey == "scheme"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.scheme, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "host"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.host, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "user"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.user, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "password"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.password, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "path"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.path, itochild->value.GetString(), MAX_LONG_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            send_itc_message(THREAD_SP,
                             ITC_DOWNLOAD_TM_FILE_REQ_ID,
                             sizeof(itc_download_tm_file_req),
                             (const uint8_t *)&download_tm_file_req);

            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);
} // end of bool re_thread::download_tm_file_req__tm_file(...)

bool re_thread::download_file_req__tm_file(const rapidjson::Value& req_node,
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

    //TODO: implement this
    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                download_file_req__tm_file(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Value itcnode(rapidjson::kObjectType);

            itc_download_tm_file_req download_tm_file_req;

            download_tm_file_req.uid = uid;

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                int meminstid;
                std::string memstrkey = itochild->name.GetString();
                std::string instkey = this->split_instance_id(memstrkey, meminstid);
                std::string fullkey = node_key + "/" + instkey;

                if(instkey == "scheme"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.scheme, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "host"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.host, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "user"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.user, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "password"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.password, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "path"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&download_tm_file_req.path, itochild->value.GetString(), MAX_LONG_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            send_itc_message(THREAD_SP,
                             ITC_DOWNLOAD_FILE_REQ__TM_FILE_ID,
                             sizeof(itc_download_tm_file_req),
                             (const uint8_t *)&download_tm_file_req);

            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);
} // end of bool re_thread::download_file_req__tm_file(...)

//-------------------------------------------------------------------------------
// tm_filelist:n
//-------------------------------------------------------------------------------
bool re_thread::get_param_req__tm_filelist(const rapidjson::Value& req_node,
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
                get_param_req__tm_filelist(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            // filename
            std::vector<std::string> vlist;

            if(find_tm_files(vlist) == true){
                add_member(std::string("filename"), vlist, rsp_node, rsp_at);
            }
            else{
                return(false);
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);  // true: valid object, false: invalid object
}   // void re_thread::get_param_req__tm_filelist()

//-------------------------------------------------------------------------------
// tm_playback:n
//-------------------------------------------------------------------------------
bool re_thread::get_param_req__tm_playback(const rapidjson::Value& req_node,
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
                get_param_req__tm_playback(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            add_member(std::string("state"),
                          (tmplaybackreq.tmplaybackobj[inst_id].state > 0)? std::string("enable"): std::string("disable"),
                          rsp_node, rsp_at);
            // tx_ant_id
            {
                // int txantid[MAX_TX_ANT];
                std::vector<int> vlist;
                int antidx = 0;
                for(int kk = 0; kk < TX_ANT_BITMASK64; kk++){
                    for(int ll = 0; ll < 64; ll++){
                        if(tmplaybackreq.tmplaybackobj[inst_id].txantbitmask[kk] & (1UL << ll)){
                            vlist.push_back(antidx);
                        }
                        antidx++;
                    }
                }
                add_member(std::string("tx_ant_id"), vlist, rsp_node, rsp_at);
            }

            // section_id
            if(tmplaybackreq.tmplaybackobj[inst_id].numsection > 1){
                std::vector<int> vlist;
                for(int kk = 0; kk < tmplaybackreq.tmplaybackobj[inst_id].numsection; kk++){
                    vlist.push_back(tmplaybackreq.tmplaybackobj[inst_id].sectionid[kk]);
                }
                add_member(std::string("section_id"), vlist, rsp_node, rsp_at);
            }
            else if(tmplaybackreq.tmplaybackobj[inst_id].numsection == 1){
                add_member(std::string("section_id"),
                              tmplaybackreq.tmplaybackobj[inst_id].sectionid[0],
                              rsp_node, rsp_at);
            }

            // filename
            std::string filenamestr(tmplaybackreq.tmplaybackobj[inst_id].filename);
            add_member(std::string("filename"), filenamestr, rsp_node, rsp_at);
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);  // true: valid object, false: invalid object
}   // void re_thread::get_param_req__tm_playback()

bool re_thread::create_obj_req__tm_playback(const rapidjson::Value& req_node,
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

    if((inst_id < 0) || (inst_id >= MAX_PLAYBACK)){
        return(false);
    }

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                create_obj_req__tm_playback(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            // if the first obj item in a array
            if(item_idx == 0){
                tmplaybackreq.num_playbackobj = 0;
                // zero-clear
                memset(&tmplaybackreq, 0, sizeof(itc_playback_req));
            }

            if(tmplaybackreq.num_playbackobj >= MAX_PLAYBACK) {
                TRACE0OBJ() << "tmplaybackreq.num_playbackobj(" << tmplaybackreq.num_playbackobj << ") should be less than " << MAX_PLAYBACK << std::endl;
                return(false);
            }

            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].playbackid = inst_id;
            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numsection = 0;

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                int meminstid;
                std::string memstrkey = itochild->name.GetString();
                std::string instkey = this->split_instance_id(memstrkey, meminstid);
                std::string fullkey = node_key + "/" + instkey;

                if(instkey == "state"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].state = (itochild->value.GetString() == std::string("enable"))? 1: 0;
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "filename"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy(tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].filename,
                                std::string(itochild->value.GetString()).c_str(),
                                MAX_FILENAME);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "tx_ant_id"){
                    if(itochild->value.GetType() == rapidjson::kArrayType){
                        size_t txantidx = 0;
                        for(rapidjson::Value::ConstValueIterator ita = itochild->value.Begin(); ita != itochild->value.End(); ++ita){
                            int txant = ita->GetInt();
                            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].txantbitmask[txant >> 3] |= (1UL << (txant % 64));
                            txantidx++;
                        }
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numtxant = txantidx;
                    }
                    else if(itochild->value.GetType() == rapidjson::kNumberType){
                        int txant = itochild->value.GetInt();
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].txantbitmask[txant >> 3] |= (1UL << (txant % 64));
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numtxant = 1;
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if((instkey == "tm_timing_adv") || (instkey == "tx_timing_adv")){
                    // tx_timing_adv is obsolete
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].tm_timing_adv = itochild->value.GetDouble();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "num_segments"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].num_segments = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "off_duty_mode"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string offdutymode(itochild->value.GetString());
                        if((offdutymode == "zeropad") || (offdutymode == "continuous")){
                            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].off_duty_mode = 0;
                        }
                        else if((offdutymode == "dtx") || (offdutymode == "gated")){
                            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].off_duty_mode = 1;
                        }
                        else{
                            tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].off_duty_mode = 0;
                        }
                    }
                    else if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].off_duty_mode = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].tdd_ul_dl_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].tdd_ssf_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "section"){
                    if(itochild->value.GetType() == rapidjson::kObjectType){
                        int sectionarrayidx = tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numsection;
                        if(sectionarrayidx >= NUM_SECTION_PER_CARRIER) {
                            TRACE0OBJ() <<
                                "numsection(" << tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numsection <<
                                ") should be less than " << NUM_SECTION_PER_CARRIER << std::endl;
                            return(false);
                        }
                        section_struct *p1section = &tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].section[sectionarrayidx];

                        p1section->section_id = meminstid;

                        for(rapidjson::Value::ConstMemberIterator subchild = itochild->value.MemberBegin();
                            subchild != itochild->value.MemberEnd();
                            ++subchild)
                        {
                            std::string sectionchildkey(subchild->name.GetString());

                            if(sectionchildkey == "start_symbol_id"){
                                if(subchild->value.GetType() == rapidjson::kArrayType){
                                    int symbidx = 0;
                                    for(rapidjson::Value::ConstValueIterator itsymarray = subchild->value.Begin(); itsymarray != subchild->value.End(); ++itsymarray) {
                                        p1section->start_symbol_bitmask |= (1UL << itsymarray->GetInt());
                                    }
                                    p1section->numsymbol = symbidx;
                                }
                                else if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->start_symbol_bitmask |= (1UL << subchild->value.GetInt());
                                    p1section->numsymbol = 1;
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else if(sectionchildkey == "frame_structure"){
                                if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->frame_structure = subchild->value.GetInt();
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else if(sectionchildkey == "scs"){
                                if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->scs = subchild->value.GetInt();
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else if(sectionchildkey == "cp_length"){
                                if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->cp_length = subchild->value.GetInt();
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else if(sectionchildkey == "fs"){
                                if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->fs = subchild->value.GetDouble();
                                    p1section->fs_x = (long)(p1section->fs*1000);
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else if(sectionchildkey == "osr"){
                                if(subchild->value.GetType() == rapidjson::kNumberType){
                                    p1section->osr = subchild->value.GetInt();
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            }
                            else {
                                TRACE0OBJ() << "unknown " << sectionchildkey << std::endl;
                            }
                        }

                        // increment the numsection
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].numsection++;
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(instkey == "ul_only"){
                    if(itochild->value.GetType() == rapidjson::kTrueType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].ul_only = 1;
                    }
                    else if(itochild->value.GetType() == rapidjson::kFalseType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].ul_only = 0;
                    }
                    else if(itochild->value.GetType() == rapidjson::kNumberType){
                        tmplaybackreq.tmplaybackobj[tmplaybackreq.num_playbackobj].ul_only = itochild->value.GetInt();
                    }
                }
                else {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, fullkey, inst_id, uid, rspparam, rsp_at);
                }
            }

            // increment the tmplaybackreq.num_playbackobj
            tmplaybackreq.num_playbackobj++;

            // if the last obj item in a array
            if(num_item == (item_idx + 1)){
                send_itc_message(THREAD_SP,
                                 ITC_PLAYBACK_REQ_ID,
                                 sizeof(itc_playback_req),
                                 (const uint8_t *)&tmplaybackreq);
            }

            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default:;
    }

    return(status);  // true: valid object, false: invalid object
}   // void re_thread::create_obj_req__tm_playback()


bool re_thread::delete_obj_req__tm_playback(const rapidjson::Value& req_node,
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

    if((inst_id < 0) || (inst_id >= MAX_PLAYBACK)){
        return(false);
    }

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                delete_obj_req__tm_playback(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType:{
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            // if the first obj item in a array
            if(item_idx == 0){
                tmplaybackreq.num_playbackobj = 0;
                // zero-clear
                memset(&tmplaybackreq, 0, sizeof(itc_playback_req));
            }

            // increment the tmplaybackreq.num_playbackobj
            tmplaybackreq.num_playbackobj = 0;

            // if the last obj item in a array
            if(num_item == (item_idx + 1)){
#if 0
                TRACE0OBJ()
                    << "num_playbackobj=" << tmplaybackreq.num_playbackobj
                    << ", num_item=" << num_item
                    << std::endl;
#endif
                send_itc_message(THREAD_SP,
                                 ITC_PLAYBACK_REQ_ID,
                                 sizeof(itc_playback_req),
                                 (const uint8_t *)&tmplaybackreq);
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default:;
    }


    return(status);  // true: valid object, false: invalid object
} // bool re_thread::delete_obj_req__tm_playback()

bool re_thread::create_obj_req__frc_capture(const rapidjson::Value& req_node,
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

    if(inst_id != 0) return(false);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                create_obj_req__frc_capture(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            itc_create_capture_req  createcapturereq;

            memset(&createcapturereq, 0, sizeof(itc_create_capture_req));
            // default
            createcapturereq.uid = uid;
            createcapturereq.sdu_mask = 0;
            createcapturereq.sdu_target = 0;
            createcapturereq.num_segments = 1;
            createcapturereq.off_duty_mode = 0; // "zeropad" | "nongated"
            createcapturereq.scs = 0;           // 0: 15kHz, 1: 30 kHz
            createcapturereq.tdd_ul_dl_config = 0;
            createcapturereq.tdd_ssf_config = 0;
            createcapturereq.osr = 1;

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "duration"){
                    // TODO
                }
                else if(currkey == "rx_ant_idx"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        int rxantidx = itochild->value.GetInt();
                        createcapturereq.sdu_mask     |= IL_SDU_HEADER__RUPORTID_M;
                        createcapturereq.sdu_target   |= ((rxantidx << IL_SDU_HEADER__RUPORTID_SHL) & IL_SDU_HEADER__RUPORTID_M);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "section_id"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        int sectionid = itochild->value.GetInt();
                        createcapturereq.sdu_mask     |= IL_SDU_HEADER__SECTIONID_M;
                        createcapturereq.sdu_target   |= ((sectionid << IL_SDU_HEADER__SECTIONID_SHL) & IL_SDU_HEADER__SECTIONID_M);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "num_segments"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.num_segments = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "off_duty_mode"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string offdutymode(itochild->value.GetString());
                        if((offdutymode == "zeropad") || (offdutymode == "nongated")){
                            createcapturereq.off_duty_mode = 0;
                        }
                        else if((offdutymode == "dtx") || (offdutymode == "gated")){
                            createcapturereq.off_duty_mode = 1;
                        }
                        else{
                            createcapturereq.off_duty_mode = 0;
                        }
                    }
                    else if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.off_duty_mode = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "scs"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.scs = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "osr"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.osr = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.tdd_ul_dl_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_uldl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.tdd_ul_dl_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        createcapturereq.tdd_ssf_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            send_itc_message(THREAD_SP,
                             ITC_CREATE_CAPTURE_REQ_ID,
                             sizeof(itc_create_capture_req),
                             (const uint8_t *)&createcapturereq);

            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default:;
    }

    return(status);  // true: valid object, false: invalid object
} // bool re_thread::create_obj_req__frc_capture()

//-------------------------------------------------------------------------------
// frc_file:n
//-------------------------------------------------------------------------------
bool re_thread::capture_file_req__frc_file(const rapidjson::Value& req_node,
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

    //TODO: implement this
    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                capture_file_req__frc_file(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Value itcnode(rapidjson::kObjectType);

            itc_capture_frc_file_req capture_frc_file_req;

            capture_frc_file_req.uid = uid;

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "scheme"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&capture_frc_file_req.scheme, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "host"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&capture_frc_file_req.host, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "user"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&capture_frc_file_req.user, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "password"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&capture_frc_file_req.password, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "path"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&capture_frc_file_req.path, itochild->value.GetString(), MAX_LONG_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            send_itc_message(THREAD_SP,
                             ITC_CAPTURE_FILE_REQ__FRC_FILE_ID,
                             sizeof(itc_capture_frc_file_req),
                             (const uint8_t *)&capture_frc_file_req);

            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);
} // end of bool re_thread::capture_file_req__frc_file(...)

bool re_thread::upload_file_req__frc_file(const rapidjson::Value& req_node,
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

    //TODO: implement this
    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                upload_file_req__frc_file(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Value itcnode(rapidjson::kObjectType);

            itc_upload_frc_file_req upload_frc_file_req;

            upload_frc_file_req.uid = uid;

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "scheme"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&upload_frc_file_req.scheme, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "host"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&upload_frc_file_req.host, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "user"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&upload_frc_file_req.user, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "password"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&upload_frc_file_req.password, itochild->value.GetString(), MAX_SHORT_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "path"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        strncpy((char *)&upload_frc_file_req.path, itochild->value.GetString(), MAX_LONG_CHAR);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }

            send_itc_message(THREAD_SP,
                             ITC_UPLOAD_FILE_REQ__FRC_FILE_ID,
                             sizeof(itc_upload_frc_file_req),
                             (const uint8_t *)&upload_frc_file_req);

            break;
        }

        case  rapidjson::kNullType:
        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        default:;
    }

    return(status);
} // end of bool re_thread::upload_file_req__frc_file(...)

//-------------------------------------------------------------------------------
// carrier common
//-------------------------------------------------------------------------------
void re_thread::tx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ()
            << "stm_id=" << stm_id
            << ", curr_state=" << curr_state
            << ", next_state=" << next_state
            << std::endl;
    }

    if(stm_id >= NUM_CARRIER) return;

    rapidjson::Document inddoc;
    rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();
    inddoc.SetObject();
    { // header
        rapidjson::Value indheader(rapidjson::kObjectType);
        {
            indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
            indheader.AddMember("uid", rapidjson::Value(0).Move(), ind_at);
        }
        inddoc.AddMember("header", indheader, ind_at);
    }

    { // body
        rapidjson::Value indbody(rapidjson::kObjectType);
        { // primitive
            rapidjson::Value indprimitive(rapidjson::kObjectType);
            { // object
                rapidjson::Value indobj(rapidjson::kObjectType);
                {
                    std::string fst = txcarrierobj[stm_id].get_fst_str(next_state);
                    indobj.AddMember("fst", rapidjson::Value(fst.c_str(), fst.size(), ind_at).Move(), ind_at);
                }
                std::string objid = txcarrierobj[stm_id].get_obj_id();
                indprimitive.AddMember(rapidjson::Value(objid.c_str(), objid.size(), ind_at).Move(), indobj, ind_at);
            }
            indbody.AddMember("state_change_ind", indprimitive, ind_at);
        }
        inddoc.AddMember("body", indbody, ind_at);
    }

    std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

    // broadcast to all HSWI connection socket thread
    for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
        send_itc_message(THREAD_HSWI_CONN + kk,
                         ITC_PAYLOAD_FROM_HSWI_ID,
                         inddocstr.size(),
                         (const uint8_t *)inddocstr.c_str());
    }
}

void re_thread::tx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((re_thread *)arg)->tx_carrier_state_changed_callback(stm_id, curr_state, next_state);
    }
}

void re_thread::rx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ()
            << "stm_id=" << stm_id
            << ", curr_state=" << curr_state
            << ", next_state=" << next_state
            << std::endl;
    }

    if(stm_id >= NUM_CARRIER) return;

    rapidjson::Document inddoc;
    rapidjson::Document::AllocatorType& ind_at = inddoc.GetAllocator();
    inddoc.SetObject();
    { // header
        rapidjson::Value indheader(rapidjson::kObjectType);
        {
            indheader.AddMember("type", rapidjson::Value("ind").Move(), ind_at);
            indheader.AddMember("uid", rapidjson::Value(0).Move(), ind_at);
        }
        inddoc.AddMember("header", indheader, ind_at);
    }

    { // body
        rapidjson::Value indbody(rapidjson::kObjectType);
        { // primitive
            rapidjson::Value indprimitive(rapidjson::kObjectType);
            { // object
                rapidjson::Value indobj(rapidjson::kObjectType);
                {
                    std::string fst = rxcarrierobj[stm_id].get_fst_str(next_state);
                    indobj.AddMember("fst", rapidjson::Value(fst.c_str(), fst.size(), ind_at).Move(), ind_at);
                }
                std::string objid = rxcarrierobj[stm_id].get_obj_id();
                indprimitive.AddMember(rapidjson::Value(objid.c_str(), objid.size(), ind_at).Move(), indobj, ind_at);
            }
            indbody.AddMember("state_change_ind", indprimitive, ind_at);
        }
        inddoc.AddMember("body", indbody, ind_at);
    }

    std::string inddocstr = rapidjson_wrapper::str_json(inddoc);

    // broadcast to all HSWI connection socket thread
    for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++){
        send_itc_message(THREAD_HSWI_CONN + kk,
                         ITC_PAYLOAD_FROM_HSWI_ID,
                         inddocstr.size(),
                         (const uint8_t *)inddocstr.c_str());
    }
}

void re_thread::rx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((re_thread *)arg)->rx_carrier_state_changed_callback(stm_id, curr_state, next_state);
    }
}

//-------------------------------------------------------------------------------
// tx_carrier_lte:n
//-------------------------------------------------------------------------------
bool re_thread::delete_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status =  false;

    for(std::vector<int>::iterator it = tx_carrier_lte_vec.begin(); it != tx_carrier_lte_vec.end(); ++it){
        if(*it == inst_id){
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                delete_obj_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            // TODO: block below codes after CP over innolink work
            //          gpio31=1 for rf output enable,
            //          gpio31=0 for rf output disable
            itc_misc_req itcmiscreq;
            memset(&itcmiscreq, 0, sizeof(itc_misc_req)); // zero clear
            itcmiscreq.gpio31 = 0;
            send_itc_message(THREAD_SP,
                             ITC_MISC_REQ_ID,
                             sizeof(itc_misc_req),
                             (const uint8_t *)&itcmiscreq);

            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("delete_obj_req"),
                                    std::string("tx_carrier_lte:0" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);

            // TODO: mutex
            std::vector<int>::iterator it = tx_carrier_lte_vec.end();
            for(it = tx_carrier_lte_vec.begin(); it != tx_carrier_lte_vec.end(); ++it){
                if(*it == inst_id) {
                    // NOTE: do NOT call the "erase()" within this loop.
                    break;
                }
            }
            if(it != tx_carrier_lte_vec.end()){  // found
                tx_carrier_lte_vec.erase(it);
            }
            // update fst
            txcarrierobj[inst_id].transit_state(tx_carrier::NOT_OPERATIONAL_S, false);
            add_member(std::string("fst"), txcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
}   // end of bool re_thread::delete_obj_req__tx_carrier_lte()

bool re_thread::create_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
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
                create_obj_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "duplex"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        txcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "chan_bw"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "fs"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].fs = itochild->value.GetDouble();
                        add_member(std::string("fs_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_freq"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].tx_freq = itochild->value.GetDouble();
                        add_member(std::string("tx_freq_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
#if 0
                else if(currkey == "tx_sigpath:0"){
                }
                else if(currkey == "tx_sigpath:1"){
                }
                else if(currkey == "section_id"){
                }
#endif
                else if(currkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_max_pwr"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].tx_max_pwr = itochild->value.GetDouble();
                        add_member(std::string("tx_max_pwr_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "norm_iq_level"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        txcarrierobj[inst_id].norm_iq_level = itochild->value.GetDouble();
                        add_member(std::string("norm_iq_level_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_delay_adj"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_fe_timing_adj"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);

                    // NOTE: bypass any type of member
                    add_member(currkey, itochild->value, itcnode, itc_at);
                }
            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("create_obj_req"),
                                    std::string("tx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);

#if 0
            std::string itcdocstr = rapidjson_wrapper::str_json(itcdoc);
            TRACE0OBJ() << "itcdocstr=" << itcdocstr << std::endl;
#endif

            // TODO: mutex
            std::vector<int>::iterator it = tx_carrier_lte_vec.end();
            for(it = tx_carrier_lte_vec.begin(); it != tx_carrier_lte_vec.end(); ++it){
                if(*it == inst_id) {
                    break;
                }
            }
            // update fst
            if(it == tx_carrier_lte_vec.end()) { // new
                tx_carrier_lte_vec.push_back(inst_id);
                txcarrierobj[inst_id].set_obj_id(std::string("tx_carrier_lte:" + std::to_string(inst_id)));
                txcarrierobj[inst_id].transit_state(tx_carrier::NOT_OPERATIONAL_S, false);
            }
            add_member(std::string("fst"), txcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
} // end of bool re_thread::create_obj_req__tx_carrier_lte()

bool re_thread::create_obj_rsp__tx_carrier_lte(const rapidjson::Value& req_node,
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
                create_obj_rsp__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "result"){
                }
                else if(currkey == "fst"){
                }
                else{
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, itc_at);
                }

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
} // end of bool re_thread::create_obj_rsp__tx_carrier_lte()

bool re_thread::modify_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
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
                modify_param_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "duplex"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        txcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "chan_bw"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        txcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_freq"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(std::string("tx_freq_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                        txcarrierobj[inst_id].tx_freq = itochild->value.GetDouble();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        txcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        txcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tx_max_pwr"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(std::string("tx_max_pwr_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                        txcarrierobj[inst_id].tx_max_pwr = itochild->value.GetDouble();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "norm_iq_level"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(std::string("norm_iq_level_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                        txcarrierobj[inst_id].norm_iq_level = itochild->value.GetDouble();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);

                    // NOTE: bypass any type of member
                    add_member(currkey, itochild->value, itcnode, itc_at);
                }
            }
            // update fst
            add_member(std::string("fst"), txcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("modify_param_req"),
                                    std::string("tx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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
} // end of bool re_thread::modify_param_req__tx_carrier_lte()

bool re_thread::get_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status = true;
    bool  found = false;

    for(size_t kk = 0; kk < tx_carrier_lte_vec.size(); kk++){
        if(tx_carrier_lte_vec[kk] == inst_id){
            // found
            found = true;
            break;
        }
    }
    if(found == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                get_param_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            add_member(std::string("chan_bw"), txcarrierobj[inst_id].chan_bw, rsp_node, rsp_at);
            add_member(std::string("duplex"), txcarrierobj[inst_id].is_tdd? std::string("tdd") : std::string("fdd"), rsp_node, rsp_at);
            add_member(std::string("tx_freq"), txcarrierobj[inst_id].tx_freq, rsp_node, rsp_at);
            add_member(std::string("tdd_ul_dl_config"), txcarrierobj[inst_id].tdd_ul_dl_config, rsp_node, rsp_at);
            add_member(std::string("tdd_ssf_config"), txcarrierobj[inst_id].tdd_ssf_config, rsp_node, rsp_at);
            add_member(std::string("tx_max_pwr"), txcarrierobj[inst_id].tx_max_pwr, rsp_node, rsp_at);
            add_member(std::string("fst"), txcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("get_param_req"),
                                    std::string("tx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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
} // end of bool re_thread::get_param_req__tx_carrier_lte()

bool re_thread::modify_state_req__tx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status =  false;

    for(size_t kk = 0; kk < tx_carrier_lte_vec.size(); kk++){
        if(tx_carrier_lte_vec[kk] == inst_id){
            // found
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                modify_state_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "ast"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string memval = itochild->value.GetString();
                        add_member(currkey, itochild->value, rsp_node, rsp_at);
                        add_member(currkey, itochild->value, itcnode, itc_at);

                        // TODO: block below codes after CP over innolink work
                        //          gpio31=1 for rf output enable,
                        //          gpio31=0 for rf output disable
                        itc_misc_req itcmiscreq;
                        memset(&itcmiscreq, 0, sizeof(itc_misc_req)); // zero clear
                        itcmiscreq.gpio31 = (memval == std::string("unlocked"))? 1: 0;
                        send_itc_message(THREAD_SP,
                                         ITC_MISC_REQ_ID,
                                         sizeof(itc_misc_req),
                                         (const uint8_t *)&itcmiscreq);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);

                    // NOTE: bypass any type of member
                    add_member(currkey, itochild->value, itcnode, itc_at);
                }

            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("modify_state_req"),
                                    std::string("tx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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

bool re_thread::modify_state_rsp__tx_carrier_lte(const rapidjson::Value& req_node,
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
                modify_state_rsp__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "result"){
                }
                else if(currkey == "ast"){
                }
                else if(currkey == "fst"){
                }
                else{
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, itc_at);
                }

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
} // end of bool re_thread::modify_state_rsp__tx_carrier_lte()

bool re_thread::state_change_ind__tx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status =  false;

    for(size_t kk = 0; kk < tx_carrier_lte_vec.size(); kk++){
        if(tx_carrier_lte_vec[kk] == inst_id){
            // found
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                state_change_ind__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "fst"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string memval = itochild->value.GetString();

                        if(memval == std::string("not_operational")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::NOT_OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("pre_operational")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::PRE_OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("operational")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("degraded")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::DEGRADED_S, false);
                        }
                        else if(memval == std::string("failed")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::FAILED_S, false);
                        }
                        else if(memval == std::string("disabled")){
                            txcarrierobj[inst_id].transit_state(tx_carrier::DISABLED_S, false);
                        }
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }

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
// rx_carrier_lte:n
//-------------------------------------------------------------------------------
bool re_thread::delete_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
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
    bool status = false;

    for(std::vector<int>::iterator it = rx_carrier_lte_vec.begin(); it != rx_carrier_lte_vec.end(); ++it){
        if(*it == inst_id){
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                delete_obj_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            // TODO: mutex
            std::vector<int>::iterator it = rx_carrier_lte_vec.end();
            for(it = rx_carrier_lte_vec.begin(); it != rx_carrier_lte_vec.end(); ++it){
                if(*it == inst_id) {
                    // NOTE: do NOT call the "erase()" within this loop.
                    break;
                }
            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("delete_obj_req"),
                                    std::string("rx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);

            if(it != rx_carrier_lte_vec.end()){ // found
                rx_carrier_lte_vec.erase(it);
            }
            // update fst
            rxcarrierobj[inst_id].transit_state(rx_carrier::NOT_OPERATIONAL_S, false);
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
} // end of bool re_thread::delete_obj_req__rx_carrier_lte()

bool re_thread::create_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
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
                create_obj_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "duplex"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        rxcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "chan_bw"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        rxcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "fs"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        rxcarrierobj[inst_id].fs = itochild->value.GetDouble();
                        add_member(std::string("fs_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "rx_freq"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        rxcarrierobj[inst_id].rx_freq = itochild->value.GetDouble();
                        add_member(std::string("rx_freq_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        rxcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        rxcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "rx_delay_adj"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "rx_fe_timing_adj"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);

                    // NOTE: bypass any type of member
                    add_member(currkey, itochild->value, itcnode, itc_at);
                }
            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("create_obj_req"),
                                    std::string("rx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);

#if 0
            std::string itcdocstr = rapidjson_wrapper::str_json(itcdoc);
            TRACE0OBJ() << "itcdocstr=" << itcdocstr << std::endl;
#endif

            // TODO: mutex
            std::vector<int>::iterator it = rx_carrier_lte_vec.end();
            for(it = rx_carrier_lte_vec.begin(); it != rx_carrier_lte_vec.end(); ++it){
                if(*it == inst_id) {
                    break;
                }
            }
            // update fst
            if(it == rx_carrier_lte_vec.end()) { // new
                rx_carrier_lte_vec.push_back(inst_id);
                rxcarrierobj[inst_id].set_obj_id(std::string("rx_carrier_lte:" + std::to_string(inst_id)));
                rxcarrierobj[inst_id].transit_state(rx_carrier::NOT_OPERATIONAL_S, false);
            }
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
}   // end of bool re_thread::create_obj_req__rx_carrier_lte()

bool re_thread::create_obj_rsp__rx_carrier_lte(const rapidjson::Value& req_node,
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
                create_obj_rsp__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "result"){
                }
                else if(currkey == "fst"){
                }
                else{
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, itc_at);
                }

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
} // end of bool re_thread::create_obj_rsp__rx_carrier_lte()

bool re_thread::modify_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
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
                modify_param_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "duplex"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        rxcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "chan_bw"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        rxcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "rx_freq"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(std::string("rx_freq_x"), (int)(itochild->value.GetDouble()*1000), itcnode, itc_at);
                        rxcarrierobj[inst_id].rx_freq = itochild->value.GetDouble();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ul_dl_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        rxcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else if(currkey == "tdd_ssf_config"){
                    if(itochild->value.GetType() == rapidjson::kNumberType){
                        add_member(currkey, itochild->value, itcnode, itc_at);
                        rxcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }
            // update fst
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("modify_param_req"),
                                    std::string("rx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
} // end of bool re_thread::modify_param_req__rx_carrier_lte()

bool re_thread::get_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
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
    bool status = true;
    bool found = false;

    for(size_t kk = 0; kk < rx_carrier_lte_vec.size(); kk++){
        if(rx_carrier_lte_vec[kk] == inst_id){
            // found
            found = true;
            break;
        }
    }
    if(found == false) return(status);


    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                get_param_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            add_member(std::string("chan_bw"), rxcarrierobj[inst_id].chan_bw, rsp_node, rsp_at);
            add_member(std::string("duplex"), rxcarrierobj[inst_id].is_tdd? std::string("tdd") : std::string("fdd"), rsp_node, rsp_at);
            add_member(std::string("rx_freq"), rxcarrierobj[inst_id].rx_freq, rsp_node, rsp_at);
            add_member(std::string("tdd_ul_dl_config"), rxcarrierobj[inst_id].tdd_ul_dl_config, rsp_node, rsp_at);
            add_member(std::string("tdd_ssf_config"), rxcarrierobj[inst_id].tdd_ssf_config, rsp_node, rsp_at);
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("get_param_req"),
                                    std::string("rx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kNumberType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
} // end of bool re_thread::get_param_req__rx_carrier_lte()

bool re_thread::modify_state_req__rx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status =  false;

    for(size_t kk = 0; kk < rx_carrier_lte_vec.size(); kk++){
        if(rx_carrier_lte_vec[kk] == inst_id){
            // found
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                modify_state_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "ast"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string memval = itochild->value.GetString();

                        add_member(currkey, itochild->value, rsp_node, rsp_at);
                        add_member(currkey, itochild->value, itcnode, itc_at);
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }

            }

            send_hswi_msg_to_thread(THREAD_IL,
                                    uid,
                                    std::string("req"),
                                    std::string("modify_state_req"),
                                    std::string("rx_carrier_lte:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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
} // end of bool re_thread::modify_state_req__rx_carrier_lte()

bool re_thread::modify_state_rsp__rx_carrier_lte(const rapidjson::Value& req_node,
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
                modify_state_rsp__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "result"){
                }
                else if(currkey == "ast"){
                }
                else if(currkey == "fst"){
                }
                else{
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, itc_at);
                }

            }

            itc_ul_axi_stream_req  ul_axi_stream_req;
            // zero-clear
            memset(&ul_axi_stream_req, 0, sizeof(itc_ul_axi_stream_req));
            ul_axi_stream_req.enabled = true;

            send_itc_message(THREAD_SP,
                             ITC_UL_AXI_STREAM_REQ_ID,
                             sizeof(itc_ul_axi_stream_req),
                             (const uint8_t *)&ul_axi_stream_req);
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
} // end of bool re_thread::modify_state_rsp__rx_carrier_lte()

bool re_thread::state_change_ind__rx_carrier_lte(const rapidjson::Value& req_node,
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
    bool  status =  false;

    for(size_t kk = 0; kk < rx_carrier_lte_vec.size(); kk++){
        if(rx_carrier_lte_vec[kk] == inst_id){
            // found
            status = true;
            break;
        }
    }
    if(status == false) return(status);

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                state_change_ind__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                if(currkey == "fst"){
                    if(itochild->value.GetType() == rapidjson::kStringType){
                        std::string memval = itochild->value.GetString();

                        if(memval == std::string("not_operational")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::NOT_OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("pre_operational")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::PRE_OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("operational")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::OPERATIONAL_S, false);
                        }
                        else if(memval == std::string("degraded")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::DEGRADED_S, false);
                        }
                        else if(memval == std::string("failed")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::FAILED_S, false);
                        }
                        else if(memval == std::string("disabled")){
                            rxcarrierobj[inst_id].transit_state(rx_carrier::DISABLED_S, false);
                        }
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }

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

bool re_thread::modify_param_req__re_debug_mask(const rapidjson::Value &req_node,
                                                const std::string &node_key,
                                                const int &inst_id,
                                                const size_t uid,
                                                rapidjson::Value &rsp_node,
                                                rapidjson::Document::AllocatorType &rsp_at)
{
    bool  haschild =  true;

    (void)(req_node);  // avoid of warning
    (void)(node_key);    // avoid of warning
    (void)(uid);    // avoid of warning
    (void)(rsp_node); // avoid of warnig
    (void)(rsp_at); // avoid of warnig

    TRACE0OBJ()
        << "node_key=" << node_key << std::endl;

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType: {
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                default_param(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string key = itochild->name.GetString();
                std::string fullkey = node_key + "/" + key;
                rapidjson::Value    rspparam(rapidjson::kObjectType);
                // recursive call
                this->exec_param(itochild->value, fullkey, inst_id, uid, rspparam, rsp_at);
                rsp_node.AddMember(rapidjson::Value(key.c_str(), key.size(), rsp_at).Move(), rspparam, rsp_at);
            }
            break;
        }

        case  rapidjson::kStringType: {
            std::string value(req_node.GetString());

            g_debug_mask = strtoul(value.c_str(), NULL, 0);
            TRACE0OBJ()
                << "g_debug_mask=0x" << std::hex << std::setw(8) << std::setfill('0') << g_debug_mask << std::endl;

            rsp_node = rapidjson::Value(value.c_str(), value.size(), rsp_at).Move();
            break;
        }

        case  rapidjson::kNumberType: {
            int  value = req_node.GetInt();

            g_debug_mask = value;
            TRACE0OBJ()
                << "g_debug_mask=0x" << std::hex << std::setw(8) << std::setfill('0') << g_debug_mask << std::endl;

            rsp_node = value;
            break;
        }

        case  rapidjson::kFalseType: {
#if 0
            std::cout << node_key << "=" << "false" << std::endl;
#endif
            rsp_node = false;
            break;
        }

        case  rapidjson::kTrueType: {
#if 0
            std::cout << node_key << "=" << "true" << std::endl;
#endif
            rsp_node = true;
            break;
        }

        case  rapidjson::kNullType: {
#if 0
            std::cout << node_key << "=" << "null" << std::endl;
#endif
            break;
        }

        default:;
    }

    return(haschild);  // true: keep going, false: stop parsing children
} // end of bool re_thread::modify_param_req__re_debug_mask()

bool re_thread::modify_param_req__fpga(const rapidjson::Value& req_node,
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
                modify_param_req__fpga(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rapidjson::Document itcdoc;
            rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string currkey = itochild->name.GetString();
                std::string fullkey = node_key + "/" + currkey;

                add_member(currkey, itochild->value, rsp_node, rsp_at);
                add_member(currkey, itochild->value, itcnode, itc_at);
            }

            send_hswi_msg_to_thread(THREAD_SP,
                                    uid,
                                    std::string("req"),
                                    std::string("modify_param_req"),
                                    std::string("fpga:" + std::to_string(inst_id)),
                                    itcnode,
                                    itcdoc,
                                    itc_at);
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
} // end of bool re_thread::modify_param_req__fpga


//-------------------------------------------------------------------------------
// innolink_thread
//-------------------------------------------------------------------------------
void re_thread::start_re_thread()
{
    pthread_attr_init(&this->threadattr);
    pthread_attr_setdetachstate(&this->threadattr, PTHREAD_CREATE_JOINABLE);
    sigemptyset(&this->sigmask);   // clear signals
    sigaddset(&this->sigmask, EVENT_ITC);
    sigaddset(&this->sigmask, EVENT_SOCKET);
    sigaddset(&this->sigmask, SIGPIPE);    // NOTE: double-protection against crash due to SIGPIPE
    if(pthread_sigmask(SIG_BLOCK, &this->sigmask, NULL) != 0){
        perror("pthread_sigmask()");
    }

    pthread_create(&this->thread, &this->threadattr, re_thread::start_wrapper, (void *)this);
} // end of re_thread::start_re_thread()

void *re_thread::start_wrapper(void *p1arg)
{
    re_thread *p1thread = (re_thread *)p1arg;

    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT))){
        size_t pid = getpid();
        TRACE0() << "pid=" << pid << std::endl;
    }

    p1thread->run_re_thread();

    return((void *)p1thread);
} // end of re_thread::start_wrapper(void *p1arg)

void re_thread::poll_timer_wrapper(void *p1arg)
{
#if 0
    fprintf(stderr, ".");
#endif
    // send EVENT_ITC to itself
    ((re_thread *)p1arg)->send_event(EVENT_ITC);
} // end of void re_thread::poll_timer_wrapper(..)

void re_thread::kick_timer_wrapper(void *p1arg)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(re_thread);
    }
    // it is an one-shot timer, therefore it should remove itself from the timed_list
    timed_list.del_item(re_thread::kick_timer_wrapper,
                        (void *)p1arg);

    ((re_thread *)p1arg)->send_itc_message(THREAD_RE,
                                           ITC_KICKOFF_ID,
                                           0,
                                           (const uint8_t *)NULL);
} // end of void re_thread::poll_timer_wrapper(..)

void re_thread::run_re_thread()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    timed_list.add_item(re_thread::poll_timer_wrapper,
                        (void *)this,
                        /*int offset=*/ 0,
                        /*int interval=*/ 100,
                        /*bool override=*/false);

    timed_list.add_item(re_thread::kick_timer_wrapper,
                        (void *)this,
                        /*int offset=*/ 0,
                        /*int interval=*/ 100,
                        /*bool override=*/false);

    transit_state(RE_INIT_S, false);

    for( ; ; ) {    // infinite loop
        int recvsig;    // for sigwait()

        if(sigwait(&sigmask, &recvsig) != 0){
            std::stringstream sstrm;
            TRACE3(sstrm, re_thread, thread_id) << "error in sigwait()" << std::endl;
            perror(sstrm.str().c_str());
        }

        switch(recvsig) {
            case EVENT_ITC: {
                process_itc_event();
                watchdogctrl.keep_alive();
                break;
            }


            case SIGINT: { // usually cntl+c
                std::stringstream sstrm;
                TRACE3(sstrm, re_thread, thread_id) << "received SIGINT" << std::endl;
                perror(sstrm.str().c_str());
                break;
            }

            case SIGTERM:
            case SIGALRM:
            default:;
        }
    }

    TRACE3(std::cerr, re_thread, thread_id) << "thread exit" << std::endl;

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); //
} // end of void run_re_thread()

