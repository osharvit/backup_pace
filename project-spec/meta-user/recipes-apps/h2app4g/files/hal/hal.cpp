#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include <time.h>       // struct timespec, clock_gettime(), timer_create(), timer_settime(), timer_
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>     // std::cout
#include <iomanip>      // std::setw(), std::setfill()
#include "hal.h"

hal::hal()
{
}

hal::~hal()
{
}

#if 0
#define	R32(addr)		(*(volatile uint32_t *)(addr))
#define	R16(addr)		(*(volatile uint16_t *)(addr))
#define	R8(addr)		(*(volatile uint8_t *)(addr))

#define	W32(addr, value)	(*(volatile uint32_t *)(addr) = (value))
#define	W16(addr, value)	(*(volatile uint16_t *)(addr) = (value))
#define	W8(addr, value)		(*(volatile uint8_t *)(addr) = (value))
#endif

uint32_t hal::rd32(uint64_t addr)
{
    return(*(volatile uint32_t *)(addr));
}

void hal::wr32(uint64_t addr, uint32_t value, bool verbose)
{
    *(volatile uint32_t *)(addr) = value;
    if(verbose){
        std::cout << "poke " << "0x" << std::hex << std::setw(8) << addr << " " << "0x" << std::hex << value << std::endl;
    }
}

uint16_t hal::rd16(uint64_t addr)
{
    return(*(volatile uint16_t *)(addr));
}

void hal::wr16(uint64_t addr, uint16_t value)
{
    *(volatile uint16_t *)(addr) = value;
}

uint8_t hal::rd08(uint64_t addr)
{
    return(*(volatile uint8_t *)(addr));
}

void hal::wr08(uint64_t addr, uint8_t value)
{
    *(volatile uint8_t *)(addr) = value;
}

uint32_t hal::peek(uint64_t addr)
{
    int fd;
    void *ptr;
    unsigned page_addr, page_offset;
    unsigned page_size=sysconf(_SC_PAGESIZE);

    fd=open("/dev/mem",O_RDONLY);
    if(fd<1) {
        perror("error in hal::peek()");
        exit(-1);
    }

    page_addr=(addr & ~(page_size-1));
    page_offset=addr-page_addr;

    ptr=mmap(NULL,page_size,PROT_READ,MAP_SHARED,fd,(addr & ~(page_size-1)));
    if(ptr == (caddr_t)-1){
        perror("error in hal::peek()");
        exit(-1);
    }

    uint32_t data = *((unsigned *)(ptr+page_offset));

    close(fd);

    if(ptr != (caddr_t)-1){
        munmap(ptr, page_size);
    }

    return(data);
}

void hal::poke(uint64_t addr, uint32_t data)
{
    int fd;
    void *ptr;
    unsigned page_addr, page_offset;
    unsigned page_size=sysconf(_SC_PAGESIZE);

    fd=open("/dev/mem",O_RDWR);
    if(fd<1) {
        perror("error in hal::poke()");
        exit(-1);
    }
    page_addr=(addr & ~(page_size-1));
    page_offset=addr-page_addr;

    ptr=mmap(NULL,page_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(addr & ~(page_size-1)));
    if(ptr == (caddr_t)-1){
        perror("error in hal::peek()");
        exit(-1);
    }

    *((unsigned *)(ptr+page_offset))=data;

    close(fd);

    if(ptr != (caddr_t)-1){
        munmap(ptr, page_size);
    }
}


void hal::nsleep(size_t nanosec)
{
    struct timespec req;

    //  recent versions of GNU libc and the Linux kernel support the following clocks:
    // CLOCK_REALTIME: System-wide realtime clock. Setting this clock requires appropriate privileges.
    // CLOCK_MONOTONIC: Clock that cannot be set and represents monotonic time since some unspecified starting point.
    // CLOCK_PROCESS_MPLANEUTIME_ID: High-resolution per-process timer from the CPU.
    // CLOCK_THREAD_MPLANEUTIME_ID: Thread-specific CPU-time clock.
    clock_gettime(CLOCK_MONOTONIC, &req);   // unspecified starting point

    req.tv_sec  = req.tv_sec + (req.tv_nsec + nanosec)/1000000000L;    // sec
    req.tv_nsec = (req.tv_nsec + nanosec) % 1000000000L;

#if 0
    cock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, NULL);
#else
    while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &req) != 0)
    {
        continue;
    }
#endif
} // end of void hal::nsleep(size_t nanosec)

