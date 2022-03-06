#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getopt()
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>   // backtrace(), backtrace_symbols()
#include <iostream>
#include <exception>    // exception

#include "axigpio_hal.h"
#include "axidma_sg_hal.h"
#include "dma_peri_hal.h"
#include "innolink_hal.h"
#include "h2top_hal.h"
#include "re_thread.h"
#include "time_thread.h"
#include "hswi_thread.h"
#include "mplane_mon_thread.h"
#include "sp_thread.h"
#include "il_thread.h"
#include "wdt_ctrl.h"
# ifdef  USE_SYS10MS_INTERRUPT
#include "blocking_interrupt.h"
# endif
#include "version.h"

const char* p2help[] = {
    "NAME: h2app -  list commands",
    "SYNOPSIS: h2app [options]",
    "  -o [call]        call trace",
    "  -o [event]       event trace",
    "  -o [socket]      socket trace",
    "  -o [nofw]        skip fw boot procedure",
    "  -o [mpjson]      print mp json transaction",
    "  -o [mphex]       print mp binary transaction",
    "  -o [mpdly=n]     segment delay for n-msec",
    "  -o [mppatt]      test mp mode for fpga->h2",
    "  -o [port]        listen port (default: 1004)",
    "  -o [fw=fn]       fw name (default: h2fw.elf)",
    "  -o [tmplayback]  tmplayback",
    "  -h               help",
    "  -v               verson",
    NULL    // NOTE: always terminated by NULL
};

// global variable for debug
uint32_t g_debug_mask = //(1UL << MASKBIT_SOCKET) |
                        //(1UL << MASKBIT_MPLANE) |
                        0;
uint32_t g_mplane_delay_ms = 100;
int g_listen_port = 1004;
std::string g_fpga_ver;
std::string g_fw_ver;
std::string g_dsp_ver;
std::string g_fw_name("h2fw.elf");

wdt_ctrl    watchdog("/dev/watchdog");
#ifdef  USE_SYS10MS_INTERRUPT
blocking_interrupt sys10ms_interrupt("/dev/zynqirq");
#endif

pthread_mutex_t gp1mutex[NUM_MUTEX] = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
};

//======================================================
// hardware adaptation layer
//======================================================
axigpio_hal p1gpio2hermes[NUM_HERMES] = {
    axigpio_hal(AXIGPIO_1_BASE)
};

innolink_hal    p2innolink[NUM_HERMES][NUM_INNOLINK] = {
    {
        innolink_hal("il0",
                     0,
                     0x80400000 + (P_IL << 16), 0x0000FFFF,
                     0x80800000 + (P_IL << 16), 0x0000FFFF,
                     WORD_SIZE_PER_MPLANE_SEGMENT),
        innolink_hal("il1",
                     1,
                     0x80400000 + (S_IL << 16), 0x0000FFFF,
                     0x80800000 + (S_IL << 16), 0x0000FFFF,
                     WORD_SIZE_PER_MPLANE_SEGMENT),
    }
};

axidma_sg_hal   p1mm2saxidma[NUM_HERMES] = {
    {
        axidma_sg_hal(AXIDMA_BASE),
    }
};

axidma_sg_hal   p1s2mmaxidma[NUM_HERMES] = {
    {
        axidma_sg_hal(AXIDMA_BASE + 0x3000),
    }
};

dma_peri_hal    p1dmaperi[NUM_HERMES] = {
    {
        dma_peri_hal(0x80200000),
    }
};

h2top_hal   p1h2top[NUM_HERMES] = {
    h2top_hal(p1gpio2hermes[0],
              p2innolink[0],
              p1mm2saxidma[0],
              p1s2mmaxidma[0],
              p1dmaperi[0]),
};


//======================================================
// threads
//======================================================
time_thread timethread("timethread", THREAD_TIME);

mplane_mon_thread p1mpmonconnthread[NUM_MPMON_CONNSOCK] = {
    mplane_mon_thread("mpmonconn0",
                   THREAD_MPMON_CONN + MPMON_CONNSOCK_0,
                   MPMON_CONNSOCK_0,
                   THREAD_MPMON_LSTN),
    mplane_mon_thread("mpmonconn1",
                   THREAD_MPMON_CONN + MPMON_CONNSOCK_1,
                   MPMON_CONNSOCK_1,
                   THREAD_MPMON_LSTN),
    mplane_mon_thread("mpmonconn2",
                   THREAD_MPMON_CONN + MPMON_CONNSOCK_2,
                   MPMON_CONNSOCK_2,
                   THREAD_MPMON_LSTN)
};

