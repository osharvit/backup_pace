#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <new>
#include <map>
#include <iomanip>
#include <cstring>
#include <algorithm>    // std::transform()
#include <cctype>       // std::tolower()
#include <exception>    // std::exception
#include "hswi_json.h"

hswi_json::hswi_json()
{
#if defined(UNITTEST_TRACE)
    this->prmtv_map["create_obj_req"] =
        static_cast<hswi_json::PRMTV_FUNC>(&hswi_json::create_obj_req);

    this->objid_map["create_obj_req/tx_carrier_ltetdd"] =
        static_cast<hswi_json::OBJID_FUNC>(&hswi_json::create_obj_req__tx_carrier_ltetdd);
#endif

    this->param_map[""] = static_cast<hswi_json::PARAM_FUNC>(&hswi_json::default_param);
}

hswi_json::~hswi_json(){}

//---------------------------------------------------------------
//     vbgn         vend    
//      272, {       334    
//      288,         403    
//      349,         405   }
//      408, {       470    
//      424,         539    
//      485,         541   }
//      544,         606    
//      560
//      621
//      649
//      686
//---------------------------------------------------------------
//     vpair = [(408, 541), (272, 405)] <= reversed order
//---------------------------------------------------------------
size_t hswi_json::find_pair_recursively(std::vector<std::complex<size_t> > &vpair,
                                        std::vector<size_t> &vbgn,
                                        std::vector<size_t> &vend,
                                        size_t bgnpos,
                                        size_t endpos)
{
    size_t status = vpair.size();

    if((vbgn.size() == vend.size()) && (vbgn.back() < vend.back())){
        // update endpos
        if((bgnpos < endpos) && (endpos != std::string::npos)) {
#if defined(UNITTEST_TRACE)
            std::cout << "(" << bgnpos << "," << endpos << ")" << std::endl;
#endif
            vpair.push_back(std::complex<size_t>(bgnpos, endpos));
        }
        endpos = vend.back();
        if(vend.size() <= 0) {
            bgnpos = vbgn.front();
            //std::cout << "bgnpos=" << bgnpos << ", endpos=" << endpos << std::endl;
            return(vpair.size());
        }
        else {
            //std::cout << "bgnpos=" << bgnpos << ", endpos=" << endpos << std::endl;
            vend.pop_back();
            status = find_pair_recursively(vpair, vbgn, vend, bgnpos, endpos);
        }
    }
    else if(vbgn.back() > vend.back()) {
        if(vbgn.size() <= 0){
            if((bgnpos < endpos) && (endpos != std::string::npos)) {
#if defined(UNITTEST_TRACE)
                std::cout << "(" << bgnpos << "," << endpos << ")" << std::endl;
#endif
                vpair.push_back(std::complex<size_t>(bgnpos, endpos));
            }
            return(vpair.size());
        }
        else {
            // update bgnpos
            bgnpos = vbgn.back();
            vbgn.pop_back();
            status = find_pair_recursively(vpair, vbgn, vend, bgnpos, endpos);
        }
    }
    else {
        if(vend.size() <= 0) {
            // update bgnpos
            bgnpos = vbgn.front();
            if((bgnpos < endpos) && (endpos != std::string::npos)) {
#if defined(UNITTEST_TRACE)
                std::cout << "(" << bgnpos << "," << endpos << ")" << std::endl;
#endif
                vpair.push_back(std::complex<size_t>(bgnpos, endpos));
            }
            return(vpair.size());
        }
        else {
            vend.pop_back();
            status = find_pair_recursively(vpair, vbgn, vend, bgnpos, endpos);
        }
    }

    return(status);
} // end of size_t hswi_json::find_pair_recursively(...)

