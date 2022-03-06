#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // memset()
#include <unistd.h>     // getopt(), sysconf()
#include <math.h>       // round()
#include <iostream>
#include <fstream>
#include "axidma_sg_hal.h"

const char* p2mm2shelp[] = {
    "NAME: mm2s -  list commands",
    "SYNOPSIS: mm2s [options]",
    "   -o [numant=1]           number of antenna",
    "   -o [gap=2000]           gap between segmented packet (clock count)",
    "   -o [sid=4]              RU_Port_ID(8bits) << 4 + sectionId(4bits)",
    "   -o [verbose]            verbose",
    "   -l <waveformfile>       load a waveform",
    "   -d                      dma descriptor write",
    "   -k [n]                  kick axidma: n=0 for full, positive n for limited",
    "   -s [n1] [n2]            status, n1 for loopcount, n2 for delay",
    "   -t                      stop axidma",
    "   -r                      reset axidma",
    "   -m <addr> <len>         memory dump",
    "   -z <addr> <len>         zero-clear memory",
    "   -p <type>               pattern test: 0 for periodic, 1 for one-shot",
    "   -i                      info",
    "   -h                      help",
    "   -v                      version",
    "ex)",
    "mm2s -l waveforms/DL/61M44_LTE20M_TM31_Cell1_PN15-le.bin",
    "mm2s -o numant=1,gap=2000 -d",
    "mm2s -r -k",
    NULL     // NOTE: always terminated by NULL
};           

const char* p2s2mmhelp[] = {
    "NAME: s2mm -  list commands",
    "SYNOPSIS: s2mm [options]",
    "   -o [sect=0x008]         RU_Port_ID(8bits) << 4 + sectionId(4bits)",
    "   -o [verbose]            verbose",
    "   -d                      dma descriptor write",
    "   -k [n]                  kick axidma: n=0 for full, positive n for limited",
    "   -s [n1] [n2]            status, n1 for loopcount, n2 for delay",
    "   -t                      stop axidma",
    "   -r                      reset axidma",
    "   -m <addr> <len>         memory dump",
    "   -z <addr> <len>         zero-clear memory",
    "   -p <type>               pattern test: 0 for periodic, 1 for one-shot",
    "   -i                      info",
    "   -h                      help",
    "   -v                      version",
    "ex)",
    "s2mm -d",
    "s2mm -r -k",
    NULL     // NOTE: always terminated by NULL
};           
             
extern std::string get_verstr();

enum{
    UL_DESC_0,
    UL_DESC_1,
    NUM_UL_DESC
};

uint32_t g_debug_mask = (1UL << MASKBIT_TMPLAYBACK);

const int NUM_SF = 10;
const int NUM_SYMB_SF = 14;
const int MAX_ANT = 2;

const int TS_NS = DMA_TS_CLK_IN_KHZ / 61440;  // for 61.44 Msps

axidma_sg_hal   mm2saxidma(AXIDMA_BASE);
axidma_sg_hal   s2mmaxidma(AXIDMA_BASE + 0x3000);

uint64_t str2num64(const char *strarg)
{
    uint64_t num64;

    if(strlen(strarg) > 2) {
        if((strarg[1] == 'x') || (strarg[1] == 'X')){
            num64 = strtoull(strarg, NULL, 0);
        }
        else {
            num64 = atoll(strarg);
        }
    }
    else{
        num64 = atoll(strarg);
    }

    return(num64);
}

uint32_t str2num32(const char *strarg)
{
    uint32_t num32;

    if(strlen(strarg) > 2) {
        if((strarg[1] == 'x') || (strarg[1] == 'X')){
            num32 = strtoul(strarg, NULL, 0);
        }
        else {
            num32 = atol(strarg);
        }
    }
    else{
        num32 = atol(strarg);
    }

    return(num32);
}

