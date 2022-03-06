#
# This file is the h2app recipe.
#

SUMMARY = "Simple h2app appl ication"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
 
#FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI = "file://version_info.c\
	   file://version_info.h \
	   file://Makefile \
	   file://env.mk \
	" 

SRC_URI += "file://h2a0/main_h2a0.cpp"
SRC_URI += "file://h2a0/num_def.h"
SRC_URI += "file://h2a0/nvdata.json"
SRC_URI += "file://h2a0/version.cpp"
SRC_URI += "file://h2a0/version.h"
SRC_URI += "file://carrier/ant_carrier.cpp"
SRC_URI += "file://carrier/ant_carrier.h"
SRC_URI += "file://carrier/rx_carrier.cpp"
SRC_URI += "file://carrier/rx_carrier.h"
SRC_URI += "file://carrier/tx_carrier.cpp"
SRC_URI += "file://carrier/tx_carrier.h"
SRC_URI += "file://hal/axidma_sg_hal.cpp"
SRC_URI += "file://hal/axidma_sg_hal.h"
SRC_URI += "file://hal/axigpio_hal.cpp"
SRC_URI += "file://hal/axigpio_hal.h"
SRC_URI += "file://hal/dma_peri_hal.cpp"
SRC_URI += "file://hal/dma_peri_hal.h"
SRC_URI += "file://hal/h2_fpga_reg.h"
SRC_URI += "file://hal/h2top_hal.cpp"
SRC_URI += "file://hal/h2top_hal.h"
SRC_URI += "file://hal/hal.cpp"
SRC_URI += "file://hal/hal.h"
SRC_URI += "file://hal/il_ctrl.h"
SRC_URI += "file://hal/innolink_dig.h"
SRC_URI += "file://hal/innolink_hal.cpp"
SRC_URI += "file://hal/innolink_hal.h"
SRC_URI += "file://hal/innolink_hal_sim.cpp"
SRC_URI += "file://hal/mmap_hal.cpp"
SRC_URI += "file://hal/mmap_hal.h"
SRC_URI += "file://hal/mmap_hal_sim.cpp"
SRC_URI += "file://hal/offs_mmap_hal.cpp"
SRC_URI += "file://hal/offs_mmap_hal.h"
SRC_URI += "file://hal/xil_gth.h"

SRC_URI  += "file://hswi/hswi_thread.cpp"
SRC_URI  += "file://hswi/hswi_thread.h"
SRC_URI  += "file://hswi/mplane_mon_thread.cpp"
SRC_URI  += "file://hswi/mplane_mon_thread.h"
SRC_URI  += "file://il/il_mplane_deframer.cpp"
SRC_URI  += "file://il/il_mplane_deframer.h"
SRC_URI  += "file://il/il_thread.cpp"
SRC_URI  += "file://il/il_thread.h"
SRC_URI  += "file://il/il_thread_sim.cpp"
SRC_URI  += "file://re/re_thread.cpp"
SRC_URI  += "file://re/re_thread.h"

SRC_URI  += "file://sp/sp_thread.cpp"
SRC_URI  += "file://sp/sp_thread.h"
SRC_URI  += "file://sp/sp_thread_sim.cpp"

SRC_URI  += "file://tcpserv/tcpserv_thread.h"
SRC_URI  += "file://tcpserv/tcpserv_thread.cpp"
SRC_URI  += "file://time/time_thread.cpp"
SRC_URI  += "file://time/time_thread.h"


SRC_URI  += "file://util/blocking_interrupt.cpp"
SRC_URI  += "file://util/blocking_interrupt.h"
SRC_URI  += "file://util/hswi_json.cpp"
SRC_URI  += "file://util/hswi_json.h"
SRC_URI  += "file://util/itc_msg.h"
SRC_URI  += "file://util/itc_queue.cpp"
SRC_URI  += "file://util/itc_queue.h"
SRC_URI  += "file://util/mutex_lock.h"
SRC_URI  += "file://util/rapidjson_wrapper.h"
SRC_URI  += "file://util/stm.cpp"
SRC_URI  += "file://util/stm.h"
SRC_URI  += "file://util/thread_base.cpp"
SRC_URI  += "file://util/thread_base.h"
SRC_URI  += "file://util/thread_timer.h"
SRC_URI  += "file://util/timed_func.cpp"
SRC_URI  += "file://util/timed_func.h"
SRC_URI  += "file://util/wdt_ctrl.cpp"
SRC_URI  += "file://util/wdt_ctrl.h"