//---------------------------------------------------------------
// residual=
//          "rx_ant_port": [ "rx_ant_port:0", "rx_ant_port:1" ], "tdd_ul_dl_config": 2, "tdd_ssf_config": 3, "cp_length": "normal" } } } }
//[{ "header": { "type": "req", "uid": 1 }, "body": {"delete_obj_req": "rx_carrier_ltetdd:0" } },
// { "header": { "type": "req", "uid": 2 }, "body": {"delete_obj_req": "tx_carrier_ltetdd:0" } } ]
// { "header": { "type": "req", "uid": 3 }, "body": {"create_obj_req": { "rx_carrier_ltetdd:1": { "chan_bw": 20, "rx_freq": 3610.4,
//---------------------------------------------------------------
// lifo_bgnpos  lifo_endpos 
//      272, {       334    
//      288,         403    
//      349,         405   }
//      408, {       470    
//      424,         539    
//      485,         541   }
//      544,         606    
//      560
//      621
//      649
//      686
//---------------------------------------------------------------
//     vpair = [(408, 541), (272, 405)] <= reversed order
//---------------------------------------------------------------
size_t hswi_json::segment_reassemble(std::vector<std::string> &vsdu, std::string &residual, std::string &in)
{
    size_t status = 0;
    std::vector<size_t> lifo_bgnpos;
    std::vector<size_t> lifo_endpos;
    std::string sdustr;
    std::vector<std::complex<size_t> > vpair;
    size_t  pos1;
    size_t  pos2; 
    size_t  pos3;
    size_t  pos4;

    residual = residual + in;
    //std::cout << residual << std::endl;

    // find \"header\"
    if((pos1 = residual.find("\"header\"")) != std::string::npos) {
        pos2 =  residual.rfind("{", pos1);
        if(pos2 != std::string::npos) {
            lifo_bgnpos.push_back(pos2);
            pos3 = pos2;
            while((pos3 = residual.find("{", pos3 + 1)) != std::string::npos){
                lifo_bgnpos.push_back(pos3);
            }
            pos4 = pos2;
            while((pos4 = residual.find("}", pos4 + 1)) != std::string::npos){
                lifo_endpos.push_back(pos4);
            } 
            //std::cout << "lifo_bgnpos.size()=" << lifo_bgnpos.size() << std::endl;
            if(lifo_bgnpos.size() >= lifo_endpos.size()){
#if defined(UNITTEST_TRACE)
                for(size_t kk = 0; kk < lifo_endpos.size(); kk++){
                    std::cout << lifo_bgnpos[kk] << ",\t" << lifo_endpos[kk] << std::endl;
                }
                for(size_t kk = lifo_endpos.size(); kk < lifo_bgnpos.size(); kk++){
                    std::cout << lifo_bgnpos[kk] << std::endl;
                }
#endif
                if((status = find_pair_recursively(vpair, lifo_bgnpos, lifo_endpos,
                                                   std::string::npos, std::string::npos)) > 0){
                    //std::cout << "pos5=" << pos5 << std::endl;
                    //sdustr = residual.substr(pos2, pos5+1-pos2);
                    //residual = residual.substr(pos5+1);
                    for(std::vector<std::complex<size_t> >::reverse_iterator rit = vpair.rbegin(); rit != vpair.rend(); ++rit){
                        //std::cout << *rit << std::endl;
                        vsdu.push_back(residual.substr(rit->real(), rit->imag() + 1 - rit->real()));
                    }
                    if(vpair.size() > 0) {
                        // truncate residual
                        residual = residual.substr(vpair[0].imag() + 1);
                    }
                }
                else{
                    if(residual.size() >= BUF_SZ) residual.clear();
                }
            }
            else {
                if(residual.size() >= BUF_SZ) residual.clear();
            }
        }
    }
    else {
        if(residual.size() >= BUF_SZ) residual.clear();
    }

    return(status);
}   // end of void hswi_json::segment_reassemble()

bool hswi_json::load(rapidjson::Document &doc, std::string& str)
{
    bool status = false;
    try{
        status = rapidjson_wrapper::load_json(doc, str);  // class method
    }
    catch (std::exception &e){
        status = false;
        std::cerr << "[" << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << "standard exception=" << e.what() << std::endl;
        std::cerr << "str=" << str << std::endl;
    }

    return(status);
}

bool hswi_json::load(rapidjson::Document &doc, std::ifstream& ifs)
{ // IStream Wrapper with std::ifstream
    bool status = false;
    
    try{
        status = rapidjson_wrapper::load_json(doc, ifs);  // class method
    }
    catch (std::exception &e){
        status = false;
        std::cerr << "[" << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << "standard exception=" << e.what() << std::endl;
    }

    return(status);
}

