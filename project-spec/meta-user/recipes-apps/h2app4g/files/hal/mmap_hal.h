#ifndef __MMAP_HAL_H__
#define __MMAP_HAL_H__

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
#include "hal.h"        // class hal

// hardware adaptation layer
class   mmap_hal: public hal
{
    public:
        mmap_hal(uint64_t addr_base);
        virtual ~mmap_hal();

        bool is_pow_of_2(uint32_t num);

        uint32_t rd32(uint64_t addr);
        void wr32(uint64_t addr, uint32_t value, bool verbose=false);

        uint16_t rd16(uint64_t addr);
        void wr16(uint64_t addr, uint16_t value);

        uint8_t rd08(uint64_t addr);
        void wr08(uint64_t addr, uint8_t value);

    protected:
        void check_pagesize(uint64_t addr);

    public:
        const uint64_t pagesize;
        const uint64_t pageoffsmask;
        const uint64_t pageaddrmask;

    protected:
        uint64_t pageaddrbase;
	void *p1mmap;
        const int instanceid;
        static int instance_gid;

    private:
        mmap_hal();  // NOTE: block to use the default constructor
};

#endif // __MMAP_HAL_H__
