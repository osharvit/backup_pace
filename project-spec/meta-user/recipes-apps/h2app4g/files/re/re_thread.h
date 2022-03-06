#ifndef _RE_THREAD_H_
#define _RE_THREAD_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getpid()
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "thread_base.h"// class thread_base
#include "stm.h"        // class stm
#include "hswi_json.h"  // class hswi_json
#include "tx_carrier.h" // class tx_carrier
#include "rx_carrier.h" // class rx_carrier
#include "wdt_ctrl.h"   // class wdt_ctrl

class re_thread : public thread_base, public stm, public hswi_json
{
    public:
        enum _state{
            RE_INIT_S,
            RE_FWLOAD_S,
            RE_INIT_INNOLINK_S,
            RE_INIT_SYNC_S,
            RE_READY_S,
            NUM_RE_STATE
        };

    public:
        re_thread(const std::string &name,
                  size_t id,
                  wdt_ctrl &watchdog);
        virtual ~re_thread();

        void start_re_thread();
        static void *start_wrapper(void *p1arg);

    protected:
        static void poll_timer_wrapper(void *p1arg);
        static void kick_timer_wrapper(void *p1arg);
        void run_re_thread();

        void install_state_handler(size_t state,
                                   void (re_thread::*entry_func)(),
                                   size_t (re_thread::*state_func)(void *, int),
                                   void (re_thread::*exit_func)());

        void read_nvdata(const std::string &file_name);

        void re_init_entry();
        size_t re_init_state(void *p1arg, int event);
        void re_init_exit();

        void re_fwload_entry();
        size_t re_fwload_state(void *p1arg, int event);
        void re_fwload_exit();

        void re_initinnolink_entry();
        size_t re_initinnolink_state(void *p1arg, int event);
        void re_initinnolink_exit();

        void re_sync_entry();
        size_t re_sync_state(void *p1arg, int event);
        void re_sync_exit();

        void re_ready_entry();
        size_t re_ready_state(void *p1arg, int event);
        void re_ready_exit();

        void send_hswi_msg_to_thread(int dst_thread_id,
                                     const size_t uid,
                                     std::string type,
                                     std::string primitive,
                                     std::string objid,
                                     rapidjson::Value &itc_node,
                                     rapidjson::Document &itcdoc,
                                     rapidjson::Document::AllocatorType& itc_at,
                                     uint16_t msg_id=ITC_PAYLOAD_ID);

        void process_itc_event();
        void setup_hswi_handler();
        void setup_hswi_param_handler();
        bool find_tm_files(std::vector<std::string> &vlist);

        void process_itc_kickoff();

        //---------------------------------------------------------------------
        // re:0
        //---------------------------------------------------------------------
        bool get_param_req__re(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at,
                               size_t item_idx = 0,
                               size_t num_item = 1);
        bool get_parame_req__re(const rapidjson::Value& req_node,
                                const std::string& node_key,
                                const int &inst_id,
                                const size_t uid,
                                rapidjson::Value &rsp_node,
                                rapidjson::Document::AllocatorType &rsp_at,
                                size_t item_idx = 0,
                                size_t num_item = 1);
        bool reset_req__re(const rapidjson::Value& req_node,
                           const std::string& node_key,
                           const int &inst_id,
                           const size_t uid,
                           rapidjson::Value &rsp_node,
                           rapidjson::Document::AllocatorType &rsp_at,
                           size_t item_idx = 0,
                           size_t num_item = 1);
        bool resync_req__re(const rapidjson::Value& req_node,
                            const std::string& node_key,
                            const int &inst_id,
                            const size_t uid,
                            rapidjson::Value &rsp_node,
                            rapidjson::Document::AllocatorType &rsp_at,
                            size_t item_idx = 0,
                            size_t num_item = 1);
        bool resync_rsp__re(const rapidjson::Value& req_node,
                            const std::string& node_key,
                            const int &inst_id,
                            const size_t uid,
                            rapidjson::Value &rsp_node,
                            rapidjson::Document::AllocatorType &rsp_at,
                            size_t item_idx = 0,
                            size_t num_item = 1);
        bool get_swver_req__re(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at,
                               size_t item_idx = 0,
                               size_t num_item = 1);
        bool prepare_swupdate_req__re(const rapidjson::Value& req_node,
                                      const std::string& node_key,
                                      const int &inst_id,
                                      const size_t uid,
                                      rapidjson::Value &rsp_node,
                                      rapidjson::Document::AllocatorType &rsp_at,
                                      size_t item_idx = 0,
                                      size_t num_item = 1);
        bool activate_sw_req__re(const rapidjson::Value& req_node,
                                 const std::string& node_key,
                                 const int &inst_id,
                                 const size_t uid,
                                 rapidjson::Value &rsp_node,
                                 rapidjson::Document::AllocatorType &rsp_at,
                                 size_t item_idx = 0,
                                 size_t num_item = 1);
        bool restarted_ind__re(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at,
                               size_t item_idx = 0,
                               size_t num_item = 1);
        bool resynchronized_ind__re(const rapidjson::Value& req_node,
                               const std::string& node_key,
                               const int &inst_id,
                               const size_t uid,
                               rapidjson::Value &rsp_node,
                               rapidjson::Document::AllocatorType &rsp_at,
                               size_t item_idx = 0,
                               size_t num_item = 1);
        bool modify_param_req__re_debug_mask(const rapidjson::Value& req_node,
                                             const std::string& node_key,
                                             const int &inst_id,
                                             const size_t uid,
                                             rapidjson::Value &rsp_node,
                                             rapidjson::Document::AllocatorType &rsp_at);
        bool modify_param_req__fpga(const rapidjson::Value& req_node,
                                    const std::string& node_key,
                                    const int &inst_id,
                                    const size_t uid,
                                    rapidjson::Value &rsp_node,
                                    rapidjson::Document::AllocatorType &rsp_at,
                                    size_t item_idx = 0,
                                    size_t num_item = 1);

