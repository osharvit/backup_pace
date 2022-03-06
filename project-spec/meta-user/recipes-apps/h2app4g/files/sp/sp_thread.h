#ifndef _SP_THREAD_H_
#define _SP_THREAD_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream
#include <vector>       // std::vector
#include <list>         // std::list
#include "thread_base.h"
#include "stm.h"
#include "hswi_json.h"      // class hswi_json
#include "hal.h"            // class hal
#include "axidma_sg_hal.h"  // class axidma_sg_hal
#include "axigpio_hal.h"    // class axigpio_hal
#include "innolink_hal.h"   // class innolink_hal
#include "dma_peri_hal.h"   // class dma_peri_hal
#include "h2top_hal.h"      // class h2top_hal
//#include "blocking_interrupt.h" // class blocking_interrupt

// special purpose task: boot, fwload, resync
class sp_thread : public thread_base, public stm, public hswi_json
{
    public:
        enum _state{
            SP_IDLE_S,
            SP_FWLOAD_S,
            SP_INITSYNC_S,
            NUM_SP_STATE
        };

        static int TS_NS;
        //static const int DMA_TS_FS_X = 122880;
        // max clock count over 10ms
        static const int DMA_TS_CNT_MAX = DMA_TS_CLK_IN_KHZ*10;

    public:
        sp_thread(const std::string &name,
                  size_t id,
#ifdef  USE_SYS10MS_INTERRUPT
                  blocking_interrupt &sys10ms_interrupt,
#endif
                  h2top_hal (&h2top_ctrl)[NUM_HERMES]
                  );
        virtual ~sp_thread();

        uint64_t load_waveform(const std::string &tmwav_path, const uint64_t &addr);

        size_t gen_axidma_tmdesc(std::vector<axidma_sg_hal::axidmadesc> &dmadesclist,
                                 const playback_struct *p1playback,
                                 uint64_t data_addr,
                                 int playback_idx,
                                 int ruport_id,
                                 const std::string &tmwav_path);

        void start_sp_thread();
        static void *start_wrapper(void *p1arg);

    protected:
        void run_sp_thread();

        void install_state_handler(size_t state,
                                   void (sp_thread::*entry_func)(),
                                   size_t (sp_thread::*state_func)(void *, int),
                                   void (sp_thread::*exit_func)());
        void sp_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        static void sp_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);

        void sp_idle_entry();
        size_t sp_idle_state(void *p1arg, int event);
        void sp_idle_exit();

        void sp_fwload_entry();
        size_t sp_fwload_state(void *p1arg, int event);
        void sp_fwload_exit();

        void sp_initsync_entry();
        size_t sp_initsync_state(void *p1arg, int event);
        void sp_initsync_exit();

        bool download_tm_file(const itc_download_tm_file_req &download_tm_file_req);
        void print_tmplayback(const itc_playback_req &tmplaybackreq);
        bool prepare_tmplayback(const itc_playback_req &tmplaybackreq);

        void zeroclear_mem(uint64_t addr, uint32_t len);
        void dump_mem(uint64_t addr, uint32_t len, const char *p1dump_file_name, int truncate_bit=0);
        bool write_s2mm_axidma_desc_fdd_fs8(uint32_t section_mask,
                                            uint32_t section_target,
                                            const int num_s2mm_desc = 2,
                                            const bool verbose=false);
        bool create_capture(const itc_create_capture_req &prep_capture_req);
        bool capture_frc_file(const itc_capture_frc_file_req &upload_frc_file_req);
        bool upload_frc_file(const itc_upload_frc_file_req &upload_frc_file_req);

        std::vector<int> gen_valid_tdd_symb_list(int tdd_ul_dl_config, int tdd_ssf_config, int ul_only=0);

        void process_itc_event();
        void process_itc_kickoff();

        void setup_hswi_handler();
        //-------------------------------------------------------------------------------
        // register:n
        //-------------------------------------------------------------------------------
        bool load_param_req__register(const rapidjson::Value& req_node,
                                      const std::string& node_key,
                                      const int &inst_id,
                                      const size_t uid,
                                      rapidjson::Value &rsp_node,
                                      rapidjson::Document::AllocatorType &rsp_at,
                                      size_t item_idx = 0,
                                      size_t num_item = 1);
        bool modify_param_req__fpga(const rapidjson::Value& req_node,
                                    const std::string& node_key,
                                    const int &inst_id,
                                    const size_t uid,
                                    rapidjson::Value &rsp_node,
                                    rapidjson::Document::AllocatorType &rsp_at,
                                    size_t item_idx = 0,
                                    size_t num_item = 1);

    protected:
#ifdef  USE_SYS10MS_INTERRUPT
        blocking_interrupt &sys10msintr;
#endif
        h2top_hal       (&p1h2top)[NUM_HERMES];

        size_t fpga_major_version;
        size_t fpga_minor_version;

        int phase;       // -1: for None, 0: for P1, 1: for P2
        int antport_on_reset;   // 0: antport-A, 1: antport-B
        int dl_il_sel;          // 0: innolink0, 1: innolink1
        int ul_il_sel;          // 0: innolink0, 1: innolink1
        int n_dpa;              // 0: unset,     1~4: set

        std::string frc_file_name;

        uint32_t sdu_mask;
        uint32_t sdu_target;
        int      nums2mmdesc;
        uint8_t  num_segments;
        uint8_t  off_duty_mode;     // 0: "zeropad" | "nongated", 1: "dtx" | "gated"
        uint8_t  scs;               // 0: 15 kHz, 1: 30 kHz
        uint8_t  tdd_ul_dl_config;
        uint8_t  tdd_ssf_config;
        uint8_t  osr;

        rapidjson::Document nvreg_json;
}; // end of class sp_thread

#endif // _SP_THREAD_H_
