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
#include "il_ctrl.h"        // innolink control
#include "innolink_dig.h"   // innolink digital
#include "innolink_hal.h"   // innolink_hal
#include "xil_gth.h"        // xilink GTH SerDes IP


#if defined(__x86_64__) // for simulation only
void innolink_hal::send_mplane(uint32_t data)
{
    simfifo.push(data);
} // end of void innolink_hal::send_mplane(..)
#endif

#if defined(__x86_64__) // for simulation only
uint32_t innolink_hal::recv_mplane()
{
    uint32_t data = (simfifo.empty())? 0x16161616: simfifo.front();    // preamble or data
    simfifo.pop();

    if(data == 0x16161616) rx_word_index = 0;
    if(g_debug_mask & (1UL << MASKBIT_CP_HEX)){
        std::cerr << "rx[" << std::dec << std::setw(3) << std::setfill('0') << rx_word_index << "]:0x"
            << std::hex << std::setw(8) << std::setfill('0') << data << std::endl;
    }

    rx_word_index++;
    return(data);
} // end of uint32_t innolink_hal::recv_mplane()
#endif

#if defined(__x86_64__) // for simulation only
size_t innolink_hal::poll_rxfifo_level()
{
    return(simfifo.size());
} // end of size_t innolink_hal::get_rxfifo_level()
#endif
