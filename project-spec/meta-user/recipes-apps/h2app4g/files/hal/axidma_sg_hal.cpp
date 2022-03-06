#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>         // rand()
#include <stdint.h>	    // uint32_t
#include <stddef.h>         // offsetof()
#include <string>           // std::string
#include <iomanip>          // std::setw()
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream
#include "axidma_sg_hal.h"  // class axidma_sg_hal

axidma_sg_hal::axidma_sg_hal(uint64_t ctrl_addr_base):
    ctrladdrbase(ctrl_addr_base),
    dmactrl(ctrl_addr_base),
    axidma_mode(AXIDMA_MODE_SG)
{
    this->pl_version.u32 = 0;
    this->verbose = false;
}

axidma_sg_hal::~axidma_sg_hal()
{
}

void axidma_sg_hal::set_pl_version(uint32_t version)
{
    this->pl_version.u32 = version;
}

uint16_t axidma_sg_hal::get_major_pl_version() const
{
    return(pl_version.u16le[1].u16);
} // end of uint32_t axidma_sg_hal::get_version()

uint16_t axidma_sg_hal::get_minor_pl_version() const
{
    return(pl_version.u16le[0].u16);
} // end of uint32_t axidma_sg_hal::get_version()

void axidma_sg_hal::read_mm2s_status()
{
    hal::u64le_t    currdescaddr64le;

    currdescaddr64le.u32le[0].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC));
    currdescaddr64le.u32le[1].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC_MSB));
    printf("curdescaddr=0x%010llx, ", (long long)currdescaddr64le.u64);

    uint32_t status = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMASR));
    std::cout
        << "mm2s_status=0x"
        << std::hex << std::setw(8)
        << std::setfill('0')
        << status
        << ", ";

    if (status & 0x00000001)
        std::cout << "halted";
    else
        std::cout << "running";
    if (status & 0x00000002) std::cout << " idle";
    if (status & 0x00000008) std::cout << " SGIncld";
    if (status & 0x00000010) std::cout << " DMAIntErr";
    if (status & 0x00000020) std::cout << " DMASlvErr";
    if (status & 0x00000040) std::cout << " DMADecErr";
    if (status & 0x00000100) std::cout << " SGIntErr";
    if (status & 0x00000200) std::cout << " SGSlvErr";
    if (status & 0x00000400) std::cout << " SGDecErr";
    if (status & 0x00001000) std::cout << " IOC_Irq";
    if (status & 0x00002000) std::cout << " Dly_Irq";
    if (status & 0x00004000) std::cout << " Err_Irq";
    std::cout << std::endl;
}

void axidma_sg_hal::kick_mm2s(uint64_t currdesc_addr, uint64_t taildesc_addr)
{
    hal::u64le_t    currdescaddr64le;
    hal::u64le_t    taildescaddr64le;

    currdescaddr64le.u64 = currdesc_addr;
    taildescaddr64le.u64 = taildesc_addr;

    //step 1: write MM2S_CURDESC and MM2S_CURDESC_MSB
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC),     currdescaddr64le.u32le[0].u32);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC_MSB), currdescaddr64le.u32le[1].u32);

    // read back
    currdescaddr64le.u32le[0].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC));
    currdescaddr64le.u32le[1].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_CURDESC_MSB));

    //step 2:set run/stop bit to 1 (MM2S_DMACR.RS = 1);
    uint32_t mm2s_dmacr = 0;
    //mm2s_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR));
    mm2s_dmacr =
        (1UL << MM2S_DMACR__CYCLICBDEN) |
        (1UL << MM2S_DMACR__RS_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR), mm2s_dmacr);

    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0() << "write mm2s_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << mm2s_dmacr << std::endl;
    }

    mm2s_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR));

    //step 3(optional): if desired, enable interrupts
    //  MM2S_DMACR.IOC_IrqEn = 1
    //  MM2S_DMACR.Err_IrqEn = 1

    //step 4: write MM2S_TAILDESC and MM2S_TAILDESC_MSB
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_TAILDESC),    taildescaddr64le.u32le[0].u32);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_TAILDESC_MSB),taildescaddr64le.u32le[1].u32);
}   // end of void axidma_sg_hal::kick_mm2s()

void axidma_sg_hal::read_s2mm_status()
{
    hal::u64le_t    currdescaddr64le;

    currdescaddr64le.u32le[0].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_CURDESC));
    currdescaddr64le.u32le[1].u32 = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_CURDESC_MSB));
    printf("curdescaddr=0x%010llx, ", (long long)currdescaddr64le.u64);

    uint32_t status = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMASR));
    std::cout
        << "s2mm_status=0x"
        << std::hex << std::setw(8)
        << std::setfill('0')
        << status
        << ", ";

    if (status & 0x00000001)
        std::cout << "halted";
    else
        std::cout << "running";
    if (status & 0x00000002) std::cout << " idle";
    if (status & 0x00000008) std::cout << " SGIncld";
    if (status & 0x00000010) std::cout << " DMAIntErr";
    if (status & 0x00000020) std::cout << " DMASlvErr";
    if (status & 0x00000040) std::cout << " DMADecErr";
    if (status & 0x00000100) std::cout << " SGIntErr";
    if (status & 0x00000200) std::cout << " SGSlvErr";
    if (status & 0x00000400) std::cout << " SGDecErr";
    if (status & 0x00001000) std::cout << " IOC_Irq";
    if (status & 0x00002000) std::cout << " Dly_Irq";
    if (status & 0x00004000) std::cout << " Err_Irq";
    std::cout << std::endl;

}