void dump_mem(uint64_t addr, uint32_t len, const char *p1dump_file_name, int truncate_bit=0)
{
    uint64_t    page_addr=(addr & ~(PAGE_SIZE-1));

    mmap_hal    plddr(page_addr);

    uint64_t curraddr = addr & ~0x0f;
    const size_t displaylimit = 40;
    size_t loopcnt = ((addr & 0x0f) + len + 15) >> 4;

    FILE *fp = NULL;

    if(p1dump_file_name != NULL) fp = fopen(p1dump_file_name, "wb+");

    uint16_t sign16ext = 0;
    for(int kk = 0; kk < truncate_bit; kk++){
        sign16ext = (1UL << 15) | (sign16ext >> 1);
    }
    printf("sign16ext=%04x\n", sign16ext);

    for(size_t kk = 0; kk < loopcnt; kk++){
        hal::u64le_t addr64le;
        addr64le.u64 = curraddr;
        if(kk < displaylimit){
            printf("%02x_%04x_%04x: ", addr64le.u08[4], addr64le.u16le[1].u16, addr64le.u16le[0].u16);
        }

        for(size_t ll = 0; ll < 4; ll++, curraddr+=4) {
            hal::u32le_t u32le;
            u32le.u32 = plddr.rd32(curraddr);

            if(kk < displaylimit){
                printf("%02x%02x %02x%02x ", u32le.u08[0], u32le.u08[1], u32le.u08[2], u32le.u08[3]);
            }

            // bit shift left with sign bit extention
            u32le.u16le[0].u16 = (u32le.u16le[0].u16 & 0x8000)? (sign16ext | (u32le.u16le[0].u16 >> truncate_bit)): (u32le.u16le[0].u16 >> truncate_bit);
            u32le.u16le[1].u16 = (u32le.u16le[1].u16 & 0x8000)? (sign16ext | (u32le.u16le[1].u16 >> truncate_bit)): (u32le.u16le[1].u16 >> truncate_bit);

            if(fp != NULL) fwrite(&u32le.u32, sizeof(uint32_t), 1, fp);
        }
        if(kk < displaylimit){
            printf("\n");
        }
    }

    if(fp != NULL) fclose(fp);
} // end of void dump_mem()

void zeroclear_mem(uint64_t addr, uint32_t len)
{
    uint64_t page_addr=(addr & ~(PAGE_SIZE-1));
    uint64_t addr32 = (addr + 0x03) & ~0x03;   // the next 32bit aligned address
    uint64_t kk;

    mmap_hal plddr(page_addr);

    // heading part: byte access
    for(kk = addr; kk < addr32; kk++){
        plddr.wr08(kk, 0);
    }
    // mid part: 32bit align
    for(kk = addr32; kk < (addr + len); kk+=4){
        plddr.wr32(kk, 0);
    }
    // tailing part: byte access
    for( ; kk < (addr + len); kk++){
        plddr.wr08(kk, 0);
    }
} // end of void zeroclear_mem()

uint64_t load_waveform(char *waveformfile, const uint64_t &addr)
{
    uint64_t curraddr = addr;

    // file open
    FILE *fp = fopen(waveformfile, "rb");
    if(fp == NULL){
        TRACE0() << "error in opening " << waveformfile << std::endl;
        return(curraddr);
    }

    // check the size
    fseek(fp, 0, SEEK_END);
    uint32_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);


    mmap_hal plddr(curraddr);

    for(uint32_t kk = 0; kk < (filesize >> 2); kk++, curraddr+=4){
        hal::u32le_t    u32le;
        fread(&u32le.u32, sizeof(hal::u32le_t), 1, fp);
        plddr.wr32(curraddr, u32le.u32);
        
#if 0
        if((kk % PAGE_SIZE) == 0) {
            std::cout
                << "curraddr=0x" << std::hex << std::setw(10) << std::setfill('0') << curraddr
                << " data=0x" << std::hex << std::setw(8) << std::setfill('0') << u32le.u32
                << std::endl;
            hal::nsleep(1000);
        }
#endif

    }
    for(uint32_t kk = 0; kk < (filesize - ((filesize >> 2) << 2)); kk++, curraddr++){
        uint8_t u08;
        fread(&u08, sizeof(uint8_t), 1, fp);
        plddr.wr08(curraddr, u08);
    }
    std::cout
        << "tmdataaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << addr
        << "~0x" << std::hex << std::setw(10) << std::setfill('0') << curraddr
        << std::endl;

    fclose(fp);

    return(curraddr);
} // end of void load_waveform()

