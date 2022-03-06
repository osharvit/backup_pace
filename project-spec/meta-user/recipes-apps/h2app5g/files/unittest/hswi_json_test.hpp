#ifndef _HSWI_JSON_TEST_HPP_
#define _HSWI_JSON_TEST_HPP_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getopt()
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <cppunit/extensions/HelperMacros.h>
#include "hswi_json.h"
#include "itc_queue.h"
#include "itc_msg.h"
#include "thread_base.h"

class hswi_json_test : public CppUnit::TestFixture
{
    public:
        void setUp();
        void tearDown();

        void test01();
        void test02();
        void test_HasMember();
        void test_split_instance_id();
        void test_delete_string();
        void test_delete_array_string();
        void test_create_object();
        void test_create_rx_object();
        void test_create_array_object();
        void test_gen_hswi_array();
        void test_gen_bottom_up();
        void test_gen_bottom_up2();
        void test_itc01();

    public:
        std::string hswi_patt_delete_tx;
        std::string hswi_patt_delete_rx;
        std::string hswi_patt_create_tx;
        std::string hswi_patt_create_rx;

        hswi_json   hswijson;

    CPPUNIT_TEST_SUITE(hswi_json_test);
    CPPUNIT_TEST(test01);
    CPPUNIT_TEST(test02);
#if 0
    CPPUNIT_TEST(test_HasMember);
#endif
    CPPUNIT_TEST(test_split_instance_id);
    CPPUNIT_TEST(test_create_array_object);
    CPPUNIT_TEST(test_gen_hswi_array);
    CPPUNIT_TEST(test_gen_bottom_up);
    CPPUNIT_TEST(test_gen_bottom_up2);
    CPPUNIT_TEST(test_create_object);
    CPPUNIT_TEST(test_delete_string);
    CPPUNIT_TEST(test_delete_array_string);
    CPPUNIT_TEST(test_create_rx_object);
    CPPUNIT_TEST_SUITE_END();

};

inline
void hswi_json_test::setUp()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    this->hswi_patt_delete_tx = "{ \"header\": { \"type\": \"req\", \"uid\": 126 }, \"body\": { \"delete_obj_req\": \"tx_carrier_ltetdd:0\" } }";
    this->hswi_patt_delete_rx = "{ \"header\": { \"type\": \"req\", \"uid\": 127 }, \"body\": { \"delete_obj_req\": \"rx_carrier_ltetdd:0\" } }";

    this->hswi_patt_create_tx = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"create_obj_req\": { \"tx_carrier_ltetdd:0\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } } } } ";
    this->hswi_patt_create_rx = "{ \"header\": { \"type\": \"req\", \"uid\": 129 }, \"body\": { \"create_obj_req\": { \"rx_carrier_ltetdd:0\": { \"chan_bw\": 20, \"rx_freq\": 3610.4, \"rx_ant_port\": [ \"rx_ant_port:0\", \"rx_ant_port:1\" ], \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } } } }";

    std::cout << std::endl;
}

inline
void hswi_json_test::tearDown()
{
}

inline
void hswi_json_test::test01()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    std::vector<std::string> vsdu;
    std::string residual =
        hswi_patt_delete_tx.substr(hswi_patt_delete_tx.size()>>1);   // second-half
    std::string in =
        hswi_patt_create_tx +
        hswi_patt_create_rx +
        hswi_patt_delete_rx.substr(0, hswi_patt_delete_rx.size()>>1); // first-half

    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 2);
} // end of void hswi_json_test::test01()

inline
void hswi_json_test::test02()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    std::vector<std::string> vsdu;
    std::string residual =
        hswi_patt_create_tx.substr(hswi_patt_create_tx.size()>>1);   // second-half
    std::string in =
        "[" +
        hswi_patt_delete_tx +
        "," +
        hswi_patt_delete_rx +
        "]" +
        hswi_patt_create_rx.substr(0, hswi_patt_create_rx.size()>>1); // first-half

    std::cerr << residual + in << std::endl;
    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 2);
} // end of void hswi_json_test::test02()

