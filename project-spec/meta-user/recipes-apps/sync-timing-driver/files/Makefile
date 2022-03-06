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

SHELL=/bin/bash
  # If 'SHELL' is not set, Make will use "/bin/sh", which points to "dash"
  # (instead of "bash") on some systems.
  # "dash" has some limitations we don't want (missing "[[ ]]", no "source",
  # etc)

PROJECT_ROOT        = $(CURDIR)
SYNC_TIMING_OUTPUT_DIR  = $(PROJECT_ROOT)/output/
SYNC_TIMING_INCLUDE_DIR = $(PROJECT_ROOT)/inc/
SYNC_TIMING_BINARIES_OUTPUT_DIR = $(PROJECT_ROOT)/output/binaries/
SW_VERSION_FILE = $(PROJECT_ROOT)/sw_version.txt
ifeq ($(SYNC_TIMING_PKG_FILE),)
	PKG_INFO_FILE=pkg_info_default.mk
else
	PKG_INFO_FILE=$(SYNC_TIMING_PKG_FILE)
endif
include $(PROJECT_ROOT)/$(PKG_INFO_FILE)

#include ./config.mk



NAME := synctimingdriver

DEF    =
export DEF
export PROJECT_ROOT

.PHONY: collate_binary sync_timing_osal sync_timing_log sync_timing_oem sync_timing_cfg sync_timing_iniparser sync_timing_servo sync_timing_core sync_timing_api sync_timing_apps

all: build_sync_timing_driver
	$(MAKE) -C apps
	$(MAKE) collate_binary

sync_timing_check_env:
ifeq ($(SYNC_TIMING_BUILD_PLATFORM),)
	@echo
	@echo "++++++++++++++++++++++++++++++++++++ Hello User ++++++++++++++++++++++++++++++++++++"
	@echo "Please source sync_timing_driver_env.sh to setup the build env before attempting to compile the code !!!"
	@echo
	@echo "Usage: source ./sync_timing_driver_env.sh <Optional PKG_INFO_FILE>"
	@echo "Default PKG_INFO_FILE = pkg_info_default.mk"
	@echo
	@echo "------------------------------------- Goodbye --------------------------------------"
	@echo
	exit 1
endif

sync_timing_osal:
	$(MAKE) -C osal

sync_timing_osal_clean:
	$(MAKE) -C osal clean

sync_timing_log:
	$(MAKE) -C log

sync_timing_log_clean:
	$(MAKE) -C log clean

sync_timing_oem:
	$(MAKE) -C oem

sync_timing_oem_clean:
	$(MAKE) -C oem clean

sync_timing_cfg:
	$(MAKE) -C cfg

sync_timing_cfg_clean:
	$(MAKE) -C cfg clean

sync_timing_iniparser:
	$(MAKE) -C iniparser

sync_timing_iniparser_clean:
	$(MAKE) -C iniparser clean

sync_timing_core:
	$(MAKE) -C core

sync_timing_core_clean:
	$(MAKE) -C core clean

sync_timing_servo:
ifneq (none,$(SYNC_TIMING_SERVO_PKG_TYPE))
	$(MAKE) -C servo
else
	@echo "Donot build servo"
endif

sync_timing_servo_clean:
ifneq (none,$(SYNC_TIMING_SERVO_PKG_TYPE))
	$(MAKE) -C servo clean
else
	@echo "Donot clean servo"
endif

sync_timing_api:
	$(MAKE) -C api

sync_timing_api_clean:
	$(MAKE) -C api clean

sync_timing_apps: build_sync_timing_driver
	$(MAKE) -C apps

sync_timing_driver_test: build_sync_timing_driver
	$(MAKE) -C apps sync_timing_driver_test_app

si5388_fw_update: build_sync_timing_driver
	$(MAKE) -C apps si5388_fw_update_app

si5388_ptp2_stack: build_sync_timing_driver
	$(MAKE) -C apps sync_timing_ptp2stack_app

sync_timing_slaveloader: build_sync_timing_driver
	$(MAKE) -C apps sync_timing_slaveloader_app

sync_timing_pktanalyzer: build_sync_timing_driver
	$(MAKE) -C apps sync_timing_pktanalyzer_app

sync_timing_apps_clean:
	$(MAKE) -C apps clean

build_sync_timing_driver: sync_timing_check_env sync_timing_osal sync_timing_log sync_timing_oem sync_timing_cfg sync_timing_iniparser sync_timing_servo
	$(MAKE) -C core
	$(MAKE) -C api

collate_binary:
	mkdir -p $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/core/sync_timing_core_driver)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/core/sync_timing_core_driver $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_util)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_util/sync_timing_util $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_driver_app)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_driver_app/sync_timing_driver_app $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_esmc)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_esmc/sync_timing_esmc $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_esmcconfig)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_esmcconfig/esmcconfig $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2config)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2config/ptp2config $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2stack)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2stack/sync_timing_ptp2stack $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
ifeq ("$(wildcard  $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2applib_tester)", "")
	@echo
else
	cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_ptp2applib_tester/ptp2applib_tester $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
endif
	#cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_driver_test/sync_timing_driver_test $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
	#cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_slaveloader/sync_timing_slaveloader $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
	#cp -f $(SYNC_TIMING_OUTPUT_DIR)/sync_timing_pktanalyzer/sync_timing_pktanalyzer $(SYNC_TIMING_BINARIES_OUTPUT_DIR)
	#cp -f $(SYNC_TIMING_OUTPUT_DIR)/linuxptp/ptp4l $(SYNC_TIMING_BINARIES_OUTPUT_DIR)

clean: sync_timing_check_env sync_timing_osal_clean sync_timing_log_clean sync_timing_oem_clean sync_timing_cfg_clean sync_timing_iniparser_clean sync_timing_servo_clean sync_timing_core_clean sync_timing_api_clean sync_timing_apps_clean
	rm -rf *.o output lib
	rm -rf $(SW_VERSION_FILE)
	rm -rf $(SYNC_TIMING_OUTPUT_DIR)/binaries

