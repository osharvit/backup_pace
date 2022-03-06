#ifndef _IL_THREAD_HPP_
#define _IL_THREAD_HPP_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>          // std::stringstream
#include "thread_base.h"
#include "stm.h"
#include "hswi_json.h"      // hswi_json
#include "axigpio_hal.h"    // class axigpio_hal
#include "innolink_hal.h"   // class innolink_hal
#include "h2top_hal.h"      // class h2top_hal
#include "il_mplane_deframer.h"// class il_mplane_deframer
#if     defined(__x86_64__) // for simulation only
#include "rx_carrier.h"     // class rx_carrier
#include "tx_carrier.h"     // class tx_carrier
#endif


// Innophase Hermes Radio Protocol
class il_thread : public thread_base, public stm, public hswi_json
{
    public:
        enum _state{
            IL_INIT_S,
            IL_BIST_S,
            IL_READY_S,
            NUM_IL_STATE
        };

    public:
        il_thread(const std::string &name,
                  size_t id,
                  h2top_hal (&h2top_ctrl)[NUM_HERMES]);
        virtual ~il_thread();

        void start_il_thread();
        static void *start_wrapper(void *p1arg);

    protected:
        void run_il_thread();
        static void poll_timer_wrapper(void *p1arg);

        void install_state_handler(size_t state,
                                   void (il_thread::*entry_func)(),
                                   size_t (il_thread::*state_func)(void *, int),
                                   void (il_thread::*exit_func)());

        void il_init_entry();
        size_t il_init_state(void *p1arg, int event);
        void il_init_exit();

        void il_bist_entry();
        size_t il_bist_state(void *p1arg, int event);
        void il_bist_exit();

        void il_ready_entry();
        size_t il_ready_state(void *p1arg, int event);
        void il_ready_exit();

        void il_loopback_entry();
        size_t il_loopback_state(void *p1arg, int event);
        void il_loopback_exit();

        void process_il_mplane_event();
        void process_itc_event();
        void setup_hswi_handler();

        void process_itc_kickoff();
        void process_version_req();

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

#if defined(__x86_64__) // for simulation only
        //---------------------------------------------------------------------
        // re:0
        //---------------------------------------------------------------------
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

        void tx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        void rx_carrier_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state);
        static void tx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);
        static void rx_carrier_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state);

    protected:
        std::vector<int> tx_carrier_lte_vec;
        std::vector<int> rx_carrier_lte_vec;
        std::vector<int> tx_carrier_nr_vec;
        std::vector<int> rx_carrier_nr_vec;

        tx_carrier txcarrierobj[NUM_CARRIER];
        rx_carrier rxcarrierobj[NUM_CARRIER];
#endif
    public:
        rapidjson::Document nvreg_json;

    protected:
        const size_t innolinkid;
        h2top_hal       (&p1h2top)[NUM_HERMES];

        std::string p1residual[NUM_INNOLINK];
        size_t transactioncnt;
        il_mplane_deframer p1deframer[NUM_INNOLINK];

    private:
        il_thread();
}; // end of class il_thread

#endif // _IL_THREAD_HPP_