inline
void hswi_json_test::test_HasMember()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    std::vector<std::string> vsdu;
    std::string residual =
        hswi_patt_create_tx.substr(hswi_patt_create_tx.size()>>1);   // second-half
    std::string in =
        "[" +
        hswi_patt_delete_tx +
        "," +
        hswi_patt_delete_rx +
        "]" +
        hswi_patt_create_rx.substr(0, hswi_patt_create_rx.size()>>1); // first-half

    std::cerr << residual + in << std::endl;
    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();
        size_t  uid = -1;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            if(reqdoc.HasMember("header")){
                CPPUNIT_ASSERT(reqdoc["header"].HasMember("uid"));
                uid = reqdoc["header"]["uid"].GetUint();
                std::cout << "uid=" << uid << std::endl;
            }
            else{
                CPPUNIT_FAIL("cannot find \"header\".");
            }

            if(reqdoc.HasMember("body")){
                hswijson.process_hswi_body(reqdoc["body"], uid, rspdoc, genat);
            }
        }
    }
} // end of void hswi_json_test::test_HasMember()

inline
void hswi_json_test::test_split_instance_id()
{
    {
        std::string objid = "tx_carrier_ltetdd:1";

        int inst_id = 0;
        std::string objtype = hswijson.split_instance_id(objid, inst_id);

        std::cout << "objtype=" << objtype << ", inst_id=" << inst_id << std::endl;

        CPPUNIT_ASSERT(objtype == "tx_carrier_ltetdd");
        CPPUNIT_ASSERT(inst_id == 1);
    }

    {
        std::string objid = "tx_carrier_ltetdd:All";

        int inst_id = 0;
        std::string objtype = hswijson.split_instance_id(objid, inst_id);

        std::cout << "objtype=" << objtype << ", inst_id=" << inst_id << std::endl;

        CPPUNIT_ASSERT(objtype == "tx_carrier_ltetdd");
        CPPUNIT_ASSERT(inst_id == -1);
    }

    {
        std::string objid = "tx_carrier_ltetdd:";

        int inst_id = 0;
        std::string objtype = hswijson.split_instance_id(objid, inst_id);

        std::cout << "objtype=" << objtype << ", inst_id=" << inst_id << std::endl;

        CPPUNIT_ASSERT(objtype == "tx_carrier_ltetdd");
        CPPUNIT_ASSERT(inst_id == 0);
    }

    {
        std::string objid = "tx_carrier_ltetdd";

        int inst_id = 0;
        std::string objtype = hswijson.split_instance_id(objid, inst_id);

        std::cout << "objtype=" << objtype << ", inst_id=" << inst_id << std::endl;

        CPPUNIT_ASSERT(objtype == "tx_carrier_ltetdd");
        CPPUNIT_ASSERT(inst_id == -1);
    }
} // end of void hswi_json_test::test_split_instance_id()

inline
void hswi_json_test::test_delete_string()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"delete_obj_req\":  \"tx_carrier_ltetdd:0\" } } ";

    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 1);
}

void hswi_json_test::test_delete_array_string()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"delete_obj_req\": [\"tx_carrier_ltetdd:0\", \"rx_carrier_ltetdd:1\"] } } ";

#if defined(CALL_TRACE)
    TRACE0();
#endif

    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 1);
}



void hswi_json_test::test_create_rx_object()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{\"header\": {\"type\": \"req\", \"uid\": 15}, \"body\": {\"create_obj_req\": {\"rx_carrier_ltetdd:0\": {\"chan_bw\": 20, \"rx_freq\": 3610.4, \"rx_ant_port\": [\"rx_ant_port:0\", \"rx_ant_port:1\"], \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\"}}}}";
    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 1);
}

void hswi_json_test::test_create_object()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"create_obj_req\": { \"tx_carrier_ltetdd:0\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } } } } ";

    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 1);
}