tcpserv_thread mpmonlstnthread("mpmonlisten",
                               THREAD_MPMON_LSTN,
                               g_listen_port + 1,
                               THREAD_MPMON_CONN,
                               {MPMON_CONNSOCK_0, HSWI_CONNSOCK_1});

hswi_thread p1hswiconnthread[NUM_HSWI_CONNSOCK] = {
    hswi_thread("hswiconn0",
                THREAD_HSWI_CONN + HSWI_CONNSOCK_0,
                HSWI_CONNSOCK_0,
                THREAD_HSWI_LSTN),
    hswi_thread("hswiconn1",
                THREAD_HSWI_CONN + HSWI_CONNSOCK_1,
                HSWI_CONNSOCK_1,
                THREAD_HSWI_LSTN),
    hswi_thread("hswiconn2",
                THREAD_HSWI_CONN + HSWI_CONNSOCK_2,
                HSWI_CONNSOCK_2,
                THREAD_HSWI_LSTN)
};

hswi_thread hswilstnthread("hswilisten",
                           THREAD_HSWI_LSTN,
                           g_listen_port,
                           THREAD_HSWI_CONN,
                           {HSWI_CONNSOCK_0, HSWI_CONNSOCK_1, HSWI_CONNSOCK_2});

il_thread ilthread("ilthread", THREAD_IL, p1h2top);
sp_thread spthread("spthread",
                   THREAD_SP,
#ifdef  USE_SYS10MS_INTERRUPT
                   sys10ms_interrupt,
#endif
                   p1h2top);
re_thread h2appthread("rethread",
                      THREAD_RE,
                      watchdog);

void parse_args(int argc, char*argv[])
{
    int opt;
    enum {
        O_CALL,
        O_EVENT,
        O_SOCKET,
        O_NOFW,
        O_MPLANEJSON,
        O_MPLANEHEX,
        O_MPLANEPATT,
        O_MPLANEDLY,
        O_PORT,
        O_FW,
        O_TMPLAYBACK,
        NUM_O
    };
    char *const o_tokens[] = {
        [O_CALL]        = "call",
        [O_EVENT]       = "event",
        [O_SOCKET]      = "socket",
        [O_NOFW]        = "nofw",
        [O_MPLANEJSON]      = "mpjson",
        [O_MPLANEHEX]       = "mphex",
        [O_MPLANEPATT]      = "mppatt",
        [O_MPLANEDLY]       = "mpdly",
        [O_PORT]        = "port",
        [O_FW]          = "fw",
        [O_TMPLAYBACK]  = "tmplayback",
        NULL
    };

    while((opt = getopt(argc, argv, "ho:v")) != -1) {
        switch(opt) {
            case 'o': {
                int subopt;
                char *value;

                while((subopt = getsubopt(&optarg, o_tokens, &value)) != -1){
                    if(subopt == O_CALL){
                        g_debug_mask |= (1UL << MASKBIT_CALL);
                    }
                    else if(subopt == O_EVENT){
                        g_debug_mask |= (1UL << MASKBIT_EVENT);
                    }
                    else if(subopt == O_SOCKET){
                        g_debug_mask |= (1UL << MASKBIT_SOCKET);
                    }
                    else if(subopt == O_NOFW){
                        g_debug_mask |= (1UL << MASKBIT_NOFW);
                    }
                    else if(subopt == O_MPLANEJSON){
                        g_debug_mask |= (1UL << MASKBIT_MPLANE_JSON);
                    }
                    else if(subopt == O_MPLANEHEX){
                        g_debug_mask |= (1UL << MASKBIT_MPLANE_HEX);
                    }
                    else if(subopt == O_MPLANEDLY){
                        g_mplane_delay_ms = atol(value);
                        TRACE0() << "g_mplane_delay_ms=" << g_mplane_delay_ms << std::endl;
                    }
                    else if(subopt == O_MPLANEPATT){
                        g_debug_mask |= (1UL << MASKBIT_MPLANE_PATTERN);
                    }
                    else if(subopt == O_PORT){
                        g_listen_port = atoi(value);
                    }
                    else if(subopt == O_FW){
                        g_fw_name = std::string(value);
                    }
                    else if(subopt == O_TMPLAYBACK){
                        g_debug_mask |= (1UL << MASKBIT_TMPLAYBACK);
                    }
                }
                TRACE0() << "g_debug_mask=0x" << std::hex << std::setw(8) << std::setfill('0') << g_debug_mask << std::endl;
                break;
            }
            case 'v': {
                printf("%s\n\r", ::get_verstr().c_str());
                exit(0);
                break;
            }
            case 'h':
            case '?': {
                for(size_t kk = 0; (kk < sizeof(p2help)/sizeof(p2help[0])) && (p2help[kk] != NULL); kk++) {
                    printf("%s\n\r", p2help[kk]);
                }
                exit(0);
                break;
            }

            default:;
        }
    }
} // end of void parse_args(int argc, char*argv[])