bool hswi_json::load(rapidjson::Document &doc, std::stringstream& ss)
{ // IStream Wrapper with std::stringstream
    bool status = false;
    
    try{
        status = rapidjson_wrapper::load_json(doc, ss);   // class method
    }
    catch (std::exception &e){
        status = false;
        std::cerr << "[" << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << "standard exception=" << e.what() << std::endl;
        std::cerr << "ss=" << ss.str() << std::endl;
    }

    return(status);
}

void hswi_json::dump(rapidjson::Document &json)
{ // OStreamWrapper with std::stringstream
    rapidjson_wrapper::dump_json(json);       // class method
}

void hswi_json::dump(rapidjson::Document &json, std::stringstream& ss)
{ // OStreamWrapper with std::stringstream
    rapidjson_wrapper::dump_json(json, ss);   // class method
}

void hswi_json::dump(rapidjson::Document &json, std::string& str)
{ // OStreamWrapper with std::ofstream
    rapidjson_wrapper::dump_json(json,str);   // class method
}

void hswi_json::dump(rapidjson::Document &json, std::ofstream& ofs)
{ // OStreamWrapper with std::ofstream
    rapidjson_wrapper::dump_json(json, ofs);  // class method
}

void hswi_json::dump(const rapidjson::Value &json)
{ // OStreamWrapper with std::stringstream
    rapidjson_wrapper::dump_json(json);       // class method
}

void hswi_json::dump(const rapidjson::Value &json, std::stringstream& ss)
{ // OStreamWrapper with std::stringstream
    rapidjson_wrapper::dump_json(json, ss);   // class method
}

void hswi_json::dump(const rapidjson::Value &json, std::string& str)
{ // OStreamWrapper with std::ofstream
    rapidjson_wrapper::dump_json(json,str);   // class method
}

void hswi_json::dump(const rapidjson::Value &json, std::ofstream& ofs)
{ // OStreamWrapper with std::ofstream
    rapidjson_wrapper::dump_json(json, ofs);  // class method
}

std::string hswi_json::str_tolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); } // correct
                  );
    return s;
}

std::string hswi_json::str_strip(std::string in) 
{
    in.erase(std::remove_if(in.begin(), in.end(), [] (std::string::value_type ch)
                            { return !isalnum(ch); }
                           ), in.end());
    return in;
}

std::string hswi_json::split_instance_id(const std::string &obj_id, int &instance_id)
{
    std::string objtype;
    size_t found = std::string::npos;

    instance_id = -1;

    if((found = obj_id.find(":")) != std::string::npos){
        objtype = obj_id.substr(0, found);
        std::string instance_id_lower = this->str_tolower(this->str_strip(obj_id.substr(found + 1)));

        instance_id = (instance_id_lower.find("all") != std::string::npos)? -1 : atoi(instance_id_lower.c_str());
    }
    else {
        objtype = obj_id;
    }

    return(objtype);
} // end of std::string hswi_json::split_instance_id(...)


