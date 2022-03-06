#ifndef _DMADESC_LIST_TEST_HPP_
#define _DMADESC_LIST_TEST_HPP_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getopt()
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <iostream>
#include <list>
#include <string>
#include <cctype>
#include <cppunit/extensions/HelperMacros.h>
#include "axidma_sg_hal.h"

class dmadesc_list_test: public CppUnit::TestFixture
{
    public:
        void setUp();
        void tearDown();

        void test_sort_list();

    public:

    CPPUNIT_TEST_SUITE(dmadesc_list_test);
    CPPUNIT_TEST(test_sort_list);
    CPPUNIT_TEST_SUITE_END();

};

inline
void dmadesc_list_test::test_sort_list()
{
  std::list<axidma_sg_hal::axidmadesc> mylist;
  std::list<axidma_sg_hal::axidmadesc>::iterator it;

  axidma_sg_hal::axidmadesc   desc;

  desc.APP2 = 10;
  mylist.push_back (desc);

  desc.APP2 = 2;
  mylist.push_back (desc);

  desc.APP2 = 5;
  mylist.push_back (desc);

  //mylist.sort();

  std::cout << "mylist contains:";
  for (it=mylist.begin(); it!=mylist.end(); ++it)
    std::cout << ' ' << it->APP2;
  std::cout << '\n';

  mylist.sort(axidma_sg_hal::axidmadesc::compare);

  std::cout << "mylist contains:";
  for (it=mylist.begin(); it!=mylist.end(); ++it)
    std::cout << ' ' << it->APP2;
  std::cout << '\n';
}

#endif  // _DMADESC_LIST_TEST_HPP_
