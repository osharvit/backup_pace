#ifndef _NUM_DEF_H_
#define _NUM_DEF_H_

#include <unistd.h>     // getopt(), sysconf()

#define ALIGN4BYTE(x)   ((((x) + 3) >> 2) << 2)

#define TRACE0()  std::cout << "[" << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << std::endl
#define TRACE1(classname) std::cout << "[" << #classname << "::" << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << std::endl
#define TRACE2(classname,threadid) std::cout << "[" << #classname << "::" << __FUNCTION__ << "@" << threadid << "#" << std::dec << __LINE__ << "] " << std::endl
#define TRACE3(stream,classname,threadid) stream << "[" << #classname << "::" << __FUNCTION__ << "@" << threadid << "#" << std::dec << __LINE__ << "] " << std::endl
#define TRACE0OBJ()  std::cout << "[" << this->objname() << "." << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << std::endl
#define TRACE1OBJ(stream) stream << "[" << this->objname() << "." << __FUNCTION__ << "#" << std::dec << __LINE__ << "] " << std::endl

#define TM_DIR  "waveforms/DL"

//  CC_ID:      3 bits
//  RU_Port_ID: 8 bits
//  SECTION_ID: 4 bits
//  SUBFRAME_ID:4 bits
//  SYMBOLID:   5 bits
//  |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1|0|0|0|0|0|0|0|0|0|0|
//  |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
//  --------------->|<--->|<------------->|<----->|<----->|<------->|
//                   CC(3)  RU_PORT_ID(8)  SECTION  SF(4)  SYMBOLID(5)
// NOTE to make 0x00FFFFFF to 0x0FFFFF0F
#define IL_SDU_HEADER__RUPORTID_SHL     13
#define IL_SDU_HEADER__RUPORTID_M       (0x000000FF << IL_SDU_HEADER__RUPORTID_SHL)
#define IL_SDU_HEADER__SECTIONID_SHL    9
#define IL_SDU_HEADER__SECTIONID_M      (0x0000000F << IL_SDU_HEADER__SECTIONID_SHL)
#define IL_SDU_HEADER__SUBFRAMEID_SHL   5
#define IL_SDU_HEADER__SUBFRAMEID_M     (0x0000000F << IL_SDU_HEADER__SUBFRAMEID_SHL)
#define IL_SDU_HEADER__SYMBOLID_SHL     0
#define IL_SDU_HEADER__SYMBOLID_M       (0x0000001F << IL_SDU_HEADER__SYMBOLID_SHL)

#define GEN_IL_SDU_HEADER_MSB32(fast_gain_control, sdu_wordlength, ccid, ruportid, sectionid, subframeid, symbolid) (((fast_gain_control) << 8) | ((sdu_wordlength & 0xff00) >> 8))
#define GEN_IL_SDU_HEADER_LSB32(fast_gain_control, sdu_wordlength, ccid, ruportid, sectionid, subframeid, symbolid) (((sdu_wordlength & 0x00ff) << 24) | ((ccid) << 21) | ((ruportid) << 13) | ((sectionid) << 9) | ((subframeid) << 5) | ((symbolid) << 0))

#define USER_APP_TIMESTAMP__TX_CONSECUTIVE_M    0x80000000
#define USER_APP_TIMESTAMP__TIMESTAMP_M         0x7fffffff

# if 1  // from the pl_2_30.bit
#define DMA_TS_CLK_IN_KHZ   245760
#define DMA_TS_CLK_IN_MHZ   245.76
# else
#define DMA_TS_CLK_IN_KHZ   122880
#define DMA_TS_CLK_IN_MHZ   122.88
# endif

enum _mutex{
    MUTEX_THREAD,
    MUTEX_SOCKET,
    NUM_MUTEX
};

enum { MAX_ITC_ELEM = 100 };
enum { MAX_ITC_MSG_BYTE = 1024 };
enum { MAX_SOCKBUFF_BYTE = 1500 };
enum { NUM_CARRIER = 2 };
enum { NUM_TX_SIGPATH = 4 };
enum { NUM_RX_SIGPATH = 4 };
enum { TX_ANT_BITMASK64 = 4, MAX_TX_ANT = 256 };
enum { RX_ANT_BITMASK64 = 4, MAX_RX_ANT = 256 };
enum { NUM_HERMES = 1 };
enum {
    P_IL,
    //S_IL,
    NUM_INNOLINK };