        void re_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        static void re_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);

        //-------------------------------------------------------------------------------
        // inventory:n
        //-------------------------------------------------------------------------------
        bool load_param_req__inventory(const rapidjson::Value& req_node,
                                       const std::string& node_key,
                                       const int &inst_id,
                                       const size_t uid,
                                       rapidjson::Value &rsp_node,
                                       rapidjson::Document::AllocatorType &rsp_at,
                                       size_t item_idx = 0,
                                       size_t num_item = 1);
        bool get_param_req__inventory(const rapidjson::Value& req_node,
                                      const std::string& node_key,
                                      const int &inst_id,
                                      const size_t uid,
                                      rapidjson::Value &rsp_node,
                                      rapidjson::Document::AllocatorType &rsp_at,
                                      size_t item_idx = 0,
                                      size_t num_item = 1);

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

        //---------------------------------------------------------------------
        // section:n
        //---------------------------------------------------------------------
        bool create_obj_req__section(const rapidjson::Value& req_node,
                                     const std::string& node_key,
                                     const int &inst_id,
                                     const size_t uid,
                                     rapidjson::Value &rsp_node,
                                     rapidjson::Document::AllocatorType &rsp_at,
                                     size_t item_idx = 0,
                                     size_t num_item = 1);

        //---------------------------------------------------------------------
        // tm_file:n
        //---------------------------------------------------------------------
        // NOTE: obsolete
        bool download_tm_file_req__tm_file(const rapidjson::Value& req_node,
                                           const std::string& node_key,
                                           const int &inst_id,
                                           const size_t uid,
                                           rapidjson::Value &rsp_node,
                                           rapidjson::Document::AllocatorType &rsp_at,
                                           size_t item_idx = 0,
                                           size_t num_item = 1);
        bool download_file_req__tm_file(const rapidjson::Value& req_node,
                                        const std::string& node_key,
                                        const int &inst_id,
                                        const size_t uid,
                                        rapidjson::Value &rsp_node,
                                        rapidjson::Document::AllocatorType &rsp_at,
                                        size_t item_idx = 0,
                                        size_t num_item = 1);

        //---------------------------------------------------------------------
        // tm_playback:n
        //---------------------------------------------------------------------
        bool get_param_req__tm_filelist(const rapidjson::Value& req_node,
                                        const std::string& node_key,
                                        const int &inst_id,
                                        const size_t uid,
                                        rapidjson::Value &rsp_node,
                                        rapidjson::Document::AllocatorType &rsp_at,
                                        size_t item_idx = 0,
                                        size_t num_item = 1);
        bool get_param_req__tm_playback(const rapidjson::Value& req_node,
                                        const std::string& node_key,
                                        const int &inst_id,
                                        const size_t uid,
                                        rapidjson::Value &rsp_node,
                                        rapidjson::Document::AllocatorType &rsp_at,
                                        size_t item_idx = 0,
                                        size_t num_item = 1);
        bool create_obj_req__tm_playback(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx = 0,
                                         size_t num_item = 1);
        bool delete_obj_req__tm_playback(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx = 0,
                                         size_t num_item = 1);

        //---------------------------------------------------------------------
        // frc_capture:n
        //---------------------------------------------------------------------
        bool get_param_req__frc_capture(const rapidjson::Value& req_node,
                                        const std::string& node_key,
                                        const int &inst_id,
                                        const size_t uid,
                                        rapidjson::Value &rsp_node,
                                        rapidjson::Document::AllocatorType &rsp_at,
                                        size_t item_idx = 0,
                                        size_t num_item = 1);
        bool create_obj_req__frc_capture(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx = 0,
                                         size_t num_item = 1);
        bool delete_obj_req__frc_capture(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx = 0,
                                         size_t num_item = 1);

        //---------------------------------------------------------------------
        // frc_file:n
        //---------------------------------------------------------------------
        bool capture_file_req__frc_file(const rapidjson::Value& req_node,
                                        const std::string& node_key,
                                        const int &inst_id,
                                        const size_t uid,
                                        rapidjson::Value &rsp_node,
                                        rapidjson::Document::AllocatorType &rsp_at,
                                        size_t item_idx = 0,
                                        size_t num_item = 1);
        bool upload_file_req__frc_file(const rapidjson::Value& req_node,
                                       const std::string& node_key,
                                       const int &inst_id,
                                       const size_t uid,
                                       rapidjson::Value &rsp_node,
                                       rapidjson::Document::AllocatorType &rsp_at,
                                       size_t item_idx = 0,
                                       size_t num_item = 1);


        //---------------------------------------------------------------------
        // carrier common
        //---------------------------------------------------------------------
        void tx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        void rx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        static void tx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);
        static void rx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);

        //---------------------------------------------------------------------
        // tx_carrier_lte:n
        //---------------------------------------------------------------------
        bool delete_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool create_obj_req__tx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool create_obj_rsp__tx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool modify_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool get_param_req__tx_carrier_lte(const rapidjson::Value& req_node,
                                           const std::string& node_key,
                                           const int &inst_id,
                                           const size_t uid,
                                           rapidjson::Value &rsp_node,
                                           rapidjson::Document::AllocatorType &rsp_at,
                                           size_t item_idx = 0,
                                           size_t num_item = 1);
        bool modify_state_req__tx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool modify_state_rsp__tx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool state_change_ind__tx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);

        //---------------------------------------------------------------------
        // rx_carrier_lte:n
        //---------------------------------------------------------------------
        bool delete_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool create_obj_req__rx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool create_obj_rsp__rx_carrier_lte(const rapidjson::Value& req_node,
                                            const std::string& node_key,
                                            const int &inst_id,
                                            const size_t uid,
                                            rapidjson::Value &rsp_node,
                                            rapidjson::Document::AllocatorType &rsp_at,
                                            size_t item_idx = 0,
                                            size_t num_item = 1);
        bool modify_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool get_param_req__rx_carrier_lte(const rapidjson::Value& req_node,
                                           const std::string& node_key,
                                           const int &inst_id,
                                           const size_t uid,
                                           rapidjson::Value &rsp_node,
                                           rapidjson::Document::AllocatorType &rsp_at,
                                           size_t item_idx = 0,
                                           size_t num_item = 1);
        bool modify_state_req__rx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool modify_state_rsp__rx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);
        bool state_change_ind__rx_carrier_lte(const rapidjson::Value& req_node,
                                              const std::string& node_key,
                                              const int &inst_id,
                                              const size_t uid,
                                              rapidjson::Value &rsp_node,
                                              rapidjson::Document::AllocatorType &rsp_at,
                                              size_t item_idx = 0,
                                              size_t num_item = 1);

    protected:
        wdt_ctrl &watchdogctrl;
        rapidjson::Document inv_json;
        std::string product_id;
        std::string serial_number;
        std::string hw_version;

        std::vector<int> tx_carrier_lte_vec;
        std::vector<int> rx_carrier_lte_vec;
        std::vector<int> tx_carrier_nr_vec;
        std::vector<int> rx_carrier_nr_vec;

        tx_carrier txcarrierobj[NUM_CARRIER];
        rx_carrier rxcarrierobj[NUM_CARRIER];

        int num_sectionobj;
        struct section_struct sectionobj[MAX_SECTION];

        itc_playback_req        tmplaybackreq;

        size_t fpga_major_version;
        size_t fpga_minor_version;

        int phase;       // -1: for None, 0: for P1, 1: for P2
        int antport_on_reset;   // 0: antport-A, 1: antport-B
        int dl_il_sel;          // 0: innolink0, 1: innolink1
        int ul_il_sel;          // 0: innolink0, 1: innolink1
        int n_dpa;
}; // end of class re_thread

#endif // _RE_THREAD_H_