void hswi_json_test::test_create_array_object()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"create_obj_req\": [{ \"tx_carrier_ltetdd:0\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } }, { \"rx_carrier_ltetdd:1\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } }] } } ";

    hswijson.segment_reassemble(vsdu, residual, in);

    for(std::vector<std::string>::iterator it = vsdu.begin(); it != vsdu.end(); ++it){
        rapidjson::Document reqdoc;
        rapidjson::Document rspdoc;
        rapidjson::Document::AllocatorType& genat = rspdoc.GetAllocator();

        std::cerr << *it << std::endl;
        std::cerr << std::endl;

        bool parse_result = hswijson.load(reqdoc, *it);
        if(parse_result == true){
            hswijson.process_hswi_messages(reqdoc, rspdoc, genat);
        }

        std::cout << "rspdoc=" << std::endl; 
        std::cout << rapidjson_wrapper::str_json(rspdoc) << std::endl; 
    }

    CPPUNIT_ASSERT(vsdu.size() == 1);
}

void hswi_json_test::test_gen_hswi_array()
{
#if defined(CALL_TRACE)
    TRACE0();
#endif

    rapidjson::Document rspdoc(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& rspat = rspdoc.GetAllocator();

    rspdoc.SetArray();

    rapidjson::Document rspdoc1;
    {
        rapidjson::Document::AllocatorType& at1 = rspdoc1.GetAllocator();

        rapidjson::Value    rspdocheader(rapidjson::kObjectType);
        rapidjson::Value    rspdocbody(rapidjson::kObjectType);

        rspdoc1.SetObject();
        rspdoc1.AddMember("header", rspdocheader, at1);
        {
            rapidjson::Value    uid(123u);
            rapidjson::Value    type("req");

            rspdoc1["header"].AddMember("type", type, at1);
            rspdoc1["header"].AddMember("uid", uid, at1);
        }

        rspdoc1.AddMember("body", rspdocbody, at1);
        {
            rapidjson::Value    obj(rapidjson::kObjectType);
            rspdoc1["body"].AddMember("tx_carrier_ltetdd", obj, at1);

            {
                rapidjson::Value    rspdocbw(20);
                rspdoc1["body"]["tx_carrier_ltetdd"].AddMember("bw", rspdocbw, at1);
            }
        }

        rapidjson_wrapper::dump_json(rspdoc1);

        rspdoc.PushBack(rspdoc1, rspat);
    }

    {
        rapidjson::Document rspdoc2;
        rapidjson::Document::AllocatorType& at2 = rspdoc2.GetAllocator();

        rapidjson::Value    rspdocheader(rapidjson::kObjectType);
        rapidjson::Value    rspdocbody(rapidjson::kObjectType);

        rspdoc2.SetObject();
        rspdoc2.AddMember("header", rspdocheader, at2);
        {
            rapidjson::Value    uid(123u);
            rapidjson::Value    type("req");

            rspdoc2["header"].AddMember("type", type, at2);
            rspdoc2["header"].AddMember("uid", uid, at2);
        }

        rspdoc2.AddMember("body", rspdocbody, at2);
        {
            rapidjson::Value    obj(rapidjson::kObjectType);
            rspdoc2["body"].AddMember("tx_carrier_ltetdd", obj, at2);
        }

        rspdoc.PushBack(rspdoc2, rspat);
    }


    std::cout << "rspdoc=" << std::endl;
    std::cout << rapidjson_wrapper::str_json(rspdoc);
    std::cout << std::endl;
}

void hswi_json_test::test_gen_bottom_up()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"create_obj_req\": { \"tx_carrier_ltetdd:0\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } } } } ";

    hswijson.segment_reassemble(vsdu, residual, in);

    // NOTE: parse in top-down, and AddMember in bottom-up
    rapidjson::Document rspdoc;
    rapidjson::Document::AllocatorType& rspat = rspdoc.GetAllocator();

    rspdoc.SetObject();
    {
        rapidjson::Value rspheader(rapidjson::kObjectType);
        {
            rspheader.AddMember("type", rapidjson::Value("rsp").Move(), rspat);
            rspheader.AddMember("uid", rapidjson::Value(2).Move(), rspat);
        }

        rspdoc.AddMember("header", rspheader, rspat);
    }
    {
        rapidjson::Value rspbody(rapidjson::kObjectType);
        {
            rapidjson::Value rspprimitive(rapidjson::kObjectType);
            {
                rapidjson::Value rspobjid(rapidjson::kObjectType);
                {
                    rspobjid.AddMember("tx_max_pwr", 26.0, rspat);
                    rspobjid.AddMember("chan_bw", rapidjson::Value(2).Move(), rspat);
                    rspobjid.AddMember("tx_freq", 3610.4, rspat);
                }
                rspprimitive.AddMember("tx_carrier_ltetdd", rspobjid, rspat);
            }
            rspbody.AddMember("create_obj_rsp", rspprimitive, rspat);
        }
        rspdoc.AddMember("body", rspbody, rspat);
    }

    TRACE0() << rapidjson_wrapper::str_json(rspdoc) << std::endl;


    CPPUNIT_ASSERT(vsdu.size() == 1);
}