bool write_mm2s_axidma_desc_fs8(const int num_ant, int packetgap=2000, int section_id=101, bool verbose=false, int osr=2)
{
    TRACE0();

    bool status = true;

    // base address for waveform
    hal::u64le_t  tmdatabase64le;
    tmdatabase64le.u64 = TMDATA_BASE;

    // base address for axidma
    hal::u64le_t  tmdescbase64le;
    tmdescbase64le.u64 = TMDESC_BASE;

    uint32_t    sectionheader[NUM_SYMB_SF] = {
        4, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5 };
    int cp_length[NUM_SYMB_SF] = {
        160, 144, 144, 144, 144, 144, 144,
        160, 144, 144, 144, 144, 144, 144};
    int fftsize = 2048;
    int timestamp_step = DMA_TS_CLK_IN_KHZ / (30720*osr);  // for 61.44 Msps

    uint32_t    symbol_timestamp = 0;
    uint32_t    nextdesc = tmdescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);
    uint32_t    seg_offset = tmdatabase64le.u32le[0].u32;

    //TRACE0() << "nextdesc=0x" << std::hex << std::setw(8) << std::setfill('0') << nextdesc << std::endl;

    // NOTE: should be a power of 2
    const int NUM_SEGMENT = 4;
    axidma_sg_hal::axidmadesc p2seg[NUM_SF][NUM_SYMB_SF][NUM_SEGMENT][num_ant];
    // clear memory
    memset(&p2seg[0][0][0][0], 0, sizeof(axidma_sg_hal::axidmadesc)*NUM_SF*NUM_SYMB_SF*NUM_SEGMENT*num_ant);

    // generate axidma descriptors
    for(int kk = 0; kk < NUM_SF; kk++){
        for(int ll = 0; ll < NUM_SYMB_SF; ll++){
            size_t      symbol_numsamples = osr*(cp_length[ll] + fftsize);
            size_t      seg_numsamples = osr*(cp_length[ll] + fftsize)/NUM_SEGMENT;
            size_t      seg_bytesize = 4*seg_numsamples;
            size_t      seg_wordsize = (seg_numsamples >> 1) + 1;
            for(int ss = 0; ss < NUM_SEGMENT; ss++) {
                for(int mm = 0; mm < num_ant; mm++){
                    p2seg[kk][ll][ss][mm].NEXTDESC = nextdesc;
                    p2seg[kk][ll][ss][mm].NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
                    p2seg[kk][ll][ss][mm].BUFFER_ADDRESS = seg_offset;
                    p2seg[kk][ll][ss][mm].BUFFER_ADDRESS_MSB = tmdatabase64le.u32le[1].u32;
                    p2seg[kk][ll][ss][mm].CONTROL =
                        seg_bytesize |
                        (1UL << axidma_sg_hal::DMADESC_CONTROL__TXSOF_SHL) |
                        (1UL << axidma_sg_hal::DMADESC_CONTROL__TXEOF_SHL);
#if 0
                    p2seg[kk][ll][ss][mm].USER_APP_SECTION_HEADER = SECTIONID_TO_HEADER(0, mm, sectionheader[ll], kk, ll);
#else
                    p2seg[kk][ll][ss][mm].USER_APP_SDUHEADER_MSB = GEN_IL_SDU_HEADER_MSB32(0, seg_wordsize, 0, mm, sectionheader[ll], kk, ll);
                    p2seg[kk][ll][ss][mm].USER_APP_SDUHEADER_LSB = GEN_IL_SDU_HEADER_LSB32(0, seg_wordsize, 0, mm, sectionheader[ll], kk, ll);
#endif
                    // USER_APP_WORDLENGTH should be numsamples/2 + 1
                    p2seg[kk][ll][ss][mm].USER_APP_WORDLENGTH = seg_wordsize;
                    p2seg[kk][ll][ss][mm].USER_APP_TIMESTAMP = symbol_timestamp + (ss*num_ant + mm)*packetgap;

                    nextdesc += sizeof(axidma_sg_hal::axidmadesc);
                }
                seg_offset += seg_bytesize;
            }
            symbol_timestamp += (uint32_t)round((double)symbol_numsamples*timestamp_step);
        }
    }

    // overwrite the nextdesc of the last descriptor to make cyclic dma
    p2seg[NUM_SF-1][NUM_SYMB_SF-1][NUM_SEGMENT-1][num_ant-1].NEXTDESC = tmdescbase64le.u32le[0].u32;
    p2seg[NUM_SF-1][NUM_SYMB_SF-1][NUM_SEGMENT-1][num_ant-1].NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;

    // write axidma descriptors to the memory
    {
        uint64_t addr = TMDESC_BASE;

        mmap_hal plddr(addr);

        std::string descfilename("descdl.bin");
        FILE *fpdesc = fopen(descfilename.c_str(), "wb+");
        if(fpdesc == NULL){
            std::stringstream sstrm;
            sstrm << "error in file open: " << descfilename << std::endl;
            perror(sstrm.str().c_str());
            return(false);
        }

        if(verbose) {
            std::cout << "nextdesc," << "buffer," << "bytesize," << "app1(48bitcnt)," << "app2(ts)," << std::endl;
        }
        int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;
        for(int kk = 0; kk < NUM_SF; kk++){
            for(int ll = 0; ll < NUM_SYMB_SF; ll++){
                for(int ss = 0; ss < NUM_SEGMENT; ss++) {
                    for(int mm = 0; mm < num_ant; mm++){
                        fwrite(&p2seg[kk][ll][ss][mm], sizeof(axidma_sg_hal::axidmadesc), 1, fpdesc);

                        uint32_t *p1u32 = (uint32_t *)(&p2seg[kk][ll][ss][mm]);
                        for (int nn = 0; nn < descsize32; nn++, addr+=4){
                            // write axidma descriptors to the memory
                            plddr.wr32(addr, p1u32[nn]);
                        }
                        if(verbose) {
                            std::cout <<
                                p2seg[kk][ll][ss][mm].NEXTDESC << ", " <<
                                p2seg[kk][ll][ss][mm].BUFFER_ADDRESS << ", " <<
                                (p2seg[kk][ll][ss][mm].CONTROL & ((1L << axidma_sg_hal::DMADESC_CONTROL__TXEOF_SHL) - 1)) << ", " <<
                                p2seg[kk][ll][ss][mm].USER_APP_WORDLENGTH << ", " <<
                                p2seg[kk][ll][ss][mm].USER_APP_TIMESTAMP << std::endl;
                        }
                    }
                }
            }
        }
        std::cout
            << "tmdescaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDESC_BASE
            << "~0x" << std::hex << std::setw(10) << std::setfill('0') << addr
            << std::endl;

        if(fpdesc != NULL){
            fclose(fpdesc);
        }
    }

    return(status);
}   // end of bool write_mm2s_axidma_desc_fs8()