void hswi_json::parse_recursively(const rapidjson::Value& req_node,
                                   const std::string &key)
{
    rapidjson_wrapper::parse_json_recursively(req_node, key);

    return;
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
void hswi_json::exec_prmtv(const rapidjson::Value& req_node,
                           const std::string& node_key,
                           const size_t uid,
                           rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at)

{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }

    PRMTV_FUNC p1func = this->prmtv_map[node_key];
    (p1func != NULL)?
        (this->*p1func)(req_node, node_key, uid, rsp_node, rsp_at):
        //-------------------------------------------------
        // process_hswi_primitive()
        //-------------------------------------------------
        process_hswi_primitive(req_node, node_key, uid, rsp_node, rsp_at);
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
bool hswi_json::exec_objid(const rapidjson::Value& req_node,
                           const std::string& node_key,
                           const int &inst_id,
                           const size_t uid,
                           rapidjson::Value &rsp_node,
                           rapidjson::Document::AllocatorType &rsp_at,
                           size_t item_idx,
                           size_t num_item)

{
    bool  status = true;
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }

    OBJID_FUNC p1func = this->objid_map[node_key];
    status = (p1func != NULL)?
        (this->*p1func)(req_node, node_key, inst_id, uid, rsp_node, rsp_at, item_idx, num_item):
        default_objid(req_node, node_key, inst_id, uid, rsp_node, rsp_at, item_idx, num_item);

    return(status);
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
bool hswi_json::exec_param(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at)

{
    bool  status = true;
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }

    PARAM_FUNC p1func = this->param_map[node_key];
    status = (p1func != NULL)?
        (this->*p1func)(req_node, node_key, inst_id, uid, rsp_node, rsp_at): default_param(req_node, node_key, inst_id, uid, rsp_node, rsp_at);

    return(status);
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
void hswi_json::process_hswi_messages(const std::string &rootkey,
                                      const rapidjson::Value &req_node,
                                      rapidjson::Value &rsp_node,
                                      rapidjson::Document::AllocatorType &rsp_at)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }
    if(req_node.GetType() == rapidjson::kArrayType)
    {
        rsp_node.SetArray(); // NOTE: don't forget this line
        for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
        {
            rapidjson::Value    rspval(rapidjson::kObjectType);

            // recursive call
            process_hswi_messages(rootkey,
                                  static_cast<const rapidjson::Value &>(*ita),
                                  rspval,
                                  rsp_at);
            rsp_node.PushBack(rspval, rsp_at);
        }
    }
    else
    {
        rsp_node.SetObject();    // NOTE: don't forget this line
        // read "uid"
        size_t  uid = -1; // invalid
        if(req_node.HasMember("header")){
            if(req_node["header"].HasMember("uid")) {
                uid = req_node["header"]["uid"].GetUint();
            }
        }

        rapidjson::Value rspheader(rapidjson::kObjectType);
        {
            rspheader.AddMember("type", rapidjson::Value("rsp").Move(), rsp_at);
            rspheader.AddMember("uid", rapidjson::Value(uid).Move(), rsp_at);
        }
        rsp_node.AddMember("header", rspheader, rsp_at);

        // find "body"
        //const rapidjson::Value* p1body = rapidjson::Pointer("/body").Get(req_node);
        if(req_node.HasMember("body")){
            rapidjson::Value rspbody(rapidjson::kObjectType);
            process_hswi_body(rootkey, req_node["body"], uid, rspbody, rsp_at);
            rsp_node.AddMember("body", rspbody, rsp_at);
        }
    }
    return;
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
void hswi_json::process_hswi_body(const std::string &rootkey,
                                  const rapidjson::Value& req_node,
                                  const size_t uid,
                                  rapidjson::Value &rsp_node,
                                  rapidjson::Document::AllocatorType &rsp_at)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }

    switch(req_node.GetType())
    {
        case rapidjson::kArrayType:
            {
                rsp_node.SetArray();
                for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
                {
                    rapidjson::Value    rspval(rapidjson::kObjectType);
                    // recursive call
                    process_hswi_body(rootkey, *ita, uid, rspval, rsp_at);
                    rsp_node.PushBack(rspval, rsp_at);
                }
                return;
            }
            break;

        case rapidjson::kObjectType:
            {
                // dive into the child node of the "body"
                for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild) {
                    std::string currkey(itochild->name.GetString());
                    std::string fullkey = rootkey + "/" + currkey;

                    rapidjson::Value    rspprimitive(rapidjson::kObjectType);

                    exec_prmtv(itochild->value, fullkey, uid, rspprimitive, rsp_at);

                    std::string rspstr = currkey.substr(0, currkey.find("_req")) + "_rsp";
                    rsp_node.AddMember(rapidjson::Value(rspstr.c_str(), rspstr.size(), rsp_at).Move(),
                                      rspprimitive,
                                      rsp_at);
                }

                return;
            }
            break;

        default:
            return;
    }

    return;
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
void hswi_json::process_hswi_primitive(const rapidjson::Value& req_node,
                                       const std::string& node_key,
                                       const size_t uid,
                                       rapidjson::Value &rsp_node,
                                       rapidjson::Document::AllocatorType &rsp_at,
                                       size_t item_idx,
                                       size_t num_item)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json) 
        << "node_key=" << node_key << ", nodetype=" << req_node.GetType() << std::endl;
    }

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType: {
            rsp_node.SetArray();
            size_t itemidx = 0;
            const size_t numitem = req_node.Size();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita, itemidx++)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                process_hswi_primitive(*ita, node_key, uid, rspval, rsp_at, itemidx, numitem);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            rsp_node.SetObject();
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild) {
                int instid;
                std::string childname = itochild->name.GetString();
                std::string objtype = this->split_instance_id(childname, instid);
                std::string fullkey = node_key + "/" + objtype;
#if defined(UNITTEST_TRACE)
                TRACE1(hswi_json)
                    << "fullkey=" << fullkey << ", instid=" << instid << std::endl;
#endif

                if(instid < 0) {    // NOTE: "all" returuns instid of -1
                    size_t numinst = NUM_CARRIER;

                    if(objtype == "tx_playback")    numinst = MAX_PLAYBACK;
                    else if(objtype == "section")   numinst = NUM_SECTION_PER_CARRIER;

                    for(size_t kk = 0; kk < numinst; kk++){
                        std::string objname = objtype + ":" + std::to_string(kk);
                        rapidjson::Value    rspobj(rapidjson::kObjectType);
                        // NOTE: take an object of string type as an emptynode
                        //      ex) take {"delete_obj_req": "tx_carrier_ltetdd:0"} 
                        //          as   {"delete_obj_req": {"tx_carrier_ltetdd:0": {}}} 
                        rapidjson::Value    emptynode(rapidjson::kObjectType);
                        //-------------------------------------------------
                        // exec_objid()
                        //-------------------------------------------------
                        if(this->exec_objid(emptynode, fullkey, kk, uid, rspobj, rsp_at, item_idx, num_item) == true) {
                            // NOTE: response only if true
                            rsp_node.AddMember(rapidjson::Value(objname.c_str(), objname.size(), rsp_at).Move(), rspobj, rsp_at);
                        }
                    }
                }
                else{
                    rapidjson::Value    rspobj(rapidjson::kObjectType);
                    if(this->exec_objid(itochild->value, fullkey, instid, uid, rspobj, rsp_at, item_idx, num_item) == true) {
                        rsp_node.AddMember(rapidjson::Value(childname.c_str(), childname.size(), rsp_at).Move(), rspobj, rsp_at);
                    }
                }
            }
            break;
        }

        case  rapidjson::kStringType: {
            rsp_node.SetObject();
            int instid;
            std::string childname = req_node.GetString();
            std::string objtype = this->split_instance_id(childname, instid);
            std::string fullkey = node_key + "/" + objtype;
#if defined(UNITTEST_TRACE)
            TRACE1(hswi_json)
                << "fullkey=" << fullkey << ", instid=" << instid << std::endl;
#endif

            if(instid < 0) {    // NOTE: "all" returuns instid of -1
                size_t numinst = NUM_CARRIER;

                if(objtype == "tx_playback")    numinst = MAX_PLAYBACK;
                else if(objtype == "section")   numinst = NUM_SECTION_PER_CARRIER;
                else                            numinst = NUM_CARRIER;

                for(size_t kk = 0; kk < numinst; kk++){
                    std::string objname = objtype + ":" + std::to_string(kk);
                    rapidjson::Value    rspobj(rapidjson::kObjectType);
                    // NOTE: take an object of string type as an emptynode
                    //      ex) take {"delete_obj_req": "tx_carrier_ltetdd:0"} 
                    //          as   {"delete_obj_req": {"tx_carrier_ltetdd:0": {}}} 
                    rapidjson::Value    emptynode(rapidjson::kObjectType);
                    if(this->exec_objid(emptynode, fullkey, kk, uid, rspobj, rsp_at, item_idx, num_item) == true) {
                        rsp_node.AddMember(rapidjson::Value(objname.c_str(), objname.size(), rsp_at).Move(), rspobj, rsp_at);
                    }
                }
            }
            else{
                rapidjson::Value    rspobj(rapidjson::kObjectType);
                // NOTE: take an object of string type as an emptynode
                //      ex) take {"delete_obj_req": "tx_carrier_ltetdd:0"} 
                //          as   {"delete_obj_req": {"tx_carrier_ltetdd:0": {}}} 
                rapidjson::Value    emptynode(rapidjson::kObjectType);
                this->exec_objid(emptynode, fullkey, instid, uid, rspobj, rsp_at, item_idx, num_item);
                rsp_node.AddMember(rapidjson::Value(childname.c_str(), childname.size(), rsp_at).Move(), rspobj, rsp_at);
            }
            break;
        }

        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default: {
            TRACE1(hswi_json)
                << node_key << "=" << "null" << std::endl;
        }
    }

    return;
}

