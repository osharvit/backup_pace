#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>       // round()
#include <sys/stat.h>
#include <string>
#include <iomanip>      // std::setw(), std::setfill()
#include <vector>       // std::vector
#include <algorithm>    // std::sort
#include <list>         // std::list
#include <sstream>      // std::stringstream
#include <exception>    // std::exception

#include "sp_thread.h"
#include "ant_carrier.h"    // class ant_carrier
#include "h2_fpga_reg.h"
#include "mmap_hal.h"
#include "axidma_sg_hal.h"

extern std::string g_fw_name;


sp_thread::sp_thread(const std::string &name,
                     size_t id,
#ifdef  USE_SYS10MS_INTERRUPT
                     blocking_interrupt &sys10ms_interrupt,
#endif
                     h2top_hal (&h2top_ctrl)[NUM_HERMES]
                     ): 
    thread_base(name, id),
    stm(NUM_SP_STATE),
#ifdef  USE_SYS10MS_INTERRUPT
    sys10msintr(sys10ms_interrupt),
#endif
    p1h2top(h2top_ctrl),
    frc_file_name("frc_0.bin")
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    fpga_major_version = 0;
    fpga_minor_version = 0;

    this->sdu_mask = 0;
    this->sdu_target = 0;
    this->nums2mmdesc = 2;
    this->num_segments = 1;;
    this->off_duty_mode = 0;    // 0: "zeropad" | "nongated", 1: "dtx" | "gated"
    this->scs = 0;              // 0: 15 kHz, 1: 30 kHz
    this->tdd_ul_dl_config = 0;
    this->tdd_ssf_config = 0;
    this->osr = 1;

    this->phase = -1;       // -1: for None, 0: for P1, 1: for P2
    this->antport_on_reset = 1; // 0: antport-A, 1: antport-B
    this->dl_il_sel = 0;        // 0: innolink0, 1: innolink1
    this->ul_il_sel = 0;        // 0: innolink0, 1: innolink1

    this->install_state_handler(SP_IDLE_S,
                                &sp_thread::sp_idle_entry,
                                &sp_thread::sp_idle_state,
                                &sp_thread::sp_idle_exit);
    this->install_state_handler(SP_FWLOAD_S,
                                &sp_thread::sp_fwload_entry,
                                &sp_thread::sp_fwload_state,
                                &sp_thread::sp_fwload_exit);
    this->install_state_handler(SP_INITSYNC_S,
                                &sp_thread::sp_initsync_entry,
                                &sp_thread::sp_initsync_state,
                                &sp_thread::sp_initsync_exit);

    this->install_state_changed_callback(&sp_thread::sp_state_changed_callback_wrapper, (void *)this);

    this->nvreg_json.SetObject();

    setup_hswi_handler();
} // end of sp_thread::sp_thread(const std::string &name, size_t id)

sp_thread::~sp_thread() {}

/* call flow:--------------------------------------------------------------------
 *  <start_sp_thread class="sp_thread">
 *      <start_wrapper class="sp_thread">
 *          <run_sp_thread class="sp_thread">
 *               <process_itc_event class="sp_thread">
 *                   <dispatch_event class="sp_thread">
 *                       <sp_idle_state class="sp_thread"/>
 *                       <sp_fwload_state class="sp_thread"/>
 *                       <sp_initsync_state class="sp_thread"/>
 *                   </dispatch_event>
 *               </process_itc_event>
 *          </run_sp_thread>
 *      </start_wrapper>
 *  </start_sp_thread>
 */

void sp_thread::install_state_handler(size_t state,
                                      void (sp_thread::*entry_func)(),
                                      size_t (sp_thread::*state_func)(void *, int),
                                      void (sp_thread::*exit_func)())
{
    if(state < NUM_SP_STATE) {
        stm::install_state_handler(state,
                                   static_cast<void (stm::*)()>(entry_func),
                                   static_cast<size_t (stm::*)(void *, int)>(state_func),
                                   static_cast<void (stm::*)()>(exit_func));
    }
} // end of sp_thread::install_state_handler(...)

void sp_thread::sp_state_changed_callback(size_t stm_id, size_t curr_state, size_t next_state)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ()
            << "stm_id=" << stm_id
            << ", curr_state=" << curr_state
            << ", next_state=" << next_state
            << std::endl;
    }

    itc_state_ind itcstateind;
    itcstateind.currstate = curr_state;
    itcstateind.nextstate = next_state;

    send_itc_message(THREAD_RE,
                     ITC_STATE_IND_ID,
                     sizeof(itc_state_ind),
                     (const uint8_t *)&itcstateind);
    TRACE0OBJ() << "send ITC_STATEIND_ID from THREAD_SP to THREAD_RE" << std::endl;
} // end of void sp_thread::sp_state_changed_callback()

void sp_thread::sp_state_changed_callback_wrapper(void *arg, size_t stm_id, size_t curr_state, size_t next_state)
{
    if(arg != NULL) {
        ((sp_thread *)arg)->sp_state_changed_callback(stm_id, curr_state, next_state);
    }
}

void sp_thread::sp_idle_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
}

size_t sp_thread::sp_idle_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    (void)(event);  // to avoid of unused warning

    if(p1arg != NULL){
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid) {
            case ITC_KICKOFF_ID: {
                process_itc_kickoff();
                break;
            }
            case ITC_VERSION_IND_ID: {
                this->fpga_major_version = p1msg->version_rsp.major_version;
                this->fpga_minor_version = p1msg->version_rsp.minor_version;
                break;
            }
            case ITC_RESET_REQ_ID: {
                this->phase = p1msg->reset_req.phase;
                this->antport_on_reset = p1msg->reset_req.antport_on_reset;
                this->dl_il_sel = p1msg->reset_req.dl_il_sel;
                this->ul_il_sel = p1msg->reset_req.ul_il_sel;
                this->n_dpa = p1msg->reset_req.n_dpa;

                if(p1msg->reset_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->reset_req.nextstate;
                }
                break;
            }
            case ITC_RESYNC_REQ_ID: {
#ifdef  USE_SYS10MS_INTERRUPT
                // step 1: enable sys10ms interrupt
                p1h2top[0].p1innolink[P_IL].enable_sys10ms_interrupt(true);

                int remaincnt = 0;

                // step 2: enable syncin pulse for the 1st incoming sys10ms interrupt
                if((remaincnt = sys10msintr.wait_for_interrupt()) == 0){
                    // blocked until the next sys10ms interrupt
                    // this function returns
                    // 0 if no interrupt within a predetermined time,
                    // positive otherwise
                    TRACE0OBJ() << "no sys10ms interrupt: remaincnt=" << remaincnt << std::endl;
                }
                this->p1h2top[0].enable_syncin(true);

                // step 3: disable syncin pulse for the 2nd incoming sys10ms interrupt
                if((remaincnt = sys10msintr.wait_for_interrupt()) == 0){
                    // blocked until the next sys10ms interrupt
                    TRACE0OBJ() << "no sys10ms interrupt: remaincnt=" << remaincnt << std::endl;
                }
                this->p1h2top[0].enable_syncin(false);

                // step 4: disble sys10ms interrupt
                p1h2top[0].p1innolink[P_IL].enable_sys10ms_interrupt(false);
#else
                this->p1h2top[0].oneshot_syncin();
#endif
                break;
            }
            case ITC_UL_AXI_STREAM_REQ_ID: {
                this->p1h2top[0].enable_ul_axi_stream(this->ul_il_sel, 
                                                      p1msg->ul_axi_stream_req.enabled);
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            case ITC_PAYLOAD_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document rspdoc;
                rapidjson::Document::AllocatorType& rsp_at = rspdoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    rspdoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("re:",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(rspdoc),
                                                rsp_at);
                    std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
#if 0
                    std::cout << rspdocstr << std::endl;
#endif
                }
                break;
            }
            case ITC_PLAYBACK_REQ_ID: {
                itc_action_ind  itcactionind;

                //---------------------------------------------------------------
                // TM playback
                //---------------------------------------------------------------
                itcactionind.passfail = prepare_tmplayback(p1msg->playback_req);

                send_itc_message(THREAD_RE,
                                 ITC_PLAYBACK_IND_ID,
                                 sizeof(itc_action_ind),
                                 (const uint8_t *)&itcactionind);


                break;
            }
            case ITC_DOWNLOAD_TM_FILE_REQ_ID: 
            case ITC_DOWNLOAD_FILE_REQ__TM_FILE_ID: {
                itc_action_ind  itcactionind;

                itcactionind.uid = p1msg->download_tm_file_req.uid;
                itcactionind.passfail = this->download_tm_file(p1msg->download_tm_file_req);
                strncpy(itcactionind.filename, p1msg->download_tm_file_req.path, MAX_FILENAME);

                if(p1msg->hdr.msgid == ITC_DOWNLOAD_TM_FILE_REQ_ID){
                    send_itc_message(THREAD_RE,
                                     ITC_DOWNLOAD_TM_FILE_IND_ID,
                                     sizeof(itc_action_ind),
                                     (const uint8_t *)&itcactionind);
                }
                else{
                    send_itc_message(THREAD_RE,
                                     ITC_DOWNLOAD_FILE_IND__TM_FILE_ID,
                                     sizeof(itc_action_ind),
                                     (const uint8_t *)&itcactionind);
                }
                break;
            }
            case ITC_CREATE_CAPTURE_REQ_ID: {
                itc_action_ind  itcactionind;

                itcactionind.passfail = create_capture(p1msg->create_capture_req);

                send_itc_message(THREAD_RE,
                                 ITC_CREATE_CAPTURE_IND_ID,
                                 sizeof(itc_action_ind),
                                 (const uint8_t *)&itcactionind);


                break;
            }
            case ITC_CAPTURE_FILE_REQ__FRC_FILE_ID: {
                itc_action_ind  itcactionind;

                itcactionind.uid = p1msg->capture_frc_file_req.uid;

                itcactionind.passfail = this->capture_frc_file(p1msg->capture_frc_file_req);

                strncpy(itcactionind.filename, p1msg->capture_frc_file_req.path, MAX_FILENAME);

                send_itc_message(THREAD_RE,
                                 ITC_UPLOAD_FILE_IND__FRC_FILE_ID,
                                 sizeof(itc_action_ind),
                                 (const uint8_t *)&itcactionind);
                break;
            }
            case ITC_UPLOAD_FILE_REQ__FRC_FILE_ID: {
                itc_action_ind  itcactionind;

                itcactionind.uid = p1msg->upload_frc_file_req.uid;
                itcactionind.passfail = this->upload_frc_file(p1msg->upload_frc_file_req);
                strncpy(itcactionind.filename, p1msg->upload_frc_file_req.path, MAX_FILENAME);

                send_itc_message(THREAD_RE,
                                 ITC_UPLOAD_FILE_IND__FRC_FILE_ID,
                                 sizeof(itc_action_ind),
                                 (const uint8_t *)&itcactionind);
                break;
            }
            case ITC_NVDATA_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& tmprsp_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);
