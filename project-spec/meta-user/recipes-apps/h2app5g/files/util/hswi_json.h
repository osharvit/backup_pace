#ifndef __HSWI_JSON_H__
#define __HSWI_JSON_H__

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
#include "rapidjson_wrapper.h"

class hswi_json: public rapidjson_wrapper
{
    public:
        typedef void (hswi_json::*PRMTV_FUNC)(const rapidjson::Value &req_node,
                                              const std::string &key,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at);
        typedef bool (hswi_json::*OBJID_FUNC)(const rapidjson::Value &req_node,
                                              const std::string &key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx,
                                              size_t num_item);
        typedef bool (hswi_json::*PARAM_FUNC)(const rapidjson::Value &req_node,
                                              const std::string &key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at);
        const size_t    BUF_SZ = 1000;

    public:
        hswi_json();
        virtual ~hswi_json();

        // load instance json Document from string
        bool  load(rapidjson::Document &doc, std::string& str);
        // load instance json Document from ifstream
        bool  load(rapidjson::Document &doc, std::ifstream& ifs);
        // load instance json Document from stringstream
        bool  load(rapidjson::Document &doc, std::stringstream& ss);

        // dump instance json Document to std::cout
        void  dump(rapidjson::Document &json);
        // dump instance json Document to stringstream
        void  dump(rapidjson::Document &json, std::stringstream &ss);
        // dump instance json Document to string
        void  dump(rapidjson::Document &json, std::string& str);
        // dump instance json Document to ofstream
        void  dump(rapidjson::Document &json, std::ofstream& ofs);

        // dump instance json Value to std::cout
        void  dump(const rapidjson::Value &json);
        // dump instance json Value to stringstream
        void  dump(const rapidjson::Value &json, std::stringstream &ss);
        // dump instance json Value to string
        void  dump(const rapidjson::Value &json, std::string& str);
        // dump instance json Value to ofstream
        void  dump(const rapidjson::Value &json, std::ofstream& ofs);

        size_t segment_reassemble(std::vector<std::string> &vsdu, std::string &residual, std::string &in);

        void  parse_recursively(const rapidjson::Value& req_node,
                                const std::string &key);

        void  process_hswi_messages(const std::string &rootkey,
                                    const rapidjson::Value &reqdoc,
                                    rapidjson::Value &rsp_node,
                                    rapidjson::Document::AllocatorType &rsp_at);
        void  process_hswi_body(const std::string &rootkey,
                                const rapidjson::Value& req_node,
                                const size_t uid,
                                rapidjson::Value &rsp_node,
                                rapidjson::Document::AllocatorType &rsp_at);
        void  process_hswi_primitive(const rapidjson::Value& req_node,
                                     const std::string& node_key,
                                     const size_t uid,
                                     rapidjson::Value &rsp_node,
                                     rapidjson::Document::AllocatorType &rsp_at,
                                     size_t item_idx = 0,
                                     size_t num_item = 1);

        std::string str_tolower(std::string s);
        std::string str_strip(std::string in);
        std::string split_instance_id(const std::string &obj_id, int &instance_id);


        // NOTE: this line invoke "error: use of deleted function", but I don't know the reason.
#if 0
        rapidjson::Document doc;
#endif

    protected:
        size_t find_pair_recursively(std::vector<std::complex<size_t> > &vpair,
                                     std::vector<size_t> &vbgn,
                                     std::vector<size_t> &vend,
                                     size_t bgnpos = std::string::npos,
                                     size_t endpos = std::string::npos);

#if defined(UNITTEST_TRACE)
        void create_obj_req(const rapidjson::Value& req_node,
                            const std::string& node_key,
                            const size_t uid,
                            rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at);

        bool create_obj_req__rx_carrier_ltetdd(const rapidjson::Value& req_node,
                                               const std::string& node_key,
                                               const int &inst_id,
                                               const size_t uid,
                                               rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at);
        bool create_obj_req__tx_carrier_ltetdd(const rapidjson::Value& req_node,
                                               const std::string& node_key,
                                               const int &inst_id,
                                               const size_t uid,
                                               rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at);
#endif
        bool default_objid(const rapidjson::Value& req_node,
                           const std::string& node_key,
                           const int &inst_id,
                           const size_t uid,
                           rapidjson::Value &rsp_node, rapidjson::Document::AllocatorType &rsp_at,
                           size_t item_idx = 0,
                           size_t num_item = 1);
        bool default_param(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at);
        void exec_prmtv(const rapidjson::Value& req_node,
                        const std::string& node_key,
                        const size_t uid,
                        rapidjson::Value &rsp_node,
                        rapidjson::Document::AllocatorType &rsp_at);
        bool exec_objid(const rapidjson::Value& req_node,
                        const std::string& node_key,
                        const int &inst_id,
                        const size_t uid,
                        rapidjson::Value &rsp_node,
                        rapidjson::Document::AllocatorType &rsp_at,
                        size_t item_idx = 0,
                        size_t num_item = 1);
        bool exec_param(const rapidjson::Value& req_node,
                        const std::string& node_key,
                        const int &inst_id,
                        const size_t uid,
                        rapidjson::Value &rsp_node,
                        rapidjson::Document::AllocatorType &rsp_at);

        void add_member(const std::string &key,
                           const rapidjson::Value &val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           const std::string &val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           double val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           int val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           std::vector<std::string> &val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           std::vector<int> &val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           bool val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);
        void add_member(const std::string &key,
                           std::vector<double> &val,
                           rapidjson::Value &json_node,
                           rapidjson::Document::AllocatorType &json_at);

    protected:
        std::map<std::string, hswi_json::PRMTV_FUNC> prmtv_map;
        std::map<std::string, hswi_json::OBJID_FUNC> objid_map;
        std::map<std::string, hswi_json::PARAM_FUNC> param_map;

};

#endif  // __HSWI_JSON_H__
/*  vim:  set ts=8 sts=4 sw=4 et cin ff=unix ffs=dos,unix : */
