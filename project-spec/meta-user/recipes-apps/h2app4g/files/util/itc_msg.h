#ifndef _ITC_MSG_H_
#define _ITC_MSG_H_

#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

enum _itc_msg_id {
    ITC_RESERVED_ID,
    ITC_KICKOFF_ID,
    ITC_NEW_CONNSOCK_ID,                // itc_new_connsock
    ITC_DIS_CONNSOCK_ID,                // itc_dis_connsock
    ITC_NVDATA_ID,                      // itc_payload
    ITC_PAYLOAD_ID,                     // itc_payload
    ITC_PAYLOAD_FROM_HSWI_ID,           // itc_payload
    ITC_PAYLOAD_FROM_IL_ID,             // itc_payload
    ITC_MPMON_ID,                       // itc_payload
    ITC_RESET_REQ_ID,                   // itc_reset_req
    ITC_RESET_IND_ID,                   // itc_reset_ind
    ITC_RESYNC_REQ_ID,                  // itc_resync_req
    ITC_UL_AXI_STREAM_REQ_ID,           // itc_ul_axi_stream_req
    ITC_STATE_REQ_ID,                   // itc_state_req
    ITC_STATE_IND_ID,                   // itc_state_ind
    ITC_PLAYBACK_REQ_ID,                // itc_playback_req
    ITC_PLAYBACK_IND_ID,                // itc_action_ind
    ITC_DOWNLOAD_TM_FILE_REQ_ID,        // itc_download_tm_file_req
    ITC_DOWNLOAD_TM_FILE_IND_ID,        // itc_action_ind
    ITC_DOWNLOAD_FILE_REQ__TM_FILE_ID,  // itc_download_tm_file_req
    ITC_DOWNLOAD_FILE_IND__TM_FILE_ID,  // itc_action_ind
    ITC_CREATE_CAPTURE_REQ_ID,          // itc_create_capture_req
    ITC_CREATE_CAPTURE_IND_ID,          // itc_action_ind
    ITC_CAPTURE_FILE_REQ__FRC_FILE_ID,  // itc_capture_frc_file_req
    ITC_UPLOAD_FILE_REQ__FRC_FILE_ID,   // itc_upload_frc_file_req
    ITC_UPLOAD_FILE_IND__FRC_FILE_ID,   // itc_action_ind
    ITC_VERSION_REQ_ID,
    ITC_VERSION_RSP_ID,                 // itc_version_rsp
    ITC_VERSION_IND_ID,                 // itc_version_rsp
    ITC_MISC_REQ_ID,                    // itc_misc_req
    NUM_ITC_ID
};

typedef struct _itc_hdr {
    uint16_t msgid;
    uint16_t msglen;
    uint16_t msgsrc;
    uint16_t msgdst;
}itc_hdr;

typedef struct _itc_action_ind {
    uint16_t uid;
    bool passfail;
    char        filename[MAX_FILENAME+1];
}itc_action_ind;

typedef struct _itc_new_connsock {
    int sockfd;
}itc_new_connsock;

typedef struct _itc_dis_connsock {
    int connid;
}itc_dis_connsock;

typedef struct _itc_payload {
    char sdu[MAX_ITC_MSG_BYTE];
}itc_payload;

typedef struct _itc_reset_req {
    uint32_t nextstate;
    int phase;
    int antport_on_reset;   // 0: antport-A, 1: antport-B
    int dl_il_sel;          // 0: innolink0, 1: innolink1
    int ul_il_sel;          // 0: innolink0, 1: innolink1
    int n_dpa;              // 0: unset,     1~4: set
}itc_reset_req;

typedef struct _itc_resync_req {
    bool enabled;
}itc_resync_req;

typedef struct _itc_ul_axi_stream_req {
    bool enabled;
}itc_ul_axi_stream_req;

typedef struct _itc_state_req {
    uint32_t nextstate;
}itc_state_req;

typedef struct _itc_state_ind {
    uint32_t currstate;
    uint32_t nextstate;
}itc_state_ind;