#if defined(UNITTEST_TRACE)
void hswi_json::create_obj_req(const rapidjson::Value& req_node,
                               const std::string& key,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }

    switch(req_node.GetType())
    {
        case  rapidjson::kStringType: {
            int instid;
            std::string childname = req_node.GetString();
            std::string objtype = this->split_instance_id(childname, instid);
            std::string fullkey = key + "/" + objtype;
#if defined(UNITTEST_TRACE)
            TRACE1(hswi_json)
                << "fullkey=" << fullkey << std::endl;
#endif

            // flag to dive into subnode
            rapidjson::Value nullnode;
            this->exec_objid(nullnode, fullkey, instid, uid, rsp_node, rsp_at);
            break;
        }

        case  rapidjson::kObjectType: {
            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild)
            {
                int instid;
                std::string childname = itochild->name.GetString();
                std::string objtype = this->split_instance_id(childname, instid);
                std::string fullkey = key + "/" + objtype;
#if defined(UNITTEST_TRACE)
                TRACE1(hswi_json)
                    << "fullkey=" << fullkey << std::endl;
#endif

                // flag to dive into subnode
                this->exec_objid(itochild->value, fullkey, instid, uid, rsp_node, rsp_at);
            }
            break;
        }

        case  rapidjson::kArrayType: {
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                create_obj_req(*ita, key, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kFalseType: {
            TRACE0() << key << "=" << "false" << std::endl;
            break;
        }

        case  rapidjson::kTrueType: {
            TRACE0() << key << "=" << "true" << std::endl;
            break;
        }

        case  rapidjson::kNumberType: {
            TRACE0() << key << "=" << req_node.GetDouble() << std::endl;
            break;
        }

        case  rapidjson::kNullType: {
            TRACE0() << key << "=" << "null" << std::endl;
            break;
        }

        default:;
    }

    return;
}

