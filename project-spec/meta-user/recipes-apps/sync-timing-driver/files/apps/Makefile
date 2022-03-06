#/****************************************************************************************/
#/**                  Copyright (c) 2019, 2021 Skyworks Solution Inc.                   **/
#/****************************************************************************************/
#/** This software is provided 'as-is', without any express or implied warranty.        **/
#/** In no event will the authors be held liable for any damages arising from the use   **/
#/** of this software.                                                                  **/
#/** Permission is granted to anyone to use this software for any purpose, including    **/
#/** commercial applications, and to alter it and redistribute it freely, subject to    **/
#/** the following restrictions:                                                        **/
#/** 1. The origin of this software must not be misrepresented; you must not claim that **/
#/**    you wrote the original software. If you use this software in a product,         **/
#/**    an acknowledgment in the product documentation would be appreciated but is not  **/
#/**    required.                                                                       **/
#/** 2. Altered source versions must be plainly marked as such, and must not be         **/
#/**    misrepresented as being the original software.                                  **/
#/** 3. This notice may not be removed or altered from any source distribution.         **/
#/****************************************************************************************/

PROJECT_ROOT = $(dir $(CURDIR))
ifeq ($(SYNC_TIMING_PKG_FILE),)
	PKG_INFO_FILE=pkg_info_default.mk
else
	PKG_INFO_FILE=$(SYNC_TIMING_PKG_FILE)
endif
include $(PROJECT_ROOT)/$(PKG_INFO_FILE)

PTP4L_CFLAGS :=
ifeq (si5388,$(SYNC_TIMING_BUILD_CHIPSET))
	PTP4L_CFLAGS += '-DSYNC_TIMING_CHIP_TYPE=SI5388'
endif

ifeq (si5348,$(SYNC_TIMING_BUILD_CHIPSET))
	PTP4L_CFLAGS += '-DSYNC_TIMING_CHIP_TYPE=SI5348'
endif

ifeq (aruba,$(SYNC_TIMING_BUILD_CHIPSET))
	PTP4L_CFLAGS += '-DSYNC_TIMING_CHIP_TYPE=ARUBA'
endif

all: sync_apps

sync_timing_driver_test_app:
ifneq (none,$(SYNC_TIMING_INT_TOOLS_PKG_TYPE))
	$(MAKE) -C sync_timing_driver_test
else
	@echo "Donot build driver_test_app"
endif

si5388_fw_update_app:
	$(MAKE) -C si5388_fw_update

sync_timing_util_app:
ifneq (none,$(SYNC_TIMING_UTIL_PKG_TYPE))
	$(MAKE) -C sync_timing_util
else
	@echo "Donot build sync_timing_util"
endif

sync_timing_slaveloader_app:
ifneq (none,$(SYNC_TIMING_INT_TOOLS_PKG_TYPE))
	$(MAKE) -C sync_timing_slaveloader
else
	@echo "Donot build util_slaveloader"
endif

sync_timing_pktanalyzer_app:
ifneq (none,$(SYNC_TIMING_INT_TOOLS_PKG_TYPE))
	$(MAKE) -C sync_timing_pktanalyzer
else
	@echo "Donot build util_pktanalyzer"
endif

