#pragma once
#include <stdint.h>

struct hw_innolink_dig {
    uint8_t                        enable;
    uint8_t                        sw_resetb;
    uint8_t                        farend_lpbk_en;
    uint8_t                        speed_mode;
    uint8_t                        rx_debug_model_mode;
    uint8_t                        tx_debug_model_mode;
    struct hw_innolink_dig_rx_config {
        uint16_t                   rx_data_count;
        uint8_t                    descrambler_bypass;
        uint8_t                    rx_data_fifo_init;
    } rx_config;
    struct hw_innolink_dig_rx_stat {
        uint8_t                    block_sync_lock;
        uint8_t                    block_sync_status;
        uint8_t                    mb_sync_lock;
        uint8_t                    mb_sync_status;
        uint8_t                    fifo_overflow;
        uint8_t                    pkt_size_error;
    } rx_stat;
    struct hw_innolink_dig_tx_config {
        uint16_t                   tx_data_count;
        uint8_t                    scrambler_bypass;
    } tx_config;
    struct hw_innolink_dig_tx_test_config {
        uint8_t                    tx_test_mode;
        uint8_t                    it_enable;
        uint8_t                    prbs_tx_init;
        uint8_t                    insert_error;
        uint32_t                   prbs_init_state;
        uint32_t                   prbs_poly;
        uint32_t                   fixed_pat_lsw;
        uint32_t                   fixed_pat_msw;
    } tx_test_config;
    struct hw_innolink_dig_rx_test_config {
        uint8_t                    it_enable;
        uint8_t                    bert_en;
        uint32_t                   prbs_poly;
        uint8_t                    bert_sync;
        uint8_t                    bert_stop_count;
        uint8_t                    bert_capture_count;
    } rx_test_config;
    struct hw_innolink_dig_tx_stat {
        uint8_t                    fifo_overflow;
    } tx_stat;
    struct hw_innolink_dig_rx_test_status {
        uint32_t                   bert_word_count;
        uint32_t                   bit_err_count;
        uint32_t                   rx_sds_fifo_level;
        uint32_t                   tx_sds_fifo_level;
    } rx_test_status;
};
