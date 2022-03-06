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

SRCS := sync_timing_log.c 

OUTPUT := $(PROJECT_ROOT)$(OUTPUT_DIR)/$(notdir $(CURDIR))

LIBRARIES := $(PROJECT_ROOT)$(LIB_DIR)
PKG_LIBRARIES := $(PROJECT_ROOT)$(PKG_LIB_DIR)

OBJS := $(addprefix $(OUTPUT)/, $(patsubst %.c,%.o,$(SRCS)))
LIB := $(addprefix $(LIBRARIES)/, liblog$(LIBPOSTFIX).a)
PKG_LIB := $(addprefix $(PKG_LIBRARIES)/, liblog$(LIBPOSTFIX).a)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(OUTPUT)/$*.Td $(OUTPUT)/$*.d && touch $@

all: $(LIB)

$(LIB): $(OBJS)
ifeq (source,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Generate LOG Library"
	$(AR) rcs $(LIB) $(OBJS)
	$(STRIP) --strip-debug $(LIB)
else ifeq (binary,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Copy Pre-compiled LOG library to lib folder"
	cp -f $(PKG_LIB) $(LIB)
else
	@echo "SYNC_TIMING_CORE_PKG_TYPE Set to $(SYNC_TIMING_CORE_PKG_TYPE)"
	@echo "Unknown package type"
	exit 1
endif


$(OUTPUT)/%.o: %.c $(OUTPUT)/%.d
ifeq (source,$(SYNC_TIMING_CORE_PKG_TYPE))
	@echo "Compile LOG Sources"
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)
endif

$(OBJS): | $(OUTPUT)

$(LIB): | $(LIBRARIES)

$(OUTPUT):
	mkdir -p $(OUTPUT)

$(LIBRARIES):
	mkdir -p $(LIBRARIES)

clean:
	rm -rf $(OUTPUT)
	rm -f $(LIB)

$(OUTPUT)/%.d: ;

-include $(patsubst %,$(OUTPUT)/%.d,$(basename $(SRCS)))


