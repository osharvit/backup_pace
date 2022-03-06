#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>         // rand()
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	    // uint32_t
#include <stddef.h>         // offsetof()
#include <string>           // std::string
#include <iomanip>          // std::setw()
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream
#include "mmap_hal.h"       // class mmap_hal
#include "h2_fpga_reg.h"
#include "il_ctrl.h"        // innolink control
#include "innolink_dig.h"   // innolink digital
#include "innolink_hal.h"   // innolink_hal
#include "xil_gth.h"        // xilink GTH SerDes IP

innolink_hal::innolink_hal(const std::string &name,
                           size_t innolink_id,
                           uint64_t ildig_addr_base, uint64_t ildig_offs_mask,
                           uint64_t gth_addr_base, uint64_t gth_offs_mask,
                           size_t wordsize_per_segment):
    obj_name(name),
    ILDIG_ADDR_BASE(ildig_addr_base),
    ILDIG_OFFS_MASK(ildig_offs_mask),
    GTH_ADDR_BASE(gth_addr_base),
    GTH_OFFS_MASK(gth_offs_mask),
    innolinkid(innolink_id),
    WORDSIZE_PER_SEG(wordsize_per_segment),
    PAYLOAD_PER_SEG((WORDSIZE_PER_SEG - 3)*4 - 1),  // (WORDSIZE_PER_SEG - 3)*4(bytes/word) - 1(null string)
    il_ctrl(ILDIG_ADDR_BASE, ILDIG_OFFS_MASK),      // 0x804i_0000 ~ 0x804i_0fff
    xil_gth(GTH_ADDR_BASE, GTH_OFFS_MASK)           // 0x808i_0000 ~ 0x808i_00ff
{
    this->pl_version.u32 = 0;
    this->loopback = false;
    this->verbose = false;

    tx_word_index = 0;
    rx_word_index = 0;

    syncin_ctrl_shadow = 0;
}

innolink_hal::~innolink_hal()
{
}

void innolink_hal::initialize()
{
    syncin_ctrl_shadow = 0;
}

std::string innolink_hal::objname() const
{
    return(obj_name);
}

uint32_t innolink_hal::read_pl_version()
{
    uint32_t    ver = il_ctrl.rd32(ILDIG_ADDR_BASE + (uint64_t)offsetof(struct hw_il_ctrl, version));

    return(ver);
} // end of uint32_t innolink_hal::read_pl_version()

void innolink_hal::set_pl_version(uint32_t version)
{
    this->pl_version.u32 = version;
}

uint16_t innolink_hal::get_major_pl_version() const
{
    return(pl_version.u16le[1].u16);
} // end of uint32_t innolink_hal::get_version()

uint16_t innolink_hal::get_minor_pl_version() const
{
    return(pl_version.u16le[0].u16);
} // end of uint32_t innolink_hal::get_version()

std::string innolink_hal::get_verstr()
{
    std::string version =
        std::to_string((this->pl_version.u16le[1].u16)) +
        "." +
        std::to_string((this->pl_version.u16le[0].u16));


    return(version);
} // end of uint32_t innolink_hal::get_verstr()

bool innolink_hal::bist()
{
    bool status = true;

#if 0
    status = bist_data_bus();
    if(status == false){
        TRACE3(std::cerr, innolink_hal, innolinkid) << "error in bitst_date_bus()" << std::endl;
        return(status);
    }

    for(size_t kk = 1; kk < 256; kk++){
        status = bist_fifo_depth(kk);
        if(status == false){
            TRACE3(std::cerr, innolink_hal, innolinkid) << "error in bist_fifo_depth(" << kk << ")" << std::endl;
            return(status);
        }
    }
#endif

    return(status);
}

bool innolink_hal::bist_data_bus()
{
    TRACE3(std::cout, innolink_hal, innolinkid);
    bool status = true;

    // databus test
    for(size_t kk = 0; kk < 32; kk++){
        uint32_t wr_pattern = 1ul << kk;
        uint32_t rd_pattern = 0;

        // write
        il_ctrl.wr32(ILDIG_ADDR_BASE + (uint64_t)offsetof(struct hw_il_ctrl, scratch), wr_pattern);
        // read and compare
        rd_pattern = il_ctrl.rd32(ILDIG_ADDR_BASE + (uint64_t)offsetof(struct hw_il_ctrl, scratch));
        if(rd_pattern != wr_pattern) {
            status = false;
            std::stringstream sstrm;
            TRACE3(sstrm, innolink_hal, innolinkid);
            perror(sstrm.str().c_str());
        }
#if 1
        else {
            std::cout << "rd_pattern=0x" << std::hex << rd_pattern << std::endl;
        }
#endif
    }

    return(status);
} // end of bool innolink_hal::bist_data_bus()

