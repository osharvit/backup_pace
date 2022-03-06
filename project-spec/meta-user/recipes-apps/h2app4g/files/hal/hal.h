#ifndef __HAL_HPP__
#define __HAL_HPP__

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include <time.h>       // struct timespec, clock_gettime(), timer_create(), timer_settime(), timer_

// hardware adaptation layer
class   hal
{
    public:
        typedef union _u16le_t{
            uint16_t    u16;
            uint8_t     u08[2];
        }__attribute__((packed)) u16le_t;

        typedef union _u32le_t{
            uint32_t    u32;
            uint8_t     u08[4];
            u16le_t     u16le[2];
        }__attribute__((packed)) u32le_t;

        typedef union _u64le_t{
            uint64_t    u64;
            uint8_t     u08[8];
            u16le_t     u16le[4];
            u32le_t     u32le[2];
        }__attribute__((packed)) u64le_t;

    public:
        hal();
        virtual ~hal();

        static uint32_t rd32(uint64_t addr);
        static void wr32(uint64_t addr, uint32_t value, bool verbose = false);

        static uint16_t rd16(uint64_t addr);
        static void wr16(uint64_t addr, uint16_t value);

        static uint8_t rd08(uint64_t addr);
        static void wr08(uint64_t addr, uint8_t value);

        static uint32_t peek(uint64_t addr);
        static void poke(uint64_t addr, uint32_t data);

        // sleep for nonosec
        static void nsleep(size_t nanosec);
};

#endif // __HAL_HPP__
