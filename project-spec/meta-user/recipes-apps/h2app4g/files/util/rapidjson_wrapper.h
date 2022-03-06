#ifndef __RAPIDJSON_WRAPPER_HP__
#define __RAPIDJSON_WRAPPER_HP__

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
#include <algorithm>

#include "document.h"
#include "stringbuffer.h"
#include "filereadstream.h"
#include "filewritestream.h"
#include "writer.h"
#include "istreamwrapper.h"   // rapidjson::IStreamWrapper
#include "ostreamwrapper.h"   // rapidjson::OStreamWrapper
#include "pointer.h"          // rapidjson::Pointer


class rapidjson_wrapper
{
    public:
        // load the argument json doc from string
        static bool  load_json(rapidjson::Document &d, std::string& str);
        // load the argument json doc from ifstream
        static bool  load_json(rapidjson::Document &d, std::ifstream& ifs);
        // load the argument json doc from stringstream
        static bool  load_json(rapidjson::Document &d, std::stringstream& ss);

        // dump the argument json doc to std::cout
        static std::string  str_json(rapidjson::Document &d);

        // dump the argument json Document to std::cout
        static void  dump_json(rapidjson::Document &json);
        // dump the argument json Document to stringstream
        static void  dump_json(rapidjson::Document &json, std::stringstream &ss);
        // dump the argument json Document to string
        static void  dump_json(rapidjson::Document &json, std::string& str);
        // dump the argument json Document to ofstream
        static void  dump_json(rapidjson::Document &json, std::ofstream& ofs);

        // dump the argument json Value to std::cout
        static void  dump_json(const rapidjson::Value &json);
        // dump the argument json Value to stringstream
        static void  dump_json(const rapidjson::Value &json, std::stringstream &ss);
        // dump the argument json Value to string
        static void  dump_json(const rapidjson::Value &json, std::string& str);
        // dump the argument json Value to ofstream
        static void  dump_json(const rapidjson::Value &json, std::ofstream& ofs);

        // parse any formate of json
        static void  parse_json_recursively(const rapidjson::Value& value,
                                          const std::string &sKey
                                          //std::string &sParentKeyNode,
                                          //callback,
                                          //callbackArgs = None,
                                          //recursiveArgs = None,
                                          //depth=0,
                                          //bLowerCase=True,
                                          //nSectionIdx=0
                                         );
#if 0
        static const char* kTypeNames[];
#endif
};

#if 0
const char* rapidjson_wrapper::kTypeNames[] =
{
    "rapidjson::kNullType",   //  rapidjson::kNullType = 0,      //!< null
    "rapidjson::kFalseType",  //  rapidjson::kFalseType = 1,     //!< false
    "rapidjson::kTrueType",   //  rapidjson::kTrueType = 2,      //!< true
    "rapidjson::kObjectType", //  rapidjson::kObjectType = 3,    //!< object
    "rapidjson::kArrayType",  //  rapidjson::kArrayType = 4,     //!< array 
    "rapidjson::kStringType", //  rapidjson::kStringType = 5,    //!< string
    "rapidjson::kNumberType"  //  rapidjson::kNumberType = 6     //!< number
};
#endif

inline
bool  rapidjson_wrapper::load_json(rapidjson::Document &d, std::string& str)
{
    rapidjson::ParseResult ok = d.Parse(str.c_str());

    return(ok? true: false);
}

inline
bool  rapidjson_wrapper::load_json(rapidjson::Document &d, std::ifstream& ifs)
{ // IStream Wrapper with std::ifstream
    rapidjson::IStreamWrapper isw(ifs);

    rapidjson::ParseResult ok = d.ParseStream(isw);

    return(ok? true: false);
}

inline
bool  rapidjson_wrapper::load_json(rapidjson::Document &d, std::stringstream& ss)
{ // IStream Wrapper with std::stringstream
    rapidjson::IStreamWrapper isw(ss);

    rapidjson::ParseResult ok = d.ParseStream(isw);

    return(ok? true: false);
}

inline
std::string  rapidjson_wrapper::str_json(rapidjson::Document &d)
{
    std::stringstream ss;
    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    d.Accept(writer);

    return(ss.str());
}

inline
void  rapidjson_wrapper::dump_json(rapidjson::Document &json)
{ // OStreamWrapper with std::stringstream
    std::stringstream ss;

    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);

    std::cout << ss.str() << std::endl;
}

inline
void  rapidjson_wrapper::dump_json(rapidjson::Document &json, std::stringstream& ss)
{ // OStreamWrapper with std::stringstream
    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);
}

inline
void  rapidjson_wrapper::dump_json(rapidjson::Document &json, std::string& str)
{ // OStreamWrapper with std::ofstream
    std::stringstream ss;
    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);

    str = ss.str();
}

inline
void  rapidjson_wrapper::dump_json(rapidjson::Document &json, std::ofstream& ofs)
{ // OStreamWrapper with std::ofstream
    rapidjson::OStreamWrapper osw(ofs);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);
}

inline
void  rapidjson_wrapper::dump_json(const rapidjson::Value &json)
{ // OStreamWrapper with std::stringstream
    std::stringstream ss;

    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);

    std::cout << ss.str() << std::endl;
}

inline
void  rapidjson_wrapper::dump_json(const rapidjson::Value &json, std::stringstream& ss)
{ // OStreamWrapper with std::stringstream
    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);
}

inline
void  rapidjson_wrapper::dump_json(const rapidjson::Value &json, std::string& str)
{ // OStreamWrapper with std::ofstream
    std::stringstream ss;
    rapidjson::OStreamWrapper osw(ss);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);

    str = ss.str();
}

inline
void  rapidjson_wrapper::dump_json(const rapidjson::Value &json, std::ofstream& ofs)
{ // OStreamWrapper with std::ofstream
    rapidjson::OStreamWrapper osw(ofs);

    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);
}

inline
void  rapidjson_wrapper::parse_json_recursively(const rapidjson::Value& jNode,
                                        const std::string &sKey
                                        //std::string &sParentKeyNode
                                        //callback
                                        //callbackArgs = None
                                        //recursiveArgs = None
                                        //depth=0
                                        //bLowerCase=True
                                        //nSectionIdx=0
                                       )
{
    switch(jNode.GetType())
    {
        case  rapidjson::kObjectType:
            {
                for(rapidjson::Value::ConstMemberIterator ito = jNode.MemberBegin(); ito != jNode.MemberEnd(); ++ito)
                {
                    std::string childkey = sKey + "/" + ito->name.GetString();

                    parse_json_recursively(ito->value, childkey);
                }
            }
            break;

        case  rapidjson::kArrayType:
            {
                for(rapidjson::Value::ConstValueIterator ita = jNode.Begin(); ita != jNode.End(); ++ita)
                {
                    parse_json_recursively(*ita, sKey);
                }
            }
            break;

        case  rapidjson::kFalseType:
            {
                std::cout << sKey << "=" << "false" << std::endl;
            }
            break;

        case  rapidjson::kTrueType:
            {
                std::cout << sKey << "=" << "true" << std::endl;
            }
            break;

        case  rapidjson::kStringType:
            {
                std::cout << sKey << "=" << jNode.GetString() << std::endl;
            }
            break;

        case  rapidjson::kNumberType:
            {
                std::cout << sKey << "=" << jNode.GetDouble() << std::endl;
            }
            break;

        case  rapidjson::kNullType:
            {
                std::cout << sKey << "=" << "null" << std::endl;
            }
            break;

        default:;
    }

    return;
}
#endif  // __RAPIDJSON_WRAPPER_H__