#if 0
                TRACE0OBJ() << strsdu << std::endl; 
#endif

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                tmprsp_at);
                }
                break;
            }
            case ITC_MISC_REQ_ID:{
                this->p1h2top[0].gpio.enable_rfout(p1msg->misc_req.gpio31);
                break;
            }
            default: {
                TRACE0OBJ()
                    << "ignore the msgid=" << p1msg->hdr.msgid
                    << std::endl;
            }
        };
    }

    return(this->currstate);
} // end of size_t sp_thread::sp_idle_state()

void sp_thread::sp_idle_exit()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
}

#if defined(__aarch64__)    // for real platform
void sp_thread::sp_fwload_entry()
{
    int num_innolink = (this->p1h2top[0].get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    std::string bootarg("");
    for(rapidjson::Value::ConstMemberIterator itochild = nvreg_json.MemberBegin(); itochild != nvreg_json.MemberEnd(); ++itochild){
        std::string currkey(itochild->name.GetString());

        if((currkey == "tx.chain") && (this->antport_on_reset >= 0) && (this->antport_on_reset < 2)){
            // skip
        }
        else if((currkey == "tx.n_dpa") && (this->n_dpa >= 1) && (this->n_dpa <= 4)){
            // skip
        }
        else{
            bootarg += currkey + "=";
            switch(itochild->value.GetType())
            {
                case  rapidjson::kStringType: bootarg += std::string(itochild->value.GetString()); break;
                case  rapidjson::kNumberType: bootarg += std::to_string(itochild->value.GetInt()); break;
                default:;
            }
            bootarg += std::string(" ");
        }

    }
#if 0
    TRACE0OBJ() << "bootarg=" << bootarg << std::endl;
#endif

    // step1: reset axidma first ----------------------------
    p1h2top[0].initialize();


    // step2: reset axidma first ----------------------------
    p1h2top[0].mm2saxidma.reset_mm2s();

    // step 3: disable syncin pulse
    this->p1h2top[0].enable_syncin_dl_uplane(this->dl_il_sel, false);

    // step 4: serdes configuration before fwload
    switch(fpga_major_version){
        case 1: {   // h2hb0 emulation
            // TODO: select the internal 245M76 system clock
            // TODO: disable msb/lsb bit flip
            // TODO: disable sample swap
            // TODO: reset GTH
            // TODO: reset other digital block
            break;
        }
        case 2: {   // h2ha0 oran_platform
            // TODO: select the external 245M76 system clock
            //      onboard: 0x50000, external: 0x10000)"
            //      poke 0x80800034 0x10000
            for(int kk = 0; kk < num_innolink; kk++){
                p1h2top[0].p1innolink[kk].set_sysclk_enable(true);
            }
            hal::nsleep(100000000);    // 100 msec

            for(int kk = 0; kk < num_innolink; kk++){
                if(kk == 0){
                    // TODO: enable msb/lsb bit flip
                    //      enable:0x108, disable:0x18c
                    //      poke 0x80800018 0x108
                    p1h2top[0].p1innolink[kk].xil_gth.wr32(GTH_MSB_LSB_FLIP_REG, 0x108);
                }
                else{
                    // make the b8 of 0x808x0018 = 0
                    uint32_t regval = 0;
                    regval = p1h2top[0].p1innolink[kk].xil_gth.rd32(GTH_MSB_LSB_FLIP_REG);
                    regval = (regval & 0xFFFFFEFF);
                    p1h2top[0].p1innolink[kk].xil_gth.wr32(GTH_MSB_LSB_FLIP_REG, regval);
                }
            }
            hal::nsleep(100000000);    // 100 msec

            // TODO: enable sample swap from a 48-bit word
            //      enable: 0x101, disable:0x18c
            //      poke 0x80400010 0x101
            for(int kk = 0; kk < num_innolink; kk++){
                p1h2top[0].p1innolink[kk].il_ctrl.wr32(IL_BIT_SAMPLE_CTRL_REG, 0x101);
            }
            hal::nsleep(100000000);    // 100 msec

            // TODO: reset GTH for the primary innolink
            //      poke 0x80800004 0x1
            //      poke 0x80800004 0x0
            p1h2top[0].p1innolink[P_IL].xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x1);
            hal::nsleep(100000000);    // 100 msec

            p1h2top[0].p1innolink[P_IL].xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x0);
            hal::nsleep(100000000);    // 100 msec

            // TODO: reset other digital block
            //      poke 0x80004004 0xf7
            //      poke 0x80400100 0x2a01
            //      poke 0x80004004 0xff
            //      poke 0x80400100 0x3f01
            p1h2top[0].gpio.wr32(0x80004004, 0xf7);
            hal::nsleep(100000000);    // 100 msec

            for(int kk = 0; kk < num_innolink; kk++){
                p1h2top[0].p1innolink[kk].il_ctrl.wr32(0x80400100, 0x2a01);
            }
            hal::nsleep(100000000);    // 100 msec

            p1h2top[0].gpio.wr32(0x80004004, 0xff);
            hal::nsleep(100000000);    // 100 msec

            for(int kk = 0; kk < num_innolink; kk++){
                p1h2top[0].p1innolink[kk].il_ctrl.wr32(0x80400100, 0x3f01);
            }
            hal::nsleep(100000000);    // 100 msec
            break;
        }
        case 3: {   // h2ha0 emulation
            // TODO: select the internal 245M76 system clock
            // TODO: enable msb/lsb bit flip
            // TODO: enable sample swap
            // TODO: reset GTH
            // TODO: reset other digital block
            break;
        }
        default: {
            // TODO: select the external 245M76 system clock
            // TODO: disable msb/lsb bit flip
            // TODO: disable sample swap
            // TODO: reset GTH
            // TODO: reset other digital block
        }
    }

    // step 5: hard reset first
    //TRACE0OBJ() << "hard_reset" << std::endl;
    {
        // TODO: needs mutex here

        // read tri state register
        uint32_t tri = this->p1h2top[0].gpio.rd32(AXIGPIO_0_TRI_REG);
        // makes the dedicated bit output enable
        //      0 = I/O pin configured as output
        //      1 = I/O pin configured as input
        tri = tri & ~(1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
        this->p1h2top[0].gpio.wr32(AXIGPIO_0_TRI_REG, tri);
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;

        // write data
        this->p1h2top[0].gpio.wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
        // wait
        hal::nsleep(1000000000);    // 1 sec
        // write 0 for low active
        this->p1h2top[0].gpio.wr32(AXIGPIO_0_DATA_REG, (0UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));

        // wait
        hal::nsleep(1000000000);    // 1000 msec

        // write data
        this->p1h2top[0].gpio.wr32(AXIGPIO_0_DATA_REG, (1UL << AXIGPIO_0_DATA_REG__H2_RESET_N_SHL));
        // wait
        hal::nsleep(1000000000);    // 1000 msec

        tri = tri | (1UL << AXIGPIO_0_TRI_REG__H2_RESET_N_SHL);
        // makes the dedicated bit output disable
        //      0 = I/O pin configured as output
        //      1 = I/O pin configured as input
        this->p1h2top[0].gpio.wr32(AXIGPIO_0_TRI_REG, tri);
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << addrbase + offsetof(axigpio1_ctrl, AXI_GPIO1_TRI) << "=" << tri << std::endl;
    }

    // step 6: kill running openocd
    std::string cmd1("AA=$(ps -aef | grep openocd | grep -v grep) && [[ ! -z ${AA} ]] && killall -9 openocd");
    //if(system(cmd1.c_str()) != 0){
    //    std::stringstream sstrm;
    //    TRACE1OBJ(sstrm) << "error in system call: " << cmd1 << std::endl;
    //    perror(sstrm.str().c_str());
    //}


    // step 7: restart openocd, because openocd depends on a fpga bitstream file
    //TRACE0OBJ() << "fpga_major_version=" << fpga_major_version << std::endl;
    std::string cmd2;
    switch(fpga_major_version){
        case 0: {
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "undefined fpga_major_version=" << fpga_major_version << std::endl;
            perror(sstrm.str().c_str());
            break;
        }
        case 1: {
            cmd2 = std::string("[[ -f /home/root/openocd/openocd ]] && (cd /home/root/openocd; chmod +x openocd; ./openocd -c 'bindto 0.0.0.0' -f innoh2.cfg -f h2-jtag.cfg > openocd.log 2>&1 &)");
            break;
        }
        case 2:
        default: {
            cmd2 = std::string("[[ -f /home/root/openocd/openocd ]] && (cd /home/root/openocd; chmod +x openocd; ./openocd -c 'bindto 0.0.0.0' -f innoh2.cfg -f h2-swd.cfg > openocd.log 2>&1 &)");
        }
    }

    TRACE0OBJ() << cmd2 << std::endl;
    if(system(cmd2.c_str()) != 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in system call: " << cmd2 << std::endl;
        perror(sstrm.str().c_str());
    }
    hal::nsleep(1000000000L);  // nanosleep

    if(!(g_debug_mask & (1UL << MASKBIT_NOFW))) {
        std::string phasestr = (this->phase >= 0)?
            (std::string("phase=") + std::to_string(this->phase) + std::string(" ")) :
            std::string(" ");
        std::string antportstr = ((this->antport_on_reset >= 0) && (this->antport_on_reset < 2))?
            (std::string("tx.chain=") + std::to_string(this->antport_on_reset) + std::string(" ")) :
            std::string(" ");
        std::string ulilselstr = ((this->ul_il_sel >= 0) && (this->ul_il_sel < 2))?
            (std::string("ul_il_sel=") + std::to_string(this->ul_il_sel) + std::string(" ")) :
            std::string(" ");
        std::string ndpastr = ((this->n_dpa >= 1) && (this->n_dpa <= 4))?
            (std::string("tx.n_dpa=") + std::to_string(this->n_dpa) + std::string(" ")) :
            std::string(" ");

        // step 8:
        std::string cmd3 =
            std::string("[[ -d /home/root/boot && -f /home/root/h2fw/") + g_fw_name +
            std::string(" ]] && (cd /home/root/boot; script/gdbrun.py ../h2fw/") +
            g_fw_name + std::string(" ") +
            phasestr + std::string(" ") +
            antportstr + std::string(" ") +
            ulilselstr + std::string(" ") +
            ndpastr + std::string(" ") +
            bootarg + std::string(")");

        TRACE0OBJ() << cmd3 << std::endl;
        if(system(cmd3.c_str()) != 0){
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "error in system call: " << cmd3 << std::endl;
            perror(sstrm.str().c_str());
        }
    }
    else{
        TRACE0OBJ()
            << "!!! HW FW loading is disabled, please load H2 FW manually i.e. using GDB !!!" << std::endl;
    }

    {
        hal::nsleep(1000000000);    // 1 sec

        // TODO: reset GTH for the secondary innolink
        //      poke 0x80800004 0x1
        //      poke 0x80800004 0x0
        for(int kk=1; kk < num_innolink; kk++){
            p1h2top[0].p1innolink[kk].xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x1);
        }
        hal::nsleep(100000000);    // 100 msec

        for(int kk=1; kk < num_innolink; kk++){
            p1h2top[0].p1innolink[kk].xil_gth.wr32(GTH_XCVR_RESET_CTRL_REG, 0x0);
        }
        hal::nsleep(100000000);    // 100 msec
    }

    // NOTE: return back to the SP_IDLE_S
    transit_state(SP_IDLE_S, true);
}
#endif

