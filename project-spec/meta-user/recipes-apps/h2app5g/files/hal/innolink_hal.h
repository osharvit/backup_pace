#ifndef __INNOLINK_HAL_H__
#define __INNOLINK_HAL_H__

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
#if defined(__x86_64__) // for simulation only
#include <queue>            // std::queue
#endif
#include "mmap_hal.h"       // class mmap_hal
#include "offs_mmap_hal.h"  // class offs_mmap_hal
#include "il_ctrl.h"        // innolink control
#include "innolink_dig.h"   // innolink digital
#include "xil_gth.h"        // xilink GTH SerDes IP
#include "rapidjson_wrapper.h"

class   innolink_hal
{
    public:
        innolink_hal(const std::string &name,
                     size_t innolink_id,
                     uint64_t ildig_addr_base, uint64_t ildig_offs_mask,
                     uint64_t gth_addr_base, uint64_t gth_offs_mask,
                     size_t wordsize_per_segment);
        virtual ~innolink_hal();

        void initialize();
        std::string objname() const;

        uint32_t read_pl_version();
        void set_pl_version(uint32_t version);
        uint16_t get_major_pl_version() const;
        uint16_t get_minor_pl_version() const;
        std::string get_verstr();
        bool bist();    // built-in self test
        bool bist_data_bus();    // built-in self test
        bool bist_fifo_depth(size_t run);// built-in self test
        void set_sysclk_enable(bool enable=true);
        void reset_serdes();
        void reset_serdes(bool loopback_enable);

        void send_mplane(uint32_t data);
        void send_mplane(const std::string &str);
        uint32_t recv_mplane();
        size_t poll_rxfifo_level();

        void oneshot_syncin();
        void enable_syncin(bool enabled);
        void enable_dl_uplane(bool enable);
        void enable_syncin_dl_uplane(bool enable);
        void enable_ul_axi_stream(bool enable);
        void enable_sys10ms_interrupt(bool enable);

        void select_il(int dl_serdes_idx, int ul_serdes_idx);

    protected:
        const std::string obj_name;
        const uint64_t ILDIG_ADDR_BASE;
        const uint64_t ILDIG_OFFS_MASK;
        const uint64_t GTH_ADDR_BASE;
        const uint64_t GTH_OFFS_MASK;
        const size_t innolinkid;
        const size_t WORDSIZE_PER_SEG;
        const size_t PAYLOAD_PER_SEG;
        size_t tx_word_index;
        size_t rx_word_index;
        uint32_t    syncin_ctrl_shadow;
#if defined(__x86_64__) // for simulation only
        std::queue<uint32_t>    simfifo;
#endif

        bool loopback;
        bool verbose;

    public:
        offs_mmap_hal il_ctrl;
        offs_mmap_hal xil_gth;
        hal::u32le_t    pl_version;

    private:
        innolink_hal();  // NOTE: block to use the default constructor
};

#endif // __INNOLINK_HAL_H__
