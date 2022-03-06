#include <stdlib.h>         // rand()
#include <stdint.h>	    // uint32_t
#include <stddef.h>         // offsetof()
#include <string>           // std::string
#include <iomanip>          // std::setw()
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream
#include "dma_peri_hal.h"   // class dma_peri_hal
#include "h2_fpga_reg.h"

dma_peri_hal::dma_peri_hal(uint64_t addr_base):
    addrbase(addr_base),
    peri(addr_base)
{
    this->pl_version.u32 = 0;
    this->verbose = false;
}

dma_peri_hal::~dma_peri_hal()
{}

void dma_peri_hal::set_pl_version(uint32_t version)
{
    this->pl_version.u32 = version;
}

uint16_t dma_peri_hal::get_major_pl_version() const
{
    return(pl_version.u16le[1].u16);
} // end of uint32_t dma_peri_hal::get_version()

uint16_t dma_peri_hal::get_minor_pl_version() const
{
    return(pl_version.u16le[0].u16);
} // end of uint32_t dma_peri_hal::get_version()

void dma_peri_hal::enable_syncin(bool enabled)
{
    if((get_minor_pl_version() >= 29) && (get_minor_pl_version() <= 31)){
        peri.wr32(SYNCIN_CTRL_229_231_REG,
                  ((enabled? 1: 0) << SYNCIN_CTRL_229_231_REG__ENABLE_SYNCIN_SHL),
                  /*verbose=*/ true);
        TRACE0() << "SYNCIN " << (enabled? "enabled" : "disabled") << std::endl;
    }
}

void dma_peri_hal::select_il(int dl_serdes_idx, int ul_serdes_idx)
{
    if((get_minor_pl_version() >= 30) && (get_minor_pl_version() <= 35)){
        this->peri.wr32(IL_SEL_230_235_REG,
                        ((dl_serdes_idx << IL_SEL_230_235_REG__IL_DL_MUX_SEL_SHL) |
                         (ul_serdes_idx << IL_SEL_230_235_REG__IL_UL_MUX_SEL_SHL)),
                        this->verbose);
    }
}

void dma_peri_hal::set_num_descriptors(int num)
{
    this->peri.wr32(DMA_PERI_CAPTURE_NUM_DESC_REG, (uint32_t)num, this->verbose);
} // end of void dma_peri_hal::set_num_descriptors()

void dma_peri_hal::set_header_mask(int burst_idx, uint32_t mask)
{
    this->peri.wr32(DMA_PERI_CAPTURE_HEADER_MASK_REG + (burst_idx << 2), mask, this->verbose);
} // end of void dma_peri_hal::set_header_compare()

void dma_peri_hal::set_header_target(int burst_idx, uint32_t target)
{
    this->peri.wr32(DMA_PERI_CAPTURE_HEADER_COMPARE_REG + (burst_idx << 2), target, this->verbose);
} // end of void dma_peri_hal::set_header_compare()

void dma_peri_hal::set_header_compare(int burst_idx, uint32_t mask, uint32_t target)
{
    this->peri.wr32(DMA_PERI_CAPTURE_HEADER_MASK_REG + (burst_idx << 2), mask, this->verbose);
    this->peri.wr32(DMA_PERI_CAPTURE_HEADER_COMPARE_REG + (burst_idx << 2), target, this->verbose);
} // end of void dma_peri_hal::set_header_compare()

void dma_peri_hal::set_burst_payload_size(int burst_idx, uint32_t burst_word_size)
{
    this->peri.wr32(DMA_PERI_CAPTURE_BURST_WORD_LEN_REG + (burst_idx << 2), burst_word_size, this->verbose);
} // end of void dma_peri_hal::set_burst_payload_size()

void dma_peri_hal::kick_to_start()
{
    this->peri.wr32(DMA_PERI_CAPTURE_START_REG, 0x1, this->verbose);
} // end of void dma_peri_hal::kick_to_start()