bool hswi_json::create_obj_req__tx_carrier_ltetdd(const rapidjson::Value& req_node,
                                                  const std::string& key,
                                                  const int &inst_id,
                                                  const size_t uid,
                                                  rapidjson::Value &rsp_node,
                                                  rapidjson::Document::AllocatorType &rsp_at)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(hswi_json);
    }
    bool  haschild =  true;

    (void)(req_node);  // avoid of warning
    (void)(key);    // avoid of warning
    (void)(inst_id);    // avoid of warning
    (void)(uid);    // avoid of warning
    (void)(rsp_node); // avoid of warnig
    (void)(rsp_at); // avoid of warnig

    switch(req_node.GetType())
    {
        case  rapidjson::kObjectType: {
            if(req_node.HasMember("chan_bw")) {
                std::cout << "chan_bw=" << req_node["chan_bw"].GetFloat() << std::endl;
            }
            haschild = false;
            break;
        }

        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                create_obj_req__tx_carrier_ltetdd(*ita, key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            haschild = false;
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default: {
            haschild = true;
            break;
        }
    }

    return(haschild);  // true: keep going, false: stop parsing children
}
#endif

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
bool hswi_json::default_objid(const rapidjson::Value& req_node,
                              const std::string& node_key,
                              const int &inst_id,
                              const size_t uid,
                              rapidjson::Value &rsp_node,
                              rapidjson::Document::AllocatorType &rsp_at,
                              size_t item_idx,
                              size_t num_item)
{
    bool  haschild =  true;

    (void)(req_node);  // avoid of warning
    (void)(node_key);    // avoid of warning
    (void)(uid);    // avoid of warning
    (void)(rsp_node); // avoid of warnig
    (void)(rsp_at); // avoid of warnig

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType: {
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                default_objid(*ita, node_key, uid, inst_id, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
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
            std::cout << node_key << "=" << req_node.GetString() << std::endl;
            haschild = false;
            break;
        }

        case  rapidjson::kFalseType: {
            std::cout << node_key << "=" << "false" << std::endl;
            haschild = false;
            break;
        }
        case  rapidjson::kTrueType: {
            std::cout << node_key << "=" << "true" << std::endl;
            haschild = false;
            break;
        }
        case  rapidjson::kNumberType: {
            std::cout << node_key << "=" << req_node.GetDouble() << std::endl;
            haschild = false;
            break;
        }
        case  rapidjson::kNullType: {
            std::cout << node_key << "=" << "null" << std::endl;
            haschild = false;
            break;
        }
        default:;
    }

    return(haschild);  // true: keep going, false: stop parsing children
}