bool write_s2mm_axidma_desc_fdd_fs8(int section_id, int section_mask, bool verbose=false, int osr=2)
{
    TRACE0();

    bool status = true;

    hal::u64le_t  uldatabase64le;
    hal::u64le_t  uldescbase64le;

    uldatabase64le.u64 = ULDATA_BASE;
    uldescbase64le.u64 = ULDESC_BASE;

    int cp_length[2] = {160, 144};
    int fftsize = 2048;
    // number of 10ms frames
    const int numframe = 2;   
    uint32_t    offset = uldatabase64le.u32le[0].u32;
    uint32_t    nextdesc = uldescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);
    axidma_sg_hal::axidmadesc p1seg[numframe];
    // clear memory
    memset(&p1seg[0], 0, sizeof(axidma_sg_hal::axidmadesc)*numframe);

    // 10ms frame-by-frame
    for(int kk = 0; kk < numframe; kk++){
        // header + cp + fftsize
        size_t numsample =
            ((cp_length[0] + fftsize)*2 + (cp_length[1] + fftsize)*12)*10*osr;
        p1seg[kk].NEXTDESC = nextdesc;
        p1seg[kk].NEXTDESC_MSB = uldescbase64le.u32le[1].u32;
        p1seg[kk].BUFFER_ADDRESS = offset;
        p1seg[kk].BUFFER_ADDRESS_MSB = uldatabase64le.u32le[1].u32;
        p1seg[kk].CONTROL =
            4*numsample |
            (1UL << axidma_sg_hal::DMADESC_CONTROL__RXSOF_SHL) |
            (1UL << axidma_sg_hal::DMADESC_CONTROL__RXEOF_SHL);

        nextdesc += sizeof(axidma_sg_hal::axidmadesc);

        offset += 4*numsample;
    }

    {
        uint64_t addr = ULDESC_BASE;

        mmap_hal plddr(addr);

        int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;

        for(int kk = 0; kk < numframe; kk++) {
            uint32_t *p1u32 = (uint32_t *)(&p1seg[kk]);
            for (int nn = 0; nn < descsize32; nn++, addr+=4){
                plddr.wr32(addr, p1u32[nn]);
            }
        }

        std::cout
            << "addr=0x" << std::hex << std::setw(10) << std::setfill('0') << ULDESC_BASE
            << "~0x" << std::hex << std::setw(10) << std::setfill('0') << addr
            << std::endl;
    }

    return(status);
}   // end of bool write_s2mm_axidma_desc_fdd_fs8()