void init_devices()
{
    // TODO: code for initializing device.
    if(g_debug_mask & (1UL << MASKBIT_CALL)) TRACE0();

    // initialize watchdog
    watchdog.initialize();
#ifdef  USE_SYS10MS_INTERRUPT
    // initialize sys10ms interrupt
    sys10ms_interrupt.initialize();
#endif

    hal::u32le_t fpgaversion;
    fpgaversion.u32 = p1h2top[0].read_pl_version();
    p1h2top[0].set_pl_version(fpgaversion.u32);

    // major version
    assert(fpgaversion.u16le[1].u16 == 2);
    // minor version
    assert(fpgaversion.u16le[0].u16 >= 29);

    {
        std::cout << std::endl;
        std::cout << "============================================" << std::endl;
        std::cout << "GIT_BRANCH\t=" << std::string(GIT_BRANCH) << std::endl;
        std::cout << "VERSION\t\t=" << get_verstr() << std::endl;
        std::cout << "============================================" << std::endl;

        std::cout << "PAGE_SIZE=" << std::hex << PAGE_SIZE << std::endl;
    }

    for(size_t kk = 0; kk < NUM_HERMES; kk++){
        p1h2top[kk].initialize();
    }

} // end of void init_devices()

void init_mutexes()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }

    pthread_mutexattr_t attr[NUM_MUTEX];

    for(size_t mutexid = 0; mutexid < NUM_MUTEX; mutexid++) {
        pthread_mutexattr_init(&attr[mutexid]);
        // set mutex type attribute.
        // PTHREAD_MUTEX_RECURSIVE_NP: a thread attempting to relock this mutex without first unlocking
        //  it will succeed in locking the mutex.
        pthread_mutexattr_settype(&attr[mutexid], PTHREAD_MUTEX_RECURSIVE_NP);
        // initialize mutex object
        pthread_mutex_init(&gp1mutex[mutexid], &attr[mutexid]);
    }

    // install mutex to the itc_queue
    thread_base::initialize_statically(&gp1mutex[MUTEX_THREAD]);
    // install mutex to the timed_list
    thread_base::timed_list.install_mutex(&gp1mutex[MUTEX_THREAD]);
    //
    for(size_t kk = 0; kk < NUM_MPMON_CONNSOCK; kk++) {
        // start thread for MPMON(hermes radio software interface)
        p1mpmonconnthread[kk].install_sockmutex(&gp1mutex[MUTEX_SOCKET]);
    }
    //
    mpmonlstnthread.install_sockmutex(&gp1mutex[MUTEX_SOCKET]);
    //
    for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++) {
        // start thread for HSWI(hermes radio software interface)
        p1hswiconnthread[kk].install_sockmutex(&gp1mutex[MUTEX_SOCKET]);
    }
    //
    hswilstnthread.install_sockmutex(&gp1mutex[MUTEX_SOCKET]);
    //
} // end of void init_mutexes()

void init_threads()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }

    // common part
    signal(SIGPIPE, /*sighandler_t*/ SIG_IGN);   // makes the "SIGPIPE" ignored; any broken socke can incur SIGPIPE
} // end of void init_threads()

