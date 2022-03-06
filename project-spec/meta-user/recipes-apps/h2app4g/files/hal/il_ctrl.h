#pragma once
#include <stdint.h>

struct hw_il_ctrl {
    uint32_t                       version;
    uint32_t                       scratch;
    uint8_t                        tx_ctrl_fifo_fill_level;
    uint8_t                        rx_ctrl_fifo_fill_level;
};
