#ifndef _H2TOP_HAL_H_
#define _H2TOP_HAL_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include "mmap_hal.h"   // mmap_hal
#include "axigpio_hal.h"
#include "axidma_sg_hal.h"
#include "dma_peri_hal.h"
#include "innolink_hal.h"

class   h2top_hal
{
    public:
        h2top_hal(axigpio_hal &gpio_ctrl,
                  innolink_hal (&p1innolink_ctrl)[NUM_INNOLINK],
                  axidma_sg_hal &mm2saxidma_ctrl,
                  axidma_sg_hal &s2mmaxidma_ctrl,
                  dma_peri_hal &dmaperi_ctrl);
        virtual ~h2top_hal();

        uint32_t read_pl_version();
        void set_pl_version(uint32_t version);
        uint32_t get_pl_version();
        uint16_t get_major_pl_version() const;
        uint16_t get_minor_pl_version() const;

        void initialize();
        void oneshot_syncin();
        void enable_syncin(bool enabled);
        void enable_dl_uplane(int dl_serdes_idx, bool enable);
        void enable_syncin_dl_uplane(int dl_serdes_idx, bool enable);
        void enable_sys10ms_interrupt(bool enable);
        void enable_ul_axi_stream(int ul_serdes_idx, bool enable);
        void select_il(int dl_serdes_idx, int ul_serdes_idx);

    public:
        axigpio_hal &gpio;
        innolink_hal (&p1innolink)[NUM_INNOLINK];
        axidma_sg_hal &mm2saxidma;
        axidma_sg_hal &s2mmaxidma;
        dma_peri_hal &dmaperi;

    protected:
        hal::u32le_t    pl_version;

    private:
        h2top_hal();
};
#endif
