#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>       // round()
#include <sys/stat.h>
#include <string>
#include <iomanip>      // std::setw(), std::setfill()
#include <vector>       // std::vector
#include <algorithm>    // std::sort
#include <list>         // std::list
#include <sstream>      // std::stringstream

#include "sp_thread.h"
#include "ant_carrier.h"    // class ant_carrier
#include "h2_fpga_reg.h"
#include "mmap_hal.h"
#include "axidma_sg_hal.h"

extern std::string g_fw_name;

#if   defined(__x86_64__) // for simulation only
void sp_thread::sp_fwload_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    std::string bootarg("");
    for(rapidjson::Value::ConstMemberIterator itochild = nvreg_json.MemberBegin(); itochild != nvreg_json.MemberEnd(); ++itochild){
        bootarg += std::string(itochild->name.GetString()) + "=";
        switch(itochild->value.GetType())
        {
            case  rapidjson::kStringType: bootarg += std::string(itochild->value.GetString()); break;
            case  rapidjson::kNumberType: bootarg += std::to_string(itochild->value.GetInt()); break;
            default:;
        }
        bootarg += std::string(" ");

    }
#if 0
    TRACE0OBJ() << "bootarg=" << bootarg << std::endl;
#endif
    // step 1: disable syncin pulse

    // step 2: serdes configuration before fwload
        switch(fpga_major_version){
            case 1: {   // h2hb0 emulation
                // TODO: select the internal 245M76 system clock
                // TODO: disable msb/lsb bit flip
                // TODO: disable sample swap
                // TODO: reset GTH
                // TODO: reset other digital block
                break;
            }
            case 2: {   // h2ha0 oran_platform
                // TODO: select the external 245M76 system clock
                //      onboard: 0x50000, external: 0x10000)"
                //      poke 0x80800034 0x10000
                p1innolink[0].xil_gth.wr32(0x80800034, 0x10000);
                hal::nsleep(100000000);    // 100 msec

                // TODO: enable msb/lsb bit flip
                //      enable:0x108, disable:0x18c
                //      poke 0x80800018 0x108
                p1innolink[0].xil_gth.wr32(0x80800018, 0x108);
                hal::nsleep(100000000);    // 100 msec

                // TODO: enable sample swap from a 48-bit word
                //      enable: 0x101, disable:0x18c
                //      poke 0x80400010 0x101
                p1innolink[0].il_ctrl.wr32(0x80400010, 0x101);
                hal::nsleep(100000000);    // 100 msec

                // TODO: reset GTH
                //      poke 0x80800004 0x1
                //      poke 0x80800004 0x0
                p1innolink[0].xil_gth.wr32(0x80800004, 0x1);
                hal::nsleep(100000000);    // 100 msec

                p1innolink[0].xil_gth.wr32(0x80800004, 0x0);
                hal::nsleep(100000000);    // 100 msec

                // TODO: reset other digital block
                //      poke 0x80004004 0xf7
                //      poke 0x80400100 0x2a01
                //      poke 0x80004004 0xff
                //      poke 0x80400100 0x3f01
                p1gpio2hermes[0].wr32(0x80004004, 0xf7);
                hal::nsleep(100000000);    // 100 msec

                p1innolink[0].il_ctrl.wr32(0x80400100, 0x2a01);
                hal::nsleep(100000000);    // 100 msec

                p1gpio2hermes[0].wr32(0x80004004, 0xff);
                hal::nsleep(100000000);    // 100 msec

                p1innolink[0].il_ctrl.wr32(0x80400100, 0x3f01);
                hal::nsleep(100000000);    // 100 msec
                break;
            }
            case 3: {   // h2ha0 emulation
                // TODO: select the internal 245M76 system clock
                // TODO: enable msb/lsb bit flip
                // TODO: enable sample swap
                // TODO: reset GTH
                // TODO: reset other digital block
                break;
            }
            default: {
                // TODO: select the external 245M76 system clock
                // TODO: disable msb/lsb bit flip
                // TODO: disable sample swap
                // TODO: reset GTH
                // TODO: reset other digital block
            }
        }

    // step 3: hard reset first
    TRACE0OBJ() << "hard_reset" << std::endl;
    {
        // TODO: needs mutex here

        // read tri state register
        uint32_t tri = this->p1gpio2hermes[0].rd32(AXIGPIO_0_TRI_REG);
        // makes the dedicated bit output enable
        //      0 = I/O pin configured as output
        //      1 = I/O pin configured as input
        tri = tri & ~(1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
        this->p1gpio2hermes[0].wr32(AXIGPIO_0_TRI_REG, tri);
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;

        // write data
        this->p1gpio2hermes[0].wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
        // wait
        hal::nsleep(1000000000);    // 1 sec
        // write 0 for low active
        this->p1gpio2hermes[0].wr32(AXIGPIO_0_DATA_REG, (0UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));

        // wait
        hal::nsleep(1000000000);    // 1000 msec

        // write data
        this->p1gpio2hermes[0].wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
        // wait
        hal::nsleep(1000000000);    // 1000 msec

        tri = tri | (1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
        // makes the dedicated bit output disable
        //      0 = I/O pin configured as output
        //      1 = I/O pin configured as input
        this->p1gpio2hermes[0].wr32(AXIGPIO_0_TRI_REG, tri);
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;
    }

    // NOTE: return back to the SP_IDLE_S
    transit_state(SP_IDLE_S, true);
}
#endif

#if   defined(__x86_64__) // for simulation only
bool sp_thread::download_tm_file(const itc_download_tm_file_req &download_tm_file_req)
{
    bool status = true;

    return(status);
} // end of bool sp_thread::download_tm_file()
#endif
