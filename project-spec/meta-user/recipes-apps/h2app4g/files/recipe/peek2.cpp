#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>         // uint64_t
#include "hal/mmap_hal.h"   // mmap_hal

void usage(char *prog)
{
    printf("usage: %s ADDR\n",prog);
    printf("\n");
    printf("ADDR may be specified as hex values\n");
}


int main(int argc, char *argv[])
{
    uint64_t addr, page_addr;
    uint64_t page_size=sysconf(_SC_PAGESIZE);

    if(argc!=2) {
        usage(argv[0]);
        exit(-1);
    }

    addr=strtoull(argv[1],NULL,0);
    page_addr=(addr & ~(page_size-1));

    mmap_hal    hal(page_addr);

    printf("0x%08x\n", hal.rd32(addr));
    return 0;
}