void axidma_sg_hal::kick_s2mm(uint64_t currdesc_addr, uint64_t taildesc_addr)
{
    hal::u64le_t    currdescaddr64le;
    hal::u64le_t    taildescaddr64le;

    currdescaddr64le.u64 = currdesc_addr;
    taildescaddr64le.u64 = taildesc_addr;

    //step 1: write S2MM_CURDESC and S2MM_CURDESC_MSB
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_CURDESC),     currdescaddr64le.u32le[0].u32);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_CURDESC_MSB), currdescaddr64le.u32le[1].u32);

    //step 2:set run/stop bit to 1 (S2MM_DMACR.RS = 1);
    uint32_t s2mm_dmacr = 0;
    //s2mm_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR));
    s2mm_dmacr =
        (1UL << S2MM_DMACR__RS_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR), s2mm_dmacr);
    std::cout << "write s2mm_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << s2mm_dmacr << std::endl;

    //step 3(optional): if desired, enable interrupts
    //  S2MM_DMACR.IOC_IrqEn = 1
    //  S2MM_DMACR.Err_IrqEn = 1

    //step 4: write S2MM_TAILDESC and S2MM_TAILDESC_MSB
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_TAILDESC),    taildescaddr64le.u32le[0].u32);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_TAILDESC_MSB),taildescaddr64le.u32le[1].u32);
}

void axidma_sg_hal::stop_mm2s()
{
    //set run/stop bit to 1 (MM2S_DMACR.RS = 0);
    uint32_t mm2s_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR));

    //NOTE: The Xilinx "pg021_axi_dma" describes that
    //      setting either MM2S_DMACR.Reset = 1 or S2MM_DMACR.Reset = 1 resets the entire AXI DMA engine.
    //      Setting DMACR.RS = 0 makes DMA stop after current DMA operations are complete.
    mm2s_dmacr =
        //(mm2s_dmacr & ~(1UL << MM2S_DMACR__RS_SHL)) |
        (0UL << MM2S_DMACR__RS_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR), mm2s_dmacr);
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0() << "write mm2s_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << mm2s_dmacr << std::endl;
    }
}

void axidma_sg_hal::stop_s2mm()
{
    //set run/stop bit to 1 (S2MM_DMACR.RS = 0);
    uint32_t s2mm_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR));

    //NOTE: The Xilinx "pg021_axi_dma" describes that
    //      setting either MM2S_DMACR.Reset = 1 or S2MM_DMACR.Reset = 1 resets the entire AXI DMA engine.
    //      Setting DMACR.RS = 0 makes DMA stop after current DMA operations are complete.
    s2mm_dmacr =
        //(s2mm_dmacr & ~(1UL << S2MM_DMACR__RS_SHL)) |
        (0UL << S2MM_DMACR__RS_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR), s2mm_dmacr);
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0() << "write s2mm_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << s2mm_dmacr << std::endl;
    }
}

void axidma_sg_hal::reset_mm2s()
{
    //set run/stop bit to 1 (MM2S_DMACR.RS = 0);
    uint32_t mm2s_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR));

    //NOTE: The Xilinx "pg021_axi_dma" describes that
    //      setting either MM2S_DMACR.Reset = 1 or S2MM_DMACR.Reset = 1 resets the entire AXI DMA engine.
    //      Setting DMACR.RS = 0 makes DMA stop after current DMA operations are complete.
    mm2s_dmacr =
        //(mm2s_dmacr & ~((1UL << MM2S_DMACR__RS_SHL) |
        //               (1UL << MM2S_DMACR__RESET_SHL))) |
        //(0UL << MM2S_DMACR__RS_SHL) |
        (1UL << MM2S_DMACR__RESET_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, MM2S_DMACR), mm2s_dmacr);
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0() << "write mm2s_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << mm2s_dmacr << std::endl;
    }
}

void axidma_sg_hal::reset_s2mm()
{
    //set run/stop bit to 1 (S2MM_DMACR.RS = 0);
    uint32_t s2mm_dmacr = dmactrl.rd32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR));
    //TRACE0() << "read  s2mm_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << s2mm_dmacr << std::endl;

    //NOTE: The Xilinx "pg021_axi_dma" describes that
    //      setting either MM2S_DMACR.Reset = 1 or S2MM_DMACR.Reset = 1 resets the entire AXI DMA engine.
    //      Setting DMACR.RS = 0 makes DMA stop after current DMA operations are complete.
    s2mm_dmacr =
        //(s2mm_dmacr & ~((1UL << S2MM_DMACR__RS_SHL) |
        //               (1UL << S2MM_DMACR__RESET_SHL))) |
        //(0UL << S2MM_DMACR__RS_SHL) |
        (1UL << S2MM_DMACR__RESET_SHL);
    dmactrl.wr32(ctrladdrbase + offsetof(axidma_sg_ctrl, S2MM_DMACR), s2mm_dmacr, this->verbose);
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0() << "write s2mm_dmacr=0x" << std::hex << std::setw(8) << std::setfill('0') << s2mm_dmacr << std::endl;
    }
}
