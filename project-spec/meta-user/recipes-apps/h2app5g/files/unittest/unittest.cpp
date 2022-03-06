#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
//#include "hswi_json_test.hpp"
//#include "itc_queue_test.hpp"
#include "dmadesc_list_test.hpp"

// registers the fixture into the 'registry'
//CPPUNIT_TEST_SUITE_REGISTRATION(itc_queue_test);
//CPPUNIT_TEST_SUITE_REGISTRATION(hswi_json_test);
CPPUNIT_TEST_SUITE_REGISTRATION(dmadesc_list_test);

// global variable for debug
uint32_t g_debug_mask = 0;

int main(int argc, char* argv[])
{
    // get the top level suite from the registry
    CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

    // adds the test to list of test to run
    CppUnit::TextUi::TestRunner runner;
    runner.addTest(suite);

    // change the default outputter to a compiler error format outputter
    runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr));

    // run th test
    bool wasSuccessful = runner.run();

    // return error code 1 if the one of test failed
    return wasSuccessful? 0: 1;
}
