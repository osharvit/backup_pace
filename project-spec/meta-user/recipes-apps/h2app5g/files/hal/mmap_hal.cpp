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
#include <iostream>     // std::cout
#include <iomanip>      // std::setw(), std::setfill()
#include "mmap_hal.h"   // class mmap_hal

int mmap_hal::instance_gid = 0;

#if   defined(__aarch64__)    // for real platform
mmap_hal::mmap_hal(uint64_t addr_base):
    pagesize(sysconf(_SC_PAGESIZE)),
    pageoffsmask((uint64_t)((long)pagesize-1)),
    pageaddrmask(~pageoffsmask),
    instanceid(instance_gid++)
{
    pageaddrbase = (addr_base & pageaddrmask);
    // NOTE: the pagesize should be a power of 2
    if(is_pow_of_2(pagesize) == false) {
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "pagesize(" << pagesize << ") should be a power of 2." << std::endl;
        perror(sstrm.str().c_str());
    }

    // step 1: open the memory device "/dev/mem"
    std::string devfile = std::string("/dev/mem");
    int fd = open(devfile.c_str(), O_RDWR | O_SYNC);
    if(fd < 0){
        std::stringstream sstrm;
        TRACE3(sstrm, mmap_hal, instanceid)
            << "error in open " << devfile << std::endl;
        perror(sstrm.str().c_str());
        exit(-1);
    }

    // step 2: map
    p1mmap = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pageaddrbase);
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

#if   defined(__aarch64__)    // for real platform
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
}
#endif

bool mmap_hal::is_pow_of_2(uint32_t num)
{
    return((num & ((long)num - 1)) == 0);
}

#if   defined(__aarch64__)    // for real platform
void mmap_hal::check_pagesize(uint64_t addr)
{
    long long addroffs = ((long long)addr - (long long)this->pageaddrbase);

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

    // calculate new this->pageaddrbase
    this->pageaddrbase = (addr & pageaddrmask);

#if 0
    TRACE2(mmap_hal, instanceid) 
        << "new this->pageaddrbase=0x" << std::hex << std::setw(10) << std::setfill('0') << this->pageaddrbase << std::endl;
#endif

    // step 1: open the memory device "/dev/mem"
    std::string devfile = std::string("/dev/mem");
    int fd = open(devfile.c_str(), O_RDWR | O_SYNC);

    // step 2: map
    p1mmap = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, this->pageaddrbase);
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

uint32_t mmap_hal::rd32(uint64_t addr)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask & ~0x3; // 32-bit align
    return(*((uint32_t *)(p1mmap + addroffs)));
}

void mmap_hal::wr32(uint64_t addr, uint32_t value, bool verbose)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask & ~0x3; // 32-bit align
    *((uint32_t *)(p1mmap + addroffs)) = value;

    if(verbose){
        std::cout << "poke " << "0x" << std::hex << std::setw(8) << (this->pageaddrbase + addroffs) << " " << "0x" << value << std::endl;
    }
}

uint16_t mmap_hal::rd16(uint64_t addr)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask & ~0x1; // 16-bit align
    return(hal::rd16((uint64_t)p1mmap + addroffs));
}

void mmap_hal::wr16(uint64_t addr, uint16_t value)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask & ~0x1; // 16-bit align
    hal::wr16((uint64_t)p1mmap + addroffs, value);
}

uint8_t mmap_hal::rd08(uint64_t addr)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask;
    return(hal::rd08((uint64_t)p1mmap + addroffs));
}

void mmap_hal::wr08(uint64_t addr, uint8_t value)
{
    check_pagesize(addr);

    uint64_t addroffs = (uint64_t)((long long)addr - (long long)pageaddrbase) & pageoffsmask;
    hal::wr08((uint64_t)p1mmap + addroffs, value);
}

