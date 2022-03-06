# This file defines compilation options common to all architectures.

#/****************************************************************************************/
#/**                  Copyright (c) 2018, 2021 Skyworks Solution Inc.                   **/
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


BUILD_INFO_FILE = $(PROJECT_ROOT)build_info.mk
VERSION_INFO_FILE = $(PROJECT_ROOT)version_info.mk
SW_VERSION_FILE = $(PROJECT_ROOT)sw_version.txt
PKG_INFO_FILE = ${SYNC_TIMING_PKG_FILE}

ifeq ($(PKG_INFO_FILE),)
PKG_INFO_FILE = pkg_info_default.mk
endif

ifeq ($(shell test -e $(BUILD_INFO_FILE) && echo -n yes),yes)
    BUILD_INFO_FILE_EXISTS=1
    include $(BUILD_INFO_FILE)
else
    BUILD_INFO_FILE_EXISTS=0
endif

ifeq ($(shell test -e $(VERSION_INFO_FILE) && echo -n yes),yes)
    VERSION_INFO_FILE_EXISTS=1
    include $(VERSION_INFO_FILE)
else
    VERSION_INFO_FILE_EXISTS=0
endif

CURR_BUILD_TIME = $(shell date +%Y%m%d%H%M%S)
CFLAGS += -DOS_LINUX=1 -D_GNU_SOURCE
INCLUDE =

INCLUDE += -I$(PROJECT_ROOT)include
INCLUDE += -I$(PROJECT_ROOT)api/inc
INCLUDE += -I$(PROJECT_ROOT)oem
INCLUDE += -I$(PROJECT_ROOT)oem/drivers/generic_linux
INCLUDE += -I$(PROJECT_ROOT)iniparser/src
INCLUDE += -I$(PROJECT_ROOT)apps/sync_timing_ptp2stack/ptp2applib

CFLAGS += $(INCLUDE)

CFLAGS += -fdata-sections -ffunction-sections  -Wall -fPIC -fextended-identifiers
CFLAGS += -Werror

ifdef DEBUG
CFLAGS += -g -DDEBUG 
#CFLAGS += -lefence
endif

LDFLAGS +=

############### PKG INFO #############


############### BUILD INFO #############