void hswi_json_test::test_gen_bottom_up2()
{
    std::vector<std::string> vsdu;
    std::string residual;
    std::string in = "{ \"header\": { \"type\": \"req\", \"uid\": 128 }, \"body\": { \"create_obj_req\": { \"tx_carrier_ltetdd:0\": { \"tx_max_pwr\": 26.0, \"chan_bw\": 20, \"tx_freq\": 3610.4, \"tx_ant_port\": { \"tx_ant_port:0\": { \"enable\": true }, \"tx_ant_port:1\": { \"enable\": true } }, \"tdd_ul_dl_config\": 2, \"tdd_ssf_config\": 3, \"cp_length\": \"normal\" } } } } ";

    hswijson.segment_reassemble(vsdu, residual, in);

    // NOTE: parse in top-down, and AddMember in bottom-up
    rapidjson::Document rspdoc;
    rapidjson::Document::AllocatorType& rspat = rspdoc.GetAllocator();

    rspdoc.SetObject();
    {
        rapidjson::Value rspheader(rapidjson::kObjectType);
        {
            rspheader.AddMember("type", rapidjson::Value("rsp").Move(), rspat);
            rspheader.AddMember("uid", rapidjson::Value(2).Move(), rspat);
        }
        rspdoc.AddMember("header", rspheader, rspat);
    }
    {
        rapidjson::Value rspbody(rapidjson::kObjectType);
        {
            rapidjson::Value rspprimitive(rapidjson::kObjectType);
            {
                rapidjson::Value rspobjid(rapidjson::kObjectType);
                {
                    rspobjid.AddMember("tx_max_pwr", 26.0, rspat);
                    rspobjid.AddMember("chan_bw", rapidjson::Value(2).Move(), rspat);
                    rspobjid.AddMember("tx_freq", 3610.4, rspat);
                }
                rspprimitive.AddMember("tx_carrier_ltetdd:0", rspobjid, rspat);
            }
            {
                rapidjson::Value rspobjid(rapidjson::kObjectType);
                {
                    rspobjid.AddMember("tx_max_pwr", 26.0, rspat);
                    rspobjid.AddMember("chan_bw", rapidjson::Value(2).Move(), rspat);
                    rspobjid.AddMember("tx_freq", 3610.4, rspat);
                }
                rspprimitive.AddMember("tx_carrier_ltetdd:1", rspobjid, rspat);
            }
            rspbody.AddMember("create_obj_rsp", rspprimitive, rspat);
        }
        rspdoc.AddMember("body", rspbody, rspat);
    }

    TRACE0() << rapidjson_wrapper::str_json(rspdoc) << std::endl;


    CPPUNIT_ASSERT(vsdu.size() == 1);
}

#endif // _HSWI_JSON_TEST_HPP_
