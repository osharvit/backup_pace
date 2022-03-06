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

PROJECT_ROOT_TEMP = $(dir $(dir $(CURDIR)))
PROJECT_ROOT = $(PROJECT_ROOT_TEMP)../
include $(PROJECT_ROOT)common.mk
include $(PROJECT_ROOT)config.mk
include $(PROJECT_ROOT)$(PKG_INFO_FILE)

SRCS := src/sync_timing_driver_app_main.c \
		src/sync_timing_driver_app_drv_cmd.c \
		src/sync_timing_driver_app_fw_cmd.c \
		src/sync_timing_driver_app_helper.c

OUTPUT := $(PROJECT_ROOT)$(OUTPUT_DIR)/$(notdir $(CURDIR))

LIBRARIES := $(PROJECT_ROOT)$(LIB_DIR)
PKG_LIBRARIES := $(PROJECT_ROOT)$(PKG_LIB_DIR)

OBJS := $(addprefix $(OUTPUT)/, $(patsubst %.c,%.o,$(SRCS)))
CORE_LIB := $(LIBRARIES)/libapi_sync_timing.a
LIB := $(addprefix $(LIBRARIES)/, libdriver_app$(LIBPOSTFIX).a)
PKG_LIB := $(addprefix $(PKG_LIBRARIES)/, libdriver_app$(LIBPOSTFIX).a)
CFLAGS += -DUSE_SYNC_TIMING_DRIVER_API
CFLAGS += -I$(PROJECT_ROOT)apps/sync_timing_driver_app/include

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(OUTPUT)/$*.Td $(OUTPUT)/$*.d && touch $@

all: sync_timing_driver_app

$(LIB): $(OBJS)
ifeq (source,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	@echo "Generate DRIVER Application Library"
	$(AR) rcs $(LIB) $(OBJS)
	$(STRIP) --strip-debug $(LIB)
else ifeq (binary,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	@echo "Copy Pre-compiled DRIVER Application library to lib folder"
	cp -f $(PKG_LIB) $(LIB)
else ifeq (none,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	@echo "Nothing to do for sync timing Application"
else
	@echo "SYNC_TIMING_DRIVER_APP_PKG_TYPE Set to $(SYNC_TIMING_DRIVER_APP_PKG_TYPE)"
	@echo "Unknown package type"
	exit 1
endif

$(OUTPUT)/%.o: %.c $(OUTPUT)/%.d
ifeq (source,$(SYNC_TIMING_DRIVER_APP_PKG_TYPE))
	@echo "Compile DRIVER Application Sources"
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)
endif

$(OBJS): | $(OUTPUT)

$(LIB): | $(LIBRARIES)

$(OUTPUT):
	mkdir -p $(OUTPUT)/src/

$(LIBRARIES):
	mkdir -p $(LIBRARIES)

sync_timing_driver_app: $(LIB)
	$(CC) $(LDFLAGS) -o $(OUTPUT)/$@ $(LDLIBS) -L$(LIBRARIES) -ldriver_app_sync_timing -lapi_sync_timing $(SYNC_TIMING_LIBS_LFLAGS) -lrt -lpthread -lm $(EXTRA_LDFLAGS)

clean:
	rm -rf $(OUTPUT)
	rm -f $(LIB)

$(OUTPUT)/%.d: ;

-include $(patsubst %,$(OUTPUT)/%.d,$(basename $(SRCS)))

    
