#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "rx_carrier.h"

rx_carrier::rx_carrier():
    stm(NUM_STATE)
{

    chan_bw = 20;
    fs = 30.72;
    is_tdd = true;
    rx_freq = 2350.0;
    tdd_ul_dl_config = 3;
    tdd_ssf_config = 8;

    for(size_t kk = 0; kk < NUM_STATE; kk++){
        switch(kk){
            case NOT_OPERATIONAL_S:{
                fst_str.push_back("not_operational");
                break;
            }
            case PRE_OPERATIONAL_S:{
                fst_str.push_back("pre_operational");
                break;
            }
            case OPERATIONAL_S: {
                fst_str.push_back("operational");
                break;
            }
            case DEGRADED_S: {
                fst_str.push_back("degraded");
                break;
            }
            case FAILED_S: {
                fst_str.push_back("failed");
                break;
            }
            case DISABLED_S: {
                fst_str.push_back("disabled");
                break;
            }
            default:{
                fst_str.push_back("");
            }
        }
    }

    install_state_handler(NOT_OPERATIONAL_S,
                          &rx_carrier::not_operational_entry,
                          &rx_carrier::default_state,
                          &rx_carrier::default_exit);
    install_state_handler(PRE_OPERATIONAL_S,
                          &rx_carrier::pre_operational_entry,
                          &rx_carrier::default_state,
                          &rx_carrier::default_exit);
    install_state_handler(OPERATIONAL_S,
                          &rx_carrier::operational_entry,
                          &rx_carrier::default_state,
                          &rx_carrier::default_exit);
}

rx_carrier::~rx_carrier() {}

std::string rx_carrier::get_fst_str() const
{
    return(fst_str[this->get_curr_state()]);
};

std::string rx_carrier::get_fst_str(size_t state) const
{
    return((state < NUM_STATE)? fst_str[state] : std::string(""));
};

void rx_carrier::install_state_handler(size_t state,
                                       void (rx_carrier::*entry_func)(),
                                       size_t (rx_carrier::*state_func)(void *, int),
                                       void (rx_carrier::*exit_func)())
{
    if(state < NUM_STATE) {
        stm::install_state_handler(state,
                                   static_cast<void (stm::*)()>(entry_func),
                                   static_cast<size_t (stm::*)(void *, int)>(state_func),
                                   static_cast<void (stm::*)()>(exit_func));
    }
} // end of rx_carrier::install_state_handler(...)

void rx_carrier::not_operational_entry()
{
    // TODO: intentionally block the automatic state_changed_callback
    // for the NOT_OPERATIONAL_S, PRE_OPERATIONAL_S, OPERATIONAL_S
    stm::default_entry();
} // end of void rx_carrier::not_operational_entry()

void rx_carrier::pre_operational_entry()
{
    // TODO: intentionally block the automatic state_changed_callback
    // for the NOT_OPERATIONAL_S, PRE_OPERATIONAL_S, OPERATIONAL_S
    stm::default_entry();

    // bypass to the OPERATIONAL_S state
    transit_state(OPERATIONAL_S, true);
} // end of void rx_carrier::pre_operational_entry()

void rx_carrier::operational_entry()
{
    // TODO: intentionally block the automatic state_changed_callback
    // for the NOT_OPERATIONAL_S, PRE_OPERATIONAL_S, OPERATIONAL_S
    stm::default_entry();
} // end of void rx_carrier::operational_entry()

void rx_carrier::set_obj_id(const std::string &obj_id)
{
    this->objid = obj_id;
}

std::string rx_carrier::get_obj_id() const
{
    return(this->objid);
}

