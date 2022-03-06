#ifndef _ITC_QUEUE_TEST_HPP_
#define _ITC_QUEUE_TEST_HPP_

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
#include "itc_queue.h"
#include "itc_msg.h"
#include "thread_base.h"

class itc_queue_test : public CppUnit::TestFixture
{
    public:
        void setUp();
        void tearDown();

        void test_itc01();

    public:

    CPPUNIT_TEST_SUITE(itc_queue_test);
    CPPUNIT_TEST(test_itc01);
    CPPUNIT_TEST_SUITE_END();

};

inline
void itc_queue_test::setUp()
{
}

inline
void itc_queue_test::tearDown()
{
}

inline
void itc_queue_test::test_itc01()
{
    TRACE0();

#if 0
    itc_queue       p1itc_q[NUM_THREAD];
    itc_queue::elem p1itcelem[MAX_ITC_ELEM];
    itc_msg         p1itcmsg[MAX_ITC_ELEM];
    itc_queue       pool_q(p1itcelem, (void *)p1itcmsg, MAX_ITC_ELEM, sizeof(itc_msg));
#endif

    thread_base thread_a("a", THREAD_HSWI_CONN);
    thread_base thread_b("b", THREAD_RE);

    thread_a.send_itc_message(THREAD_HSWI_CONN, THREAD_RE, ITC_PAYLOAD_FROM_HSWI_ID, 0, NULL);
    thread_a.dump_itc_q();
    thread_a.send_itc_message(THREAD_HSWI_CONN, THREAD_RE, ITC_PAYLOAD_FROM_HSWI_ID, 0, NULL);
    thread_a.dump_itc_q();
    thread_a.send_itc_message(THREAD_HSWI_CONN, THREAD_RE, ITC_PAYLOAD_FROM_HSWI_ID, 0, NULL);
    thread_a.dump_itc_q();
};

#endif // _ITC_QUEUE_TEST_HPP_
