#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>         // memcpy()
#include <string>
#include <iostream>         // std::cout
#include <sstream>          // std::stringstream
#include "il_mplane_deframer.h"
#include "hal.h"            // hal::u32le_t

il_mplane_deframer::il_mplane_deframer():
    stm(NUM_INNOMP_STATE)
{
    this->msg_byte_len = 0;
    this->recv64.u64 = 0;
    offset = 0;
    this->segment_seq = 0;
    this->is_last_segment = false;

    this->install_state_handler(INNOMP_HUNT_S,
                                &il_mplane_deframer::il_mplane_hunt_entry,
                                &il_mplane_deframer::il_mplane_hunt_state,
                                &il_mplane_deframer::il_mplane_hunt_exit);
    this->install_state_handler(INNOMP_HEADER_S,
                                &il_mplane_deframer::il_mplane_header_entry,
                                &il_mplane_deframer::il_mplane_header_state,
                                &il_mplane_deframer::il_mplane_header_exit);
    this->install_state_handler(INNOMP_SDU_S,
                                &il_mplane_deframer::il_mplane_sdu_entry,
                                &il_mplane_deframer::il_mplane_sdu_state,
                                &il_mplane_deframer::il_mplane_sdu_exit);
    this->install_state_handler(INNOMP_EOS_S,
                                &il_mplane_deframer::il_mplane_eos_entry,
                                &il_mplane_deframer::il_mplane_eos_state,
                                &il_mplane_deframer::il_mplane_eos_exit);
    this->install_state_handler(INNOMP_EOP_S,
                                &il_mplane_deframer::il_mplane_eop_entry,
                                &il_mplane_deframer::il_mplane_eop_state,
                                &il_mplane_deframer::il_mplane_eop_exit);
}

il_mplane_deframer::~il_mplane_deframer()
{
}

void il_mplane_deframer::install_state_handler(size_t state,
                                            void (il_mplane_deframer::*entry_func)(),
                                            size_t (il_mplane_deframer::*state_func)(void *, int),
                                            void (il_mplane_deframer::*exit_func)())
{
    if(state < NUM_INNOMP_STATE) {
        stm::install_state_handler(state,
                                   static_cast<void (stm::*)()>(entry_func),
                                   static_cast<size_t (stm::*)(void *, int)>(state_func),
                                   static_cast<void (stm::*)()>(exit_func));
    }
} // end of il_mplane_deframer::install_state_handler(...)

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
//                                  +------+        |              |
//  destid                          | 0xTT |        |              |
//                                  +------+        |              |
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

size_t il_mplane_deframer::get_data_and_hunt(uint32_t curr_data)
{
    recvdata.u32 = curr_data;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: to find the starting position of four runs of "0x16" related to the curr_data 
    //       except the position 0 which is out of the curr_data boundary.
    //
    //         |------prev_data-----|  |------curr_data-----|
    //         0     1     2     3     4     5     6     7
    //      0: 0x16, 0x16, 0x16, 0x16, 0xZZ, 0xZZ, 0xZZ, 0xZZ => out of the curr_data boundary
    //      1: 0xZZ, 0x16, 0x16, 0x16, 0x16, 0xZZ, 0xZZ, 0xZZ
    //      2: 0xZZ, 0xZZ, 0x16, 0x16, 0x16, 0x16, 0xZZ, 0xZZ
    //      3: 0xZZ, 0xZZ, 0xZZ, 0x16, 0x16, 0x16, 0x16, 0xZZ
    //      4: 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0x16, 0x16, 0x16, 0x16
    //       : 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0x16, 0x16, 0x16 => not interestd
    //       : 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0x16, 0x16 => not interestd
    //       : 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0xZZ, 0x16 => not interestd
    //
    //       exceptional cases:
    //       - case for more than four runs of "0x16" such as 0x16,0x16,0x16,0x16,0x16
    //         => backward search
    //
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616) {
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
        else if(p1data32le->u32 == 0x17171717) {
            transit_state(INNOMP_HUNT_S);
            break;
        }
    }

    return(kk);
}

