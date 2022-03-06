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

SRCS := src/sync_timing_api_common.c \
		src/sync_timing_api_mem_access.c \
		src/sync_timing_api_clockadj.c \
		src/sync_timing_api_debug.c \
		internal/sync_timing_api_internal.c

ifneq ($(filter -DSYNC_TIMING_PTPCLOCK_SUPPORTED, $(CFLAGS)),)
SRCS += src/sync_timing_api_clockctrl.c
endif

ifeq ($(filter -DSYNC_TIMING_NO_PTPSERVO, $(CFLAGS)),)
SRCS += src/sync_timing_api_servoctrl.c 
endif

OUTPUT := $(PROJECT_ROOT)$(OUTPUT_DIR)/$(notdir $(CURDIR))

LIBRARIES := $(PROJECT_ROOT)$(LIB_DIR)
PKG_LIBRARIES := $(PROJECT_ROOT)$(PKG_LIB_DIR)

OBJS := $(addprefix $(OUTPUT)/, $(patsubst %.c,%.o,$(SRCS)))
LIB := $(addprefix $(LIBRARIES)/, libapi$(LIBPOSTFIX).a)
PKG_LIB := $(addprefix $(PKG_LIBRARIES)/, libapi$(LIBPOSTFIX).a)

CFLAGS += -I$(PROJECT_ROOT)api/inc
CFLAGS += -I$(PROJECT_ROOT)api/internal
ifneq ($(filter -DSYNC_TIMING_LIGHTWEIGHT_DRIVER, $(CFLAGS)),)
CFLAGS += -I$(PROJECT_ROOT)core/inc
endif
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(OUTPUT)/$*.Td $(OUTPUT)/$*.d && touch $@

all: $(LIB)

$(LIB): $(OBJS)
ifeq (source,$(SYNC_TIMING_API_PKG_TYPE))
#	$(AR) rcs $(LIB) $(OBJS)
	$(eval ar_input=)
	$(eval ar_input+=addmod $(OBJS)\n)
	$(eval ar_input+=addlib $(addprefix $(LIBRARIES)/, libcfg$(LIBPOSTFIX).a)\n)
ifneq ($(filter -DSYNC_TIMING_LIGHTWEIGHT_DRIVER, $(CFLAGS)),)
	$(eval ar_input+=addlib $(addprefix $(LIBRARIES)/, libcore$(LIBPOSTFIX).a)\n)
#	$(eval ar_input+=addlib $(addprefix $(LIBRARIES)/, liboem$(LIBPOSTFIX).a)\n)
endif
	printf '%b' "create $@\n$(ar_input)save\nend" | $(AR) -M
	$(STRIP) --strip-debug $(LIB)
else ifeq (binary,$(SYNC_TIMING_API_PKG_TYPE))
	@echo "Copy Pre-compiled API DRIVER library to lib folder"
	cp -f $(PKG_LIB) $(LIB)
else
	@echo "SYNC_TIMING_API_PKG_TYPE Set to $(SYNC_TIMING_API_PKG_TYPE)"
	@echo "Unknown package type"
	exit 1
endif

$(OUTPUT)/%.o: %.c $(OUTPUT)/%.d
ifeq (source,$(SYNC_TIMING_API_PKG_TYPE))
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)
endif

$(OBJS): | $(OUTPUT)

$(LIB): | $(LIBRARIES)

$(OUTPUT):
	mkdir -p $(OUTPUT)
	mkdir -p $(OUTPUT)/src
	mkdir -p $(OUTPUT)/internal

$(LIBRARIES):
	mkdir -p $(LIBRARIES)

clean:
	rm -rf $(OUTPUT)
	rm -f $(LIB)

$(OUTPUT)/%.d: ;

-include $(patsubst %,$(OUTPUT)/%.d,$(basename $(SRCS)))


