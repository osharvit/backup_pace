#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>          // std::stringstream
#include "il_thread.h"

#if defined(__x86_64__) // for simulation only
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

    for(size_t kk = 0; kk < NUM_CARRIER; kk++){
        txcarrierobj[kk].set_stm_id(kk);
        txcarrierobj[kk].install_state_changed_callback(&il_thread::tx_carrier_state_changed_callback_wrapper, (void *)this);

        rxcarrierobj[kk].set_stm_id(kk);
        rxcarrierobj[kk].install_state_changed_callback(&il_thread::rx_carrier_state_changed_callback_wrapper, (void *)this);
    }
} // end of il_thread::il_thread(const std::string &name, size_t id)
#endif

#if defined(__x86_64__) // for simulation only
void il_thread::setup_hswi_handler()
{
    //---------------------------------------------------------------------
    // register:1
    //---------------------------------------------------------------------
    this->objid_map["re:/load_param_req/register"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::load_param_req__register);

    //---------------------------------------------------------------------
    // re:0
    //---------------------------------------------------------------------
    this->objid_map["re:/reset_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::reset_req__re);
    this->objid_map["re:/resync_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::resync_req__re);
    this->objid_map["re:/get_swver_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::get_swver_req__re);
    this->objid_map["re:/prepare_swupdate_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::prepare_swupdate_req__re);
    this->objid_map["re:/activate_sw_req/re"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::activate_sw_req__re);

    //---------------------------------------------------------------------
    // section:n
    //---------------------------------------------------------------------
    this->objid_map["re:/create_obj_req/section"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::create_obj_req__section);

    //---------------------------------------------------------------------
    // tx_carrier_lte:n
    //---------------------------------------------------------------------
    this->objid_map["re:/delete_obj_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::delete_obj_req__tx_carrier_lte);
    this->objid_map["re:/create_obj_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::create_obj_req__tx_carrier_lte);
    this->objid_map["re:/modify_param_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::modify_param_req__tx_carrier_lte);
    this->objid_map["re:/get_param_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::get_param_req__tx_carrier_lte);
    this->objid_map["re:/modify_state_req/tx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::modify_state_req__tx_carrier_lte);

    //---------------------------------------------------------------------
    // rx_carrier_lte:n
    //---------------------------------------------------------------------
    this->objid_map["re:/delete_obj_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::delete_obj_req__rx_carrier_lte);
    this->objid_map["re:/create_obj_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::create_obj_req__rx_carrier_lte);
    this->objid_map["re:/modify_param_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::modify_param_req__rx_carrier_lte);
    this->objid_map["re:/get_param_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::get_param_req__rx_carrier_lte);
    this->objid_map["re:/modify_state_req/rx_carrier_lte"] =
        static_cast<hswi_json::OBJID_FUNC>(&il_thread::modify_state_req__rx_carrier_lte);

}   // end of void il_thread::setup_hswi_handler()
#endif

//-------------------------------------------------------------------------------
// re:
//-------------------------------------------------------------------------------
#if defined(__x86_64__) // for simulation only
bool il_thread::reset_req__re(const rapidjson::Value& req_node,
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
                reset_req__re(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                add_member(std::string(itochild->name.GetString()), std::string(itochild->value.GetString()), rsp_node, rsp_at);
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
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::resync_req__re(const rapidjson::Value& req_node,
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
            rsp_node.SetObject();
            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
#if 0
            if(req_node.HasMember("resync_type")) {
                add_member(std::string("resync_type"), std::string(req_node["resync_type"].GetString()), rsp_node, rsp_at);
            }
#else
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                add_member(std::string(itochild->name.GetString()), itochild->value, rsp_node, rsp_at);
            }
#endif
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
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::get_swver_req__re(const rapidjson::Value& req_node,
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

#if defined(__x86_64__) // for simulation only
            add_member(std::string("active_sw_ver"), std::string("v0.0.1"), rsp_node, rsp_at);
            add_member(std::string("active_fw_ver"), std::string("v0.0.1"), rsp_node, rsp_at);
            add_member(std::string("active_dsp_ver"), std::string("v0.0.1"), rsp_node, rsp_at);
            add_member(std::string("active_pl_ver"), std::string("v0.0.1"), rsp_node, rsp_at);
            add_member(std::string("active_app_ver"), std::string("v0.0.1"), rsp_node, rsp_at);
            add_member(std::string("passive_sw_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_fw_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_dsp_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_pl_ver"), std::string(""), rsp_node, rsp_at);
            add_member(std::string("passive_app_ver"), std::string(""), rsp_node, rsp_at);
#endif
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
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::prepare_swupdate_req__re(const rapidjson::Value& req_node,
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
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::activate_sw_req__re(const rapidjson::Value& req_node,
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
                add_member(std::string(itochild->name.GetString()), itochild->value, rsp_node, rsp_at);
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
#endif

//-------------------------------------------------------------------------------
// section:n
//-------------------------------------------------------------------------------
#if defined(__x86_64__) // for simulation only
bool il_thread::create_obj_req__section(const rapidjson::Value& req_node,
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);
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
#endif

//-------------------------------------------------------------------------------
// tx_carrier_lte:n
//-------------------------------------------------------------------------------
#if defined(__x86_64__) // for simulation only
bool il_thread::delete_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

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
#endif

    return(status);  // true: valid object, false: invalid object
}   // end of bool il_thread::delete_obj_req__tx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::create_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string childkey = node_key + "/" + memkey;

                if(memkey == "chan_bw"){
                    txcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                }
                else if(memkey == "duplex"){
                    txcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                }
                else if(memkey == "tx_freq"){
                    txcarrierobj[inst_id].tx_freq = itochild->value.GetDouble();
                }
                else if(memkey == "tdd_ul_dl_config"){
                    txcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                }
                else if(memkey == "tdd_ssf_config"){
                    txcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                }
                else if(memkey == "tx_max_pwr"){
                    txcarrierobj[inst_id].tx_max_pwr = itochild->value.GetDouble();
                }
                else {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, childkey, inst_id, uid, rspparam, rsp_at);
                    //rsp_node.AddMember(rapidjson::Value(memkey.c_str(), memkey.size(), rsp_at).Move(), rspparam, rsp_at);
                }
            }

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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::create_obj_req__tx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::modify_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string childkey = node_key + "/" + memkey;

                if(memkey == "chan_bw"){
                    txcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                }
                else if(memkey == "duplex"){
                    txcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                }
                else if(memkey == "tx_freq"){
                    txcarrierobj[inst_id].tx_freq = itochild->value.GetDouble();
                }
                else if(memkey == "tdd_ul_dl_config"){
                    txcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                }
                else if(memkey == "tdd_ssf_config"){
                    txcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                }
                else if(memkey == "tx_max_pwr"){
                    txcarrierobj[inst_id].tx_max_pwr = itochild->value.GetDouble();
                }
                else {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, childkey, inst_id, uid, rspparam, rsp_at);
                    //rsp_node.AddMember(rapidjson::Value(memkey.c_str(), memkey.size(), rsp_at).Move(), rspparam, rsp_at);
                }
            }
            // update fst
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::modify_param_req__tx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::get_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
                get_param_req__tx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            add_member(std::string("chan_bw"), txcarrierobj[inst_id].chan_bw, rsp_node, rsp_at);
            add_member(std::string("duplex"), txcarrierobj[inst_id].is_tdd? std::string("tdd") : std::string("fdd"), rsp_node, rsp_at);
            add_member(std::string("tx_freq"), txcarrierobj[inst_id].tx_freq, rsp_node, rsp_at);
            add_member(std::string("tdd_ul_dl_config"), txcarrierobj[inst_id].tdd_ul_dl_config, rsp_node, rsp_at);
            add_member(std::string("tdd_ssf_config"), txcarrierobj[inst_id].tdd_ssf_config, rsp_node, rsp_at);
            add_member(std::string("tx_max_pwr"), txcarrierobj[inst_id].tx_max_pwr, rsp_node, rsp_at);
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::get_param_req__tx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::modify_state_req__tx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string memval = itochild->value.GetString();
                if(memkey == "ast"){
                    if(memval == std::string("unlocked")) {
                        txcarrierobj[inst_id].transit_state(tx_carrier::PRE_OPERATIONAL_S, true);
                        TRACE0OBJ()
                            << "txcarrierobj.state=" << txcarrierobj[inst_id].get_curr_state()
                            << std::endl;
                    }
                    else{
                        txcarrierobj[inst_id].transit_state(tx_carrier::NOT_OPERATIONAL_S, true);
                    }
                    add_member(std::string(itochild->name.GetString()), std::string(itochild->value.GetString()), rsp_node, rsp_at);

                    TRACE0OBJ()
                        << "ast=" << memval
                        << ", txcarrierobj.state=" << txcarrierobj[inst_id].get_curr_state()
                        << ", txcarrierobj.fst=" << txcarrierobj[inst_id].get_fst_str()
                        << std::endl;
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
#endif

    return(status);  // true: valid object, false: invalid object
}
#endif

#if     defined(__x86_64__) // for simulation only
void il_thread::tx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    TRACE0OBJ()
        << "stm_id=" << stm_id
        << ", curr_state=" << curr_state
        << ", next_state=" << next_state
        << std::endl;

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
    TRACE0OBJ()
        << inddocstr << std::endl;

    send_itc_message(THREAD_RE,
                     ITC_PAYLOAD_FROM_IL_ID,
                     inddocstr.size(),
                     (const uint8_t *)inddocstr.c_str());
} // end of void il_thread::tx_carrier_state_changed_callback()
#endif

#if     defined(__x86_64__) // for simulation only
void il_thread::rx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    TRACE0OBJ()
        << "stm_id=" << stm_id
        << ", curr_state=" << curr_state
        << ", next_state=" << next_state
        << std::endl;

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
    TRACE0OBJ()
        << inddocstr << std::endl;

    send_itc_message(THREAD_RE,
                     ITC_PAYLOAD_FROM_IL_ID,
                     inddocstr.size(),
                     (const uint8_t *)inddocstr.c_str());
} // end of void il_thread::rx_carrier_state_changed_callback()
#endif

#if     defined(__x86_64__) // for simulation only
void il_thread::tx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((il_thread *)arg)->tx_carrier_state_changed_callback(stm_id, curr_state, next_state);
    }
}
#endif

#if     defined(__x86_64__) // for simulation only
void il_thread::rx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((il_thread *)arg)->rx_carrier_state_changed_callback(stm_id, curr_state, next_state);
    }
}
#endif

//-------------------------------------------------------------------------------
// rx_carrier_lte:n
//-------------------------------------------------------------------------------
#if defined(__x86_64__) // for simulation only
bool il_thread::delete_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::delete_obj_req__rx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::create_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string childkey = node_key + "/" + memkey;

                if(memkey == "chan_bw"){
                    rxcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                }
                else if(memkey == "duplex"){
                    rxcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                }
                else if(memkey == "rx_freq"){
                    rxcarrierobj[inst_id].rx_freq = itochild->value.GetDouble();
                }
                else if(memkey == "tdd_ul_dl_config"){
                    rxcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                }
                else if(memkey == "tdd_ssf_config"){
                    rxcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                }
                else {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, childkey, inst_id, uid, rspparam, rsp_at);
                    //rsp_node.AddMember(rapidjson::Value(memkey.c_str(), memkey.size(), rsp_at).Move(), rspparam, rsp_at);
                }
            }

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
#endif

    return(status);  // true: valid object, false: invalid object
}   // end of bool il_thread::create_obj_req__rx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::modify_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string childkey = node_key + "/" + memkey;

                if(memkey == "chan_bw"){
                    rxcarrierobj[inst_id].chan_bw = itochild->value.GetInt();
                }
                else if(memkey == "duplex"){
                    rxcarrierobj[inst_id].is_tdd = (itochild->value.GetString() == std::string("tdd"));
                }
                else if(memkey == "rx_freq"){
                    rxcarrierobj[inst_id].rx_freq = itochild->value.GetDouble();
                }
                else if(memkey == "tdd_ul_dl_config"){
                    rxcarrierobj[inst_id].tdd_ul_dl_config = itochild->value.GetInt();
                }
                else if(memkey == "tdd_ssf_config"){
                    rxcarrierobj[inst_id].tdd_ssf_config = itochild->value.GetInt();
                }
                else {
                    rapidjson::Value    rspparam(rapidjson::kObjectType);
                    // recursive call
                    this->exec_param(itochild->value, childkey, inst_id, uid, rspparam, rsp_at);
                    //rsp_node.AddMember(rapidjson::Value(memkey.c_str(), memkey.size(), rsp_at).Move(), rspparam, rsp_at);
                }
            }
            // update fst
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::modify_param_req__rx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::get_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
                get_param_req__rx_carrier_lte(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            add_member(std::string("chan_bw"), rxcarrierobj[inst_id].chan_bw, rsp_node, rsp_at);
            add_member(std::string("duplex"), rxcarrierobj[inst_id].is_tdd? std::string("tdd") : std::string("fdd"), rsp_node, rsp_at);
            add_member(std::string("rx_freq"), rxcarrierobj[inst_id].rx_freq, rsp_node, rsp_at);
            add_member(std::string("tdd_ul_dl_config"), rxcarrierobj[inst_id].tdd_ul_dl_config, rsp_node, rsp_at);
            add_member(std::string("tdd_ssf_config"), rxcarrierobj[inst_id].tdd_ssf_config, rsp_node, rsp_at);
            add_member(std::string("fst"), rxcarrierobj[inst_id].get_fst_str(), rsp_node, rsp_at);
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::get_param_req__rx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
bool il_thread::modify_state_req__rx_carrier_lte(const rapidjson::Value& req_node,
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

#if     defined(__x86_64__) // for simulation only
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
            rsp_node.SetObject();

            add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                std::string memkey = itochild->name.GetString();
                std::string memval = itochild->value.GetString();
                if(memkey == "ast"){
                    if(memval == std::string("unlocked")){
                        rxcarrierobj[inst_id].transit_state(rx_carrier::PRE_OPERATIONAL_S, true);
                        TRACE0OBJ()
                            << "rxcarrierobj.state=" << rxcarrierobj[inst_id].get_curr_state()
                            << std::endl;
                    }
                    else {
                        rxcarrierobj[inst_id].transit_state(rx_carrier::NOT_OPERATIONAL_S, true);
                    }
                    add_member(std::string(itochild->name.GetString()), std::string(itochild->value.GetString()), rsp_node, rsp_at);

                    TRACE0OBJ()
                        << "ast=" << memval
                        << ", rxcarrierobj.state=" << rxcarrierobj[inst_id].get_curr_state()
                        << ", rxcarrierobj.fst=" << rxcarrierobj[inst_id].get_fst_str()
                        << std::endl;
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
#endif

    return(status);  // true: valid object, false: invalid object
} // end of bool il_thread::modify_state_req__rx_carrier_lte()
#endif

#if defined(__x86_64__) // for simulation only
size_t il_thread::il_ready_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    if(event == EVENT_SOCKET) {
        rapidjson::Document rspdoc;

        bool parse_result = this->load(rspdoc, residual);
        if(parse_result){
            // NOTE: do not call the "process_hswi_messages()" here.
            std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
            if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_MPLANE_JSON))){
                TRACE0OBJ() << "rspdoc=" << rspdocstr << std::endl;
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
                    }
                    break;
                }
                case ITC_PAYLOAD_ID: {
                    if((g_debug_mask & (1UL << MASKBIT_MPLANE_PATTERN))) {    // CP test mode (fpga -> h2)
                        std::string sdustr("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
                        if(g_debug_mask & (1UL << MASKBIT_MPLANE_JSON)){
                            TRACE0OBJ()
                                << sdustr << std::endl;
                        }
                        p1h2top[0].p1innolink[P_IL].send_mplane(sdustr);
                    }
                    else {
                        std::string sdustr(p1msg->payload.sdu);
                        if(g_debug_mask & (1UL << MASKBIT_MPLANE_JSON)){
                            TRACE0OBJ()
                                << sdustr << std::endl;
                        }
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

