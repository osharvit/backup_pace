#pragma once
#include <stdint.h>

struct hw_xil_gth {
    uint32_t                       version;
    uint16_t                       reset_ctrl;
#define XIL_GTH_RESET_CTRL_RESET_ALL 0x0001
#define XIL_GTH_RESET_CTRL_RESET_TX_PLL_AND_DATAPATH 0x0002
#define XIL_GTH_RESET_CTRL_RESET_TX_DATAPATH 0x0004
#define XIL_GTH_RESET_CTRL_RESET_TX_BUFFBYPASS 0x0008
#define XIL_GTH_RESET_CTRL_RESET_TX_USER_CLOCK 0x0010
#define XIL_GTH_RESET_CTRL_RESET_RX_PLL_AND_DATAPATH 0x0020
#define XIL_GTH_RESET_CTRL_RESET_RX_DATAPATH 0x0040
#define XIL_GTH_RESET_CTRL_RESET_RX_BUFFBYPASS 0x0080
#define XIL_GTH_RESET_CTRL_RESET_RX_USER_CLOCK 0x0100
    uint8_t                        loopback;
    uint16_t                       xcvr_status;
#define XIL_GTH_XCVR_STATUS_GTPOWERGOOD 0x0001
#define XIL_GTH_XCVR_STATUS_TXPMARESETDONE 0x0002
#define XIL_GTH_XCVR_STATUS_TXPRGDIVRESETDONE 0x0004
#define XIL_GTH_XCVR_STATUS_RESET_TX_DONE 0x0008
#define XIL_GTH_XCVR_STATUS_BUFFBYPASS_TX_DONE_OUT 0x0010
#define XIL_GTH_XCVR_STATUS_BUFFBYPASS_TX_ERROR_OUT 0x0020
#define XIL_GTH_XCVR_STATUS_TX_CLOCK_ACTIVE 0x0040
#define XIL_GTH_XCVR_STATUS_RESET_RX_CDR_STABLE 0x0080
#define XIL_GTH_XCVR_STATUS_RXPMARESETDONE 0x0100
#define XIL_GTH_XCVR_STATUS_RESET_RX_DONE 0x0200
#define XIL_GTH_XCVR_STATUS_BUFFBYPASS_RX_DONE_OUT 0x0400
#define XIL_GTH_XCVR_STATUS_BUFFBYPASS_RX_ERROR_OUT 0x0800
#define XIL_GTH_XCVR_STATUS_RX_CLOCK_ACTIVE 0x1000
    uint32_t                       test_reg0;
    uint32_t                       test_reg1;
    uint32_t                       test_reg2;
    uint32_t                       test_reg3;
    uint32_t                       flip_control;
    uint32_t                       tx_slip_wait;
    uint32_t                       if_loopback;
};