void segfault_sigaction(int signum, siginfo_t *si, void *arg)
{
    std::string signalstr;
    switch(signum) {
        case SIGABRT:   signalstr = std::string("SIGABRT"); break;
        case SIGALRM:   signalstr = std::string("SIGALRM"); break;
        case SIGBUS:    signalstr = std::string("SIGBUS"); break;
        case SIGCHLD:   signalstr = std::string("SIGCHLD"); break;
        //case SIGCLD:    signalstr = std::string("SIGCLD"); break;
        case SIGCONT:   signalstr = std::string("SIGCONT"); break;
        //case SIGEMT:    signalstr = std::string("SIGEMT"); break;
        case SIGFPE:    signalstr = std::string("SIGFPE"); break;
        case SIGHUP:    signalstr = std::string("SIGHUP"); break;
        case SIGILL:    signalstr = std::string("SIGILL"); break;
        //case SIGINFO:   signalstr = std::string("SIGINFO"); break;
        case SIGINT:    signalstr = std::string("SIGINT"); break;
        case SIGIO:     signalstr = std::string("SIGIO"); break;
        //case SIGIOT:    signalstr = std::string("SIGIOT"); break;
        case SIGKILL:   signalstr = std::string("SIGKILL"); break;
        //case SIGLOST:   signalstr = std::string("SIGLOST"); break;
        case SIGPIPE:   signalstr = std::string("SIGPIPE"); break;
        //case SIGPOLL:   signalstr = std::string("SIGPOLL"); break;
        case SIGPROF:   signalstr = std::string("SIGPROF"); break;
        case SIGPWR:    signalstr = std::string("SIGPWR"); break;
        case SIGQUIT:   signalstr = std::string("SIGQUIT"); break;
        case SIGSEGV:   signalstr = std::string("SIGSEGV"); break;
        case SIGSTKFLT: signalstr = std::string("SIGSTKFLT"); break;
        case SIGSTOP:   signalstr = std::string("SIGSTOP"); break;
        case SIGTSTP:   signalstr = std::string("SIGTSTP"); break;
        case SIGSYS:    signalstr = std::string("SIGSYS"); break;
        case SIGTERM:   signalstr = std::string("SIGTERM"); break;
        case SIGTRAP:   signalstr = std::string("SIGTRAP"); break;
        case SIGTTIN:   signalstr = std::string("SIGTTIN"); break;
        case SIGTTOU:   signalstr = std::string("SIGTTOU"); break;
        //case SIGUNUSED: signalstr = std::string("SIGUNUSED"); break;
        case SIGURG:    signalstr = std::string("SIGURG"); break;
        case SIGUSR1:   signalstr = std::string("SIGUSR1"); break;
        case SIGUSR2:   signalstr = std::string("SIGUSR2"); break;
        case SIGVTALRM: signalstr = std::string("SIGVTALRM"); break;
        case SIGXCPU:   signalstr = std::string("SIGXCPU"); break;
        case SIGXFSZ:   signalstr = std::string("SIGXFSZ"); break;
        case SIGWINCH:  signalstr = std::string("SIGWINCH"); break;
        default: signalstr = std::string("unlisted");
    }

    if(signum != 0){
        std::cerr << "caught " << signalstr << "(" << signum << ") from pid=" << getpid() << std::endl;
    }
    if ( si != NULL )
    {
        std::cerr << "Caught segfault at address : " << si->si_addr << '\n';
        std::cerr << "sigdata : si_signo=" << si->si_signo
            << ", si_code=" << si->si_code
            //<< ", si_value" << si->si_value
            << ", si_errno=" << si->si_errno
            << ", si_pid=" << si->si_pid
            << ", si_uid=" << si->si_uid
            << ", si_status=" << si->si_status
            << ", si_band=" << si->si_band
            << std::endl;
    }

    if ((signum != SIGRTMAX) && (signum != SIGSEGV))
    {
        signal(signum, SIG_IGN);
        return;
    }

    // Manage File //
    //---------------------------------------- -----------------------//
    std::string crashlogname("h2app_crash.log");
    FILE  *fp;
    if(!(fp = fopen(crashlogname.c_str(), "w")))
    {
        TRACE0() << crashlogname << std::endl;
        return;
    }

    // Get Current time //
    //---------------------------------------- -----------------------//
    {
        char  p1stamp[100];
        time_t    now;
        struct tm *ts;
        now = time(NULL);
        ts  = localtime(&now);
        strftime(p1stamp, sizeof(p1stamp), "[DATE: %Y.%m.%d %H:%M:%S]\n", ts);
        fprintf(fp, "%s", p1stamp);
    }

    if ( signum == SIGRTMAX )
    {
        fprintf(fp, "[CATEGORY: Timeout(soft watchdog)]\n");
    }
    else if ( signum == SIGSEGV )
    {
        fprintf(fp, "[CATEGORY: SW_Failure]\n");
    }
    else if ( signum == SIGILL )
    {
        fprintf(fp, "[CATEGORY: Catch SIGILL]\n");
    }
    else if ( signum == SIGTRAP )
    {
        fprintf(fp, "[CATEGORY: Catch SIGTRAP]\n");
    }
    else
    {
        fprintf(fp, "[CATEGORY: Exception]\n");
    }

    fprintf(fp, "[Caught signal : %d@%d]\n", signum, getpid());


    // Stack backtrace //
    //---------------------------------------- -----------------------//
    int nptrs;
    void *buffer[100+1];

    nptrs = backtrace(buffer, 100);
    fprintf(stderr, "backtrace() rturned %d addresses\n\r", nptrs);

    char **strings;

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL )
    {
        perror("backtrace_symbols");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "[TAG1:\n");
    fprintf(fp, "STACK: Called by:\n");

    for (int j=0; j<nptrs; j++ )
    {
        fprintf(fp, "%s\n", strings[j]);
    }
    fprintf(fp, "]\n");

    fprintf(stderr, "[TAG1:\n");
    fprintf(stderr, "STACK: Called by:\n");
    if(si != NULL) fprintf(stderr, "[%p]\n", si->si_addr);

    for (int j=0; j<nptrs; j++ )
    {
        fprintf(stderr, "%s\n", strings[j]);
    }
    fprintf(stderr, "]\n");

    free(strings);
    fflush(fp);
    fclose(fp);


    {
        FILE *fp;
        char buf[100+1];

        if((fp = fopen("/proc/meminfo", "r")))
        {
            while( fgets(buf, sizeof(buf), fp) )
            {
                std::cerr << buf;
            }
            fclose(fp);
        }
    }

    exit(EXIT_FAILURE);

    UNUSED(arg);
} // end of void segfault_sigaction(...)

