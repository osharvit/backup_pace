APP=si5518

PREFIX=
ARCH =

CC := $(PREFIX)gcc
CPP:= $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp -c
LD := $(PREFIX)gcc
AR := $(PREFIX)ar
OBJDUMP := $(PREFIX)objdump

DEFS=
INC= -I. 
OPT= -O0 -g $(ARCH) -DSTAND_ALONE

CFLAGS= -Wall $(DEFS) $(INC) $(OPT)
CPPFLAGS= -Wall $(DEFS) $(INC) $(OPT)
ASFLAGS= $(DEFS) $(INC) -Wa,-ac

#LIBS= -L. -Llua/src -lpthread -lncurses -llua -ldl
LIBS= 
#LIBS=

CSRC= 
CPPSRC= files/main.cpp files/spimgr.cpp files/cmdmgr.cpp files/file.cpp
ASRC= 

OBJS=$(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(ASRC:.s=.o)

DEF: x86

$(APP): $(OBJS)
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o $(APP)
#	$(OBJDUMP) -d $(APP) > $(APP).asm

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
