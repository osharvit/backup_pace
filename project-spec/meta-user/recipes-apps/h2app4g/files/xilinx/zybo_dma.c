/**
 * Proof of concept offloaded memcopy using AXI Direct Memory Access v7.1
 * url=https://lauri.xn--vsandi-pxa.com/hdl/zynq/xilinx-dma.html
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <string.h>     // memset()

#define MM2S_CONTROL_REGISTER 0x00
#define MM2S_STATUS_REGISTER 0x04
#define MM2S_START_ADDRESS 0x18
#define MM2S_LENGTH 0x28

#define S2MM_CONTROL_REGISTER 0x30
#define S2MM_STATUS_REGISTER 0x34
#define S2MM_DESTINATION_ADDRESS 0x48
#define S2MM_LENGTH 0x58

extern void dma_set(unsigned int* dma_virtual_address, int offset, unsigned int value);
extern unsigned int dma_get(unsigned int* dma_virtual_address, int offset);
extern void dma_mm2s_sync(unsigned int* dma_virtual_address);
extern void dma_s2mm_sync(unsigned int* dma_virtual_address);
extern void dma_s2mm_status(unsigned int* dma_virtual_address);
extern void dma_mm2s_status(unsigned int* dma_virtual_address);
extern void memdump(void* virtual_address, int byte_count);

void dma_set(unsigned int* dma_virtual_address, int offset, unsigned int value)
{
    dma_virtual_address[offset>>2] = value;
}

unsigned int dma_get(unsigned int* dma_virtual_address, int offset)
{
    return dma_virtual_address[offset>>2];
}

void dma_mm2s_sync(unsigned int* dma_virtual_address)
{
    unsigned int mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    while(!(mm2s_status & 1<<12) || !(mm2s_status & 1<<1) ){
        dma_s2mm_status(dma_virtual_address);
        dma_mm2s_status(dma_virtual_address);

        mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    }
}

void dma_s2mm_sync(unsigned int* dma_virtual_address)
{
    unsigned int s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    while(!(s2mm_status & 1<<12) || !(s2mm_status & 1<<1)){
        dma_s2mm_status(dma_virtual_address);
        dma_mm2s_status(dma_virtual_address);

        s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    }
}

void dma_s2mm_status(unsigned int* dma_virtual_address)
{
    unsigned int status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    printf("[%s#%d] Stream to memory-mapped status (0x%08x@0x%02x):", __FUNCTION__, __LINE__, status, S2MM_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void dma_mm2s_status(unsigned int* dma_virtual_address)
{
    unsigned int status = dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    printf("[%s#%d] Memory-mapped to stream status (0x%08x@0x%02x):", __FUNCTION__, __LINE__, status, MM2S_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void memdump(void* virtual_address, int byte_count)
{
    char *p = virtual_address;
    int offset;
    printf("[%s#%d]", __FUNCTION__, __LINE__);
    for (offset = 0; offset < byte_count; offset++) {
        printf("%02x", p[offset]);
        if (offset % 4 == 3) { printf(" "); }
    }
    printf("\n");
}


int main()
{
    int dh = open("/dev/mem", O_RDWR | O_SYNC); // Open /dev/mem which represents the whole physical memory
    unsigned int* virtual_address
        = mmap(/*void *addr*/       NULL,
               /*size_t length*/    65535,
               /*int prot*/         PROT_READ | PROT_WRITE,
               /*int flags*/        MAP_SHARED,
               /*int fd*/           dh,
               /* off_t offset*/    0x40400000); // Memory map AXI Lite register block
    unsigned int* virtual_source_address
        = mmap(/*void *addr*/       NULL,
               /*size_t length*/    65535,
               /*int prot*/         PROT_READ | PROT_WRITE,
               /*int flags*/        MAP_SHARED,
               /*int fd*/           dh,
               /* off_t offset*/    0x0e000000); // Memory map source address
    unsigned int* virtual_destination_address
        = mmap(/*void *addr*/       NULL,
               /*size_t length*/    65535,
               /*int prot*/         PROT_READ | PROT_WRITE,
               /*int flags*/        MAP_SHARED,
               /*int fd*/           dh,
               /* off_t offset*/    0x0f000000); // Memory map destination address

    virtual_source_address[0]= 0x11223344; // Write random stuff to source block
    memset(virtual_destination_address, 0, 32); // Clear destination block

    printf("[%s#%d] Source memory block:      ", __FUNCTION__, __LINE__);
    memdump(virtual_source_address, 32);
    printf("[%s#%d] Destination memory block: ", __FUNCTION__, __LINE__);
    memdump(virtual_destination_address, 32);

    printf("[%s#%d] Resetting DMA\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, S2MM_CONTROL_REGISTER, 4);
    dma_set(virtual_address, MM2S_CONTROL_REGISTER, 4);
    dma_s2mm_status(virtual_address);
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Halting DMA\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, S2MM_CONTROL_REGISTER, 0);
    dma_set(virtual_address, MM2S_CONTROL_REGISTER, 0);
    dma_s2mm_status(virtual_address);
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Writing destination address\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, S2MM_DESTINATION_ADDRESS, 0x0f000000); // Write destination address
    dma_s2mm_status(virtual_address);

    printf("[%s#%d] Writing source address...\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, MM2S_START_ADDRESS, 0x0e000000); // Write source address
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Starting S2MM channel with all interrupts masked...\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, S2MM_CONTROL_REGISTER, 0xf001);
    dma_s2mm_status(virtual_address);

    printf("[%s#%d] Starting MM2S channel with all interrupts masked...\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, MM2S_CONTROL_REGISTER, 0xf001);
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Writing S2MM transfer length...\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, S2MM_LENGTH, 32);
    dma_s2mm_status(virtual_address);

    printf("[%s#%d] Writing MM2S transfer length...\n", __FUNCTION__, __LINE__);
    dma_set(virtual_address, MM2S_LENGTH, 32);
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Waiting for MM2S synchronization...\n", __FUNCTION__, __LINE__);
    dma_mm2s_sync(virtual_address);

    printf("[%s#%d] Waiting for S2MM sychronization...\n", __FUNCTION__, __LINE__);
    dma_s2mm_sync(virtual_address); // If this locks up make sure all memory ranges are assigned under Address Editor!

    dma_s2mm_status(virtual_address);
    dma_mm2s_status(virtual_address);

    printf("[%s#%d] Destination memory block: ", __FUNCTION__, __LINE__);
    memdump(virtual_destination_address, 32);

    return(0);
}

