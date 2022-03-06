#ifndef __OFFS_MMAP_HAL_H__
#define __OFFS_MMAP_HAL_H__

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

class   offs_mmap_hal: public mmap_hal
{
    public:
        offs_mmap_hal(uint64_t addr_base, uint64_t offs_mask);
        virtual ~offs_mmap_hal();

        uint32_t rd32(uint64_t addr);
        void wr32(uint64_t addr, uint32_t value, bool verbose=false);

        uint16_t rd16(uint64_t addr);
        void wr16(uint64_t addr, uint16_t value);

        uint8_t rd08(uint64_t addr);
        void wr08(uint64_t addr, uint8_t value);

    protected:
        uint64_t masked_addr(uint64_t addr_base,
                             uint64_t offs_mask,
                             uint64_t addr);

    protected:
        const uint64_t addrbase;
        const uint64_t offsmask;
};


#endif // __OFFS_MMAP_HAL_H__