void pattern_test(int patt_type)
{
    switch(patt_type){
        case 0: {   // periodic dma operation, number of samples: 2048, 2 times repeat 0x0000~0x3ff0
            uint64_t tmdataaddr = TMDATA_BASE;
            int numsample = 2048;
            // USER_APP_WORDLENGTH should be numsample/2 + 1
            size_t seg_wordsize = (numsample >> 1) + 1;

            // step1: generate test pattern and write to the PL-DDR memory of TMDATA_BASE
            mmap_hal plddrdata(tmdataaddr);
            for(int kk = 0; kk < numsample; kk++){
                hal::u32le_t    u32le;
                u32le.u16le[0].u16 = (uint16_t)((kk % 1024) << 4);
                u32le.u16le[1].u16 = (uint16_t)(((numsample - kk) % 1024) << 4);

                plddrdata.wr32(tmdataaddr, u32le.u32);
                tmdataaddr += 4;
            }

            // step2: generate axidma descriptors and write to the PL-DDR of TMDESC_BASE
            hal::u64le_t  tmdatabase64le;
            hal::u64le_t  tmdescbase64le;

            tmdatabase64le.u64 = TMDATA_BASE;
            tmdescbase64le.u64 = TMDESC_BASE;

            uint64_t tmdescaddr = TMDESC_BASE;

            mmap_hal plddrdesc(tmdescaddr);

            const int numdesc = 10;
            uint32_t    timestampns = 0;
            uint32_t    offset = tmdatabase64le.u32le[0].u32;
            uint32_t    nextdesc = tmdescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);

            // this pattern has only ten dma descriptors
            for(int kk = 0; kk < numdesc; kk++){
                uint32_t    bytesize = 4*numsample;

                axidma_sg_hal::axidmadesc descriptor;
                // zero initialize
                memset(&descriptor, 0, sizeof(axidma_sg_hal::axidmadesc));

                if(kk == (numdesc - 1)){    // the last descriptor only
                    // overwrite the nextdesc of the last descripto to make cyclic transfer
                    descriptor.NEXTDESC     = tmdescbase64le.u32le[0].u32;
                    descriptor.NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
                }
                else{
                    descriptor.NEXTDESC     = nextdesc;
                    descriptor.NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
                }
                descriptor.BUFFER_ADDRESS = offset;
                descriptor.BUFFER_ADDRESS_MSB = tmdatabase64le.u32le[1].u32;
                descriptor.CONTROL =
                    bytesize |
                    (1UL << axidma_sg_hal::DMADESC_CONTROL__TXSOF_SHL) |
                    (1UL << axidma_sg_hal::DMADESC_CONTROL__TXEOF_SHL);
#if 0
                descriptor.USER_APP_SECTION_HEADER = SECTIONID_TO_HEADER(0, 0, 101, 0, 0);
#else
                descriptor.USER_APP_SDUHEADER_MSB = GEN_IL_SDU_HEADER_MSB32(0, seg_wordsize, 0, 0, 4, 0, 0);
                descriptor.USER_APP_SDUHEADER_LSB = GEN_IL_SDU_HEADER_LSB32(0, seg_wordsize, 0, 0, 4, 0, 0);
#endif
                // USER_APP_WORDLENGTH should be numsample/2 + 1
                descriptor.USER_APP_WORDLENGTH = seg_wordsize;
                descriptor.USER_APP_TIMESTAMP = timestampns;

                int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;
                uint32_t *p1u32 = (uint32_t *)(&descriptor);
                for (int nn = 0; nn < descsize32; nn++){
                    plddrdesc.wr32(tmdescaddr, p1u32[nn]);
                    tmdescaddr += 4;
                }

                nextdesc += sizeof(axidma_sg_hal::axidmadesc);
                // NOTE: use the same data for each descriptor
                // offset += bytesize;
                timestampns += (uint32_t)round((double)307200/numdesc*TS_NS);
            }

            // step3: kick
            {
                size_t loopcnt = 100;
                size_t delayns = 10000000;  // 10ms

                uint64_t currdesc_addr = TMDESC_BASE;
                // NOTE: this taildesc_addr should be out of range for cyclic DMA mode
                uint64_t taildesc_addr = tmdescaddr;
                mm2saxidma.kick_mm2s(currdesc_addr, taildesc_addr);
                for(size_t kk = 0; kk < loopcnt; kk++) {
                    mm2saxidma.read_mm2s_status();
                    hal::nsleep(delayns);
                }
            }

            // step4: display memory info
            std::cout
                << "tmdataaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDATA_BASE
                << "~0x" << std::hex << std::setw(10) << std::setfill('0') << tmdataaddr
                << std::endl;
            std::cout
                << "tmdescaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDESC_BASE
                << "~0x" << std::hex << std::setw(10) << std::setfill('0') << tmdescaddr
                << std::endl;
            break;
        }

        case 1: {   // one-shot dma operation, number of samples: 2048, 
            uint64_t tmdataaddr = TMDATA_BASE;
            int numsample = 2048;
            // USER_APP_WORDLENGTH should be numsample/2 + 1
            size_t seg_wordsize = (numsample >> 1) + 1;

            // step1: generate test pattern and write to the PL-DDR memory of TMDATA_BASE
            mmap_hal plddrdata(tmdataaddr);
            for(int kk = 0; kk < numsample; kk++){
                hal::u32le_t    u32le;
                u32le.u16le[0].u16 = (uint16_t)((kk % 1024) << 4);
                u32le.u16le[1].u16 = (uint16_t)(((numsample - kk) % 1024) << 4);

                plddrdata.wr32(tmdataaddr, u32le.u32);
                tmdataaddr += 4;
            }

            // step2: generate axidma descriptors and write to the PL-DDR of TMDESC_BASE
            hal::u64le_t  tmdatabase64le;
            hal::u64le_t  tmdescbase64le;

            tmdatabase64le.u64 = TMDATA_BASE;
            tmdescbase64le.u64 = TMDESC_BASE;

            uint64_t tmdescaddr = TMDESC_BASE;

            mmap_hal plddrdesc(tmdescaddr);

            const int numdesc = 1;
            uint32_t    timestampns = 0;
            uint32_t    offset = tmdatabase64le.u32le[0].u32;
            uint32_t    nextdesc = tmdescbase64le.u32le[0].u32 + sizeof(axidma_sg_hal::axidmadesc);

            // this pattern has only ten dma descriptors
            for(int kk = 0; kk < numdesc; kk++){
                uint32_t    bytesize = 4*numsample;

                axidma_sg_hal::axidmadesc descriptor;
                // zero initialize
                memset(&descriptor, 0, sizeof(axidma_sg_hal::axidmadesc));

                if(kk == (numdesc - 1)){    // the last descriptor only
                    // overwrite the nextdesc of the last descripto to make cyclic transfer
                    descriptor.NEXTDESC = tmdescbase64le.u32le[0].u32;
                    descriptor.NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
                }
                else{
                    descriptor.NEXTDESC = nextdesc;
                    descriptor.NEXTDESC_MSB = tmdescbase64le.u32le[1].u32;
                }
                descriptor.BUFFER_ADDRESS = offset;
                descriptor.BUFFER_ADDRESS_MSB = tmdatabase64le.u32le[1].u32;
                descriptor.CONTROL =
                    bytesize |
                    (1UL << axidma_sg_hal::DMADESC_CONTROL__TXSOF_SHL) |
                    (1UL << axidma_sg_hal::DMADESC_CONTROL__TXEOF_SHL);
#if 0
                descriptor.USER_APP_SECTION_HEADER = SECTIONID_TO_HEADER(0, 0, 101, 0, 0);
#else
                descriptor.USER_APP_SDUHEADER_MSB = GEN_IL_SDU_HEADER_MSB32(0, seg_wordsize, 0, 0, 4, 0, 0);
                descriptor.USER_APP_SDUHEADER_LSB = GEN_IL_SDU_HEADER_LSB32(0, seg_wordsize, 0, 0, 4, 0, 0);
#endif
                // USER_APP_WORDLENGTH should be numsample/2 + 1
                descriptor.USER_APP_WORDLENGTH = seg_wordsize;
                descriptor.USER_APP_TIMESTAMP = timestampns;

                int descsize32 = sizeof(axidma_sg_hal::axidmadesc) >> 2;
                uint32_t *p1u32 = (uint32_t *)(&descriptor);
                for (int nn = 0; nn < descsize32; nn++){
                    plddrdesc.wr32(tmdescaddr, p1u32[nn]);
                    tmdescaddr += 4;
                }

                nextdesc += sizeof(axidma_sg_hal::axidmadesc);
                // NOTE: use the same data for each descriptor
                // offset += bytesize;
                timestampns += (uint32_t)round((double)307200/numdesc*TS_NS);
            }

            // step3: kick
            {
                size_t loopcnt = 10;
                size_t delayns = 10000000;  // 10ms

                uint64_t currdesc_addr = TMDESC_BASE;
                uint64_t taildesc_addr = tmdescbase64le.u64;
                mm2saxidma.kick_mm2s(currdesc_addr, taildesc_addr);
                for(size_t kk = 0; kk < loopcnt; kk++) {
                    mm2saxidma.read_mm2s_status();
                    hal::nsleep(delayns);
                }
            }

            // step4: display memory info
            std::cout
                << "tmdataaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDATA_BASE
                << "~0x" << std::hex << std::setw(10) << std::setfill('0') << tmdataaddr
                << std::endl;
            std::cout
                << "tmdescaddr=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDESC_BASE
                << "~0x" << std::hex << std::setw(10) << std::setfill('0') << tmdescaddr
                << std::endl;
            break;
        }

        default:  std::cout << "undefined type=" << patt_type << std::endl;
    }
}