size_t sp_thread::sp_fwload_state(void *p1arg, int event)
{
    if((g_debug_mask & (1UL << MASKBIT_CALL)) || true){
        TRACE0OBJ();
    }
    (void)(event);  // to avoid of unused warning

    if(p1arg != NULL){
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid) {
            case ITC_KICKOFF_ID: {
                process_itc_kickoff();
                break;
            }
            case ITC_VERSION_IND_ID: {
                this->fpga_major_version = p1msg->version_rsp.major_version;
                this->fpga_minor_version = p1msg->version_rsp.minor_version;
                break;
            }
            case ITC_RESET_REQ_ID: {
                if(p1msg->reset_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->reset_req.nextstate;
                }
                this->phase = p1msg->reset_req.phase;
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            case ITC_PAYLOAD_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document rspdoc;
                rapidjson::Document::AllocatorType& rsp_at = rspdoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    rspdoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("re:",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(rspdoc),
                                                rsp_at);
                    std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
#if 0
                    std::cout << rspdocstr << std::endl;
#endif
                }
                break;
            }
            case ITC_NVDATA_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& tmprsp_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);
#if 0
                TRACE0OBJ() << strsdu << std::endl; 
#endif

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                tmprsp_at);
                }
                break;
            }
            default: {
                TRACE0OBJ()
                    << "ignore the msgid=" << p1msg->hdr.msgid
                    << std::endl;
            }
        };
    }

    return(this->currstate);
} // end of size_t sp_thread::sp_fwload_state()

void sp_thread::sp_fwload_exit()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
}

void sp_thread::sp_initsync_entry()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    // NOTE: return back to the SP_IDLE_S
    transit_state(SP_IDLE_S, true);
}

size_t sp_thread::sp_initsync_state(void *p1arg, int event)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    (void)(event);  // to avoid of unused warning

    if(p1arg != NULL){
        itc_queue::elem *p1elem = (itc_queue::elem *)p1arg;
        itc_msg *p1msg = (itc_msg *)p1elem->p1body;

        if(p1msg == NULL) return(this->currstate);

        switch(p1msg->hdr.msgid) {
            case ITC_KICKOFF_ID: {
                process_itc_kickoff();
                break;
            }
            case ITC_VERSION_IND_ID: {
                this->fpga_major_version = p1msg->version_rsp.major_version;
                this->fpga_minor_version = p1msg->version_rsp.minor_version;
                break;
            }
            case ITC_RESET_REQ_ID: {
                if(p1msg->reset_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->reset_req.nextstate;
                }
                this->phase = p1msg->reset_req.phase;
                break;
            }
            case ITC_STATE_REQ_ID: {
                if(p1msg->state_req.nextstate < NUM_SP_STATE){
                    this->nextstate = p1msg->state_req.nextstate;
                }
                break;
            }
            case ITC_PAYLOAD_ID: {
                rapidjson::Document reqdoc;
                rapidjson::Document rspdoc;
                rapidjson::Document::AllocatorType& rsp_at = rspdoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    rspdoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("re:",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(rspdoc),
                                                rsp_at);
                    std::string rspdocstr = rapidjson_wrapper::str_json(rspdoc);
#if 0
                    std::cout << rspdocstr << std::endl;
#endif
                }
                break;
            }
            case ITC_NVDATA_ID: {
                rapidjson::Document reqdoc;;
                rapidjson::Document nildoc;
                rapidjson::Document::AllocatorType& tmprsp_at = nildoc.GetAllocator();

                std::string strsdu(p1msg->payload.sdu);
#if 1
                TRACE0OBJ() << strsdu << std::endl; 
#endif

                bool parse_result = this->load(reqdoc, strsdu);
                if(parse_result){
                    nildoc.SetObject();
                    //---------------------------------------------------------------
                    // call hswi message handler
                    //---------------------------------------------------------------
                    this->process_hswi_messages("",
                                                static_cast<const rapidjson::Value &>(reqdoc),
                                                static_cast<rapidjson::Value &>(nildoc),
                                                tmprsp_at);
                }
                break;
            }
            default:{
                TRACE0OBJ()
                    << "ignore the msgid=" << p1msg->hdr.msgid
                    << std::endl;
            }
        };
    }

    return(this->currstate);
} // end of size_t sp_thread::sp_initsync_state()

void sp_thread::sp_initsync_exit()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
}

void sp_thread::process_itc_event()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    itc_queue::elem *p1elem;

    while(p1itc_q[this->thread_id].count() > 0) {
        if((p1elem = p1itc_q[this->thread_id].get()) != NULL){
            this->transit_state(this->nextstate, /*bool reentry*/false);

            // TODO: add more code to handle p1elem->p1comp
            this->dispatch_event((void *)p1elem, EVENT_ITC);

#if 1
            p1elem->p1pool->put(p1elem);
#else
            // loop#0)
            //       p1elem              p1next_comp 
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|------>|     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //
            // loop#1)
            //       in a pool           p1elem           p1next_comp 
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|->NULL |     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //                       
            //                       
            for( ; p1elem != NULL; ) {
                itc_queue::elem *p1next_comp = p1elem->p1comp;  // for next
                p1elem->p1comp = NULL;                          // clear current p1elem
                p1elem->p1pool->put(p1elem);                    // push back the current p1elem to pool
                p1elem = p1next_comp;                           // swap
            }
#endif
        }
    } // end of while(...)

    this->transit_state(this->nextstate, /*bool reentry*/false);
} // end of void sp_thread::process_itc_event()

void sp_thread::process_itc_kickoff()
{
    // not yet defined
}

bool sp_thread::load_param_req__register(const rapidjson::Value& req_node,
                                         const std::string& node_key,
                                         const int &inst_id,
                                         const size_t uid,
                                         rapidjson::Value &rsp_node,
                                         rapidjson::Document::AllocatorType &rsp_at,
                                         size_t item_idx,
                                         size_t num_item)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    bool  status =  true;

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                load_param_req__register(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            if(inst_id == 0){
                // the object "register:0" for hermes2 fw
                rapidjson::Document::AllocatorType& nvreg_at = this->nvreg_json.GetAllocator();

                this->nvreg_json.CopyFrom(req_node, nvreg_at);
            }

            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
}

bool sp_thread::modify_param_req__fpga(const rapidjson::Value& req_node,
                                       const std::string& node_key,
                                       const int &inst_id,
                                       const size_t uid,
                                       rapidjson::Value &rsp_node,
                                       rapidjson::Document::AllocatorType &rsp_at,
                                       size_t item_idx,
                                       size_t num_item)
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }
    bool  status =  true;
    int num_innolink = (this->p1h2top[0].get_minor_pl_version() >= 31)? NUM_INNOLINK: 1;

    switch(req_node.GetType())
    {
        case  rapidjson::kArrayType:{
            rsp_node.SetArray();
            for(rapidjson::Value::ConstValueIterator ita = req_node.Begin(); ita != req_node.End(); ++ita)
            {
                rapidjson::Value    rspval(rapidjson::kObjectType);
                // recursive call
                modify_param_req__fpga(*ita, node_key, inst_id, uid, rspval, rsp_at);
                rsp_node.PushBack(rspval, rsp_at);
            }
            break;
        }

        case  rapidjson::kObjectType: {
            //rapidjson::Document itcdoc;
            //rapidjson::Document::AllocatorType& itc_at = itcdoc.GetAllocator();
            //rapidjson::Value itcnode(rapidjson::kObjectType);

            rsp_node.SetObject();
            //add_member(std::string("result"), std::string("ack"), rsp_node, rsp_at);

            for(rapidjson::Value::ConstMemberIterator itochild = req_node.MemberBegin(); itochild != req_node.MemberEnd(); ++itochild){
                int meminstid;
                std::string currkey = this->split_instance_id(itochild->name.GetString(), meminstid);
                std::string fullkey = node_key + "/" + currkey;
                TRACE0OBJ() << "name=" << itochild->name.GetString() << ", meminstid=" << meminstid << std::endl;

                if((currkey == "serdes") && (meminstid >= 0) && (meminstid < num_innolink)){
                    if(itochild->value.GetType() == rapidjson::kObjectType){
                        //add_member(currkey, itochild->value, rsp_node, rsp_at);
                        for(rapidjson::Value::ConstMemberIterator subchild = itochild->value.MemberBegin();
                            subchild != itochild->value.MemberEnd();
                            ++subchild)
                        {
                            std::string subchildkey = subchild->name.GetString();

                            if(subchildkey == "sysclk_enable"){
                                if(subchild->value.GetType() == rapidjson::kTrueType){
                                    //TRACE0();
                                    p1h2top[0].p1innolink[meminstid].set_sysclk_enable(true);
                                }
                                else if(subchild->value.GetType() == rapidjson::kFalseType){
                                    //TRACE0();
                                    p1h2top[0].p1innolink[meminstid].set_sysclk_enable(false);
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            } 
                            else if(subchildkey == "loopback_enable"){
                                if(subchild->value.GetType() == rapidjson::kTrueType){
                                    //TRACE0();
                                    p1h2top[0].p1innolink[meminstid].reset_serdes(/*bool loopback_enable=*/true);
                                }
                                else if(subchild->value.GetType() == rapidjson::kFalseType){
                                    //TRACE0();
                                    p1h2top[0].p1innolink[meminstid].reset_serdes(/*bool loopback_enable=*/false);
                                }
                                else{
                                    TRACE0() << ">>> HSWI type error" << std::endl;
                                }
                            } 
                            else{
                                rapidjson::Value    nullval(rapidjson::kObjectType);
                                this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                            }
                        }
                    }
                    else{
                        TRACE0() << ">>> HSWI type error" << std::endl;
                    }
                }
                else {
                    rapidjson::Value    nullval(rapidjson::kObjectType);
                    this->exec_param(itochild->value, fullkey, inst_id, uid, nullval, rsp_at);
                }
            }
            break;
        }

        case  rapidjson::kStringType:
        case  rapidjson::kFalseType:
        case  rapidjson::kTrueType:
        case  rapidjson::kNumberType:
        case  rapidjson::kNullType:
        default: {
        }
    }

    return(status);  // true: valid object, false: invalid object
} // end of bool sp_thread::modify_param_req__fpga

