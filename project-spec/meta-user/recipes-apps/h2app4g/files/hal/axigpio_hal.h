#ifndef _AXIGPIO_HAL_HPP_
#define _AXIGPIO_HAL_HPP_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include "mmap_hal.h"   // mmap_hal

class   axigpio_hal
{
    public:
        enum{
            AXIGPIO1_RESET_SHL      = 0,
            AXIGPIO1_SYNC_SHL       = 1,
            AXIGPIO1_RFONOFF_SHL    = 2,
        };

        typedef struct _axigpio1_ctrl{
            uint32_t AXI_GPIO1_DATA;
            uint32_t AXI_GPIO1_TRI;
        }axigpio1_ctrl;

    public:
        axigpio_hal(uint64_t addr_base);
        virtual ~axigpio_hal();

        void set_pl_version(uint32_t version);
        uint16_t get_major_pl_version() const;
        uint16_t get_minor_pl_version() const;
        void hard_reset();
        void enable_rfout(bool enable);

        uint32_t rd32(uint64_t addr);
        void wr32(uint64_t addr, uint32_t value);

    protected:
        const uint64_t addrbase;
        mmap_hal axigpio;
        hal::u32le_t    pl_version;
};

#endif // _AXIGPIO_HAL_HPP_