ifneq ($(DRIVER_VERSION_CHIP_TYPE),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_CHIP_TYPE=$(DRIVER_VERSION_CHIP_TYPE)
endif

ifneq ($(DRIVER_VERSION_MAJOR),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_MAJOR=$(DRIVER_VERSION_MAJOR)
endif

ifneq ($(DRIVER_VERSION_MINOR),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_MINOR=$(DRIVER_VERSION_MINOR)
endif

ifneq ($(DRIVER_BUILD_INFO),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_BUILD_INFO=\"$(DRIVER_BUILD_INFO)\"
endif

ifneq ($(DRIVER_BUILD_TYPE),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_BUILD_TYPE=$(DRIVER_BUILD_TYPE)
    DEF_CFLAGS += -DSYNC_PTP_STACK_VERSION_BUILD_TYPE=$(DRIVER_BUILD_TYPE)
    
    ifeq ($(DRIVER_BUILD_TYPE),1)
        DRIVER_BUILD_TYPE_STR = "-Hotfix"
	else
    ifeq ($(DRIVER_BUILD_TYPE),2)
        DRIVER_BUILD_TYPE_STR = "-CustomerSpecial"
	else
    ifeq ($(DRIVER_BUILD_TYPE),3)
        DRIVER_BUILD_TYPE_STR = "-Pre-Release"
	else
    ifeq ($(DRIVER_BUILD_TYPE),4)
        DRIVER_BUILD_TYPE_STR = "-Experimental"
	else
    ifeq ($(DRIVER_BUILD_TYPE),15)
        DRIVER_BUILD_TYPE_STR = "-Dev"
    endif
    endif
    endif
    endif    
    endif
endif

ifneq ($(DRIVER_BUILD_NUM),)
    DEF_CFLAGS += -DSYNC_TIMING_DRIVER_VERSION_BUILD_NUM=$(DRIVER_BUILD_NUM)
    DEF_CFLAGS += -DSYNC_PTP_STACK_VERSION_BUILD_NUM=$(DRIVER_BUILD_NUM)
endif

ifeq (si5388,$(SYNC_TIMING_BUILD_CHIPSET))
	ifneq ($(MIN_FW_MAJOR_VERSION),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_MAJOR_VERSION=$(MIN_FW_MAJOR_VERSION)
	endif

	ifneq ($(MIN_FW_MINOR_VERSION),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_MINOR_VERSION=$(MIN_FW_MINOR_VERSION)
	endif

	ifneq ($(MIN_FW_BUILD_NUM),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_BUILD_NUM=$(MIN_FW_BUILD_NUM)
	endif
else ifeq (aruba,$(SYNC_TIMING_BUILD_CHIPSET))
	ifneq ($(MIN_FW_MAJOR_VERSION),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_MAJOR_VERSION=$(MIN_ARUBA_FW_MAJOR_VERSION)
	endif

	ifneq ($(MIN_FW_MINOR_VERSION),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_MINOR_VERSION=$(MIN_ARUBA_FW_MINOR_VERSION)
	endif

	ifneq ($(MIN_FW_BUILD_NUM),)
	    DEF_CFLAGS += -DSYNC_TIMING_MIN_FW_BUILD_NUM=$(MIN_ARUBA_FW_BUILD_NUM)
	endif
endif

DRIVER_VERSION = $(DRIVER_VERSION_CHIP_TYPE).$(DRIVER_VERSION_MAJOR).$(DRIVER_VERSION_MINOR)_$(DRIVER_BUILD_NUM)$(DRIVER_BUILD_TYPE_STR)

ifneq ($(PTP_STACK_VERSION_MAJOR),)
    DEF_CFLAGS += -DSYNC_PTP_STACK_VERSION_MAJOR=$(PTP_STACK_VERSION_MAJOR)
endif

ifneq ($(PTP_STACK_VERSION_MINOR),)
    DEF_CFLAGS += -DSYNC_PTP_STACK_VERSION_MINOR=$(PTP_STACK_VERSION_MINOR)
endif

PTP_STACK_VERSION = $(PTP_STACK_VERSION_MAJOR).$(PTP_STACK_VERSION_MINOR)_$(DRIVER_BUILD_NUM)$(DRIVER_BUILD_TYPE_STR)

ifeq ($(shell test -e $(SW_VERSION_FILE) && echo -n yes),yes)
else
    $(shell echo "sync_timing_driver_version = $(DRIVER_VERSION)" >> $(SW_VERSION_FILE))
    $(shell echo "sync_ptp_stack_version = $(PTP_STACK_VERSION)" >> $(SW_VERSION_FILE))
endif
	
############### FEATURES #############

# Define Chip Type used
#DEF_CFLAGS += -DSI5388=1
#DEF_CFLAGS += -DSI5348=2
#DEF_CFLAGS += -DARUBA=3
ifeq (si5388,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_CHIP_TYPE=SI5388
else ifeq (si5348,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_CHIP_TYPE=SI5348
else ifeq (aruba,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_CHIP_TYPE=ARUBA
endif

# Define PLATFORM specific features
ifeq (si5388,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_1PPSOUT_FROM_TIMING_CHIP
else ifeq (aruba,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_1PPSOUT_FROM_TIMING_CHIP
endif

# Define OEM specific features
DEF_CFLAGS += -DSYNC_TIMING_DATAPATH_SUPPORTED
DEF_CFLAGS += -DSYNC_TIMING_CHIP_RESET_SUPPORTED
DEF_CFLAGS += -DSYNC_TIMING_GPIO_SUPPORTED

ifeq (si5388,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_IRQCTRL_SUPPORTED
#else ifeq (si5348,$(SYNC_TIMING_BUILD_CHIPSET))
#	DEF_CFLAGS += -DSYNC_TIMING_IRQCTRL_SUPPORTED
else ifeq (aruba,$(SYNC_TIMING_BUILD_CHIPSET))
	DEF_CFLAGS += -DSYNC_TIMING_IRQCTRL_SUPPORTED
endif

ifeq (no,$(SYNC_TIMING_BUILD_ACCUTIME_SUPPORT))
	DEF_CFLAGS += -DSYNC_TIMING_LIGHTWEIGHT_DRIVER
ifeq (source,$(SYNC_TIMING_GNSS_ONLY_APP_PKG_TYPE))
	DEF_CFLAGS += -DSYNC_TIMING_TODINPUT_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_PTPCLOCK_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_PTPCLOCK_ADJ_SYNC_TIMING_CHIPSET
endif
else
	DEF_CFLAGS += -DSYNC_TIMING_LEDCTRL_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_TODOUTPUT_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_TODINPUT_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_PTPCLOCK_SUPPORTED
	DEF_CFLAGS += -DSYNC_TIMING_PTPCLOCK_ADJ_SYNC_TIMING_CHIPSET
	DEF_CFLAGS += -DSYNC_TIMING_ACCUTIME_SUPPORTED
endif

DEF_CFLAGS += -DSYNC_TIMING_OEM_GENERIC_LINUX

ifeq (source,$(SYNC_TIMING_BUILD_SERVO_PKG_TYPE))
	DEF_CFLAGS += -DSYNC_TIMING_PTPSERVO_ON_HOST
else ifeq (binary,$(SYNC_TIMING_BUILD_SERVO_PKG_TYPE))
	DEF_CFLAGS += -DSYNC_TIMING_PTPSERVO_ON_HOST
else ifeq (none,$(SYNC_TIMING_BUILD_SERVO_PKG_TYPE))
	DEF_CFLAGS += -DSYNC_TIMING_NO_PTPSERVO
endif

# OS, Log, Misc. features
DEF_CFLAGS += -DSYNC_TIMING_OS_IMPLEMENTED
DEF_CFLAGS += -DSYNC_TIMING_LOG_SUPPORTED
DEF_CFLAGS += -DSYNC_TIMING_HOST_LITTLE_ENDIAN
DEF_CFLAGS += -DSYNC_TIMING_DRIVER_BUILD_TIME=$(CURR_BUILD_TIME)

CFLAGS += $(DEF_CFLAGS)
############################

# NOTE: linuxptp_build does not use these defintions.
SYNC_TIMING_LIBS = \
	osal$(LIBPOSTFIX) \
	log$(LIBPOSTFIX) \
	iniparser$(LIBPOSTFIX) 

ifneq ($(filter -DSYNC_TIMING_LIGHTWEIGHT_DRIVER, $(CFLAGS)),)
SYNC_TIMING_LIBS += oem$(LIBPOSTFIX)
endif

SYNC_TIMING_LIBS_LFLAGS = $(foreach lib, $(SYNC_TIMING_LIBS), $(addprefix -l, $(lib)))


