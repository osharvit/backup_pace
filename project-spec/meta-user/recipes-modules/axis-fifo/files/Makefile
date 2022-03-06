obj-m := axis-fifo.o

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}

SRC := $(shell pwd)
KERNEL_SRC ?= $(KDIR) # buildroot uses KDIR instead of KERNEL_SRC

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