bool innolink_hal::bist_fifo_depth(size_t run)
{
    bool status = true;
    const size_t base = rand() % 256;

    size_t dummycnt = poll_rxfifo_level();
    for(size_t kk = 0; kk < dummycnt; kk++){
        recv_mplane();
    }

    // burst write
    for(size_t kk = 0; kk < run; kk++){
        uint32_t wr_pattern = base + kk;

        // write
        send_mplane(wr_pattern);
    }

    // burst read
    for(size_t kk = 0; kk < run; kk++){
        uint32_t wr_pattern = base + kk;
        uint32_t rd_pattern = 0;

        // read and compare
        rd_pattern = recv_mplane();
        if(rd_pattern != wr_pattern) {
            status = false;
            std::stringstream sstrm;
            TRACE3(sstrm, innolink_hal, innolinkid) << "error in burst read" << std::endl;
            perror(sstrm.str().c_str());
        }
#if 0
        else {
            std::cout << "rd_pattern=0x" << std::hex << rd_pattern << std::endl;
        }
#endif
    }

    return(status);
} // end of bool innolink_hal::bist_data_bus()

void innolink_hal::set_sysclk_enable(bool enable)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE3(std::cerr, innolink_hal, innolinkid);
    }

    if(get_minor_pl_version() >= 30){
        uint32_t value = enable? 0x00000: 0x10000;
        xil_gth.wr32(GTH_XCVR_CLK_CTRL_REG, value);
    }
    else if(get_minor_pl_version() < 30){
        uint32_t value = enable? 0x10000: 0x50000;
        xil_gth.wr32(GTH_XCVR_CLK_CTRL_REG, value);
    }
    else{
        TRACE0OBJ() << "NOT applicable" << std::endl;
    }
} // end of void innolink_hal::set_sysclk_enable()

void innolink_hal::reset_serdes()
{
    this->reset_serdes(this->loopback);
}

void innolink_hal::reset_serdes(bool loopback_enable)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE3(std::cerr, innolink_hal, innolinkid);
    }

    // update
    this->loopback = loopback_enable;

    if(this->loopback){
        // disable MSB/LSB flip (enable:0x108, disable:0x18c)
        xil_gth.wr32(GTH_MSB_LSB_FLIP_REG, 0x18c);

        // assert reset while keeping bit field for serdes loopback
        xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x20001);

        hal::nsleep(1000L);  // nano sleep for 1000 ns

        // release reset while keeping bit field for serdes loopback
        xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x20000);

        hal::nsleep(1000000L);  // nano sleep for 1 ms

        // disable sample swap (enable: 0x101, disable: 0x001)
        il_ctrl.wr32(GTH_SAMPLE_SWAP_REG, 0x001);
    }
    else{
        // enable MSB/LSB flip (enable:0x108, disable:0x18c) due to the h2ha0 bug
        xil_gth.wr32(GTH_MSB_LSB_FLIP_REG, 0x108);

        // assert reset
        xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x00001);

        hal::nsleep(1000L);  // nano sleep for 1000 ns

        // release reset
        xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x00000);

        hal::nsleep(1000000L);  // nano sleep for 1 ms

        // enable sample swap (enable: 0x101, disable: 0x001) due to the h2ha0 bug
        il_ctrl.wr32(GTH_SAMPLE_SWAP_REG, 0x101);
    }
} // end of void innolink_hal::reset_serdes()

#if   defined(__aarch64__)    // for real platform
void innolink_hal::send_mplane(uint32_t data)
{
    if(data == 0x16161616) tx_word_index = 0;

    il_ctrl.wr32(IL_MPLANE_DATA_RW_REG, data);
    if(g_debug_mask & (1UL << MASKBIT_MPLANE_HEX)){
        std::cerr << "tx[" << std::dec << std::setw(3) << std::setfill('0') << tx_word_index << "]:0x"
            << std::hex << std::setw(8) << std::setfill('0') << data << std::endl;
    }

    tx_word_index++;
} // end of void innolink_hal::send_mplane(..)
#endif

