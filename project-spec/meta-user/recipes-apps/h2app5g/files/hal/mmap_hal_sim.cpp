#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include <sys/mman.h>	// mmap(), munmap()
#include <fcntl.h>	// O_RDWR, O_SYNC
#include <unistd.h>	// close()
#include <string>       // std::string
#include <iomanip>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "mmap_hal.h"   // class mmap_hal

#if   defined(__x86_64__) // for simulation only
mmap_hal::mmap_hal(uint64_t addr_base):
    pagesize(sysconf(_SC_PAGESIZE)),
    offsmask((uint64_t)((long)pagesize-1)),
    addrmask(~offsmask),
    instanceid(instance_gid++)
{
    addrbase = (addr_base & addrmask);
    // NOTE: the pagesize should be a power of 2
    if(is_pow_of_2(pagesize) == false) {
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "pagesize(" << pagesize << ") should be a power of 2." << std::endl;
        perror(sstrm.str().c_str());
    }

    // step 1: open the memory device "/dev/mem"
#if   defined(__x86_64__) // for simulation only
    std::string devfile = std::string("/tmp/mmap_hal") + std::to_string(this->instanceid);
    // create a zero initialized file for simulation purpose
    std::string systemcmd = std::string("dd if=/dev/zero of=") + devfile + std::string(" bs=1 count=") + std::to_string(pagesize);
    int syscallsts = system(systemcmd.c_str());
    if(syscallsts != 0){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "syscallsts=" << syscallsts
            << std::endl;
        perror(sstrm.str().c_str());
        //exit(-1);
    }
    int fd = open(devfile.c_str(), O_RDWR | O_SYNC);
#endif
    if(fd < 0){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "error in open " << devfile << std::endl;
        perror(sstrm.str().c_str());
        exit(-1);
    }

    // step 2: map
#if     defined(__x86_64__) // for simulation only
    p1mmap = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif
    if(p1mmap == (caddr_t)-1){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "error in mmap" << std::endl;
        perror(sstrm.str().c_str());
        exit(-1);
    }

    // step 3: close the file dscriptor to the "/dev/mem"
    close(fd);
}
#endif

#if defined(__x86_64__) // for simulation only
mmap_hal::~mmap_hal()
{
    // step 5: unmap
    if(p1mmap != (caddr_t)-1){
        if(munmap(p1mmap, pagesize) != 0) {
            std::stringstream sstrm;
            TRACE3(sstrm, mmap_hal, instanceid)
                << "error in munmap" << std::endl;
            perror(sstrm.str().c_str());
        }
    }
    // static member
    instance_gid--;
}
#endif

#if defined(__x86_64__) // for simulation only
void mmap_hal::check_pagesize(uint64_t addr)
{
    long long addroffs = ((long long)addr - (long long)this->addrbase);

    if((0 <= addroffs) && (addroffs < (const long long)pagesize)) return; // nothing to do

    // step 0: unmap first
    if(p1mmap != (caddr_t)-1){
        if(munmap(p1mmap, pagesize) != 0) {
            std::stringstream sstrm;
            TRACE3(sstrm, mmap_hal, instanceid)
                << "error in munmap" << std::endl;
            perror(sstrm.str().c_str());
        }
    }

    // calculate new this->addrbase
    this->addrbase = (addr & addrmask);

#if 0
    TRACE2(mmap_hal, instanceid) 
        << "new this->addrbase=0x" << std::hex << std::setw(10) << std::setfill('0') << this->addrbase << std::endl;
#endif

    // step 1: open the memory device "/dev/mem"
#if   defined(__x86_64__) // for simulation only
    std::string devfile = std::string("/tmp/mmap_hal") + std::to_string(this->instanceid);
    // create a zero initialized file for simulation purpose
    std::string systemcmd = std::string("dd if=/dev/zero of=") + devfile + std::string(" bs=1 count=") + std::to_string(pagesize);
    int syscallsts = system(systemcmd.c_str());
    if(syscallsts != 0){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "syscallsts=" << syscallsts
            << std::endl;
        perror(sstrm.str().c_str());
        //exit(-1);
    }
    int fd = open(devfile.c_str(), O_RDWR | O_SYNC);
#endif

    // step 2: map
#if     defined(__x86_64__) // for simulation only
    p1mmap = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif
    if(p1mmap == (caddr_t)-1){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "error in mmap" << std::endl;
        perror(sstrm.str().c_str());
        exit(-1);
    }

    // step 3: close the file dscriptor to the "/dev/mem"
    close(fd);
} // end of void mmap_hal::check_pagesize()
#endif

