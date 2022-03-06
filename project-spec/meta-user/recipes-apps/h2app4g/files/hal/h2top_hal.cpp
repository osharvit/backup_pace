#include "num_def.h"

#include "h2top_hal.h"


h2top_hal::h2top_hal(axigpio_hal &gpio_ctrl,
                     innolink_hal (&p1innolink_ctrl)[NUM_INNOLINK],
                     axidma_sg_hal &mm2saxidma_ctrl,
                     axidma_sg_hal &s2mmaxidma_ctrl,
                     dma_peri_hal &dmaperi_ctrl):
    gpio(gpio_ctrl),
    p1innolink(p1innolink_ctrl),
    mm2saxidma(mm2saxidma_ctrl),
    s2mmaxidma(s2mmaxidma_ctrl),
    dmaperi(dmaperi_ctrl)
{
    this->pl_version.u32 = 0;
}

h2top_hal::~h2top_hal()
{ }

uint32_t h2top_hal::read_pl_version()
{
    uint32_t version = p1innolink[P_IL].read_pl_version();

    return(version);
}

void h2top_hal::set_pl_version(uint32_t version)
{
    this->pl_version.u32 = version;

    gpio.set_pl_version(this->pl_version.u32);

    int num_innolink = (this->get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int kk = 0; kk < num_innolink; kk++){
        p1innolink[kk].set_pl_version(this->pl_version.u32);
    }
    mm2saxidma.set_pl_version(this->pl_version.u32);
    s2mmaxidma.set_pl_version(this->pl_version.u32);
    dmaperi.set_pl_version(this->pl_version.u32);
}

uint32_t h2top_hal::get_pl_version()
{
    return(this->pl_version.u32);
}

uint16_t h2top_hal::get_major_pl_version() const
{
    return(pl_version.u16le[1].u16);
} // end of uint32_t h2top_hal::get_major_version()

uint16_t h2top_hal::get_minor_pl_version() const
{
    return(pl_version.u16le[0].u16);
} // end of uint32_t h2top_hal::get_minor_version()

void h2top_hal::initialize()
{
    this->enable_sys10ms_interrupt(false);
    this->enable_syncin(false);
    this->enable_ul_axi_stream(P_IL, false);
    this->select_il(P_IL, P_IL);

    int num_innolink = (this->get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int kk = 0; kk < num_innolink; kk++){
        p1innolink[kk].initialize();
    }
}

void h2top_hal::oneshot_syncin()
{
    if(get_minor_pl_version() >= 34){
        p1innolink[P_IL].oneshot_syncin();
    }
}

void h2top_hal::enable_syncin(bool enable)
{
    if(get_minor_pl_version() >= 32){
        p1innolink[P_IL].enable_syncin(enable);
    }
    else if((get_minor_pl_version() >= 29) && (get_minor_pl_version() <= 31)){
        dmaperi.enable_syncin(enable);
    }
}

void h2top_hal::enable_dl_uplane(int dl_serdes_idx, bool enable)
{
    int num_innolink = (this->get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int kk = 0; kk < num_innolink; kk++){
        if(kk == dl_serdes_idx){
            p1innolink[kk].enable_dl_uplane(enable);
            TRACE0() << "dl_uplane interface: enable=" << enable << " for il=" << dl_serdes_idx << std::endl;
        }
        else{
            p1innolink[kk].enable_dl_uplane(false);
        }
    }
}

void h2top_hal::enable_syncin_dl_uplane(int dl_serdes_idx, bool enable)
{
    if(get_minor_pl_version() >= 32){
        int num_innolink = (this->get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

        for(int kk = 0; kk < num_innolink; kk++){
            if(kk == dl_serdes_idx){
                p1innolink[kk].enable_syncin_dl_uplane(enable);
            }
            else{
                p1innolink[kk].enable_syncin_dl_uplane(false);
            }
        }
    }
    else if((get_minor_pl_version() >= 29) && (get_minor_pl_version() <= 31)){
        dmaperi.enable_syncin(enable);
    }
}

void h2top_hal::enable_sys10ms_interrupt(bool enable)
{
    p1innolink[P_IL].enable_sys10ms_interrupt(enable);
}

void h2top_hal::enable_ul_axi_stream(int ul_serdes_idx, bool enable)
{
    int num_innolink = (this->get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    for(int kk = 0; kk < num_innolink; kk++){
        if(kk == ul_serdes_idx){
            p1innolink[kk].enable_ul_axi_stream(enable);
            TRACE0() << "ul_axi_stream interface: enable=" << enable << " for il=" << ul_serdes_idx << std::endl;
        }
        else{
            p1innolink[kk].enable_ul_axi_stream(false);
        }
    }
}

void h2top_hal::select_il(int dl_serdes_idx, int ul_serdes_idx)
{
    if(get_minor_pl_version() >= 36){
        // only the innolink#0
        p1innolink[0].select_il(dl_serdes_idx, ul_serdes_idx);
    }
    else if((get_minor_pl_version() >= 30) && (get_minor_pl_version() <= 35)){
        dmaperi.select_il(dl_serdes_idx, ul_serdes_idx);
    }
}