void sp_thread::setup_hswi_handler()
{
    //---------------------------------------------------------------------
    // register:0
    //---------------------------------------------------------------------
    this->objid_map["/load_param_req/register"] =
        static_cast<hswi_json::OBJID_FUNC>(&sp_thread::load_param_req__register);

    //---------------------------------------------------------------------
    // fpga:0 from re_thread to sp_thread
    //---------------------------------------------------------------------
    this->objid_map["re:/modify_param_req/fpga"] =
        static_cast<hswi_json::OBJID_FUNC>(&sp_thread::modify_param_req__fpga);
}

void sp_thread::start_sp_thread()
{
    // Initialize and set thread joinable
    pthread_attr_init(&this->threadattr);
    //pthread_attr_setdetachstate(&this->threadattr, PTHREAD_CREATE_JOINABLE);
    sigemptyset(&this->sigmask);   // clear signals
    sigaddset(&this->sigmask, EVENT_ITC); 
    sigaddset(&this->sigmask, EVENT_SOCKET);
    sigaddset(&this->sigmask, SIGPIPE);    // NOTE: double-protection against crash due to SIGPIPE
    if(pthread_sigmask(SIG_BLOCK, &this->sigmask, NULL) != 0){
        perror("pthread_sigmask()");
    }

    pthread_create(&this->thread, &this->threadattr, sp_thread::start_wrapper, (void *)this);
    pthread_detach(this->thread); // mark the thread to return its reources automatically in terminating
} // end of sp_thread::start_sp_thread()

