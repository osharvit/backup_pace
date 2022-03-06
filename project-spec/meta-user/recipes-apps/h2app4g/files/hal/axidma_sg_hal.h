#ifndef _AXIDMA_SG_HAL_HPP_
#define _AXIDMA_SG_HAL_HPP_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include "mmap_hal.h"   // mmap_hal

class   axidma_sg_hal
{
    public:
        enum{
            AXIDMA_MODE_SG = 0,
            AXIDMA_MODE_DI,
            NUM_AXIDMA_MODE
        };
        
        enum{
            MM2S_DMACR__RS_SHL      = 0,
            MM2S_DMACR__RESET_SHL   = 2,
            MM2S_DMACR__CYCLICBDEN  = 4,
        };
        
        enum{
            S2MM_DMACR__RS_SHL    = 0,
            S2MM_DMACR__RESET_SHL = 2,
        };

        enum{
            DMADESC_CONTROL__TXEOF_SHL = 26,
            DMADESC_CONTROL__TXSOF_SHL = 27,
        };

        enum{
            DMADESC_CONTROL__RXEOF_SHL = 26,
            DMADESC_CONTROL__RXSOF_SHL = 27,
        };

        // NOTE: the size of dma_descriptor should be 0x40
        typedef struct _mm2s_axidma_desc{
            public:
                uint32_t NEXTDESC;                  // 0x00
                uint32_t NEXTDESC_MSB;              // 0x04
                uint32_t BUFFER_ADDRESS;            // 0x08
                uint32_t BUFFER_ADDRESS_MSB;        // 0x0c
                uint32_t reserved_0x10;             // 0x10
                uint32_t reserved_0x14;             // 0x14
                uint32_t CONTROL;                   // 0x18
                uint32_t STATUS;                    // 0x1c
#if 1
                uint32_t USER_APP_TIMESTAMP;        // 0x20
                uint32_t USER_APP_WORDLENGTH;       // 0x24
                uint32_t USER_APP_SECTION_HEADER;   // 0x28
                uint32_t USER_APP_SDUHEADER_MSB;    // 0x2c
                uint32_t USER_APP_SDUHEADER_LSB;    // 0x30
#else   // ealry of P1 style
                uint32_t USER_APP_SECTION_HEADER;   // 0x20
                uint32_t USER_APP_WORDLENGTH;       // 0x24
                uint32_t USER_APP_TIMESTAMP;        // 0x28
                uint32_t APP3;                      // 0x2c
                uint32_t APP4;                      // 0x30
#endif
                uint32_t reserved_0x34;             // 0x34
                uint32_t reserved_0x38;             // 0x38
                uint32_t reserved_0x3c;             // 0x3c

            public:
                inline static bool compare(const struct _mm2s_axidma_desc& first, const struct _mm2s_axidma_desc &second){
                    return ( (first.USER_APP_TIMESTAMP & USER_APP_TIMESTAMP__TIMESTAMP_M) < (second.USER_APP_TIMESTAMP & USER_APP_TIMESTAMP__TIMESTAMP_M));
                }
        } axidmadesc;

        // NOTE: the size of dma_descriptor should be 0x40
        typedef struct _s2mm_axidma_desc{
            public:
                uint32_t NEXTDESC;                  // 0x00
                uint32_t NEXTDESC_MSB;              // 0x04
                uint32_t BUFFER_ADDRESS;            // 0x08
                uint32_t BUFFER_ADDRESS_MSB;        // 0x0c
                uint32_t reserved_0x10;             // 0x10
                uint32_t reserved_0x14;             // 0x14
                uint32_t CONTROL;                   // 0x18
                uint32_t STATUS;                    // 0x1c
                uint32_t USER_APP_0x20;             // 0x20
                uint32_t USER_APP_0x24;             // 0x24
                uint32_t USER_APP_0x28;             // 0x28
                uint32_t USER_APP_0x2C;             // 0x2c
                uint32_t USER_APP_0x30;             // 0x30
                uint32_t reserved_0x34;             // 0x34
                uint32_t reserved_0x38;             // 0x38
                uint32_t reserved_0x3c;             // 0x3c
        } s2mm_axidma_desc;