void il_mplane_deframer::il_mplane_hunt_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << this->prevstate << "->" << this->currstate << std::endl;
    }
    this->msg_byte_len = 0;
} // end of void il_mplane_deframer::il_mplane_hunt_entry()

size_t il_mplane_deframer::il_mplane_hunt_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << "data=" << std::hex << event << std::endl;
    }
#if 1
    get_data_and_hunt(event);
#else
    recvdata.u32 = (uint32_t)event;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: starting from 1
    //   1       2       3       4
    // "xx"    "xx"    "xx"    "xx"
    // "16"    "xx"    "xx"    "xx"
    // "16"    "16"    "xx"    "xx"
    // "16"    "16"    "16"    "xx"
    // "16"    "16"    "16"    "16"
    // "xx"    "16"    "16"    "16"
    // "xx"    "xx"    "16"    "16"
    // "xx"    "xx"    "xx"    "16"
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616){
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
    }
#endif

    return(this->currstate);
} // end of size_t il_mplane_hunt_entry::il_mplane_hunt_state(..)

void il_mplane_deframer::il_mplane_hunt_exit()
{
} // end of void il_mplane_deframer::il_mplane_hunt_exit()

void il_mplane_deframer::il_mplane_header_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << this->prevstate << "->" << this->currstate << std::endl;
    }
} // end of void il_mplane_deframer::il_mplane_header_entry()

size_t il_mplane_deframer::il_mplane_header_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer) << "data=" << std::hex << event << std::endl;
    }

#if 1
    size_t  kk = get_data_and_hunt(event);
#else
    recvdata.u32 = (uint32_t)event;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: starting from 1
    //   1       2       3       4
    // "xx"    "xx"    "xx"    "xx"
    // "16"    "xx"    "xx"    "xx"
    // "16"    "16"    "xx"    "xx"
    // "16"    "16"    "16"    "xx"
    // "16"    "16"    "16"    "16"
    // "xx"    "16"    "16"    "16"
    // "xx"    "xx"    "16"    "16"
    // "xx"    "xx"    "xx"    "16"
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616){
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
    }
#endif
    if(kk <= 0){
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[offset];

        is_last_segment = (p1data32le->u08[2] & 0x80)? true: false;
        size_t segnum = (p1data32le->u08[2] & 0x7F);

        this->msg_byte_len = std::min<int>(47, (int)(p1data32le->u08[3]));

        if(g_debug_mask & (1UL << MASKBIT_CALL)){
            TRACE1(il_mplane_deframer) << "msg_byte_len(dec)=" << this->msg_byte_len << std::endl;
        }
        if(this->msg_byte_len > 0){
            if(segnum == 0){
                // clear buffer
                std::string *s = static_cast<std::string *>(p1arg);
                s->clear();

                this->segment_seq = segnum;
                transit_state(INNOMP_SDU_S);
            }
            else if(segnum == (this->segment_seq + 1)){
                this->segment_seq = segnum;
                transit_state(INNOMP_SDU_S);
            }
            else{
                transit_state(INNOMP_HUNT_S);
            }
        }
        else{
            transit_state(INNOMP_HUNT_S);
        }
    }

    return(this->currstate);
} // end of size_t il_mplane_deframer::il_mplane_header_state(..)

void il_mplane_deframer::il_mplane_header_exit()
{
} // end of void il_mplane_deframer::il_mplane_header_exit()

void il_mplane_deframer::il_mplane_sdu_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << this->prevstate << "->" << this->currstate << std::endl;
    }
} // end of void il_mplane_deframer::il_mplane_sdu_entry()

size_t il_mplane_deframer::il_mplane_sdu_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << "data=" << std::hex << event << ", msg_byte_len=" << this->msg_byte_len << std::endl;
    }

#if 1
    size_t  kk = get_data_and_hunt(event);
#else
    recvdata.u32 = (uint32_t)event;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: starting from 1
    //   1       2       3       4
    // "xx"    "xx"    "xx"    "xx"
    // "16"    "xx"    "xx"    "xx"
    // "16"    "16"    "xx"    "xx"
    // "16"    "16"    "16"    "xx"
    // "16"    "16"    "16"    "16"
    // "xx"    "16"    "16"    "16"
    // "xx"    "xx"    "16"    "16"
    // "xx"    "xx"    "xx"    "16"
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616) {
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
        else if(p1data32le->u32 == 0x17171717) {
            transit_state(INNOMP_HUNT_S);
            break;
        }
    }