#if defined(__aarch64__)    // for real platform
bool sp_thread::download_tm_file(const itc_download_tm_file_req &download_tm_file_req)
{
    bool status = true;

    std::string cmd1;

    if(download_tm_file_req.scheme == std::string("ftp")){
        // ftpget [OPTIONS] HOST [LOCAL_FILE] REMOTE_FILE
        //         -c       Continue previous transfer
        //         -v       Verbose
        //         -u USER  Username
        //         -p PASS  Password
        //         -P NUM   Port
        cmd1 =
            std::string("ftpget ") +
            std::string("-u ") + std::string(download_tm_file_req.user) + std::string(" ") +
            std::string("-p ") + std::string(download_tm_file_req.password) + std::string(" ") +
            std::string(download_tm_file_req.host) + std::string(" ") +
            std::string(TM_DIR) + "/" + std::string(basename(download_tm_file_req.path)) + std::string(" ") +
            std::string(download_tm_file_req.path);
    }
    else if(download_tm_file_req.scheme == std::string("tftp")){
        // tftp [OPTIONS]   HOST [PORT]
        //       -l FILE    Local FILE
        //       -r FILE    Remote FILE
        //       -g         Get file
        //       -p         Put file
        cmd1 =
            std::string("tftp ") + 
            std::string("-g ") +
            std::string("-l ") + std::string(TM_DIR) + "/" + std::string(basename(download_tm_file_req.path)) + std::string(" ") +
            std::string("-r ") + std::string(download_tm_file_req.path) + std::string(" ") +
            std::string(download_tm_file_req.host);
    }
    else{
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "not supported " << download_tm_file_req.scheme << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    TRACE0OBJ() << "cmd1=" << cmd1 << std::endl;

    if(system(cmd1.c_str()) != 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in system call: " << cmd1 << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    return(status);
} // end of bool sp_thread::download_tm_file()
#endif

void sp_thread::zeroclear_mem(uint64_t addr, uint32_t len)
{
    uint64_t page_addr=(addr & ~(PAGE_SIZE-1));
    uint64_t addr32 = (addr + 0x03) & ~0x03;   // the next 32bit aligned address
    uint64_t kk;

    mmap_hal plddr(page_addr);

    // heading part: byte access
    for(kk = addr; kk < addr32; kk++){
        plddr.wr08(kk, 0);
    }
    // mid part: 32bit align
    for(kk = addr32; kk < (addr + len); kk+=4){
        plddr.wr32(kk, 0);
    }
    // tailing part: byte access
    for( ; kk < (addr + len); kk++){
        plddr.wr08(kk, 0);
    }
} // end of void zeroclear_mem()

void sp_thread::dump_mem(uint64_t addr, uint32_t len, const char *p1dump_file_name, int truncate_bit)
{
    uint64_t    page_addr=(addr & ~(PAGE_SIZE-1));

    mmap_hal    plddr(page_addr);

    uint64_t curraddr = addr & ~0x0f;
    size_t loopcnt = ((addr & 0x0f) + len + 15) >> 4;
    size_t displaylimit = 10;
    size_t displayupper = (size_t)((int)loopcnt - (int)displaylimit);

    FILE *fp = NULL;

    if(p1dump_file_name != NULL) fp = fopen(p1dump_file_name, "wb+");

    //mutex_lock  lock(this->p1mutex);
    uint16_t sign16ext = 0;
    for(int kk = 0; kk < truncate_bit; kk++){
        sign16ext = (1UL << 15) | (sign16ext >> 1);
    }
#if 0
    printf("sign16ext=%04x\n", sign16ext);
#endif

    for(size_t kk = 0; kk < loopcnt; kk++){
        hal::u64le_t addr64le;
        addr64le.u64 = curraddr;
        if((kk < displaylimit) || (kk > displayupper)){
            printf("%02x_%04x_%04x: ", addr64le.u08[4], addr64le.u16le[1].u16, addr64le.u16le[0].u16);
        }

        for(size_t ll = 0; ll < 4; ll++, curraddr+=4) {
            hal::u32le_t u32le;
            u32le.u32 = plddr.rd32(curraddr);

            if((kk < displaylimit) || (kk > displayupper)){
                printf("%02x%02x %02x%02x ", u32le.u08[0], u32le.u08[1], u32le.u08[2], u32le.u08[3]);
            }

            // bit shift left with sign bit extention
            u32le.u16le[0].u16 = (u32le.u16le[0].u16 & 0x8000)? (sign16ext | (u32le.u16le[0].u16 >> truncate_bit)): (u32le.u16le[0].u16 >> truncate_bit);
            u32le.u16le[1].u16 = (u32le.u16le[1].u16 & 0x8000)? (sign16ext | (u32le.u16le[1].u16 >> truncate_bit)): (u32le.u16le[1].u16 >> truncate_bit);

            if(fp != NULL) fwrite(&u32le.u32, sizeof(uint32_t), 1, fp);
        }
        if((kk < displaylimit) || (kk > displayupper)){
            printf("\n");
        }
    }

    if(fp != NULL) fclose(fp);
} // end of void dump_mem()

bool sp_thread::write_s2mm_axidma_desc_fdd_fs8(uint32_t sdu_mask,
                                               uint32_t sdu_target,
                                               const int num_s2mm_desc,
                                               const bool verbose)
{
    //TRACE0();

    bool status = true;

    hal::u64le_t  uldatabase64le;
    hal::u64le_t  uldescbase64le;

    uldatabase64le.u64 = ULDATA_BASE;
    uldescbase64le.u64 = ULDESC_BASE;

    int cp_length[2] = {160, 144};
    int fftsize = 2048;
    // oversampling ratio
    int ovsr = 2;
    uint32_t    offset = uldatabase64le.u32le[0].u32;
    uint32_t    nextdesc = uldescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);
    axidma_sg_hal::axidmadesc p1seg[num_s2mm_desc];

    // clear memory
    memset(&p1seg[0], 0, sizeof(axidma_sg_hal::axidmadesc)*num_s2mm_desc);

    // prepare dma descriptors
    for(int kk = 0; kk < num_s2mm_desc; kk++){
        // header + cp + fftsize
        size_t numsample =
            ((cp_length[0] + fftsize)*2 + (cp_length[1] + fftsize)*12)*10*ovsr;
        p1seg[kk].NEXTDESC = nextdesc;
        p1seg[kk].NEXTDESC_MSB = uldescbase64le.u32le[1].u32;
        p1seg[kk].BUFFER_ADDRESS = offset;
        p1seg[kk].BUFFER_ADDRESS_MSB = uldatabase64le.u32le[1].u32;
        p1seg[kk].CONTROL =
            4*numsample |
            (1UL << axidma_sg_hal::DMADESC_CONTROL__RXSOF_SHL) |
            (1UL << axidma_sg_hal::DMADESC_CONTROL__RXEOF_SHL);

        nextdesc += sizeof(axidma_sg_hal::axidmadesc);

        offset += 4*numsample;
    }

    // write the s2mm_axidma descriptors into the PL_DDR memory
    //for(int kk = 0; kk < num_s2mm_desc; kk++)
    {
        uint64_t addr = ULDESC_BASE;

        mmap_hal plddr(addr);

        int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;

        for(int kk = 0; kk < num_s2mm_desc; kk++) {
            uint32_t *p1u32 = (uint32_t *)(&p1seg[kk]);
            for (int nn = 0; nn < descsize32; nn++, addr+=4){
                plddr.wr32(addr, p1u32[nn]);
            }
        }

    }
    hal::nsleep(100000000);    // 100 msec

    // numbe of frames: 10ms frame-by-frame
    this->p1h2top[0].dmaperi.set_num_descriptors(num_s2mm_desc);
    hal::nsleep(100000000);    // 100 msec

    TRACE0OBJ() << "sdu_mask=" << std::hex << sdu_mask << ", sdu_target=" << std::hex << sdu_target << std::endl;

    // header + cp + fftsize
    size_t numsample =
        ((cp_length[0] + fftsize)*2 + (cp_length[1] + fftsize)*12)*10*ovsr;
    for(int kk = 0; kk < num_s2mm_desc; kk++){
        this->p1h2top[0].dmaperi.set_header_mask(kk, sdu_mask);
    }
    for(int kk = 0; kk < num_s2mm_desc; kk++){
        this->p1h2top[0].dmaperi.set_header_target(kk, sdu_target);
    }
    for(int kk = 0; kk < num_s2mm_desc; kk++){
        this->p1h2top[0].dmaperi.set_burst_payload_size(kk, /*burst_word_size=*/ numsample >> 1);
    }

    return(status);
}   // end of bool write_s2mm_axidma_desc_fdd_fs8()

bool sp_thread::create_capture(const itc_create_capture_req &create_capture_req)
{
    bool status = true;

    this->sdu_mask = create_capture_req.sdu_mask;
    this->sdu_target = create_capture_req.sdu_target;
    this->nums2mmdesc = 2;
    this->off_duty_mode = create_capture_req.off_duty_mode;
    this->scs = create_capture_req.scs;
    this->tdd_ul_dl_config = create_capture_req.tdd_ul_dl_config;
    this->tdd_ssf_config = create_capture_req.tdd_ssf_config;
    this->osr = create_capture_req.osr;

    // step1: reset axidma first ----------------------------
    p1h2top[0].s2mmaxidma.reset_s2mm();

    p1h2top[0].select_il(this->dl_il_sel, this->ul_il_sel);

    // step2: write the s2mm_axidma descriptors -----------------------
    write_s2mm_axidma_desc_fdd_fs8(this->sdu_mask, 
                                   this->sdu_target,
                                   this->nums2mmdesc);


    return(status);
}

bool sp_thread::capture_frc_file(const itc_capture_frc_file_req &capture_frc_file_req)
{
    bool status = true;

    // copied from the write_s2mm_axidma_desc_fdd_fs8()
    {
        hal::u64le_t  uldatabase64le;
        hal::u64le_t  uldescbase64le;

        uldatabase64le.u64 = ULDATA_BASE;
        uldescbase64le.u64 = ULDESC_BASE;

        int cp_length[2] = {160, 144};
        int fftsize = 2048;
        // oversampling ratio
        int ovsr = 2;
        uint32_t    offset = uldatabase64le.u32le[0].u32;
        uint32_t    nextdesc = uldescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);
        axidma_sg_hal::axidmadesc p1seg[this->nums2mmdesc];

        // clear memory
        memset(&p1seg[0], 0, sizeof(axidma_sg_hal::axidmadesc)*this->nums2mmdesc);

        // prepare dma descriptors
        for(int kk = 0; kk < this->nums2mmdesc; kk++){
            // header + cp + fftsize
            size_t numsample =
                ((cp_length[0] + fftsize)*2 + (cp_length[1] + fftsize)*12)*10*ovsr;
            p1seg[kk].NEXTDESC = nextdesc;
            p1seg[kk].NEXTDESC_MSB = uldescbase64le.u32le[1].u32;
            p1seg[kk].BUFFER_ADDRESS = offset;
            p1seg[kk].BUFFER_ADDRESS_MSB = uldatabase64le.u32le[1].u32;
            p1seg[kk].CONTROL =
                4*numsample |
                (1UL << axidma_sg_hal::DMADESC_CONTROL__RXSOF_SHL) |
                (1UL << axidma_sg_hal::DMADESC_CONTROL__RXEOF_SHL);

            nextdesc += sizeof(axidma_sg_hal::axidmadesc);

            offset += 4*numsample;
        }

        // write the s2mm_axidma descriptors into the PL_DDR memory
        //for(int kk = 0; kk < this->nums2mmdesc; kk++)
        {
            uint64_t addr = ULDESC_BASE;

            mmap_hal plddr(addr);

            int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;

            for(int kk = 0; kk < this->nums2mmdesc; kk++) {
                uint32_t *p1u32 = (uint32_t *)(&p1seg[kk]);
                for (int nn = 0; nn < descsize32; nn++, addr+=4){
                    plddr.wr32(addr, p1u32[nn]);
                }
            }

        }
        hal::nsleep(100000000);    // 100 msec

        // numbe of frames: 10ms frame-by-frame
        this->p1h2top[0].dmaperi.set_num_descriptors(this->nums2mmdesc);
        hal::nsleep(100000000);    // 100 msec

        TRACE0OBJ() << "sdu_mask=" << std::hex << this->sdu_mask << ", sdu_target=" << std::hex << this->sdu_target << std::endl;

        // header + cp + fftsize
        size_t numsample =
            ((cp_length[0] + fftsize)*2 + (cp_length[1] + fftsize)*12)*10*ovsr;
        for(int kk = 0; kk < this->nums2mmdesc; kk++){
            this->p1h2top[0].dmaperi.set_header_mask(kk, this->sdu_mask);
        }
        for(int kk = 0; kk < this->nums2mmdesc; kk++){
            this->p1h2top[0].dmaperi.set_header_target(kk, this->sdu_target);
        }
        for(int kk = 0; kk < this->nums2mmdesc; kk++){
            this->p1h2top[0].dmaperi.set_burst_payload_size(kk, /*burst_word_size=*/ numsample >> 1);
        }
    }   // end of bool write_s2mm_axidma_desc_fdd_fs8()

    // step1: reset axidma first ----------------------------
    p1h2top[0].s2mmaxidma.reset_s2mm();

    // step2: clear the status of s2mm axidma descriptors ---
    mmap_hal plddr(ULDESC_BASE);
    int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;
    for(int kk = 0; kk < this->nums2mmdesc; kk++){
        uint64_t addr = 0;

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, STATUS);
        plddr.wr32(addr, 0);

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, USER_APP_0x20);
        plddr.wr32(addr, 0);

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, USER_APP_0x24);
        plddr.wr32(addr, 0);

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, USER_APP_0x28);
        plddr.wr32(addr, 0);

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, USER_APP_0x2C);
        plddr.wr32(addr, 0);

        addr = ULDESC_BASE + kk*descsize32 + offsetof(axidma_sg_hal::s2mm_axidma_desc, USER_APP_0x30);
        plddr.wr32(addr, 0);
    }

    // step3: zero-clear the target buffer ----------------------------
    // TODO: replace the 2nd parameter as a formula
    zeroclear_mem(ULDATA_BASE, 0x4b0000);

    // step4: enable the uplink AXI Stream interface  -----------------
    p1h2top[0].enable_ul_axi_stream(this->ul_il_sel, true);

    // step5: kick the s2mm_axidma  ----------------------------
    p1h2top[0].s2mmaxidma.kick_s2mm(ULDESC_BASE,
                                    ULDESC_BASE + sizeof(axidma_sg_hal::axidmadesc)*(this->nums2mmdesc - 1));

    // step6: kick the dma_peri
    this->p1h2top[0].dmaperi.kick_to_start();
    
    // step7: check status and sleep
    hal::nsleep(1000000000L);  // 1000ms

    // step8: read the target buffer and write to a file
    dump_mem(ULDATA_BASE, this->nums2mmdesc*307200*4, this->frc_file_name.c_str(), /*int truncate_bit=*/ 0);

    struct stat buffer;
    // check if file exists
    if(stat(this->frc_file_name.c_str(), &buffer) != 0){
        return(false);
    }

    std::string localfilename = this->frc_file_name;

    std::string cmd1;

    if(capture_frc_file_req.scheme == std::string("ftp")){
        // ftpput [OPTIONS] HOST [REMOTE_FILE] LOCAL_FILE
        //         -v       Verbose
        //         -u USER  Username
        //         -p PASS  Password
        //         -P NUM   Port
        cmd1 =
            std::string("ftpput ") +
            std::string("-u ") + std::string(capture_frc_file_req.user) + std::string(" ") +
            std::string("-p ") + std::string(capture_frc_file_req.password) + std::string(" ") +
            std::string(capture_frc_file_req.host) + std::string(" ") +
            std::string(capture_frc_file_req.path) + std::string(" ") +
            localfilename + std::string(" ");
    }
#if 0
    else if(upload_frc_file_req.scheme == std::string("sftp")){
        // sshpass -p password -e sftp -o StrictHostKeyChecking=no user@host:path -t"cd incoming; get a_file; bye" 
    }
