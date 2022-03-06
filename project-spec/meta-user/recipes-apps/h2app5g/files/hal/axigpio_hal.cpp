#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>         // rand()
#include <stdint.h>	    // uint32_t
#include <stddef.h>         // offsetof()
#include <string>           // std::string
#include <iomanip>          // std::setw()
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream

#include "h2_fpga_reg.h"
#include "axigpio_hal.h"    // class axigpio_hal

axigpio_hal::axigpio_hal(uint64_t addr_base):
    addrbase(addr_base),
    axigpio(addr_base)
{
    pl_version.u32 = 0;
}

axigpio_hal::~axigpio_hal()
{}

void axigpio_hal::set_pl_version(uint32_t version)
{
    this->pl_version.u32 = version;
}

uint16_t axigpio_hal::get_major_pl_version() const
{
    return(pl_version.u16le[1].u16);
} // end of uint32_t axigpio_hal::get_version()

uint16_t axigpio_hal::get_minor_pl_version() const
{
    return(pl_version.u16le[0].u16);
} // end of uint32_t axigpio_hal::get_version()

uint32_t axigpio_hal::rd32(uint64_t addr)
{
    return(axigpio.rd32(addr));
}

void axigpio_hal::wr32(uint64_t addr, uint32_t value)
{
    axigpio.wr32(addr, value);
}

void axigpio_hal::hard_reset()
{
    uint32_t tri = 0;

#if 0
    // TODO: needs mutex here

    // read tri state register
    tri = axigpio.rd32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI));
    // makes the dedicated bit output enable
    //      0 = I/O pin configured as output
    //      1 = I/O pin configured as input
    tri = tri & ~(1UL << AXIGPIO1_RESET_SHL);
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI), tri);
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;

    // write data
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_DATA), (1UL << AXIGPIO1_RESET_SHL));
    // wait
    hal::nsleep(1000000000);
    // write 0 for low active
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_DATA), (0UL << AXIGPIO1_RESET_SHL));
    // wait
    hal::nsleep(1000000000);
    // write data
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_DATA), (1UL << AXIGPIO1_RESET_SHL));
    // wait
    hal::nsleep(1000000000);

    tri = tri | (1UL << AXIGPIO1_RESET_SHL);
    // makes the dedicated bit output disable
    //      0 = I/O pin configured as output
    //      1 = I/O pin configured as input
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI), tri);
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;
#else
    // TODO: needs mutex here

    // read tri state register
    tri = axigpio.rd32(AXIGPIO_0_TRI_REG);
    // makes the dedicated bit output enable
    //      0 = I/O pin configured as output
    //      1 = I/O pin configured as input
    tri = tri & ~(1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
    axigpio.wr32(AXIGPIO_0_TRI_REG, tri);
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;

    // write data
    axigpio.wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
    // wait
    hal::nsleep(1000000000);    // 1 sec
    // write 0 for low active
    axigpio.wr32(AXIGPIO_0_DATA_REG, (0UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
    // wait
    hal::nsleep(1000000000);    // 1 sec
    // write data
    axigpio.wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
    // wait
    hal::nsleep(1000000000);    // 1 sec

    tri = tri | (1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
    // makes the dedicated bit output disable
    //      0 = I/O pin configured as output
    //      1 = I/O pin configured as input
    axigpio.wr32(AXIGPIO_0_TRI_REG, tri);
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;
#endif
}

void axigpio_hal::enable_rfout(bool enable)
{
    uint32_t tri = 0;
    uint32_t data = 0;

    // TODO: needs mutex here
    // read tri state register
    tri = axigpio.rd32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI));
    // makes the dedicated bit output enable
    //      0 = I/O pin configured as output
    //      1 = I/O pin configured as input
    data = tri & ~(1UL << AXIGPIO1_RFONOFF_SHL);
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI), data);
    std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << data << std::endl;

    // write data
    data = (enable? 1UL: 0UL) << AXIGPIO1_RFONOFF_SHL;
    axigpio.wr32(addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_DATA), data);
    std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_DATA) << "=" << data << std::endl;
}