int mm2s_main(int argc, char *argv[])
{
    int opt;
    int numant = 1;
    int sectionid = 101;
    //int fs=8;
    bool verbose = false;
    int packetgap = 2000;

    uint64_t next_tmdata_addr = TMDATA_BASE;

    enum {
        O_NUMANT,
        O_GAP,
        O_SID,
        O_VERBOSE,
        NUM_O
    };
    char *const o_tokens[] = {
        [O_NUMANT]  = "numant",
        [O_GAP] = "gap",
        [O_SID] = "sid",
        [O_VERBOSE] = "verbose",
        NULL
    };

    if(argc < 2) {
        printf("NAME: %s -  list commands", argv[0]);
        for(size_t kk = 1; (kk < sizeof(p2mm2shelp)/sizeof(p2mm2shelp[0])) && (p2mm2shelp[kk] != NULL); kk++) {
            printf("%s\n\r", p2mm2shelp[kk]);
        }
        exit(1);
    }
    while((opt = getopt(argc, argv, "dhikl:m:o:p:rstvz:")) != -1) {
        switch(opt) {
            case 'o': {
                int subopt;
                char *value;

                while((subopt = getsubopt(&optarg, o_tokens, &value)) != -1){
                    if(subopt == O_VERBOSE){
                        verbose = true;
                    }
                    else if(subopt == O_NUMANT){
                        numant = atoi(value);
                    }
                    else if(subopt == O_GAP){
                        packetgap = atoi(value);
                    }
                    else if(subopt == O_SID){
                        // strtoul(const char* str, char** endptr, int base)
                        //  base=0: the base used is determined by the format
                        //  base=16: hexdecimal
                        sectionid = strtoul(value, NULL, 0);
                    }
                }
                break;
            }
            case 'm': {
                // 64-bit address
                uint64_t addr64 = str2num64(optarg);
                // length(optional)
                uint32_t len = (optind < argc)? str2num32(argv[optind++]) : 0x10;
                // filename (optional)
                const char *p1dumpfilename = (optind < argc)? argv[optind++]: NULL;

                dump_mem(addr64, len, p1dumpfilename);
                break;
            }
            case 'z': {
                uint64_t addr64 = str2num64(optarg);
                uint32_t len = (optind < argc)? str2num32(argv[optind++]) : 0x10;

                zeroclear_mem(addr64, len);
                break;
            }
            case 'l': {
                uint64_t curr_tmdata_addr = (next_tmdata_addr + 7) & (~(0x7));

                next_tmdata_addr += load_waveform(optarg, curr_tmdata_addr);

                break;
            }
            case 'd': {
                write_mm2s_axidma_desc_fs8(numant, packetgap, sectionid, verbose);
                break;
            }
            case 'k': {
                size_t loopcnt = (optind < argc)? atoi(argv[optind++]): 1;
                size_t delayns = 10000000;  // 10ms

                uint64_t currdesc_addr = TMDESC_BASE;
                // NOTE: this taildesc_addr should be out of range for cyclic DMA mode
                uint64_t taildesc_addr = TMDATA_BASE;
                mm2saxidma.kick_mm2s(currdesc_addr, taildesc_addr);
                for(size_t kk = 0; kk < loopcnt; kk++) {
                    mm2saxidma.read_mm2s_status();
                    hal::nsleep(delayns);
                }
                break;
            }
            case 's': {
                size_t loopcnt = (optind < argc)? atoi(argv[optind++]): 1;
                size_t delayns = (optind < argc)? atoi(argv[optind++]): 10000000;   // 10ms
                
                for(size_t kk = 0; kk < loopcnt; kk++) {
                    mm2saxidma.read_mm2s_status();
                    hal::nsleep(delayns);
                }
                break;
            }
            case 'r': {
                mm2saxidma.reset_mm2s();
                mm2saxidma.read_mm2s_status();
                break;
            }
            case 't': {
                mm2saxidma.stop_mm2s();
                mm2saxidma.read_mm2s_status();
                break;
            }
            case 'v': {
                printf("%s\n\r", ::get_verstr().c_str());
                break;
            }
            case 'p': {
                pattern_test(atoi(optarg));
                break;
            }
            case 'i': {
                std::cout
                    << "tmdesc_base=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDESC_BASE
                    << std::endl
                    << "tmdata_base=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDATA_BASE
                    << std::endl
                    << "uldesc_base=0x" << std::hex << std::setw(10) << std::setfill('0') << ULDESC_BASE
                    << std::endl
                    << "uldata_base=0x" << std::hex << std::setw(10) << std::setfill('0') << ULDATA_BASE
                    << std::endl;
                break;
            }
            case 'h':
            case '?': {
                printf("NAME: %s -  list commands", argv[0]);
                for(size_t kk = 1; (kk < sizeof(p2mm2shelp)/sizeof(p2mm2shelp[0])) && (p2mm2shelp[kk] != NULL); kk++) {
                    printf("%s\n\r", p2mm2shelp[kk]);
                }
                exit(1);
                break;
            }

            default:;
        }
    }

    return(0);
} // end of mm2s_main()