#endif
    else if(capture_frc_file_req.scheme == std::string("tftp")){
        // tftp [OPTIONS]   HOST [PORT]
        //       -l FILE    Local FILE
        //       -r FILE    Remote FILE
        //       -g         Get file
        //       -p         Put file
        cmd1 =
            std::string("tftp ") + 
            std::string(capture_frc_file_req.host) + std::string(" ") +
            std::string("-p ") +
            std::string("-l ") + localfilename + std::string(" ") +
            std::string("-r ") + std::string(capture_frc_file_req.path) + std::string(" ");
    }
    else{
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "not supported " << capture_frc_file_req.scheme << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    //TRACE0OBJ() << "cmd1=" << cmd1 << std::endl;

    if(system(cmd1.c_str()) != 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in system call: " << cmd1 << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    return(status);
} // end of bool sp_thread::capture_frc_file()


bool sp_thread::upload_frc_file(const itc_upload_frc_file_req &upload_frc_file_req)
{
    bool status = true;

    struct stat buffer;
    // check if file exists
    if(stat(this->frc_file_name.c_str(), &buffer) != 0){
        return(false);
    }

    std::string localfilename = this->frc_file_name;

    std::string cmd1;

    if(upload_frc_file_req.scheme == std::string("ftp")){
        // ftpput [OPTIONS] HOST [REMOTE_FILE] LOCAL_FILE
        //         -v       Verbose
        //         -u USER  Username
        //         -p PASS  Password
        //         -P NUM   Port
        cmd1 =
            std::string("ftpput ") +
            std::string("-u ") + std::string(upload_frc_file_req.user) + std::string(" ") +
            std::string("-p ") + std::string(upload_frc_file_req.password) + std::string(" ") +
            std::string(upload_frc_file_req.host) + std::string(" ") +
            std::string(upload_frc_file_req.path) + std::string(" ") +
            localfilename + std::string(" ");
    }
#if 0
    else if(upload_frc_file_req.scheme == std::string("sftp")){
        // sshpass -p password -e sftp -o StrictHostKeyChecking=no user@host:path -t"cd incoming; put a_file; bye" 
    }
#endif
    else if(upload_frc_file_req.scheme == std::string("tftp")){
        // tftp [OPTIONS]   HOST [PORT]
        //       -l FILE    Local FILE
        //       -r FILE    Remote FILE
        //       -g         Get file
        //       -p         Put file
        cmd1 =
            std::string("tftp ") + 
            std::string(upload_frc_file_req.host) + std::string(" ") +
            std::string("-p ") +
            std::string("-l ") + localfilename + std::string(" ") +
            std::string("-r ") + std::string(upload_frc_file_req.path) + std::string(" ");
    }
    else{
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "not supported " << upload_frc_file_req.scheme << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    //TRACE0OBJ() << "cmd1=" << cmd1 << std::endl;

    if(system(cmd1.c_str()) != 0) {
        std::stringstream sstrm;
        TRACE1OBJ(sstrm) << "error in system call: " << cmd1 << std::endl;
        perror(sstrm.str().c_str());

        return(false);
    }

    return(status);
} // end of bool sp_thread::upload_frc_file()

void sp_thread::print_tmplayback(const itc_playback_req &tmplaybackreq)
{
    TRACE0OBJ() << "num_playbackobj=" << tmplaybackreq.num_playbackobj << std::endl;
    for(int plbk = 0; plbk < tmplaybackreq.num_playbackobj; plbk++){
        const playback_struct *p1playback = &tmplaybackreq.tmplaybackobj[plbk];

        std::cout << "[" << plbk << "]" <<
            " playbackid=" << (int)p1playback->playbackid <<
            " state=" << (int)p1playback->state <<
            " filename=" << p1playback->filename <<
            std::endl;

        std::cout << "numtxant=" << (int)p1playback->numtxant << std::endl;
        std::cout << "txantid=";
        for(int nant = 0; nant < TX_ANT_BITMASK64; nant++){
            std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') <<
                p1playback->txantbitmask[nant] << ", ";
        }
        std::cout << std::dec << std::endl;

        std::cout << "numsection=" << (int)p1playback->numsection << std::endl;
        for(int ns = 0; ns < p1playback->numsection; ns++){
            const section_struct *p1section = &p1playback->section[ns];
            std::cout << "[" << ns << "]" <<
                " section_id=" << (int)p1section->section_id <<
                " frame_structure=" << (int)p1section->frame_structure <<
                " scs=" << (int)p1section->scs <<
                " cp_length=" << (int)p1section->cp_length <<
                " fs=" << p1section->fs <<
                " fs_x=" << (int)p1section->fs_x << 
                std::endl;

            std::cout << "numsymbol=" << p1section->numsymbol << ":";
            for(int nsymb = 0; nsymb < MAX_SYMBOL; nsymb++){
                if(p1section->start_symbol_bitmask & (1UL << nsymb)){
                    std::cout << nsymb << ", ";
                }
            }
            std::cout << std::endl;
        }
    }
} // end of void sp_thread::print_tmplayback()

bool sp_thread::prepare_tmplayback(const itc_playback_req &tmplaybackreq)
{
    bool status = true;
    hal::u64le_t  tmdescbase64le;

    tmdescbase64le.u64 = TMDESC_BASE;

    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        print_tmplayback(tmplaybackreq);
    }

    // step1: select innolink for axidma --------------------
    p1h2top[0].select_il(this->dl_il_sel, this->ul_il_sel);

    // step2: reset axidma first ----------------------------
    p1h2top[0].mm2saxidma.reset_mm2s();

    // if no valid playback, return with status 
    if(tmplaybackreq.num_playbackobj <= 0){
        p1h2top[0].mm2saxidma.read_mm2s_status();
        return(status);
    }

    const std::string tmdir(TM_DIR);
    uint64_t next_tmdata_addr = TMDATA_BASE;

    std::vector<axidma_sg_hal::axidmadesc> dmadescvec;

    // support multiple tm playback objects
    for(int plbkidx = 0; plbkidx < tmplaybackreq.num_playbackobj; plbkidx++){
        for(int nant = 0; nant < TX_ANT_BITMASK64; nant++){
            for(int antbit = 0; antbit < 64; antbit++){
                // for each antenn port out of max 256 (=4x64) antenna ports
                const playback_struct *p1playback = &tmplaybackreq.tmplaybackobj[plbkidx];

                // only for selected antenna port
                if(p1playback->txantbitmask[nant] & (1UL << antbit)){
                    int ruportid = nant*64 + antbit;
                    std::string tmwavpath = tmdir + "/" + p1playback->filename;

                    // 8-bytes (64-bits) aligned
                    uint64_t curr_tmdata_addr = (next_tmdata_addr + 7) & (~(0x7));

                    // step3: load each waveform into the PL-DDR --------------
                    next_tmdata_addr = load_waveform(tmwavpath, curr_tmdata_addr);
                    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
                        TRACE0OBJ()
                            << "filename=" << p1playback->filename
                            << ", curr_tmdata_addr=0x" << std::hex << std::setw(10) << std::setfill('0') << curr_tmdata_addr
                            << ", next_tmdata_addr=0x" << std::hex << std::setw(10) << std::setfill('0') << next_tmdata_addr
                            << std::endl;
                    }

                    // ---------------------------------------------------------
                    // step4: generate dma descriptors for each playbackobj wise
                    // ---------------------------------------------------------
                    gen_axidma_tmdesc(dmadescvec,
                                      p1playback,
                                      curr_tmdata_addr,
                                      plbkidx,
                                      ruportid,
                                      tmwavpath);
                }
            }
        }
    }

    // step5: sort dma descrptors for all playback objs ----------
    std::sort(dmadescvec.begin(), dmadescvec.end(), axidma_sg_hal::axidmadesc::compare);

#if 0
    TRACE0OBJ();
    std::vector<axidma_sg_hal::axidmadesc>::iterator it;
    for(it = dmadescvec.begin(); it != dmadescvec.end(); ++it){
        std::cout << it->USER_APP_TIMESTAMP << std::endl;
    }
#endif

    // step6: write dma descriptors ---------
    uint64_t currtmdescaddr = TMDESC_BASE;

    mmap_hal plddrdesc(currtmdescaddr);

    uint32_t    nextdesc = tmdescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);
    uint32_t    prevts = 0;
    // TODO: revisit here !!!
#if 1
    uint32_t    mintsgap = 0;
#else
    uint32_t    mintsgap = 2000;
#endif
    std::string descfilename("descdl.bin");
    FILE *fpdesc = fopen(descfilename.c_str(), "wb+");
    if(fpdesc == NULL){
        std::stringstream sstrm;
        sstrm << "error in file open: " << descfilename << std::endl;
        perror(sstrm.str().c_str());
        return(false);
    }
    

#if 1
    TRACE0OBJ() 
        << "dmadescvec.size=" << dmadescvec.size() 
        << std::endl;
#endif
    for(size_t kk = 0; kk < dmadescvec.size(); kk++) {
        uint32_t currts = (dmadescvec[kk].USER_APP_TIMESTAMP & USER_APP_TIMESTAMP__TIMESTAMP_M);

        if(kk == 0){ // for the 1st desciptor
            dmadescvec[kk].NEXTDESC     = nextdesc;
            dmadescvec[kk].NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
        }
        else if((int)kk == ((int)dmadescvec.size() - 1)){ // for the last descriptor
            dmadescvec[kk].NEXTDESC     = tmdescbase64le.u32le[0].u32;
            dmadescvec[kk].NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;

            if(currts < (prevts + mintsgap)){
                currts += mintsgap;
            }
        }
        else{
            dmadescvec[kk].NEXTDESC     = nextdesc;
            dmadescvec[kk].NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;

            if(currts < (prevts + mintsgap)){
                currts += mintsgap;
            }
        }
        dmadescvec[kk].USER_APP_TIMESTAMP = 
            (dmadescvec[kk].USER_APP_TIMESTAMP & USER_APP_TIMESTAMP__TX_CONSECUTIVE_M) |
            ((currts % DMA_TS_CNT_MAX) & USER_APP_TIMESTAMP__TIMESTAMP_M);

        prevts = dmadescvec[kk].USER_APP_TIMESTAMP & USER_APP_TIMESTAMP__TIMESTAMP_M;

        int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;
        uint32_t *p1u32 = (uint32_t *)(&dmadescvec[kk]);
        for (int nn = 0; nn < descsize32; nn++){
            plddrdesc.wr32(currtmdescaddr, p1u32[nn]);
            currtmdescaddr += 4;
        }
        fwrite(&dmadescvec[kk], sizeof(axidma_sg_hal::axidmadesc), 1, fpdesc);

        nextdesc += sizeof(axidma_sg_hal::axidmadesc);
    }

    if(fpdesc != NULL){
        fclose(fpdesc);
    }

#ifdef  USE_SYS10MS_INTERRUPT
    // step7: wait for sys10ms interrupt
    {
        // enable sys10ms interrupt
        p1h2top[0].p1innolink[P_IL].enable_sys10ms_interrupt(true);

        int remaincnt = 0;
        if((remaincnt = sys10msintr.wait_for_interrupt()) == 0){
            // blocked until the next sys10ms interrupt
            // this function returns
            // 0 if no interrupt within a predetermined time,
            // positive otherwise
            TRACE0OBJ() << "no sys10ms interrupt: remaincnt=" << remaincnt << std::endl;
        }
        
        // disble sys10ms interrupt
        p1h2top[0].p1innolink[P_IL].enable_sys10ms_interrupt(false);
    }