typedef struct _itc_version_rsp {
    uint16_t major_version;
    uint16_t minor_version;
}itc_version_rsp;

struct /* __attribute__ ((packed)) */ section_struct{
    uint32_t start_symbol_bitmask;
    uint32_t fs_x;    // sampling rate in K samples/second
    uint16_t section_id;
    uint16_t fftsize;
    uint16_t cp_length;
    uint8_t  numsymbol;
    uint8_t  frame_structure;
    uint8_t  scs;
    uint8_t  osr;
    double   fs;
};

struct /* __attribute__ ((packed)) */ playback_struct{
    uint64_t    txantbitmask[TX_ANT_BITMASK64];
    section_struct  section[NUM_SECTION_PER_CARRIER];
    char        filename[MAX_FILENAME+1];
    uint16_t    sectionid[NUM_SECTION_PER_CARRIER];
    uint8_t     playbackid;
    uint8_t     state;
    uint8_t     numtxant;
    uint8_t     numsection;
    uint8_t     num_segments;
    uint8_t     off_duty_mode;  // 0: "zeropad" | "nongated", 1: "dtx" | "gated"
    uint8_t     ul_only;		// 0: dl, 1: ul
    uint8_t     tdd_ul_dl_config;
    uint8_t     tdd_ssf_config;
    double      tm_timing_adv;  // usec
};

typedef struct _itc_playback_req {
    uint16_t uid;
    uint16_t num_playbackobj;
    playback_struct tmplaybackobj[MAX_PLAYBACK];
}itc_playback_req;

typedef struct _itc_download_tm_file_req {
    char scheme[MAX_SHORT_CHAR];
    char host[MAX_SHORT_CHAR];
    char user[MAX_SHORT_CHAR];
    char password[MAX_SHORT_CHAR];
    char path[MAX_LONG_CHAR];
    uint16_t uid;
}itc_download_tm_file_req;

typedef struct _itc_create_capture_req {
    uint16_t uid;
    uint32_t sdu_mask;
    uint32_t sdu_target;
    uint8_t  num_segments;
    uint8_t  off_duty_mode;     // 0: "zeropad" | "nongated", 1: "dtx" | "gated"
    uint8_t  scs;               // 0: 15 kHz, 1: 30 kHz
    uint8_t  tdd_ul_dl_config;
    uint8_t  tdd_ssf_config;
    uint8_t  osr;
}itc_create_capture_req;

typedef struct _itc_capture_frc_file_req {
    char scheme[MAX_SHORT_CHAR];
    char host[MAX_SHORT_CHAR];
    char user[MAX_SHORT_CHAR];
    char password[MAX_SHORT_CHAR];
    char path[MAX_LONG_CHAR];
    uint16_t uid;
}itc_capture_frc_file_req;

typedef struct _itc_upload_frc_file_req {
    char scheme[MAX_SHORT_CHAR];
    char host[MAX_SHORT_CHAR];
    char user[MAX_SHORT_CHAR];
    char password[MAX_SHORT_CHAR];
    char path[MAX_LONG_CHAR];
    uint16_t uid;
}itc_upload_frc_file_req;

typedef struct _itc_misc_req {
    uint32_t gpio31;
}itc_misc_req;

typedef struct _itc_msg {
    itc_hdr hdr;
    union {
        itc_action_ind              startofbody;
        itc_new_connsock            new_connsock;
        itc_dis_connsock            dis_connsock;
        itc_payload                 payload;
        itc_reset_req               reset_req;
        itc_resync_req              resync_req;
        itc_ul_axi_stream_req       ul_axi_stream_req;
        itc_state_req               state_req;
        itc_state_ind               state_ind;
        itc_playback_req            playback_req;
        itc_create_capture_req      create_capture_req;
        itc_action_ind              action_ind;
        itc_download_tm_file_req    download_tm_file_req;
        itc_version_rsp             version_rsp;
        itc_capture_frc_file_req    capture_frc_file_req;
        itc_upload_frc_file_req     upload_frc_file_req;
        itc_misc_req                misc_req;
    };
}itc_msg;

#endif // _ITC_MSG_H_
