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
include $(PROJECT_ROOT)common.mk
include $(PROJECT_ROOT)config.mk
include $(PROJECT_ROOT)$(PKG_INFO_FILE)

SRCS := main/sync_timing_core_main.c \
		internal/sync_timing_core_driver.c \
		internal/sync_timing_core_communication.c \
		internal/sync_timing_core_ctrl.c \
		internal/sync_timing_core_mem_access.c \
		internal/sync_timing_core_clockadj.c \
		internal/sync_timing_core_log.c

ifeq ($(filter -DSYNC_TIMING_NO_PTPSERVO, $(CFLAGS)),)
SRCS += internal/sync_timing_core_servoctrl.c 
endif

ifneq ($(filter -DSYNC_TIMING_PTPCLOCK_SUPPORTED, $(CFLAGS)),)
SRCS += internal/sync_timing_core_clockctrl.c 
ifneq ($(filter -DSYNC_TIMING_CHIP_TYPE=ARUBA, $(CFLAGS)),)
SRCS += internal/chipset/sync_timing_core_clkctrl_aruba.c
endif
endif

ifneq ($(filter -DSYNC_TIMING_CHIP_TYPE=SI5388, $(CFLAGS)),)
SRCS += internal/chipset/sync_timing_core_device_si5388.c
endif

ifneq ($(filter -DSYNC_TIMING_CHIP_TYPE=SI5348, $(CFLAGS)),)
SRCS += internal/chipset/sync_timing_core_device_si5348.c
SRCS += internal/chipset/sync_timing_core_clkctrl_si53xx.c
endif

ifneq ($(filter -DSYNC_TIMING_CHIP_TYPE=ARUBA, $(CFLAGS)),)
SRCS += internal/chipset/sync_timing_core_device_aruba.c
endif

OUTPUT := $(PROJECT_ROOT)$(OUTPUT_DIR)/$(notdir $(CURDIR))

LIBRARIES := $(PROJECT_ROOT)$(LIB_DIR)
PKG_LIBRARIES := $(PROJECT_ROOT)$(PKG_LIB_DIR)

OBJS := $(addprefix $(OUTPUT)/, $(patsubst %.c,%.o,$(SRCS)))
LIB := $(addprefix $(LIBRARIES)/, libcore$(LIBPOSTFIX).a)
PKG_LIB := $(addprefix $(PKG_LIBRARIES)/, libcore$(LIBPOSTFIX).a)

CFLAGS += -I$(PROJECT_ROOT)core/inc
CFLAGS += -I$(PROJECT_ROOT)core/inc/chipset
CFLAGS += -I$(PROJECT_ROOT)log

ifeq ($(filter -DSYNC_TIMING_NO_PTPSERVO, $(CFLAGS)),)
CFLAGS += -I$(PROJECT_ROOT)servo/inc
CFLAGS += -I$(PROJECT_ROOT)servo/src
endif

ifneq ($(filter -DSYNC_TIMING_LIGHTWEIGHT_DRIVER, $(CFLAGS)),)
CFLAGS += -I$(PROJECT_ROOT)api/internal
endif

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(OUTPUT)/$*.Td $(OUTPUT)/$*.d && touch $@

all: sync_timing_core_driver

$(LIB): $(OBJS)
ifeq (source,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Generate CORE DRIVER Library"
	$(AR) rcs $(LIB) $(OBJS)
	$(STRIP) --strip-debug $(LIB)
else ifeq (binary,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Copy Pre-compiled CORE DRIVER library to lib folder"
	cp -f $(PKG_LIB) $(LIB)
else
	@echo "SYNC_TIMING_CORE_PKG_TYPE Set to $(SYNC_TIMING_CORE_PKG_TYPE)"
	@echo "Unknown package type"
	exit 1
endif


$(OUTPUT)/%.o: %.c $(OUTPUT)/%.d
ifeq (source,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Compile CORE DRIVER Sources"
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)
endif

$(OBJS): | $(OUTPUT)

$(LIB): | $(LIBRARIES)

$(OUTPUT):
	mkdir -p $(OUTPUT)
	mkdir -p $(OUTPUT)/main
	mkdir -p $(OUTPUT)/internal
	mkdir -p $(OUTPUT)/internal/chipset

$(LIBRARIES):
	mkdir -p $(LIBRARIES)

sync_timing_core_driver: $(LIB)
ifneq (none,$(SYNC_TIMING_SERVO_PKG_TYPE))
	$(CC) $(LDFLAGS) -o $(OUTPUT)/$@ $(LDLIBS) -L$(LIBRARIES) -lcore_sync_timing -lservo_sync_timing -lcfg_sync_timing -loem_sync_timing -liniparser_sync_timing -llog_sync_timing -losal_sync_timing -lrt -lpthread -lm $(EXTRA_LDFLAGS)
	$(STRIP) --strip-debug $(OUTPUT)/$@
else
	@echo "Light Weight Driver - just use as library"	
endif
clean:
	rm -rf $(OUTPUT)
	rm -f $(LIB)

$(OUTPUT)/%.d: ;

-include $(patsubst %,$(OUTPUT)/%.d,$(basename $(SRCS)))


