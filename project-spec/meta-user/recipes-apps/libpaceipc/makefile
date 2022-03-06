APP=paceipc

PREFIX=
ARCH =

MAJOR = 1.0
MINOR = 1
VERSION = $(MAJOR).$(MINOR)

CC := $(PREFIX)gcc
CPP:= $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp -c
LD := $(PREFIX)gcc
AR := $(PREFIX)ar
OBJDUMP := $(PREFIX)objdump

DEFS=
INC= -I.
OPT= -O0 -g $(ARCH)

SRCFOLDER = files
lib = yes

CFLAGS= -Wall $(DEFS) $(INC) $(OPT)
CPPFLAGS= -Wall $(DEFS) $(INC) $(OPT)
ASFLAGS= $(DEFS) $(INC) -Wa,-ac

#LIBS= -L. -Llua/src -lpthread -lncurses -llua -ldl
LIBS= -L. -lpthread

CSRC= $(SRCFOLDER)/socket.c $(SRCFOLDER)/threads.c $(SRCFOLDER)/ipclib.c $(SRCFOLDER)/protocol.c
CPPSRC= 
ASRC= 

OBJS=$(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(ASRC:.s=.o)

DEF: x86

$(APP): $(OBJS)
ifeq ($(lib),yes)
	@echo "-----------------------------------"
	@echo "        To generate a library      "
	@echo "-----------------------------------"
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o lib$(APP).so.$(VERSION) -shared
#	$(OBJDUMP) -d lib$(APP).so > $(APP).asm
else
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o $(APP)
	$(OBJDUMP) -d $(APP) > $(APP).asm
endif

PACE: PACE_INIT $(APP)
pace: PACE_INIT $(APP)
x86: x86_INIT $(APP)
CWS: CWS_INIT $(APP)
cws: CWS_INIT $(APP)

PACE_INIT:
	$(eval PREFIX := aarch64-linux-gnu-)
	$(eval ARCH := -mcpu=cortex-a53)
	$(eval CPP := $(PREFIX)gcc)
	$(eval CC := $(PREFIX)gcc)
	$(eval CPP := $(PREFIX)gcc)
	$(eval AS := $(PREFIX)gcc -x assembler-with-cpp -c)
	$(eval LD := $(PREFIX)g++)
	$(eval AR := $(PREFIX)ar)
	$(eval OBJDUMP := $(PREFIX)objdump)

CWS_INIT:
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
	$(eval PREFIX := )
	$(eval ARCH := )
	$(eval CPP := $(PREFIX)gcc)
	$(eval CC := $(PREFIX)gcc)
	$(eval CPP := $(PREFIX)gcc)
	$(eval AS := $(PREFIX)gcc -x assembler-with-cpp -c)
	$(eval LD := $(PREFIX)g++)
	$(eval AR := $(PREFIX)ar)
	$(eval OBJDUMP := $(PREFIX)objdump)

.c.o:
ifeq ($(lib),yes)
	$(CC) $(CFLAGS) -c -fPIC $< -o $@
else
	$(CC) $(CFLAGS) -c $< -o $@
endif

.cpp.o:
ifeq ($(lib),yes)
	$(CPP) $(CPPFLAGS) -c -fPIC $< -o $@
else
	$(CPP) $(CPPFLAGS) -c $< -o $@
endif

clean:
	rm -f $(OBJS)
	rm -f $(APP)
	rm -f $(APP).asm
	rm -f lib$(APP).so.$(VERSION)


install:
	sudo cp lib$(APP).so.$(VERSION) /usr/lib/