/*  call_flow:
 *  <process_hswi_messages>
 *      <process_hswi_body>
 *          <exec_prmtv>
 *              <process_hswi_primitive>
 *                  <exec_objid>
 *                      <default_objid>
 *                          <exec_param>
 *                              <default_param>
 *                                  <exec_param/>
 *                              </default_param>
 *                          </exec_param>
 *                      </default_objid>
 *                  </exec_objid>
 *              </process_hswi_primitive>
 *          </exec_prmtv>
 *      </process_hswi_body>
 *  </process_hswi_messages>
 */
bool hswi_json::default_param(const rapidjson::Value& req_node,
                                  const std::string& node_key,
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
#if 1
            std::cout << node_key << "=" << value << std::endl;
#endif
            rsp_node = rapidjson::Value(value.c_str(), value.size(), rsp_at).Move();
            break;
        }

        case  rapidjson::kNumberType: {
            double  value = req_node.GetDouble();
#if 1
            std::cout << node_key << "=" << value << std::endl;
#endif
            rsp_node = value;
            break;
        }

        case  rapidjson::kFalseType: {
#if 1
            std::cout << node_key << "=" << "false" << std::endl;
#endif
            rsp_node = false;
            break;
        }

        case  rapidjson::kTrueType: {
#if 1
            std::cout << node_key << "=" << "true" << std::endl;
#endif
            rsp_node = true;
            break;
        }

        case  rapidjson::kNullType: {
#if 1
            std::cout << node_key << "=" << "null" << std::endl;
#endif
            break;
        }

        default:;
    }

    return(haschild);  // true: keep going, false: stop parsing children
}

void hswi_json::add_member(const std::string &key,
                              const rapidjson::Value &val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(val, json_at);

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}


void hswi_json::add_member(const std::string &key,
                              const std::string &val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kStringType);
    copiedval = rapidjson::Value(val.c_str(), val.size(), json_at).Move();

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

void hswi_json::add_member(const std::string &key,
                              double val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kStringType);
    copiedval = val;

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

void hswi_json::add_member(const std::string &key,
                              int val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kStringType);
    copiedval = val;

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

void hswi_json::add_member(const std::string &key,
                              bool val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kStringType);
    copiedval = val;

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(), copiedval, json_at);
}

void hswi_json::add_member(const std::string &key,
                              std::vector<std::string> &val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kArrayType);

    for(size_t kk = 0; kk < val.size(); kk++){
        rapidjson::Value rjval;
        rjval.SetString(val[kk].c_str(), val[kk].size(), json_at);
        copiedval.PushBack(rjval, json_at);
    }
    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

void hswi_json::add_member(const std::string &key,
                              std::vector<int> &val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kArrayType);
    for(size_t kk = 0; kk < val.size(); kk++){
        copiedval.PushBack(val[kk], json_at);
    }

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

void hswi_json::add_member(const std::string &key,
                              std::vector<double> &val,
                              rapidjson::Value &json_node,
                              rapidjson::Document::AllocatorType &json_at)
{
    // NOTE: copy the val first, because the rapidjson::AddMember() cannot accept the const reference type.
    rapidjson::Value copiedval(rapidjson::kArrayType);
    for(size_t kk = 0; kk < val.size(); kk++){
        copiedval.PushBack(val[kk], json_at);
    }

    json_node.AddMember(rapidjson::Value(key.c_str(), key.size(), json_at).Move(),
                        copiedval,
                        json_at);
}

/*  vim:  set ts=8 sts=4 sw=4 et cin ff=unix ffs=dos,unix : */
