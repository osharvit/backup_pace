#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include "time_thread.h"

time_thread *time_thread::sp1instance = NULL;
#if defined(USE_SIGACTION_TIMER)
struct sigaction    time_thread::sa;
timer_t time_thread::timer_id;
#endif


time_thread::time_thread(const std::string &name, size_t id):
    thread_base(name, id)
{
#if defined(USE_SIGACTION_TIMER)
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = timer_wrapper;
    sigaction(SIGALRM, &sa, NULL);
    if(timer_create(CLOCK_MONOTONIC, NULL, &timer_id) != 0)
    {
        std::stringstream sstm;
        TRACE3(sstrm, time_thread, thread_id) << "error in timer_create()" << std::endl;
        perror(sstm.str().c_str());
    }
#endif
} // end of time_thread()

time_thread::~time_thread()
{
#if defined(USE_SIGACTION_TIMER)
    if(timer_delete(timer_id) != 0)
    {
        std::stringstream sstm;
        TRACE3(sstrm, time_thread, thread_id) << "error in timer_delete()" << std::endl;
        perror(sstm.str().c_str());
    }
#endif
}

time_thread *time_thread::singleton(const std::string &name, size_t id)
{
    if(time_thread::sp1instance == NULL) {
        time_thread::sp1instance = new time_thread(name, id);
    }

    return(time_thread::sp1instance);
} // end of time_thread *time_thread::singleton()

size_t  time_thread::get_interval_ms() const
{
    return(interval_in_msec);
} // end of get_interval_ms()

size_t  time_thread::get_tick() const
{
    return(time_tick);
} // end of get_tick()

void    time_thread::inc_tick()
{
    time_tick++;
} // end of time_thread::inc_tick()

void time_thread::start_time_thread()
{
#if defined(USE_SIGACTION_TIMER)
    struct itimerspec   itspec;

    itspec.it_value.tv_sec      =  interval_in_msec / 1000;               // sec
    itspec.it_value.tv_nsec     = (interval_in_msec % 1000) * 1000000;    // nsec
    itspec.it_interval.tv_sec   =  interval_in_msec / 1000;               //sec
    itspec.it_interval.tv_nsec  = (interval_in_msec % 1000) * 1000000;    // nsec

    if(timer_settime(timer_id, 0, &itspec, NULL) != 0)
    {
        std::stringstream sstm;
        TRACE3(sstrm, time_thread, thread_id) << "error in timer_settime()" << std::endl;
        perror(sstm.str().c_str());
    }
#else
    // timethread part
    pthread_attr_init(&this->threadattr);
    //pthread_attr_setdetachstate(&this->threadattr, PTHREAD_CREATE_JOINABLE);
    sigemptyset(&this->sigmask);   // clear signals
    sigaddset(&this->sigmask, EVENT_ITC); 
    sigaddset(&this->sigmask, EVENT_SOCKET);
    sigaddset(&this->sigmask, SIGPIPE);    // NOTE: double-protection against crash due to SIGPIPE
    if(pthread_sigmask(SIG_BLOCK, &this->sigmask, NULL) != 0){
        perror("pthread_sigmask()");
    }

    pthread_create(&this->thread, &this->threadattr, time_thread::start_wrapper, (void *)this);
    pthread_detach(this->thread); // mark the thread to return its reources automatically in terminating
#endif
} // end of time_thread::start_time_thread()

#if defined(USE_SIGACTION_TIMER)
void  time_thread::timer_wrapper(int arg)
{
    if(time_thread::sp1instance != NULL){
        time_thread::sp1instance->inc_tick();

        for(size_t kk = 0; kk < time_thread::sp1instance->timed_list.get_count(); kk++) {
            if(time_thread::sp1instance->timed_list.is_time(time_thread::sp1instance->get_tick(), kk)) {
                time_thread::sp1instance->timed_list.exec_item(kk);
            }
        }
    }
} // end of void  time_thread::timer_wrapper(...)
#else
void *time_thread::start_wrapper(void *p1arg)
{
    time_thread *p1thread = (time_thread *)p1arg;

    p1thread->run_time_thread();

    return((void *)p1thread);
} // end of void* time_thread::start_wrapper(void* p1arg)
#endif


void time_thread::run_time_thread()
{
    struct timespec req;

    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }

    //  recent versions of GNU libc and the Linux kernel support the following clocks:
    // CLOCK_REALTIME: System-wide realtime clock. Setting this clock requires appropriate privileges.
    // CLOCK_MONOTONIC: Clock that cannot be set and represents monotonic time since some unspecified starting point.
    // CLOCK_PROCESS_MPLANEUTIME_ID: High-resolution per-process timer from the CPU.
    // CLOCK_THREAD_MPLANEUTIME_ID: Thread-specific CPU-time clock.
    clock_gettime(CLOCK_MONOTONIC, &req);   // unspecified starting point

    for( ; ; ) {    // infinite loop
        size_t interval = get_interval_ms()*1000000L;  // nsec
        req.tv_sec  = req.tv_sec + (req.tv_nsec + interval)/1000000000L;    // sec
        req.tv_nsec = (req.tv_nsec + interval) % 1000000000L;

        // NOTE: clock_nanosleep() returns either 0 for requested time elapsed or error value for interruption.
        //       therefore, if not 0, then try again.
        size_t max_retry = 10;
        for(size_t jj = 0; jj < max_retry; jj++){
            if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, NULL) == 0) break;
        }

        this->inc_tick();

        for(size_t kk = 0; kk < this->timed_list.get_count(); kk++) {
            if(this->timed_list.is_time(this->get_tick(), kk)) {
                this->timed_list.exec_item(kk);
            }
        }
    }

    if(g_debug_mask & (1UL << MASKBIT_EVENT)){
        TRACE3(std::cerr, time_thread, thread_id) << "thread exit" << std::endl;
    }

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); // 
} // end of void time_thread::run_time_thread()