SRC_URI  += "file://h2b0emu102/main_h2b0emu102.cpp"
SRC_URI  += "file://h2b0emu102/num_def.h"
SRC_URI  += "file://h2b0emu102/nvdata.json"
SRC_URI  += "file://h2b0emu102/version.cpp"
SRC_URI  += "file://h2b0emu102/version.h"



SRC_URI  += "file://rapidjson/allocators.h"
SRC_URI  += "file://rapidjson/cursorstreamwrapper.h"
SRC_URI  += "file://rapidjson/document.h"
SRC_URI  += "file://rapidjson/encodedstream.h"
SRC_URI  += "file://rapidjson/encodings.h"

SRC_URI  += "file://rapidjson/filereadstream.h"
SRC_URI  += "file://rapidjson/filewritestream.h"
SRC_URI  += "file://rapidjson/fwd.h"

SRC_URI  += "file://rapidjson/istreamwrapper.h"
SRC_URI  += "file://rapidjson/memorybuffer.h"
SRC_URI  += "file://rapidjson/memorystream.h"

SRC_URI  += "file://rapidjson/ostreamwrapper.h"
SRC_URI  += "file://rapidjson/pointer.h"
SRC_URI  += "file://rapidjson/prettywriter.h"
SRC_URI  += "file://rapidjson/rapidjson.h"
SRC_URI  += "file://rapidjson/reader.h"
SRC_URI  += "file://rapidjson/schema.h"
SRC_URI  += "file://rapidjson/stream.h"
SRC_URI  += "file://rapidjson/stringbuffer.h"
SRC_URI  += "file://rapidjson/writer.h"


SRC_URI  += "file://rapidjson/error/en.h"
SRC_URI  += "file://rapidjson/error/error.h"


SRC_URI  += "file://rapidjson/internal/biginteger.h"
SRC_URI  += "file://rapidjson/internal/clzll.h"
SRC_URI  += "file://rapidjson/internal/diyfp.h"
SRC_URI  += "file://rapidjson/internal/dtoa.h"
SRC_URI  += "file://rapidjson/internal/ieee754.h"
SRC_URI  += "file://rapidjson/internal/itoa.h"
SRC_URI  += "file://rapidjson/internal/meta.h"
SRC_URI  += "file://rapidjson/internal/pow10.h"
SRC_URI  += "file://rapidjson/internal/regex.h"
SRC_URI  += "file://rapidjson/internal/stack.h"
SRC_URI  += "file://rapidjson/internal/strfunc.h"
SRC_URI  += "file://rapidjson/internal/strtod.h"
SRC_URI  += "file://rapidjson/internal/swap.h"


SRC_URI  += "file://rapidjson/msinttypes/inttypes.h"
SRC_URI  += "file://rapidjson/msinttypes/stdint.h"


  
S = "${WORKDIR}"

#function gen_version_file {

gen_version_file() {

    FILE_VER="${THISDIR}/files/version_info.c"

    #       git rev-parse --abbrev-ref --symbolic-full-name @{u}
    git_branch_d="$(git rev-parse --abbrev-ref HEAD)"
    git_commit_d="$(git rev-parse --verify HEAD)"
    git_tag_d="$(git --no-pager describe --tags --always)"
    git_commit_date_d="$(git --no-pager show --date=iso8601 --format="%ad" --name-only |head -n 1)"
    build_date_d="$(date --iso=seconds)"


cat <<EOF >$FILE_VER
/* PW version_info file */
/* version_info.c */
#include "version_info.h"

const char * git_branch = "${git_branch_d}";
const char * git_commit = "${git_commit_d}";
const char * git_tag = "${git_tag_d}";
const char * git commit_date = "${git_commit_date_d}";
const char * build_date = "${build_date_d}";
const char * name_recipe = "${PN}";
const char * revision_recipe = "${PR}";
const char * version_recipe = "${PV}";
EOF



    cp ${FILE_VER} $1

}









do_compile() {
	

    #    gen_version_file ${S}
        
        echo "${S}"
        cd ${S}
        oe_runmake h2a0
      #  make all
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 ${S}/_out/h2app.elf ${D}${bindir}/h2app4g.elf
}
