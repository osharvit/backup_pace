#ifndef _H2_FPGA_REG_H_
#define _H2_FPGA_REG_H_

//------------------------------------------------------------
// AXI_GPIO
//------------------------------------------------------------
#define AXIGPIO_0_DATA_REG                          0x80004000
#define AXIGPIO_0_DATA_REG__H2_RESET_N_SHL                   0
#define AXIGPIO_0_DATA_REG__H2_SYNC_SHL                      1

#define AXIGPIO_0_TRI_REG                           0x80004004
#define AXIGPIO_0_TRI_REG__H2_RESET_N_SHL                    0
#define AXIGPIO_0_TRI_REG__H2_SYNC_SHL                       1

//------------------------------------------------------------
// TM
//------------------------------------------------------------
#define DMA_PERI_CAPTURE_START_REG                  0x80200100
#define DMA_PERI_CAPTURE_NUM_DESC_REG               0x80200104

#define IL_SEL_230_235_REG                          0x8020010C
#define IL_SEL_230_235_REG__IL_DL_MUX_SEL_SHL                0
#define IL_SEL_230_235_REG__IL_UL_MUX_SEL_SHL                8

#define DMA_PERI_CAPTURE_HEADER_COMPARE_REG         0x80201000
#define DMA_PERI_CAPTURE_HEADER_MASK_REG            0x80201800
#define DMA_PERI_CAPTURE_BURST_WORD_LEN_REG         0x80202000

#define SYNCIN_CTRL_229_231_REG                     0x80200108      // 2_29~2_31
#define SYNCIN_CTRL_229_231_REG__ENABLE_SYNCIN_SHL           0

//------------------------------------------------------------
// INNOLINK
//------------------------------------------------------------
#define IL_MPLANE_FIFO_LEVEL_REG                    0x80400008

#define IL_BIT_SAMPLE_CTRL_REG                      0x80400010

#define SYNCIN_CTRL_REG                             0x80400014      // 2_32~
#define SYNCIN_CTRL_REG__ENABLE_SYNCIN_SHL                   0
#define SYNCIN_CTRL_REG__ONESHOT_SYNCIN_SHL                  1      // 2_34~
#define SYNCIN_CTRL_REG__ENABLE_UPLANE_SHL                   2      // 2_34~

#define IL_INTERRUPT_CTRL_REG                       0x80400018      // 2_33~
#define IL_INTERRUPT_CTRL_REG__ENABLE_SYS10MS_SHL            0

#define IL_UL_AXISTREAM_CTRL_REG                    0x8040001C      // 2_33~
#define IL_UL_AXISTREAM_CTRL_REG__ENABLE_UL_AXISTREAM_SHL    0

#define IL_SEL_REG                                  0x80400020
#define IL_SEL_REG__IL_DL_MUX_SEL_SHL                        0
#define IL_SEL_REG__IL_UL_MUX_SEL_SHL                        8

#define IL_MAC_STATUS_REG                           0x8040010C

#define IL_MPLANE_DATA_RW_REG                       0x80400200

//------------------------------------------------------------
// GTH_XCVR
//------------------------------------------------------------
#define GTH_XCVR_RESET_CTRL_REG                     0x80800004
#define GTH_XCVR_RESET_CTRL_REG__SERDES_RESET_SHL            0
#define GTH_XCVR_RESET_CTRL_REG__SERDES_LOOPBACK_SHL        16 

#define GTH_XCVR_STATUS_REG                         0x80800008

#define GTH_SAMPLE_SWAP_REG                         0x80800010

#define GTH_MSB_LSB_FLIP_REG                        0x80800018

// TODO: revisit if this is the correct registername
#define GTH_FLIP_CTRL_REG                           0x8080001C

#define GTH_POST_CURSOR_REG                         0x80800030

#define GTH_XCVR_CLK_CTRL_REG                       0x80800034
#define GTH_XCVR_CLK_CTRL_REG__QPLL0_REF_CLK_SEL_SHL        16

#endif // _H2_FPGA_REG_H_
