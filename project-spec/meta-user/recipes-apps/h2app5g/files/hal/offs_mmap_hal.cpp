#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	    // uint32_t
#include <sys/mman.h>	    // mmap(), munmap()
#include <fcntl.h>	    // O_RDWR, O_SYNC
#include <unistd.h>	    // close()
#include <string>           // std::string
#include <iomanip>
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream
#include "offs_mmap_hal.h"  // class offs_mmap_hal

offs_mmap_hal::offs_mmap_hal(uint64_t addr_base, uint64_t offs_mask):
    mmap_hal(addr_base),
    addrbase(addr_base),
    offsmask(offs_mask)
{
}

offs_mmap_hal::~offs_mmap_hal()
{ }

uint64_t offs_mmap_hal::masked_addr(uint64_t addr_base,
                                   uint64_t offs_mask,
                                   uint64_t addr)
{
    return((addr_base & (~offs_mask)) | (addr & offs_mask));
}

uint32_t offs_mmap_hal::rd32(uint64_t addr)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    return(mmap_hal::rd32(newaddr));
}

void offs_mmap_hal::wr32(uint64_t addr, uint32_t value, bool verbose)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    mmap_hal::wr32(newaddr, value, verbose);
}

uint16_t offs_mmap_hal::rd16(uint64_t addr)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    return(mmap_hal::rd16(newaddr));
}

void offs_mmap_hal::wr16(uint64_t addr, uint16_t value)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    mmap_hal::wr16(newaddr, value);
}

uint8_t offs_mmap_hal::rd08(uint64_t addr)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    return(mmap_hal::rd08(newaddr));
}

void offs_mmap_hal::wr08(uint64_t addr, uint8_t value)
{
    uint64_t newaddr = masked_addr(this->addrbase, this->offsmask, addr);
    mmap_hal::wr08(newaddr, addr);
}
