#ifndef _RX_CARRIER_HPP_
#define _RX_CARRIER_HPP_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "stm.h"

class rx_carrier : public stm 
{
    public:
        enum _state{
            NOT_OPERATIONAL_S,  // object is available, but not in operation
            PRE_OPERATIONAL_S,  // preparing for operation 
            OPERATIONAL_S,      // fully functional
            DEGRADED_S,         // operates with degraded performance
            FAILED_S,           // faulty and not operating
            DISABLED_S,         // unavailabe due to either unimplementation or not-equipped HW
            NUM_STATE
        };

    public:
        rx_carrier();
        virtual ~rx_carrier();

        std::string get_fst_str() const;
        std::string get_fst_str(size_t state) const;
        void set_obj_id(const std::string &obj_id);
        std::string get_obj_id() const;
        void install_state_handler(size_t state,
                                   void (rx_carrier::*entry_func)(),
                                   size_t (rx_carrier::*state_func)(void *, int),
                                   void (rx_carrier::*exit_func)());

    public:
        int chan_bw;
        double fs;
        bool is_tdd;
        double rx_freq;
        int tdd_ul_dl_config;
        int tdd_ssf_config;
        double dl_ul_periodicity;
        int nrofdl_slots;
        int nrofdl_symbols;
        int nroful_slots;
        int nroful_symbols;

    protected:
        void not_operational_entry();
        void pre_operational_entry();
        void operational_entry();

    protected:
        std::vector<std::string> fst_str;
        std::string objid;
};

#endif // _RX_CARRIER_HPP_