// due to both the H2HW-57 & H2HW-81 issue
// - packet should be segmented into 15 word size.
// - 3 words are reserved for the preamble, header, and postamble.
// - only 12 (=15-3) words are for payload.
// - but at least one null string '\0' run is required at the end of payload.
// - therefore, each segment can have a payload of up to 47 (=12*4-1) bytes.
// little-endian                   memory_view       register_view
//                                  +------+        +--------------+
//  preamble:                       | 0x16 |        |  0x16161616  |
//                                  | 0x16 |        |              |
//                                  | 0x16 |        |              |
//                                  | 0x16 |        |              |
//                                  +------+        +--------------+
//  control                         | 0x00 |        |  0xLLSSTT00  |
//  destid                          | 0xTT |        |              |
//  0x80(last seg) | 0x7F (seg seq) | 0xSS |        |              |
//  seg payload length              | 0xLL |        |              |
//                                  +------+        +--------------+
//                            '0'   | 0x30 |        |  0x33323130  |
//                            '1'   | 0x31 |        |              |
//                            '2'   | 0x32 |        |              |
//                            '3'   | 0x33 |        |              |
//                                  +------+        +--------------+
//                            '4'   | 0x34 |        |  0x37363534  |
//                            '5'   | 0x35 |        |              |
//                            '6'   | 0x36 |        |              |
//                            '7'   | 0x37 |        |              |
//                                  +------+        +--------------+
//                            '8'   | 0x38 |        |  0x42413938  |
//                            '9'   | 0x39 |        |              |
//                            'A'   | 0x41 |        |              |
//                            'B'   | 0x42 |        |              |
//                                  +------+        +--------------+
//                            'C'   | 0x43 |        |  0x46454443  |
//                            'D'   | 0x44 |        |              |
//                            'E'   | 0x45 |        |              |
//                            'F'   | 0x46 |        |              |
//                                  +------+        +--------------+
//                            'G'   | 0x47 |        |  0x4a494847  |
//                            'H'   | 0x48 |        |              |
//                            'I'   | 0x49 |        |              |
//                            'J'   | 0x4a |        |              |
//                                  +------+        +--------------+
//                            'K'   | 0x4b |        |  0x4e4d4c4b  |
//                            'L'   | 0x4c |        |              |
//                            'M'   | 0x4d |        |              |
//                            'N'   | 0x4e |        |              |
//                                  +------+        +--------------+
//                            'O'   | 0x4f |        |  0x5251504f  |
//                            'P'   | 0x50 |        |              |
//                            'Q'   | 0x51 |        |              |
//                            'R'   | 0x52 |        |              |
//                                  +------+        +--------------+
//                            'S'   | 0x53 |        |  0x56555453  |
//                            'T'   | 0x54 |        |              |
//                            'U'   | 0x55 |        |              |
//                            'V'   | 0x56 |        |              |
//                                  +------+        +--------------+
//                            'W'   | 0x57 |        |  0x5a595857  |
//                            'X'   | 0x58 |        |              |
//                            'Y'   | 0x59 |        |              |
//                            'Z'   | 0x5a |        |              |
//                                  +------+        +--------------+
//                            'a'   | 0x61 |        |  0x64636261  |
//                            'b'   | 0x62 |        |              |
//                            'c'   | 0x63 |        |              |
//                            'd'   | 0x64 |        |              |
//                                  +------+        +--------------+
//                            'e'   | 0x65 |        |  0x68676665  |
//                            'f'   | 0x66 |        |              |
//                            'g'   | 0x67 |        |              |
//                            'h'   | 0x68 |        |              |
//                                  +------+        +--------------+
//                            'i'   | 0x69 |        |  0x006b6a69  |
//                            'j'   | 0x6a |        |              |
//                            'k'   | 0x6b |        |              |
//                            '\0'  | 0x00 |        |              |
//                                  +------+        +--------------+
//  postamble:                      | 0x17 |        |  0x17171717  |
//                                  | 0x17 |        |              |
//                                  | 0x17 |        |              |
//                                  | 0x17 |        |              |
//                                  +------+        +--------------+
void innolink_hal::send_mplane(const std::string &str)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE3(std::cerr, innolink_hal, innolinkid);
    }
    const char *p1strbuf = str.c_str();

    int pos = 0;
    size_t lastsegsize = 0;
    size_t numfullsegment = 0;

    if(g_debug_mask & (1UL << MASKBIT_MPLANE_JSON)){
        TRACE0() << str << std::endl;
    }

    if((lastsegsize = (str.size() % PAYLOAD_PER_SEG)) > 0)
    {
        numfullsegment = (str.size() / PAYLOAD_PER_SEG);
    }
    else{
        numfullsegment = (size_t)((int)(str.size() / PAYLOAD_PER_SEG) - 1);
        lastsegsize = PAYLOAD_PER_SEG;
    }

    hal::u32le_t le32;
    size_t segnum = 0;

    for(segnum = 0; segnum < numfullsegment; segnum++) {
        // send preamble
        send_mplane(0x16161616);

        // send control
        le32.u08[0] = (segnum == 0)? (uint8_t)((str.size() + PAYLOAD_PER_SEG - 1)/PAYLOAD_PER_SEG): 0;
        // TODO: how to define the destid for innolink daisychain mode
        le32.u08[1] = 0;    // dest
        le32.u08[2] = segnum & 0x7F;
        le32.u08[3] = PAYLOAD_PER_SEG;    // payload length per segment
        send_mplane(le32.u32);

        for(size_t kk = 0; kk < (PAYLOAD_PER_SEG >> 2); kk++){
            hal::u32le_t *p1data32le = (hal::u32le_t *)(&p1strbuf[pos]);
            send_mplane(p1data32le->u32);
            pos += 4;
        }
        {
            hal::u32le_t *p1data32le = (hal::u32le_t *)(&p1strbuf[pos]);
            le32.u32 = 0;
            size_t ll = 0;
            for(ll = 0; ll < (PAYLOAD_PER_SEG - ((PAYLOAD_PER_SEG >> 2) << 2)); ll++){
                le32.u08[ll] = p1data32le->u08[ll];
            }
            send_mplane(le32.u32);
            pos += ll;
        }

        // send postamble
        send_mplane(0x17171717);

        // delay
        hal::nsleep(g_mplane_delay_ms*1000000);
    }
    if(lastsegsize > 0){
        // send preamble
        send_mplane(0x16161616);

        // send control
        le32.u08[0] = (segnum == 0)? (uint8_t)((str.size() + PAYLOAD_PER_SEG - 1)/PAYLOAD_PER_SEG): 0;
        // TODO: how to define the destid for innolink daisychain mode
        le32.u08[1] = 0;    // dest
        le32.u08[2] = 0x80 | (segnum & 0x7F); // last segment | segnum
        le32.u08[3] = lastsegsize;    // payload length per segment
        send_mplane(le32.u32);

        size_t kk = 0;
        for( ; kk < (lastsegsize >> 2); kk++){
            hal::u32le_t *p1data32le = (hal::u32le_t *)(&p1strbuf[pos]);
            send_mplane(p1data32le->u32);
            pos += 4;
        }
        {
            hal::u32le_t *p1data32le = (hal::u32le_t *)(&p1strbuf[pos]);
            le32.u32 = 0;
            size_t ll = 0;
            for(ll = 0; ll < (lastsegsize - ((lastsegsize >> 2) << 2)); ll++){
                le32.u08[ll] = p1data32le->u08[ll];
            }
            send_mplane(le32.u32);
            pos += ll;
            kk++;
        }
        for( ; kk < ((PAYLOAD_PER_SEG + 1) >> 2); kk++){
            send_mplane(0);
        }

        // send postamble
        send_mplane(0x17171717);

        // delay
        hal::nsleep(g_mplane_delay_ms*1000000);
    }
} // end of void innolink_hal::send_mplane(..)