int s2mm_main(int argc, char *argv[])
{
    int opt;
    //int fs=8;
    bool verbose = false;

    enum {
        O_VERBOSE,
        NUM_O
    };
    char *const o_tokens[] = {
        [O_VERBOSE] = "verbose",
        NULL
    };

    if(argc < 2) {
        printf("NAME: %s -  list commands", argv[0]);
        for(size_t kk = 1; (kk < sizeof(p2s2mmhelp)/sizeof(p2s2mmhelp[0])) && (p2s2mmhelp[kk] != NULL); kk++) {
            printf("%s\n\r", p2s2mmhelp[kk]);
        }
        exit(1);
    }
    while((opt = getopt(argc, argv, "dhikm:o:rstvz:")) != -1) {
        switch(opt) {
            case 'o': {
                int subopt;
                char *value;

                while((subopt = getsubopt(&optarg, o_tokens, &value)) != -1){
                    if(subopt == O_VERBOSE){
                        verbose = true;
                    }
                }
                break;
            }
            case 'm': {
                // 64-bit address
                uint64_t addr64 = str2num64(optarg);
                // length(optional)
                uint32_t len = (optind < argc)? str2num32(argv[optind++]) : 0x10;
                // filename (optional)
                const char *p1dumpfilename = (optind < argc)? argv[optind++]: NULL;

                dump_mem(addr64, len, p1dumpfilename);
                break;
            }
            case 'z': {
                uint64_t addr64 = str2num64(optarg);
                uint32_t len = (optind < argc)? str2num32(argv[optind++]) : 0x10;

                zeroclear_mem(addr64, len);
                break;
            }
            case 'd': {
                write_s2mm_axidma_desc_fdd_fs8(0, 0, verbose);
                break;
            }
            case 'k': {
                size_t loopcnt = (optind < argc)? atoi(argv[optind++]): 1;
                //size_t delayns = 10000000;  // 10ms

                uint64_t currdesc_addr = ULDESC_BASE;
                // NOTE: this taildesc_addr should be out of range for cyclic DMA mode
                uint64_t taildesc_addr = ULDESC_BASE + sizeof(axidma_sg_hal::axidmadesc)*(NUM_UL_DESC - 1);
                s2mmaxidma.kick_s2mm(currdesc_addr, taildesc_addr);
                for(size_t kk = 0; kk < loopcnt; kk++) {
                    s2mmaxidma.read_s2mm_status();
                    //hal::nsleep(delayns);
                }
                break;
            }
            case 's': {
                size_t loopcnt = (optind < argc)? atoi(argv[optind++]): 1;
                size_t delayns = (optind < argc)? atoi(argv[optind++]): 10000000;   // 10ms

                for(size_t kk = 0; kk < loopcnt; kk++) {
                    s2mmaxidma.read_s2mm_status();
                    hal::nsleep(delayns);
                }
                break;
            }
            case 'r': {
                s2mmaxidma.reset_s2mm();
                s2mmaxidma.read_s2mm_status();
                break;
            }
            case 't': {
                s2mmaxidma.stop_s2mm();
                s2mmaxidma.read_s2mm_status();
                break;
            }
            case 'v': {
                printf("%s\n\r", ::get_verstr().c_str());
                break;
            }
            case 'i': {
                std::cout
                    << "tmdesc_base=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDESC_BASE
                    << std::endl
                    << "tmdata_base=0x" << std::hex << std::setw(10) << std::setfill('0') << TMDATA_BASE
                    << std::endl
                    << "uldesc_base=0x" << std::hex << std::setw(10) << std::setfill('0') << ULDESC_BASE
                    << std::endl
                    << "uldata_base=0x" << std::hex << std::setw(10) << std::setfill('0') << ULDATA_BASE
                    << std::endl;
                break;
            }
            case 'h':
            case '?': {
                printf("NAME: %s -  list commands", argv[0]);
                for(size_t kk = 1; (kk < sizeof(p2s2mmhelp)/sizeof(p2s2mmhelp[0])) && (p2s2mmhelp[kk] != NULL); kk++) {
                    printf("%s\n\r", p2s2mmhelp[kk]);
                }
                exit(1);
                break;
            }

            default:;
        }
    }

    return(0);
} // end of s2mm_main()


int main(int argc, char *argv[])
{
    if(strcmp(argv[0], "s2mm") == 0){
        s2mm_main(argc, argv);
    }
    else{
        mm2s_main(argc, argv);
    }

    return(0);
} // end of main()
