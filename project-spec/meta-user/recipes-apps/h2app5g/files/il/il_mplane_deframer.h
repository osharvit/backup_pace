#ifndef _IL_MPLANE_DEFRAMER_HPP_
#define _IL_MPLANE_DEFRAMER_HPP_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>          // std::stringstream
#include "stm.h"
#include "hal.h"            // class hal

class   il_mplane_deframer: public stm
{
    public:
        enum _state{
            INNOMP_HUNT_S,
            INNOMP_HEADER_S,    // header of segment
            INNOMP_SDU_S,
            INNOMP_EOS_S,       // end of segment
            INNOMP_EOP_S,       // end of packet
            NUM_INNOMP_STATE
        };

        il_mplane_deframer();
        virtual ~il_mplane_deframer();

        size_t get_data_and_hunt(uint32_t curr_data);

        void install_state_handler(size_t state,
                                   void (il_mplane_deframer::*entry_func)(),
                                   size_t (il_mplane_deframer::*state_func)(void *, int),
                                   void (il_mplane_deframer::*exit_func)());

        void il_mplane_hunt_entry();
        size_t il_mplane_hunt_state(void *p1arg, int event);
        void il_mplane_hunt_exit();

        void il_mplane_header_entry();
        size_t il_mplane_header_state(void *p1arg, int event);
        void il_mplane_header_exit();

        void il_mplane_sdu_entry();
        size_t il_mplane_sdu_state(void *p1arg, int event);
        void il_mplane_sdu_exit();

        void il_mplane_eos_entry();
        size_t il_mplane_eos_state(void *p1arg, int event);
        void il_mplane_eos_exit();

        void il_mplane_eop_entry();
        size_t il_mplane_eop_state(void *p1arg, int event);
        void il_mplane_eop_exit();

    protected:
        int msg_byte_len;
        hal::u32le_t    recvdata;
        hal::u64le_t    recv64;
        int offset;
        size_t segment_seq;
        bool is_last_segment;
};

#endif // _IL_MPLANE_DEFRAMER_HPP_
