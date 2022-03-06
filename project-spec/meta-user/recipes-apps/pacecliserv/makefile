APP=pacecliserv

PREFIX=
ARCH =

CC := $(PREFIX)gcc
CPP:= $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp -c
LD := $(PREFIX)g++
AR := $(PREFIX)ar
OBJDUMP := $(PREFIX)objdump

DEFS=
INC= -I. -Ilua/src
OPT= -O0 -g $(ARCH) 

CFLAGS= -Wall $(DEFS) $(INC) $(OPT)
CPPFLAGS= -Wall $(DEFS) $(INC) $(OPT)
ASFLAGS= $(DEFS) $(INC) -Wa,-ac

#LIBS= -L. -Llua/src -lpthread -lncurses -llua -ldl
LIBS= -L. -lpthread

SRCDIR = ./files

CSRC= 
CPPSRC= $(SRCDIR)/main.cpp $(SRCDIR)/socket.cpp $(SRCDIR)/connmgr.cpp $(SRCDIR)/clientmgr.cpp $(SRCDIR)/client.cpp $(SRCDIR)/h2emul.cpp
ASRC= 

OBJS=$(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(ASRC:.s=.o)

DEF: x86

$(APP): $(OBJS)
	$(LD) $(OBJS) $(LIBS) $(LDFLAGS) -o $(APP)
	$(OBJDUMP) -d $(APP) > $(APP).asm

PACE: PACE_INIT $(APP)
pace: PACE_INIT $(APP)
x86: x86_INIT $(APP)
CWS: CWS_INIT $(APP)
cws: CWS_INIT $(APP)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CPP) $(CPPFLAGS) -c $< -o $@

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

clean:
	rm -f $(OBJS)
	rm -f $(APP)
	rm -f $(APP).asm