sync_timing_ptp2stack_app:
ifneq (none,$(SYNC_TIMING_SYNCPTP_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2stack
else
	@echo "Donot build sync_timing_ptp2stack"
endif

sync_timing_ptp2config_app:
ifneq (none,$(SYNC_TIMING_SYNCPTP2CONFIG_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2config
else
	@echo "Donot build sync_timing_ptp2config"
endif

sync_timing_esmc_app:
ifneq (none,$(SYNC_TIMING_ESMC_PKG_TYPE))
	$(MAKE) -C sync_timing_esmc
else
	@echo "Donot build sync_timing_esmc"
endif

sync_timing_esmcconfig_app:
ifneq (none,$(SYNC_TIMING_ESMC_PKG_TYPE))
	$(MAKE) -C sync_timing_esmcconfig
else
	@echo "Donot build sync_timing_esmcconfig"
endif

sync_timing_ptp2applib_tester_app:
ifneq (none,$(SYNC_TIMING_SYNCPTP2APPLIBTESTER_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2applib_tester
else
	@echo "Donot build sync_timing_ptp2applib_tester"
endif

linuxptp_build: 
ifneq (none,$(SYNC_TIMING_LINUXPTP_PKG_TYPE))
	@echo "Generate LinuxPTP binaries"
	mkdir -p $(PROJECT_ROOT)/output/linuxptp 
	make -C linuxptp ARCH=${TARGET_ARCH} EXTRA_CFLAGS='-D_GNU_SOURCE -DHAVE_CLOCK_ADJTIME -DHAVE_POSIX_SPAWN -DHAVE_ONESTEP_SYNC -DOS_LINUX=1 -DSYNC_TIMING_HOST_LITTLE_ENDIAN -DUSE_SYNC_TIMING_DRIVER_API -DSYNC_TIMING_LOG_SUPPORTED -DSYNC_TIMING_ACCUTIME_SUPPORTED -DSYNC_TIMING_TODINPUT_SUPPORTED -DSYNC_TIMING_PTPCLOCK_SUPPORTED $(PTP4L_CFLAGS) -I$(PROJECT_ROOT)/include/ -I$(PROJECT_ROOT)/api/inc/ -I$(PROJECT_ROOT)/oem/' EXTRA_LDFLAGS='-L$(PROJECT_ROOT)/lib/ -lapi_sync_timing -losal_sync_timing -llog_sync_timing -liniparser_sync_timing -loem_sync_timing -lrt -lpthread' 
	mv $(PROJECT_ROOT)/apps/linuxptp/ptp4l $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/hwstamp_ctl $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/nsm $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/phc2sys $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/phc_ctl $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/pmc $(PROJECT_ROOT)/output/linuxptp 
	mv $(PROJECT_ROOT)/apps/linuxptp/timemaster $(PROJECT_ROOT)/output/linuxptp
else
	@echo "Donot build linuxptp"
endif

linuxptp_clean:
ifneq (none,$(SYNC_TIMING_LINUXPTP_PKG_TYPE))
	cd $(PROJECT_ROOT)/apps/linuxptp && make clean && rm -rf $(PROJECT_ROOT)/output/linuxptp
endif

sync_apps: linuxptp_build
ifneq (none,$(SYNC_TIMING_UTIL_PKG_TYPE))
	$(MAKE) -C sync_timing_util 
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2stack
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP2CONFIG_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2config
endif
ifneq (none,$(SYNC_TIMING_ESMC_PKG_TYPE))
	$(MAKE) -C sync_timing_esmc
	$(MAKE) -C sync_timing_esmcconfig
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP2APPLIBTESTER_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2applib_tester
endif
ifneq (none,$(SYNC_TIMING_INT_TOOLS_PKG_TYPE))
	$(MAKE) -C sync_timing_internal_utils
	$(MAKE) -C sync_timing_driver_test
	$(MAKE) -C sync_timing_slaveloader
	$(MAKE) -C sync_timing_pktanalyzer
endif
ifneq (none,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	$(MAKE) -C sync_timing_driver_app 
endif

sync_apps_clean: linuxptp_clean
ifneq (none,$(SYNC_TIMING_UTIL_PKG_TYPE))
	$(MAKE) -C sync_timing_util clean
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2stack clean
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP2CONFIG_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2config clean
endif
ifneq (none,$(SYNC_TIMING_ESMC_PKG_TYPE))
	$(MAKE) -C sync_timing_esmc clean
	$(MAKE) -C sync_timing_esmcconfig clean
endif
ifneq (none,$(SYNC_TIMING_SYNCPTP2APPLIBTESTER_PKG_TYPE))
	$(MAKE) -C sync_timing_ptp2applib_tester clean
endif
ifneq (none,$(SYNC_TIMING_INT_TOOLS_PKG_TYPE))
	$(MAKE) -C sync_timing_driver_test clean
	$(MAKE) -C sync_timing_slaveloader clean
	$(MAKE) -C sync_timing_pktanalyzer clean
endif
ifneq (none,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	$(MAKE) -C sync_timing_driver_app clean
endif

clean: sync_apps_clean