void innolink_hal::oneshot_syncin()
{
    if(get_minor_pl_version() >= 34){
        // TODO: mutex_lock is required 
        this->syncin_ctrl_shadow &= (~(1 << SYNCIN_CTRL_REG__ONESHOT_SYNCIN_SHL));

        uint32_t val = this->syncin_ctrl_shadow | (1 << SYNCIN_CTRL_REG__ONESHOT_SYNCIN_SHL);
        il_ctrl.wr32(SYNCIN_CTRL_REG, val, /*verbose=*/true);
    }
}

void innolink_hal::enable_syncin(bool enable)
{
    if(get_minor_pl_version() >= 34){
        // TODO: mutex_lock is required 
        this->syncin_ctrl_shadow &= (~(1 << SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL));
        this->syncin_ctrl_shadow |= ((enable? 1: 0) << SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL);

        il_ctrl.wr32(SYNCIN_CTRL_REG, this->syncin_ctrl_shadow, /*verbose=*/ true);
        TRACE0() << "SYNCIN " << (enable? "enable" : "disable") << std::endl;
    }
    else if((get_minor_pl_version() >= 32) && (get_minor_pl_version() < 34)){
        il_ctrl.wr32(SYNCIN_CTRL_REG, (enable? 1: 0) << SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL, /*verbose=*/ true);
        TRACE0() << "SYNCIN " << (enable? "enable" : "disable") << std::endl;
    }
}