#endif
    if(kk <= 0){
        std::string *s = static_cast<std::string *>(p1arg);
        int  min_size = std::min<int>(this->msg_byte_len, 4);

        for(int kk = 0; kk < min_size; kk++){
            s->append((const char *)&this->recv64.u08[offset + kk], 1);
        }
        this->msg_byte_len -= min_size;

        if(this->msg_byte_len <= 0){
            if(is_last_segment){
                transit_state(INNOMP_EOP_S);
            }
            else{
                transit_state(INNOMP_EOS_S);
            }
        }
    }

    return(this->currstate);
} // end of size_t il_mplane_deframer::il_mplane_sdu_state(..)

void il_mplane_deframer::il_mplane_sdu_exit()
{
} // end of void il_mplane_deframer::il_mplane_sdu_exit()

void il_mplane_deframer::il_mplane_eos_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer);
    }
} // end of void il_mplane_deframer::il_mplane_eos_entry()

size_t il_mplane_deframer::il_mplane_eos_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << "data=" << std::hex << event << std::endl;
    }

#if 1
    size_t  kk = get_data_and_hunt(event);
#else
    recvdata.u32 = (uint32_t)event;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: starting from 1
    //   1       2       3       4
    // "xx"    "xx"    "xx"    "xx"
    // "16"    "xx"    "xx"    "xx"
    // "16"    "16"    "xx"    "xx"
    // "16"    "16"    "16"    "xx"
    // "16"    "16"    "16"    "16"
    // "xx"    "16"    "16"    "16"
    // "xx"    "xx"    "16"    "16"
    // "xx"    "xx"    "xx"    "16"
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616) {
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
        else if(p1data32le->u32 == 0x17171717) {
            transit_state(INNOMP_HUNT_S);
            break;
        }
    }
#endif
    if(kk <= 0){
        transit_state(INNOMP_HUNT_S);
    }

    return(this->currstate);
} // end of size_t il_mplane_deframer::il_mplane_eos_state(..)

void il_mplane_deframer::il_mplane_eos_exit()
{
} // end of void il_mplane_deframer::il_mplane_eos_exit()

void il_mplane_deframer::il_mplane_eop_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer);
    }
} // end of void il_mplane_deframer::il_mplane_eop_entry()

size_t il_mplane_deframer::il_mplane_eop_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE1(il_mplane_deframer)
            << "data=" << std::hex << event << std::endl;
    }

#if 1
    size_t  kk = get_data_and_hunt(event);
#else
    recvdata.u32 = (uint32_t)event;

    // shift
    this->recv64.u32le[0].u32 = this->recv64.u32le[1].u32;
    this->recv64.u32le[1].u32 = recvdata.u32;

    // NOTE: starting from 1
    //   1       2       3       4
    // "xx"    "xx"    "xx"    "xx"
    // "16"    "xx"    "xx"    "xx"
    // "16"    "16"    "xx"    "xx"
    // "16"    "16"    "16"    "xx"
    // "16"    "16"    "16"    "16"
    // "xx"    "16"    "16"    "16"
    // "xx"    "xx"    "16"    "16"
    // "xx"    "xx"    "xx"    "16"
    size_t kk=4;
    for( kk=4; kk>0; kk--) {
        hal::u32le_t *p1data32le = (hal::u32le_t *)&this->recv64.u08[kk];
        if(p1data32le->u32 == 0x16161616) {
            offset = kk;
            transit_state(INNOMP_HEADER_S);
            break;
        }
        else if(p1data32le->u32 == 0x17171717) {
            transit_state(INNOMP_HUNT_S);
            break;
        }
    }
#endif
    if(kk <= 0){
        transit_state(INNOMP_HUNT_S);
    }

    return(this->currstate);
} // end of size_t il_mplane_deframer::il_mplane_eop_state(..)

void il_mplane_deframer::il_mplane_eop_exit()
{
} // end of void il_mplane_deframer::il_mplane_eop_exit()
