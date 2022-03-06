#ifndef _DMA_PERI_HAL_H_
#define _DMA_PERI_HAL_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include "mmap_hal.h"   // mmap_hal

class dma_peri_hal
{
    public:
        dma_peri_hal(uint64_t addr_base);
        virtual ~dma_peri_hal();

        void set_pl_version(uint32_t version);
        uint16_t get_major_pl_version() const;
        uint16_t get_minor_pl_version() const;
        void select_il(int dl_serdes_idx, int ul_serdes_idx);
        void enable_syncin(bool enabled);

        void set_num_descriptors(int num);
        void set_header_mask(int burst_idx, uint32_t mask);
        void set_header_target(int burst_idx, uint32_t target);
        void set_header_compare(int burst_idx, uint32_t mask, uint32_t target);
        void set_burst_payload_size(int burst_idx, uint32_t burst_word_size);
        void kick_to_start();

    protected:
        const uint64_t  addrbase;
        mmap_hal    peri;
        hal::u32le_t pl_version;
        bool verbose;
};

#endif // _DMA_PERI_HAL_H_
