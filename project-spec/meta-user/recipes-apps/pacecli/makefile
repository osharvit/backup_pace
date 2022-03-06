APP=pacecli

CC := $(PREFIX)gcc
CPP:= $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp -c
LD := $(PREFIX)g++
AR := $(PREFIX)ar
OBJDUMP := $(PREFIX)objdump
LUALIB := lua_pace

SRCDIR= ./files
DEFS=
INC= -I. -I$(SRCDIR)/lua/src -I../../ipc/libpaceipc/files
OPT= -O0 -g $(ARCH)

CFLAGS= -Wall $(DEFS) $(INC) $(OPT)
CPPFLAGS= -Wall $(DEFS) $(INC) $(OPT)
ASFLAGS= $(DEFS) $(INC) -Wa,-ac

LIBS= -L. -L$(SRCDIR)/lua/libs -L../../ipc/libpaceipc -lpthread -l$(LUALIB) -ldl -lrt -l:libpaceipc.so.1.0.1

CSRC= 
CPPSRC= $(SRCDIR)/main.cpp		\
	$(SRCDIR)/lexer.cpp		\
	$(SRCDIR)/varmgr.cpp		\
	$(SRCDIR)/cmdmgr.cpp		\
	$(SRCDIR)/cmd.cpp		\
	$(SRCDIR)/math.cpp		\
	$(SRCDIR)/helpmgr.cpp		\
	$(SRCDIR)/clicpmgr.cpp		\
	$(SRCDIR)/socket.cpp		\
	$(SRCDIR)/extcmdmgr.cpp		\
	$(SRCDIR)/file.cpp		\
	$(SRCDIR)/json.cpp		\
	$(SRCDIR)/hwmgr.cpp		\
	$(SRCDIR)/luamgr.cpp		\
	$(SRCDIR)/h2mgr.cpp		\
	$(SRCDIR)/sync.cpp		\
	$(SRCDIR)/luasockets.cpp	\
	$(SRCDIR)/rc_cmdmgr.cpp		\
	$(SRCDIR)/thread.cpp		\
	$(SRCDIR)/luaipcmgr.cpp		\
	$(SRCDIR)/ipcmgr.cpp		\
	$(SRCDIR)/cmdeditor.cpp		\
	$(SRCDIR)/path.cpp
ASRC= 

OBJS=$(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(ASRC:.s=.o)

DEF: x86

$(APP): platform_info $(OBJS)
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o $(APP)
	$(OBJDUMP) -d $(APP) > $(APP).asm

PACE: PACE_INIT $(APP)
pace: PACE_INIT $(APP)
x86: x86_INIT $(APP)
CWS: CWS_INIT $(APP)
cws: CWS_INIT $(APP)

platform_info:
	@echo "----------------------------------"
	@echo "The compilation for $(PLAT)"
	@echo "----------------------------------"

PACE_INIT:
	$(eval PLAT := PACE)
	$(eval PREFIX := aarch64-linux-gnu-)
	$(eval ARCH := -mcpu=cortex-a53)
	$(eval CPP := $(PREFIX)gcc)
	$(eval CC := $(PREFIX)gcc)
	$(eval CPP := $(PREFIX)gcc)
	$(eval AS := $(PREFIX)gcc -x assembler-with-cpp -c)
	$(eval LD := $(PREFIX)g++)
	$(eval AR := $(PREFIX)ar)
	$(eval OBJDUMP := $(PREFIX)objdump)
	$(eval LUALIB := lua_pace)

CWS_INIT:
	$(eval PLAT := CWS)
	$(eval PREFIX := arm-none-linux-gnueabi-)
	$(eval ARCH := -mcpu=arm9)
	$(eval CPP := $(PREFIX)gcc)
	$(eval CC := $(PREFIX)gcc)
	$(eval CPP := $(PREFIX)gcc)
	$(eval AS := $(PREFIX)gcc -x assembler-with-cpp -c)
	$(eval LD := $(PREFIX)g++)
	$(eval AR := $(PREFIX)ar)
	$(eval OBJDUMP := $(PREFIX)objdump)

x86_INIT:
	$(eval PLAT := x86)
	$(eval PREFIX := )
	$(eval ARCH := )
	$(eval CPP := $(PREFIX)gcc)
	$(eval CC := $(PREFIX)gcc)
	$(eval CPP := $(PREFIX)gcc)
	$(eval AS := $(PREFIX)gcc -x assembler-with-cpp -c)
	$(eval LD := $(PREFIX)g++)
	$(eval AR := $(PREFIX)ar)
	$(eval OBJDUMP := $(PREFIX)objdump)
ifeq ($(MACHINE_BITS),x86_64)
	$(eval PLAT := x86 64Bits)
	$(eval LUALIB := lua_x86_64)
else
	$(eval PLAT := x86 32Bits)
	$(eval LUALIB := lua_x86)
endif

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f $(APP)
	rm -f $(APP).asm