int main(int argc, char *argv[])
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV,  &sa, NULL);
    sigaction(SIGRTMAX, &sa, NULL);
    sigaction(SIGABRT,  &sa, NULL);
    sigaction(SIGBUS,   &sa, NULL);
    sigaction(SIGILL,   &sa, NULL);
    sigaction(SIGFPE,   &sa, NULL);
    sigaction(SIGIOT,   &sa, NULL);
    sigaction(SIGTRAP,  &sa, NULL);
    sigaction(SIGSYS,   &sa, NULL);
    sigaction(SIGPWR,   &sa, NULL);

    signal(SIGTERM,     SIG_IGN);
    signal(SIGINT,      SIG_IGN);
    signal(SIGQUIT,     SIG_IGN);
    signal(SIGTSTP,     SIG_IGN);
    signal(SIGHUP,      SIG_IGN);
    signal(SIGVTALRM,   SIG_IGN);
    signal(SIGXCPU,     SIG_IGN);
    signal(SIGXFSZ,     SIG_IGN);
    signal(SIGPWR,      SIG_IGN);
    signal(SIGPROF,     SIG_IGN);
    signal(SIGIO,       SIG_IGN);

    try{
        //------------------------------------------
        // initialize
        //------------------------------------------
        // arguments handler
        parse_args(argc, argv);

        // initialize ------------------------------
        init_devices();
        init_mutexes();
        init_threads();

        //------------------------------------------
        // start threads
        //------------------------------------------
        // start thread for timed functions
        timethread.start_time_thread();
        // do NOT change the order of hswi_thread
        hswilstnthread.start_hswi_thread();
        //
        for(size_t kk = 0; kk < NUM_HSWI_CONNSOCK; kk++) {
            // start thread for HSWI(hermes radio software interface)
            p1hswiconnthread[kk].start_hswi_thread();
        }
        // do NOT change the order of mplane_mon_thread
        mpmonlstnthread.start_tcpserv_thread();
        //
        for(size_t kk = 0; kk < NUM_MPMON_CONNSOCK; kk++) {
            // start thread for MPMON(hermes radio software interface)
            p1mpmonconnthread[kk].start_tcpserv_thread();
        }
        // start thread for innolink carriers
        ilthread.start_il_thread();
        // start thread for special purpose such as nofw and initsync, etc)
        spthread.start_sp_thread();
        // start thread for hermes2 application at the last order.
        h2appthread.start_re_thread();

        //------------------------------------------
        // wait for thread termination
        //------------------------------------------
        h2appthread.join();
        pthread_exit(NULL);
        fprintf(stdout, "bye\n\r");
    }
    catch(std::exception &e){
        std::cerr << "exceptional caught: " << e.what() << std::endl;
        segfault_sigaction(0, NULL, NULL);
    }

    return(0);
} // end of int main(int argc, char *argv[])

/*  vim:  set ts=4 sts=4 sw=4 et cin   : */