enum { NUM_SECTION_PER_CARRIER = 2, MAX_SECTION = 64 };
enum { MAX_PLAYBACK = 4 };
enum { MAX_FILENAME = 79 };
enum { MAX_SYMBOL = 14 };
enum { MAX_SHORT_CHAR = 20, MAX_LONG_CHAR = 80};
enum { WORD_SIZE_PER_MPLANE_SEGMENT = 8 };

enum {
    MPMON_CONNSOCK_0,
    MPMON_CONNSOCK_1,
    MPMON_CONNSOCK_2,
    NUM_MPMON_CONNSOCK
};

enum {
    HSWI_CONNSOCK_0,
    HSWI_CONNSOCK_1,
    HSWI_CONNSOCK_2,
    NUM_HSWI_CONNSOCK
};

enum _threadid{
    THREAD_TIME,
    THREAD_RE,
    THREAD_SP,
    THREAD_HSWI_CONN,
    THREAD_HSWI_LSTN = THREAD_HSWI_CONN + NUM_HSWI_CONNSOCK,
    THREAD_MPMON_CONN,
    THREAD_MPMON_LSTN = THREAD_MPMON_CONN + NUM_MPMON_CONNSOCK,
    THREAD_IL,
    NUM_THREAD
};

#include <stdint.h>     // uint64_t
#include <string>
extern std::string g_fpga_ver;
extern std::string g_fw_ver;
extern std::string g_dsp_ver;
extern uint32_t g_debug_mask;
extern uint32_t g_mplane_delay_ms;
enum {
    MASKBIT_CALL,           // "-o call"
    MASKBIT_EVENT,          // "-o event"
    MASKBIT_SOCKET,         // "-o socket"
    MASKBIT_NOFW,           // "-o nofw"
    MASKBIT_MPLANE_HEX,         // "-o mphex"
    MASKBIT_MPLANE_JSON,        // "-o mpjson"
    MASKBIT_MPLANE_PATTERN,     // "-o mppatt"
    MASKBIT_TMPLAYBACK,     // "-o tmplayback"
    NUM_MASKBIT
};

const uint64_t  AXIGPIO_0_BASE  = 0x80000000;   // JTAG, SWD
const uint64_t  AXIGPIO_1_BASE  = 0x80004000;   // reset, resync, rfonoff
const uint64_t  AXIDMA_BASE     = 0xA0000000;
//==================================================
// "/ddr/axi_dma_il"
//  interface: S_AXI_LITE
//  slave segment: Reg
//  master base address:    0x00_A000_0000
//  range: 0x1000 (4K)
//  master high address:    0x00_A000_0FFF

// "/ddr/ddr4" - PL-DDR4
//  master base address:    0x04_0000_0000
//  range: (0x20000000)     512MB
//  master high address:    0x04_1FFF_FFFF
//  tmdesc(16MB)            0x04_1000_0000
//  tmdata(32MB)            0x04_1100_0000
//  uldesc(16MB)            0x04_1300_0000
//  uldata(32MB)            0x04_1400_0000
//  others                  0x04_1600_0000
//                         ~0x04_1FFF_FFFF
//  reserved for dummy hal ~0x04_1FFF_F000
//                         ~0x04_1FFF_EFFF
//==================================================
const uint64_t  PLDDR_BASE  =            0x0400000000;
const uint64_t  TMDESC_BASE = PLDDR_BASE + 0x10000000;
const uint64_t  TMDATA_BASE = PLDDR_BASE + 0x11000000;
const uint64_t  ULDESC_BASE = PLDDR_BASE + 0x13000000;
const uint64_t  ULDATA_BASE = PLDDR_BASE + 0x14000000;
const uint64_t  DUMHAL_BASE = PLDDR_BASE + 0x1FFFF000;

// PAGE_SIZE = 0x1000
const uint32_t  PAGE_SIZE   = sysconf(_SC_PAGESIZE);

#endif // _NUM_DEF_H_