void innolink_hal::enable_dl_uplane(bool enable)
{
    //TRACE0() << "minor_pl_version=" << get_minor_pl_version() << std::endl;
    if(get_minor_pl_version() >= 34){
        // TODO: mutex_lock is required 
        this->syncin_ctrl_shadow &= (~(1 << SYNCIN_CTRL_REG__ENABLE_UPLANE_SHL));
        this->syncin_ctrl_shadow |= ((enable? 1: 0) << SYNCIN_CTRL_REG__ENABLE_UPLANE_SHL);

        il_ctrl.wr32(SYNCIN_CTRL_REG, this->syncin_ctrl_shadow, /*verbose=*/ true);
        TRACE0() << "DL_UPLANE " << (enable? "enable" : "disable") << std::endl;
    }
}

void innolink_hal::enable_syncin_dl_uplane(bool enable)
{
    if(get_minor_pl_version() >= 34){
        // TODO: mutex_lock is required 
        this->syncin_ctrl_shadow &= (~(1 << SYNCIN_CTRL_REG__ENABLE_UPLANE_SHL));
        this->syncin_ctrl_shadow &= (~(1 << SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL));
        this->syncin_ctrl_shadow |= ((enable? 1: 0) << SYNCIN_CTRL_REG__ENABLE_UPLANE_SHL);
        this->syncin_ctrl_shadow |= ((enable? 1: 0) << SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL);

        il_ctrl.wr32(SYNCIN_CTRL_REG, this->syncin_ctrl_shadow, /*verbose=*/ true);
        TRACE0() << "SYNCIN & DL_UPLANE " << (enable? "enable" : "disable") << std::endl;
    }
}

void innolink_hal::enable_ul_axi_stream(bool enable)
{
    //TRACE0() << "minor_pl_version=" << get_minor_pl_version() << std::endl;
    if(get_minor_pl_version() >= 33){
        //TRACE0() << "enable=" << enable << std::endl;
        uint32_t    value = (enable? 1: 0) << IL_UL_AXISTREAM_CTRL_REG__ENABLE_UL_AXISTREAM_SHL;

        il_ctrl.wr32(IL_UL_AXISTREAM_CTRL_REG, value);
    }
}

void innolink_hal::enable_sys10ms_interrupt(bool enable)
{
    if(get_minor_pl_version() >= 33){
        uint32_t value = (enable? 1: 0) << IL_INTERRUPT_CTRL_REG__ENABLE_SYS10MS_SHL;

        il_ctrl.wr32(IL_INTERRUPT_CTRL_REG, value);
    }
}

#if   defined(__aarch64__)    // for real platform
uint32_t innolink_hal::recv_mplane()
{
    uint32_t data = il_ctrl.rd32(IL_MPLANE_DATA_RW_REG);

    if(data == 0x16161616) rx_word_index = 0;
    if(g_debug_mask & (1UL << MASKBIT_MPLANE_HEX)){
        std::cerr << "rx[" << std::dec << std::setw(3) << std::setfill('0') << rx_word_index << "]:0x"
            << std::hex << std::setw(8) << std::setfill('0') << data << std::endl;
    }

    rx_word_index++;
    return(data);
} // end of uint32_t innolink_hal::recv_mplane()
#endif

#if   defined(__aarch64__)    // for real platform
size_t innolink_hal::poll_rxfifo_level()
{
    return((il_ctrl.rd32(IL_MPLANE_FIFO_LEVEL_REG) & 0x1ff0000) >> 16);
} // end of size_t innolink_hal::get_rxfifo_level()
#endif

#if   defined(__aarch64__)    // for real platform
void innolink_hal::select_il(int dl_serdes_idx, int ul_serdes_idx)
{
    if(get_minor_pl_version() >= 36){
        this->il_ctrl.wr32(IL_SEL_REG,
                           ((dl_serdes_idx << IL_SEL_REG__IL_DL_MUX_SEL_SHL) |
                            (ul_serdes_idx << IL_SEL_REG__IL_UL_MUX_SEL_SHL)),
                           this->verbose);
    }
}
#endif