        // refer to Xilinx PG021 LogiCORE IP Product Guide
        // AXI DMA: scatter/gather mode register address map
        typedef struct _axidma_sg_ctrl{
            uint32_t MM2S_DMACR;            // 0x00 mm2s dma control
            uint32_t MM2S_DMASR;            // 0x04 mm2s dma status
            uint32_t MM2S_CURDESC;          // 0x08 mm2s current descriptor pointer, lower 32
            uint32_t MM2S_CURDESC_MSB;      // 0x0c mm2s current descriptor pointer, upper 32
            uint32_t MM2S_TAILDESC;         // 0x10 mm2s tail descriptor pointer, lower 32
            uint32_t MM2S_TAILDESC_MSB;     // 0x14 mm2s tail descriptor pointer, upper 32
            uint32_t reserved_0x18;         // 0x18 reserved for direct register mode
            uint32_t reserved_0x1c;         // 0x1c reserved for direct register mode
            uint32_t reserved_0x20;         // 0x20
            uint32_t reserved_0x24;         // 0x24
            uint32_t reserved_0x28;         // 0x28 reserved for direct register mode
            uint32_t SG_CTL;                // 0x2c scatter/gather user and cache
            uint32_t S2MM_DMACR;            // 0x30 s2mm dma control
            uint32_t S2MM_DMASR;            // 0x34 s2mm dma status
            uint32_t S2MM_CURDESC;          // 0x38 s2mm current descriptor pointer, lower 32
            uint32_t S2MM_CURDESC_MSB;      // 0x3c s2mm current descriptor pointer, upper 32
            uint32_t S2MM_TAILDESC;         // 0x40 s2mm tail descriptor pointer, lower 32
            uint32_t S2MM_TAILDESC_MSB;     // 0x44 s2mm tail descriptor pointer, upper 32
            uint32_t reserved_0x48;         // 0x48 reserved for direct register mode
            uint32_t reserved_0x4c;         // 0x4c reserved for direct register mode
            uint32_t reserved_0x50;         // 0x50
            uint32_t reserved_0x54;         // 0x54
            uint32_t reserved_0x58;         // 0x58 reserved for direct register mode
            uint32_t reserved_0x5c;         // 0x5c
        } axidma_sg_ctrl;

        // refer to Xilinx PG021 LogiCORE IP Product Guide
        // AXI DMA: direct register mode register address map
        typedef struct _axidma_di_ctrl{
            uint32_t MM2S_DMACR;            // 0x00 mm2s dma control
            uint32_t MM2S_DMASR;            // 0x04 mm2s dma status
            uint32_t reserved_0x08;         // 0x08 reserved for scatter gather mode
            uint32_t reserved_0x0c;         // 0x0c mm2s current descriptor pointer, upper 32
            uint32_t reserved_0x10;         // 0x10 reserved for scatter gather mode
            uint32_t reserved_0x14;         // 0x14 reserved for scatter gather mode
            uint32_t MM2S_SA;               // 0x18 mm2s source address, lower 32
            uint32_t MM2S_SA_MSB;           // 0x1c mm2s source address, upper 32
            uint32_t reserved_0x20;         // 0x20
            uint32_t reserved_0x24;         // 0x24
            uint32_t MM2S_LENGTH;           // 0x28 mm2s transfer length (bytes)
            uint32_t SG_CTL;                // 0x2c scatter/gather user and cache
            uint32_t S2MM_DMACR;            // 0x30 s2mm dma control
            uint32_t S2MM_DMASR;            // 0x34 s2mm dma status
            uint32_t reserved_0x38;         // 0x38 reserved for scatter gather mode
            uint32_t reserved_0x3c;         // 0x3c reserved for scatter gather mode
            uint32_t reserved_0x40;         // 0x40 reserved for scatter gather mode
            uint32_t reserved_0x44;         // 0x44 reserved for scatter gather mode
            uint32_t S2MM_DA;               // 0x48 s2mm destination address, lower 32
            uint32_t S2MM_DA_MSB;           // 0x4c s2mm destination address, upper 32
            uint32_t reserved_0x50;         // 0x50
            uint32_t reserved_0x54;         // 0x54
            uint32_t S2MM_LENGTH;           // 0x58 s2mm buffer length (bytes)
            uint32_t reserved_0x5c;         // 0x5c
        } axidma_di_ctrl;

    public:
        // constructor for scatter/gather mode
        axidma_sg_hal(uint64_t ctrl_addr_base);
        virtual ~axidma_sg_hal();

        void set_pl_version(uint32_t version);
        uint16_t get_major_pl_version() const;
        uint16_t get_minor_pl_version() const;

        void read_mm2s_status();
        void kick_mm2s(uint64_t currdesc_addr, uint64_t taildesc_addr);
        void stop_mm2s();
        void reset_mm2s();

        void read_s2mm_status();
        void kick_s2mm(uint64_t currdesc_addr, uint64_t taildesc_addr);
        void stop_s2mm();
        void reset_s2mm();

    protected:
        const uint64_t  ctrladdrbase;
        mmap_hal dmactrl;
        size_t axidma_mode;
        hal::u32le_t pl_version;

        bool verbose;
};

#endif // _AXIDMA_SG_HAL_HPP_ 