#endif

    // step8: kick the mm2s_axidma -----------------
    {
        size_t loopcnt = 10;
        size_t delayns = 10000000;  // 10ms

        uint64_t currdesc_addr = tmdescbase64le.u64;
        // NOTE: this taildesc_addr should be out of range for cyclic DMA mode
        uint64_t taildesc_addr = TMDATA_BASE;

        p1h2top[0].mm2saxidma.kick_mm2s(currdesc_addr, taildesc_addr);

        this->p1h2top[0].enable_syncin_dl_uplane(this->dl_il_sel, true);

        std::cout << std::endl;
        for(size_t kk = 0; kk < loopcnt; kk++) {
            p1h2top[0].mm2saxidma.read_mm2s_status();
            hal::nsleep(delayns);
        }
    }

    // step9: display memory info -------------
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        std::cout
            << "tmdescaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << tmdescbase64le.u64
            << "~0x" << std::hex << std::setw(10) << std::setfill('0') << currtmdescaddr
            << std::endl;
    }

    return(status);
} // end of bool sp_thread::prepare_tmplayback()

uint64_t sp_thread::load_waveform(const std::string &waveformfile, const uint64_t &addr)
{
    uint64_t curraddr = addr;

    // file open
    FILE *fp = fopen(waveformfile.c_str(), "rb");
    if(fp == NULL){
        TRACE0() << "error in opening " << waveformfile << std::endl;
        return(curraddr);
    }

    // check the size
    fseek(fp, 0, SEEK_END);
    uint32_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);


    mmap_hal plddrdata(curraddr);

    for(uint32_t kk = 0; kk < (filesize >> 2); kk++, curraddr+=4){
        hal::u32le_t    u32le;
        fread(&u32le.u32, sizeof(hal::u32le_t), 1, fp);
        plddrdata.wr32(curraddr, u32le.u32);
        
#if 0
        if((kk % PAGE_SIZE) == 0) {
            std::cout
                << "curraddr=0x" << std::hex << std::setw(10) << std::setfill('0') << curraddr
                << " data=0x" << std::hex << std::setw(8) << std::setfill('0') << u32le.u32
                << std::endl;
            hal::nsleep(1000);
        }
#endif

    }
    for(uint32_t kk = 0; kk < (filesize - ((filesize >> 2) << 2)); kk++, curraddr++){
        uint8_t u08;
        fread(&u08, sizeof(uint8_t), 1, fp);
        plddrdata.wr08(curraddr, u08);
    }
    if(g_debug_mask & (1UL << MASKBIT_TMPLAYBACK)){
        TRACE0OBJ()
            << "tmdataaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << addr
            << "~0x" << std::hex << std::setw(10) << std::setfill('0') << curraddr
            << std::endl;
    }

    fclose(fp);

    return(curraddr);
} // end of uint64_t sp_thread::load_waveform()

std::vector<int> sp_thread::gen_valid_tdd_symb_list(int tdd_ul_dl_config, int tdd_ssf_config, int ul_only)
{
    std::vector<int> symblist;
    bool istdd = false;
    //TRACE0OBJ() << "tdd_ul_dl_config=" << tdd_ul_dl_config << ", tdd_ssf_config=" << tdd_ssf_config << std::endl;

    if(tdd_ul_dl_config == 2){
        int num_symb = 14;

        switch(tdd_ssf_config){
            case    6: { //  9 OFDM symbols for Dw and 2 OFDM symbols for Up
                int sf = 0;

                if(ul_only == 0){    // downlink as the default
                    istdd = true;

                    for(sf = 0; sf < 1; sf++){ // DL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 1; sf < 2; sf++){ // SSF
                        for(int symb = 0; symb < 9; symb++){   // ssf
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 3; sf < 6; sf++){ // DL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 6; sf < 7; sf++){ // SSF
                        for(int symb = 0; symb < 9; symb++){   // ssf
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 8; sf < 10; sf++){ // DL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    // DL OFDM symbols over 10ms: 102 = 6x14 + 2x9
                    TRACE0OBJ() << "the number of symbols: 102, " << ((102 == symblist.size())? "true":"false") << std::endl;
                }
                else if(ul_only == 1){ // uplink
                    istdd = true;

                    for(sf = 1; sf < 2; sf++){ // SSF
                        for(int symb = 12; symb < num_symb; symb++){   // ssf
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 2; sf < 3; sf++){ // UL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 6; sf < 7; sf++){ // SSF
                        for(int symb = 12; symb < num_symb; symb++){   // ssf
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 7; sf < 8; sf++){ // UL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    // DL OFDM symbols over 10ms: 102 = 2x14 + 2x2
                    TRACE0OBJ() << "the number of symbols: 32, " << ((32 == symblist.size())? "true":"false") << std::endl;
                }
                break;
            }
            case    7: { // 10 OFDM symbols for Dw and 2 OFDM symbols for Up
                int sf = 0;

                if(ul_only == 0){    // downlink
                    istdd = true;

                    for(sf = 0; sf < 1; sf++){ // DL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 1; sf < 2; sf++){ // SSF
                        for(int symb = 0; symb < 10; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 3; sf < 6; sf++){ // DL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 6; sf < 7; sf++){ // SSF
                        for(int symb = 0; symb < 10; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 8; sf < 10; sf++){ // SSF
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    // DL OFDM symbols over 10ms: 104 = 6x14 + 2x10
                    TRACE0OBJ() << "the number of symbols: 104, " << ((104 == symblist.size())? "true":"false") << std::endl;
                }
                else if(ul_only == 1){ // uplink
                    istdd = true;

                    for(sf = 1; sf < 2; sf++){ // SSF
                        for(int symb = 12; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 2; sf < 3; sf++){ // UL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 6; sf < 7; sf++){ // SSF
                        for(int symb = 12; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    for(sf = 7; sf < 8; sf++){ // UL
                        for(int symb = 0; symb < num_symb; symb++){
                            symblist.push_back(sf*num_symb + symb);
                        }
                    }
                    // DL OFDM symbols over 10ms: 102 = 2x14 + 2x2
                    TRACE0OBJ() << "the number of symbols: 32, " << ((32 == symblist.size())? "true":"false") << std::endl;
                }
                break;
            }
            default:{
                TRACE0OBJ() << "not yet implemented for the tdd_ul_dl_config=" << tdd_ul_dl_config << std::endl;
            }
        }
    }
    else{
        TRACE0OBJ() << "not yet implemented for the tdd_ul_dl_config=" << tdd_ul_dl_config << std::endl;
    }

    if(istdd == false){
        symblist.clear();
        for(int kk = 0; kk < 280; kk++){
            symblist.push_back(kk);
        }
    }

    return(symblist);
} // end of std::vector<int> sp_thread::gen_valid_tdd_symb_list(int tdd_ul_dl_config, int tdd_ssf_config, int ul_only)

size_t sp_thread::gen_axidma_tmdesc(std::vector<axidma_sg_hal::axidmadesc> &dmadescvec,
                                    const playback_struct *p1playback,
                                    uint64_t data_addr,
                                    int playback_idx,
                                    int ruport_id,
                                    const std::string &tmwav_path)
{
    // generate dma descriptors for each playbackobj wise
    if(g_debug_mask & ((1UL << MASKBIT_CALL) | (1UL << MASKBIT_EVENT))){
        TRACE0();
    }

    std::string gatedwavpath = std::string("tmwav_") + std::to_string(ruport_id) + std::string(".bin");
    size_t count = 0;
    const int NUM_SF = 10;
    int num_segments = (p1playback->num_segments < 1)? 4: p1playback->num_segments;
    int segmentgap = 2000;

    int num_symb_per_sf = 14;
    int u = p1playback->section[0].scs; // numerology

    // the section[0].scs represens the common scs, because all sections shoud have the same scs.
    // CP length:
    //      (144*(2**-u) + 16)*Ts/Tc   for l=0 or l=7*(2**u)
    //       144*(2**-u)*Ts/Tc         for other symbols
    // , where Ts = 1/30.72 MHz and Tc = 1/sample_rate_in_Hz
    switch(u){
        case 0:{    // numerology=0 for 15KHz
            // short symbol duration for either LTE20 or NR50:
            //      <a> = {2048 + 144*(2**0)*30.72/30.72}/30.72 = 71.354 us
            //      or
            //      <a> = {4096 + 144*(2**0)*61.44/30.72}/61.44 = 71.354 us
            // number of symbols in DMA_TS_CLK_IN_MHZ Msps which is the AXIDMA timer clock:
            //      <b> = <a>*DMA_TS_CLK_IN_MHZ = 8768
            // evenly distributed gap between segments in DMA_TS_CLK_IN_MHZ clock
            //      <c> = floor(<b> / num_segments)
            num_symb_per_sf = 14;
            segmentgap = (int)((2048 + 144*pow<double>(2.0, -u)*30.72/30.72)/30.72*DMA_TS_CLK_IN_MHZ/num_segments);
            break;
        }
        case 1:{    // numerology=1 for 30KHz
            // short symbol duration for either NT50 or NR100:
            //      <a> = {2048 + 144*(2**-1)*61.44/30.72}/61.44 = 35.677 us
            //      or
            //      <a> = {4096 + 144*(2**-1)*DMA_TS_CLK_IN_MHZ/30.72}/DMA_TS_CLK_IN_MHZ = 35.677 us
            // number of symbols in DMA_TS_CLK_IN_MHZ Msps which is the AXIDMA timer clock:
            //      <b> = <a>*DMA_TS_CLK_IN_MHZ = 4384
            // evenly distributed gap between segments in DMA_TS_CLK_IN_MHZ clock
            //      <c> = floor(<b> / num_segments)
            num_symb_per_sf = 28;
            segmentgap = (int)((4096 + 144*pow<double>(2.0, -u)*DMA_TS_CLK_IN_MHZ/30.72)/30.72*DMA_TS_CLK_IN_MHZ/num_segments);
            break;
        }
        case 2:     // numerology=2 for 60KHz
        case 13:    // 3.75KHz
        case 15:    // 7.5KHz
        default: {
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "does not support: scs=" << u << std::endl;
            perror(sstrm.str().c_str());
            return(count);
        }
    }
    TRACE0OBJ() << "segmentgap=" << segmentgap << ", num_segments=" << num_segments << std::endl;

    std::vector<section_struct> symbinfolist;
    // zero initialize
    for(int kk = 0; kk < num_symb_per_sf; kk++){
        section_struct  dummysection;
        memset(&dummysection, 0, sizeof(section_struct));
        symbinfolist.push_back(dummysection);
    }
    for(int kk = 0; kk < p1playback->numsection; kk++){
        for(int ll = 0; ll < num_symb_per_sf; ll++) {
            if(p1playback->section[kk].start_symbol_bitmask & (1UL << ll)){
                //TRACE0OBJ() << "symbol=" << ll << std::endl;
                // copy this section to the symbinfolist
                symbinfolist[ll] = p1playback->section[kk];
            }
        }
    }

    // the section[0].frame_structure represens the common frame_structure,
    // because all sections shoud have the same frame_structure.
    switch(p1playback->section[0].frame_structure){
        case 0:{    // FS 8
            int fftsize = (int)(2048*p1playback->section[0].fs_x/30720);
            //TRACE0OBJ() << "fftsize=" << fftsize << std::endl;
            for(int kk = 0; kk < num_symb_per_sf; kk++){
                symbinfolist[kk].fftsize = fftsize;
            }

            int half_symb_per_sf = num_symb_per_sf >> 1;

            if(u <= 2){    // numerology=0 for 15KHz
                int kk = 0;
                symbinfolist[kk].cp_length = (int)((144*pow<double>(2.0, -u) + 16)*p1playback->section[0].fs_x/30720);
                for(kk = 1; kk < half_symb_per_sf; kk++){
                    symbinfolist[kk].cp_length = (int)(144*pow<double>(2.0, -u)*p1playback->section[0].fs_x/30720);
                }
                kk = half_symb_per_sf;
                symbinfolist[kk].cp_length = (int)((144*pow<double>(2.0, -u) + 16)*p1playback->section[0].fs_x/30720);
                for(kk = half_symb_per_sf + 1; kk < num_symb_per_sf; kk++){
                    symbinfolist[kk].cp_length = (int)(144*pow<double>(2.0, -u)*p1playback->section[0].fs_x/30720);
                }
            }
            else {
                // TODO:
            }
            break;
        }
        case 9:{    // 512
            for(int kk = 0; kk < num_symb_per_sf; kk++){
                symbinfolist[kk].fftsize = 512;
                symbinfolist[kk].cp_length = 0;
            }
            break;
        }
        case 10:{   // 1024
            for(int kk = 0; kk < num_symb_per_sf; kk++){
                symbinfolist[kk].fftsize = 1024;
                symbinfolist[kk].cp_length = 0;
            }
            break;
        }
        case 11:{   // 2048
            for(int kk = 0; kk < num_symb_per_sf; kk++){
                symbinfolist[kk].fftsize = 2048;
                symbinfolist[kk].cp_length = 0;
            }
            break;
        }
        case 12:{   // 4096
            for(int kk = 0; kk < num_symb_per_sf; kk++){
                symbinfolist[kk].fftsize = 4096;
                symbinfolist[kk].cp_length = 0;
            }
            break;
        }
        case 13:    // 1536
        default: {
            std::stringstream sstrm;
            TRACE1OBJ(sstrm) << "does not support: frame_structure=" << p1playback->section[0].frame_structure << std::endl;
            perror(sstrm.str().c_str());
            return(count);
        }
    }

    std::vector<int> validsymblist = this->gen_valid_tdd_symb_list(p1playback->tdd_ul_dl_config, p1playback->tdd_ssf_config, p1playback->ul_only);
    if((validsymblist.size() != 140) && (validsymblist.size() != 280)) {
        // print the validsymblist for gated(dtx) mode
        TRACE0OBJ();
        for(std::vector<int>::iterator it=validsymblist.begin(); it != validsymblist.end(); ++it){
            std::cout << *it << ", ";
        }
        std::cout << std::endl;
    }

    // base address for waveform
    hal::u64le_t  tmdatabase64le;
    tmdatabase64le.u64 = data_addr;

    FILE *fpi = fopen(tmwav_path.c_str(), "rb");
    FILE *fpo = fopen(gatedwavpath.c_str(), "wb+");

    uint32_t    symbol_ts_in_clk = (uint32_t)(DMA_TS_CNT_MAX - (int)(p1playback->tm_timing_adv*1e-3*DMA_TS_CLK_IN_KHZ))%DMA_TS_CNT_MAX;
    TRACE0OBJ() << "tm_timing_adv_in_clk=" << symbol_ts_in_clk << " for playback_idx=" << playback_idx << std::endl;

    uint32_t    offset = tmdatabase64le.u32le[0].u32;

    int ts_clock_ratio = DMA_TS_CLK_IN_KHZ/(symbinfolist[0].osr*symbinfolist[0].fs_x);

    int symbpos = 0;
    for(int kk = 0; kk < NUM_SF; kk++){
        for(int ll = 0; ll < num_symb_per_sf; ll++){
            size_t  numsamples = symbinfolist[ll].osr*(symbinfolist[ll].cp_length + symbinfolist[ll].fftsize);

            symbpos = kk*num_symb_per_sf + ll;

            bool isgated = (p1playback->off_duty_mode == 1);  // 0: "zeropad", 1: "dtx"
            bool isvalidsymbol = (std::find(validsymblist.begin(), validsymblist.end(), symbpos) != validsymblist.end());
#if 0
            TRACE0OBJ()
                << "sf=" << kk << ", symb=" << ll
                << ", symbpos=" << symbpos
                << ", isvalidsymbol=" << isvalidsymbol
                << std::endl;
#endif
            // if zeropad mode or valid symbol
            if((isgated == false) || isvalidsymbol){
                size_t  seg_numsamples = numsamples/num_segments;
                uint32_t    bytesize = sizeof(uint32_t)*seg_numsamples;
                // Matt said that USER_APP_WORDLENGTH should be seg_numsamples/2 + 1
                size_t  seg_wordsize = (seg_numsamples >> 1) + 1;

                for(int ss = 0; ss < num_segments; ss++){
                    axidma_sg_hal::axidmadesc  descriptor;

                    // zero clear
                    memset(&descriptor, 0, sizeof(axidma_sg_hal::axidmadesc));

                    descriptor.BUFFER_ADDRESS = offset;
                    descriptor.BUFFER_ADDRESS_MSB = tmdatabase64le.u32le[1].u32;
                    descriptor.CONTROL = 
                        bytesize |
                        (1UL << axidma_sg_hal::DMADESC_CONTROL__TXSOF_SHL) |
                        (1UL << axidma_sg_hal::DMADESC_CONTROL__TXEOF_SHL);
                    descriptor.USER_APP_SDUHEADER_MSB = GEN_IL_SDU_HEADER_MSB32(0, seg_wordsize, 0, ruport_id, symbinfolist[ll].section_id, kk, ll);
                    descriptor.USER_APP_SDUHEADER_LSB = GEN_IL_SDU_HEADER_LSB32(0, seg_wordsize, 0, ruport_id, symbinfolist[ll].section_id, kk, ll);

                    // Matt said that USER_APP_WORDLENGTH should be seg_numsamples/2 + 1
                    descriptor.USER_APP_WORDLENGTH = seg_wordsize;
                    // NOTE: to avoid of duplicated timestamp between playbackobj
                    descriptor.USER_APP_TIMESTAMP = (symbol_ts_in_clk + ss*segmentgap + playback_idx);
                    if(playback_idx > 0){
                        descriptor.USER_APP_TIMESTAMP |= USER_APP_TIMESTAMP__TX_CONSECUTIVE_M;
                    }

                    dmadescvec.push_back(descriptor);
                    count++;

                    if((fpi != NULL) && (fpo != NULL)){
                        uint8_t p1wavbuf[sizeof(uint32_t)];

                        // copy data from the tmwav_path to the gatedwavpath
                        for(size_t cpcnt = 0; cpcnt < seg_numsamples; cpcnt++){
                            fread(p1wavbuf, sizeof(uint32_t), 1, fpi);
                            fwrite(p1wavbuf, sizeof(uint32_t), 1, fpo);
                        }
                    }

                    offset += bytesize;
#if 0
                    TRACE0OBJ()
                        << "bytesize=" << bytesize
                        << ", offset=" << offset
                        << std::endl;
#endif
                }
            }
            else{
                uint32_t bytesize =  sizeof(uint32_t)*numsamples;

                if((fpi != NULL) && (fpo != NULL)){
                    uint8_t p1wavbuf[sizeof(uint32_t)];

                    // jump 
                    fseek(fpi, bytesize, SEEK_CUR);
                    // zero insertion
                    memset(p1wavbuf, 0, sizeof(uint32_t));
                    for(size_t cpcnt = 0; cpcnt < numsamples; cpcnt++){
                        fwrite(p1wavbuf, sizeof(uint32_t), 1, fpo);
                    }
                }

                offset += bytesize;
#if 0
                TRACE0OBJ()
                    << "bytesize=" << bytesize
                    << ", offset=" << offset
                    << std::endl;
#endif
            }

            symbol_ts_in_clk += (uint32_t)round(numsamples*ts_clock_ratio);
#if 0
            TRACE0OBJ() << "symbol_ts_in_clk=" << symbol_ts_in_clk << std::endl;
#endif
        }
    }

    if(fpo != NULL) fclose(fpo);
    if(fpi != NULL) fclose(fpi);

    return(count);
}   // end of bool gen_axidma_tmdesc()

void *sp_thread::start_wrapper(void *p1arg)
{
    sp_thread *p1thread = (sp_thread *)p1arg;

    p1thread->run_sp_thread();

    return((void *)p1thread);
} // end of sp_thread::start_wrapper(void *p1arg)

void sp_thread::run_sp_thread()
{
    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0OBJ();
    }

    // TODO: to check why this line makes app crashed
    transit_state(SP_IDLE_S, false);

    // infinite loop
    for( ; ; ) {
        int recvsig;    // for sigwait()

        if(sigwait(&sigmask, &recvsig) != 0){
            std::stringstream sstrm;
            perror(sstrm.str().c_str());
        }

        switch(recvsig) {
            case EVENT_ITC: {
                process_itc_event();
                break;
            }


            case SIGINT: { // usually cntl+c
                std::stringstream sstrm;
                TRACE1OBJ(sstrm) << "received SIGINT" << std::endl;
                perror(sstrm.str().c_str());
                break;
            }

            case SIGTERM:
            case SIGALRM:
            default:;
        }
    }

    if(g_debug_mask & (1UL << MASKBIT_EVENT)){
        TRACE0OBJ() << "thread exit" << std::endl;
    }

    // prepare for thread termination after pthread_create()
    pthread_attr_destroy(&this->threadattr);
    pthread_exit(NULL); // 
} // end of void run_sp_thread()
