
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#define DEBUG


#include "common.h"

#define HEADER_VER_MAJ 1
#define HEADER_VER_MIN 1


#define PRE_REG_OFFSET(x) reg_buf[((x/sizeof(uint32_t))-0x10)]
#define PRE_REG_OFFSET_IN_FPGA(x) preg_virtual_addr[x/sizeof(uint32_t)]

typedef struct
{
    uint32_t PL_protocol_version;
    uint32_t PS_protocol_version;
    uint32_t PL_version;
    uint32_t PL_status;
} ps_pl_header_t;

typedef struct
{

    uint32_t control;
    uint32_t status;
    uint32_t Sequence_number_of_slot;
    uint32_t Sequence_number_of_symbol;
    uint32_t Sequence_number_of_layer;
    uint32_t dut_error;
    uint32_t dut_error_status;
} dut_header_t;



typedef struct
{

    uint32_t length;//	Length of the vector in bytes
    uint16_t slot_num;//	Sequence number of the slot
    uint16_t sym_num;//	Sequence number of the symbol
    uint16_t layer_num;//	Sequence number of the layer
    uint16_t dir;//	0 – Matlab to MPSOC, 1 – MPSOC to Matlab
    uint16_t type;//	0 - Control, 1- IFFT, 2 - FFT 3 – PRACH, 4 – Precoding, 5 – Combining, 6 – Precoding Matrix, 7 – Combining Matrix,
    uint16_t Version_maj;//	Major version msb xx 01
    uint16_t Version_min;//	Minor version lsb yy 01
    uint16_t Register0;	//General register 0
    uint16_t Register1;	//General register 0
    uint16_t  Register2;//	General register 0
    //uint16_t  Register3;//	General register 0


} matlab_header_t;

typedef struct
{
    uint32_t h_length;//	Header length
    uint16_t nFFT;//	Size of IFFT
    uint16_t start_re;//	Index of first SC
    uint16_t nSCS;//	Number of SCS to map
    uint16_t MaxnSCS; // Maximal number of SCS
    uint16_t CpLen;//	Length of CP
    uint16_t win_size;//	Length of window
    uint16_t win[60];//	Window coefficients
} ifft_header_t;


typedef struct
{

    uint32_t	h_length;//	Header length
    uint16_t nFFT;//	Size of FFT
    uint16_t start_re;//	Index of first SC
    uint16_t nSCS;//	Number of SCS to map
    uint16_t MaxnSCS;//	Maximal number of SCS
    uint16_t CpLen;//	Length of CP
    uint16_t fftStart;//	Location of Symbol start
    int16_t 	timeOffset;//	Additional time offset samples
} fft_header_t;

typedef union
{
    uint16_t data;
    struct
    {
        uint16_t rept: 1;
        uint16_t interp: 3;
        uint16_t cont: 1;
    } bits;
} fpga_data_rb_t;
#pragma pack(2)
typedef union
{
    uint16_t data;
    struct //interpolation_fields
    {
        uint16_t interpolation: 5; //Interpolation value:x1, x2, x4, x8, x12 etc.. with x1 means that no interpolation is used :5 bits
        uint16_t rep: 1; //means that the same value is used for the entire RB (overrides the interp value)
        uint16_t cont: 1; //continuous with following RB – rather the interpolation continuous with the following RB
        uint16_t ssb: 1; //SSB indication – so the precoding values are taken from a different source
    }  bits_t;
} interpolation_t ;

#pragma pack(2)
typedef struct
{

    uint32_t	h_length;//	Header length
    uint16_t	startSFN;//	The first SFN this precoding matrix applies to
    uint16_t	startSlot;//	The first Symbol this precoding matrix applies to
    uint16_t	nLayers;//	Number of layers
    uint16_t	nAnt;//	Number of Antennas
    uint16_t	nSCSin;//	Number of SCS in
    uint16_t	nSCSout;//	Number of SCS out
    uint16_t	nRbs;//	Number of RBs
    uint16_t	iAnt;//	Index of antenna to monitor for linear interpolation
    uint16_t	iLayer;//	Index of layer to monitor for linear interpolation
//   interpolation_t	*InterpArray;



} precoding_matrix_t;



#pragma pack(2)
typedef struct
{
    uint32_t	h_length;//	Header length
    int16_t	total_ant_num;
    int16_t	ant_num;//	Index of antenna
    int16_t	TotalSymbol;//	Total number of symbols
    int16_t	CurrentSymbol;//	Current number of symbol
    uint16_t	FrameToRecord;//

} iq_sample_t;

#pragma pack(2)
typedef struct
{
    uint32_t	h_length;//	Header length
    int16_t	total_ant_num;
    int16_t	ant_num;//	Index of antenna
    int16_t	TotalSymbol;//	Total number of symbols
    int16_t	CurrentSymbol;//	Current number of symbol
    uint16_t	FrameToRecord;//
    uint16_t	SymbolLength;//

} iq_sample_ul_t;

#pragma pack(2)
typedef struct
{
    uint32_t	h_length;//	Header length
    int16_t	upSampling;
    int16_t lastSym	;//Indication for last symbol 	1 indicate start 	2 indicate last 	0 indicate middle

} upSampling_t;

int GetIFFTregs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs, \
                uint32_t *offset_to_write, int log_fd);

int GetFFTregs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs, \
               uint32_t *offset_to_write, int log_fd);

int GetPrecodingMatrix_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                            uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr);

int GetIQsample__DL_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                         uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr);

int GetIQsample__UL_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                         uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr);
int GetUpsampler_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                      uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr);


int setFPGA_muxRegs(uint32_t *reg_virtual_addr, int mode);

void SetFPGA_Regs(uint32_t *reg_virtual_addr);

int CheckFpgaStatus(long *virt_addr)
{



    ps_pl_header_t *ps_pl_header = (ps_pl_header_t *)virt_addr;
    //ps_pl_header
    return 0;
    if (ps_pl_header->PL_protocol_version >= 2)
        return -1;
    //set protocol version of PS
    ps_pl_header->PS_protocol_version = 0;
    printf("PL version %d PS Protocol version %d PL protocol version %d\n", ps_pl_header->PL_version, ps_pl_header->PS_protocol_version, ps_pl_header->PL_protocol_version);
    //check PL ready
    if (ps_pl_header->PL_status == 0)
    {
        printf("PL status =0\n");
        //return -2;
    }

    return 0;
}



// buf - pointer to all data from matlab
// bytesSock - max bytes in sock
//

int ProtocolCheck (uint8_t *buf, int bytesSock, int *type, uint32_t fpga_id, int *data_length)
{

    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    *type = -1;
    if (bytesSock < sizeof(matlab_header_t))
    {

        DEBUG_PRINT("matlab_header is wrong socket data is too small  =%d\n", bytesSock);
        return 1;
    }
    if (bytesSock < matlab_header->length )
    {
        DEBUG_PRINT("matlab_header size %d get bytes %d\n", matlab_header->length, bytesSock);
        if (data_length != NULL)
            *data_length = matlab_header->length ;
        return 2;
    }
    if (matlab_header->dir != 0)
    {
        DEBUG_PRINT("matlab_header is wrong dir =%x\n", matlab_header->dir);
        return -2;
    }
    if (matlab_header->type > 15 )
    {
        DEBUG_PRINT("matlab_header is wrong type = %x\n", matlab_header->type);
        return -3;
    }
    *type = matlab_header->type;

    if (matlab_header->Version_maj != HEADER_VER_MAJ)
    {
        DEBUG_PRINT("matlab_header Version_maj is = %x\n", matlab_header->Version_maj);
        return -4;
    }

    if (matlab_header->Version_min != HEADER_VER_MIN)
    {
        DEBUG_PRINT("matlab_header Version_min is = %x\n", matlab_header->Version_min);
        return -5;
    }

//     if ((matlab_header->type != fpga_id ) && (matlab_header->type))
//     {
//         DEBUG_PRINT("matlab_header is wrong type 0x%x VS FPGA type 0x%x\n", matlab_header->type, fpga_id);
//         return -6;
//     }
    return 0;


}


//buf - original buf
//output
//p_data_buf - a pointer to data which will be writen to the fifo
// reg_buf - buffer will contain all the regs after format acording to the mseg type
// num_of_data -total number of the fifo.
// return - total number of the regs
int GetIFFTregs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs, \
                uint32_t *offset_to_write, int log_fd)
{
    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    ifft_header_t *ifft_header;
    int i;
    int tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf, nfft;

    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    ifft_header = (ifft_header_t *)buf;
    DEBUG_PRINT("ifft matlab_header 0x%8.8x \n", (uint32_t)matlab_header);
    DEBUG_PRINT("ifft ifft_header 0x%8.8x \n", (uint32_t)ifft_header);

    if (bytesSock < 100)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if (ifft_header->h_length + sizeof(matlab_header_t) > bytesSock)
    {
        DEBUG_PRINT("ifft_header->h_length+sizeof(matlab_header_t) %d> bytesSock %d\n", ifft_header->h_length + sizeof(matlab_header_t), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);


    DUPPRINT(log_fd, "ifft length = 0x%4.4x \n", ifft_header->h_length);
    DUPPRINT(log_fd, "ifft Size of IFFT = 0x%4.4x \n", ifft_header->nFFT);
    DUPPRINT(log_fd, "ifft Index of first SC = 0x%4.4x \n", ifft_header->start_re);
    DUPPRINT(log_fd, "ifft Number of SCS t(o map = 0x%4.4x \n", ifft_header->nSCS);
    DUPPRINT(log_fd, "ifft Max Number of SCS = 0x%4.4x \n", ifft_header->MaxnSCS);
    DUPPRINT(log_fd, "ifft Length of CP = 0x%4.4x \n", ifft_header->CpLen);
    DUPPRINT(log_fd, "ifft Length of window = 0x%4.4x \n", ifft_header->win_size);

    if (ifft_header->h_length < ifft_header->win_size)
    {
        DEBUG_PRINT("ifft_header->h_length % d < ifft_header->win %d \n", ifft_header->h_length, ifft_header->win_size);
        return -5;
    }

    if ( (ifft_header->start_re + ifft_header->nSCS) > ifft_header->nFFT)
    {
        DEBUG_PRINT("ifft_header->start_re 0x%4.4x + ifft_header->nSCS 0x%4.4x ) > ifft_header->nFFT 0x%4.4x \n", ifft_header->start_re, ifft_header->nSCS, ifft_header->nFFT);
        return -6;

    }

    for (i = 0; i < ifft_header->win_size; i++)
        DEBUG_PRINT("ifft windows data = 0x%4.4x \n", ifft_header->win[i]);
    if (ifft_header->win_size > 10)
        return -10;

    if ( (ifft_header->nFFT / 2) >= ifft_header->nSCS)
    {
        DEBUG_PRINT("(ifft_header->nFFT %d /2) >= ifft_header->nSCS %d) \n", ifft_header->nFFT, ifft_header->nSCS);
        return -11;

    }


    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));
    reg_buf += sizeof(dut_header_t) / sizeof(uint32_t);

    nfft = 0;
    if (ifft_header->nFFT == 0x1000)
        nfft = 0xc;
    if (ifft_header->nFFT == 0x800)
        nfft = 0xb ;
    if (ifft_header->nFFT == 0x400)
        nfft = 0xa;
    if (ifft_header->nFFT == 0x200)
        nfft = 0x9;
    if (!nfft)
        return -7;//

    *(reg_buf) = nfft;//Nfft_cfg[4:0]	Size of iFFT

    *(++reg_buf) = ifft_header->nSCS; //Amount_of_samples_cfg[12:0]	Number of SCS to map
    tmp2 = (ifft_header->nFFT - ifft_header->MaxnSCS) / 2 +  ifft_header->start_re;
    if (tmp2 && (tmp2 < ifft_header->nFFT / 2 ))
        *(++reg_buf) = tmp2; //Pad_before_samples_cfg[12:0]	(nFFT-MaxnSCS )/2+start_re
    else
    {
        DEBUG_PRINT("is Not (nFFT %d -MaxnSCS %d )/2+start_re  %d must be bigger than 0 and less than (nFFT)/2) \n", \
                    ifft_header->nFFT, ifft_header->MaxnSCS, ifft_header->start_re, ifft_header->nFFT );
        return -13;
    }
    *(++reg_buf) = ifft_header->nFFT - tmp2 - ifft_header->nSCS; //Pad_after_samples_cfg[12:0]	Size of iFFT- Index of first SC - Number of SCS to map
    *(++reg_buf) = ifft_header->CpLen; //Ifft_cp_len_all_cfg0[12:0]	Length of CP
    *(++reg_buf) = ifft_header->CpLen ; //Ifft_cp_len_all_cfg1[12:0]	Length of CP
    *(++reg_buf) =  0; //_seq_all_sel_cfg	0
    for (i = 0; i < ifft_header->win_size ; i++)
    {
        *(++reg_buf) = ifft_header->win[i] | ifft_header->win[i] << 16; //ifft_windowing_mult_data_0_0[31:0] 	Window coefficients[Length of window]
    }
    for (; i < 10; i++)
    {
        *(++reg_buf) = 0; //ifft_windowing_mult_data_0_0[31:0] 	Window coefficients[Length of window]
    }

    for ( i = 0; i < ifft_header->win_size ; i++)
    {
        *(++reg_buf) = ifft_header->win[i] | ifft_header->win[i] << 16; //ifft_windowing_mult_data_1_0[31:0] 	Window coefficients[Length of window]
    }

    for (; i < 10; i++)
    {
        *(++reg_buf) = 0; //ifft_windowing_mult_data_0_0[31:0] 	Window coefficients[Length of window]
    }



    *(++reg_buf) = ifft_header->win_size; // ifft_windowing_first_index_0[31:0]	Length of window
    *(++reg_buf) = ifft_header->win_size; // ifft_windowing_first_index_1[31:0]	Length of window
    *(++reg_buf) = ifft_header->nFFT + ifft_header->CpLen - ifft_header->win_size; // ifft_windowing_last_index_0[31:0]	Ifft_cp_len_all_cfg0-ifft_windowing_first_index_0
    *(++reg_buf) = ifft_header->nFFT + ifft_header->CpLen - ifft_header->win_size; // ifft_windowing_last_index_1[31:0]	Ifft_cp_len_all_cfg1-ifft_windowing_first_index_1
    *(++reg_buf) = ifft_header->nFFT + ifft_header->CpLen; // ifft_total_len_all_cfg0[12:0]	Size of iFFT+Length of CP
    *(++reg_buf) = ifft_header->nFFT + ifft_header->CpLen; // ifft_total_len_all_cfg1[12:0]	Size of iFFT+Length of CP
    *(++reg_buf) = 0; // ifft_total_len_all_sel_cfg[27:0]	0

    *(++reg_buf) = nfft; //0xe4
    tmp2 = (ifft_header->CpLen);
    tmp3 = (12 - nfft);
    tmp2 = tmp2 << tmp3;
    *(++reg_buf) = tmp2;//0xe8
    DEBUG_PRINT("shift %x , %x\n", tmp2, tmp3);
    *(++reg_buf) = 0;//0xec
    *(++reg_buf) = 0xaaa;
    *(++reg_buf) = 1;//0xf4

    // top_reg_buf[(0xe4 - 0x40) / 4] =

    DEBUG_PRINT("pointer max 0%8.8x pointer = 0x%8.8x\n", reg_buf, top_reg_buf);



    *num_of_regs = ((uint32_t) (reg_buf - top_reg_buf)) + 1;
    *num_of_data = ifft_header->nSCS * sizeof(uint32_t); //matlab_header->length - sizeof(matlab_header_t) - ifft_header->h_length;
    *p_data_buf = buf + ifft_header->h_length;

    reg_buf = top_reg_buf;
    int summ = 0;
    DEBUG_PRINT("\nifft regs sum: \n");
    for (i = 0; i < ((0x100 - 0x40) / 4) ; i ++)
    {
        //   DEBUG_PRINT("ifft location 0x%3.3x 0x%8.8x value = 0x%8.8x = %d\n", (i * 4 + 0x40), &(reg_buf[i]), reg_buf[i], reg_buf[i]);
        if ((i == (0x60 - 0x40) / 4) || (i == (0x64 - 0x40) / 4) || (i == (0x68 - 0x40) / 4))
            summ += reg_buf[i ];


    }
    DEBUG_PRINT("ifft regs end\n");
    DEBUG_PRINT("num_of_data =0%4.4x pointer = 0x%8.8x\n", *num_of_data, *p_data_buf);
    DEBUG_PRINT("Sum = %d\n", summ);
    if (summ != 4096)
        DEBUG_PRINT("Error Sum is not 4096 it is %d\n", summ);


    *offset_to_write = 0x10;
    return 0;


}


//fifo ID - fifo ID which write the data
//buf - original buf
//output
//p_data_buf - a pointer to data which will be writen to the fifo
// reg_buf - buffer will contain all the regs after format acording to the mseg type
// num_of_data -total number of the fifo.
// return - total number of the regs
int GetGegister(int fifoID, uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *reg_buf, uint32_t *num_of_regs, \
                uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *virt_addr_reserved_memory)
{
    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    int type;


    if (ProtocolCheck(buf, bytesSock, &type, 0, NULL))
        DEBUG_PRINT("ProtocolCheck return non zero\r\n");
    if (fifoID == 1)
        return ( GetIFFTregs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd) );
    if (fifoID == 2)
        return ( GetFFTregs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd) );
    if (fifoID == 6)
        return ( GetPrecodingMatrix_regs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd, reg_virtual_addr) );
    if ((type == 8) || (type == 11))
        return ( GetIQsample__DL_regs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd, reg_virtual_addr, virt_addr_reserved_memory) );
    if ((type == 9) || (type == 10))
        return ( GetIQsample__UL_regs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd, reg_virtual_addr, virt_addr_reserved_memory));
    if ((type == 12) )
        return ( GetUpsampler_regs(buf, bytesSock, p_data_buf, num_of_data, reg_buf, num_of_regs, offset_to_write, log_fd, reg_virtual_addr, virt_addr_reserved_memory));


    *p_data_buf = buf;
    *num_of_data = bytesSock;
    DEBUG_PRINT("num_of_data %x %d bytesSock=%d\n", num_of_data, *num_of_data, bytesSock);

    return 0;
}

int AddHeaderToData(int FifoID, uint8_t *buf, uint32_t total_size, uint32_t *reg_address, uint32_t fpga_id, int32_t over_symbol)
{
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    dut_header_t *dut_header = reg_address;


    memset(matlab_header, 0, sizeof(matlab_header_t));

    matlab_header->length = total_size;
    matlab_header->Version_maj = HEADER_VER_MAJ;
    matlab_header->Version_min = HEADER_VER_MIN;

    matlab_header->slot_num = dut_header->Sequence_number_of_slot;
    if (over_symbol >= 0)
        matlab_header->sym_num = over_symbol & 0xf;
    else
        matlab_header->sym_num = dut_header->Sequence_number_of_symbol;
    matlab_header->layer_num = dut_header->Sequence_number_of_layer;
    matlab_header->dir = 1;
    matlab_header->type = FifoID;
    switch (fpga_id)
    {
        case 1:
            reg_address += (0xf8 - 0x40) / sizeof(uint32_t);// read ifft_ip_current_scale
            break;
        case 2:
            reg_address += (0x1f - 0x10);// read fft_scale register
            break;
    }
    matlab_header->Register0 = *reg_address;

    switch (fpga_id)
    {
        case 8:
            matlab_header->Register0 = 0;
            break;
        case 9:
            matlab_header->Register0 = 0xffff;
            break;
    }

    DEBUG_PRINT("reg 0  0x%8.8x \n", matlab_header->Register0);
    return 0;
}



int GetUpsampler_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                      uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr)
{
    extern void reset_axi_fifo(void);
    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    upSampling_t *upSampling_header;
    int i;
    int tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf, nfft;
    uint64_t tuser;
    static int order = -10;
    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;
    uint64_t aaa;


    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    upSampling_header = (upSampling_t *)buf;

    if (bytesSock < 100)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if ((upSampling_header->h_length + sizeof(matlab_header_t)) > bytesSock)
    {
        DEBUG_PRINT(" upSampling->h_length+sizeof(matlab_header_t) %d > bytesSock %d\n", (upSampling_header->h_length + sizeof(matlab_header_t)), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);



    DUPPRINT(log_fd, "upSampling length                      = 0x%4.4x \n", upSampling_header->h_length);
    DUPPRINT(log_fd, "upSampling  upSampling                = 0x%4.4x \n", upSampling_header->upSampling);


    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));


    *offset_to_write = 0x20;

    *num_of_regs = 10;
    *num_of_data = matlab_header->length - sizeof(matlab_header_t) - upSampling_header->h_length;
    int file_data_size = matlab_header->length - sizeof(matlab_header_t) - upSampling_header->h_length;

//  if( ((upSampling_header->lastSym==1)&& (order ==0)) ||
//   		( (order ==2 ) && (upSampling_header->lastSym==0) ) ||
    //		( )
    if ( (order != 2 ) && (upSampling_header->lastSym == 1))
//    if ((upSampling_header->lastSym==1))
    {
        //do reset
        DEBUG_PRINT("doing reset \n");
        DEBUG_PRINT("0x%x,\n", preg_virtual_addr[0x40 / sizeof(uint32_t)]);
        PRE_REG_OFFSET_IN_FPGA(0x40) = 0;
        DEBUG_PRINT("0x%x,\n", preg_virtual_addr[0x40 / sizeof(uint32_t)]);
        reset_axi_fifo();
        PRE_REG_OFFSET_IN_FPGA(0x40) = 1;
        PRE_REG_OFFSET_IN_FPGA(0x48) = 1;
        DEBUG_PRINT("0x%x,\n", preg_virtual_addr[0x40 / sizeof(uint32_t)]);



    }

    order = upSampling_header->lastSym;
    if (bytesSock < ( *num_of_data )  )
    {
        DEBUG_PRINT("Error upSampling payload size %d > socket data size\n", ( *num_of_data ), bytesSock);
        return -7;
    }


    *p_data_buf = buf + upSampling_header->h_length;

    {

        tuser = dut_header.Sequence_number_of_symbol & 0xf;
        tuser <<= 24;
        aaa = (dut_header.Sequence_number_of_slot & 1);
        aaa <<= 32;
        tuser |= aaa;
    }
//   if (upSampling_header->lastSym==2)
//   {
//	   tuser=13;
//	   tuser<<=24;
//   }
    if (upSampling_header->lastSym == 2)
    {
        aaa = 1;
        aaa <<= 60;
        tuser |= aaa;
    }
    // PRE_REG_OFFSET_IN_FPGA(0x50)=1;
    (*p_data_buf) -= sizeof(uint64_t);
    memcpy((*p_data_buf), &tuser, sizeof(uint64_t));
    (*num_of_data) += sizeof(uint64_t);



    return 0;


}





//buf - original buf
//output
//p_data_buf - a pointer to data which will be writen to the fifo
// reg_buf - buffer will contain all the regs after format acording to the mseg type
// num_of_data -total number of the fifo.
// return - total number of the regs
int GetFFTregs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
               uint32_t *offset_to_write, int log_fd)
{
    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    fft_header_t *fft_header;
    int i;
    int tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf, nfft;
    uint32_t lo_off ;
#define FFT_REG_OFFSET(x) reg_buf[((x/sizeof(uint32_t))-0x10)]


    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    fft_header = (fft_header_t *)buf;

    if (bytesSock < 100)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if ((fft_header->h_length + sizeof(matlab_header_t)) > bytesSock)
    {
        DEBUG_PRINT(" fft_header->h_length+sizeof(matlab_header_t) %d > bytesSock %d\n", (fft_header->h_length + sizeof(matlab_header_t)), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);

    usleep(500);



    DUPPRINT(log_fd, "fft length                      = 0x%4.4x \n", fft_header->h_length);
    DUPPRINT(log_fd, "fft Size of FFT                 = 0x%4.4x \n", fft_header->nFFT);
    DUPPRINT(log_fd, "fft Index of first SC           = 0x%4.4x \n", fft_header->start_re);
    DUPPRINT(log_fd, "fft Number of SCS to map        = 0x%4.4x \n", fft_header->nSCS);
    DUPPRINT(log_fd, "fft Max Number of SCS           = 0x%4.4x \n", fft_header->MaxnSCS);
    DUPPRINT(log_fd, "fft Length of CP                = 0x%4.4x \n", fft_header->CpLen);
    DUPPRINT(log_fd, "fft start                       = 0x%4.4x \n", fft_header->fftStart);
    DUPPRINT(log_fd, "fft Additional time offset samples = 0x%4.4x \n", fft_header->timeOffset);



    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));
// reg_buf += sizeof(dut_header_t) / sizeof(uint32_t);

    nfft = 0;
    if (fft_header->nFFT == 0x1000)
        nfft = 0xc;
    if (fft_header->nFFT == 0x800)
        nfft = 0xb ;
    if (fft_header->nFFT == 0x400)
        nfft = 0xa;
    if (fft_header->nFFT == 0x200)
        nfft = 0x9;
    if (!nfft)
        return -7;//

    lo_off = 0x10;//sizeof(dut_header_t) / sizeof(uint32_t);

    FFT_REG_OFFSET(0x5c) = nfft; //Nfft_cfg[4:0]	Size of iFFT

    FFT_REG_OFFSET(0x60) = fft_header->nSCS; //Amount_of_samples_cfg[12:0]	Number of SCS to map
    tmp2 = (fft_header->nFFT - fft_header->MaxnSCS) / 2;
    FFT_REG_OFFSET(0x64) = tmp2;
    FFT_REG_OFFSET(0x68) = tmp2;
    if (FFT_REG_OFFSET(0x68) < 0 )
    {
        DEBUG_PRINT("fft_header->nFFT - (fft_header->fftStart+ fft_header->nSCS) < 0 \n");
        return -8;
    }

    if (fft_header->CpLen < fft_header->fftStart)
    {
        DEBUG_PRINT("fft_header->CpLen < fft_header->fftStart \n");
    }
    FFT_REG_OFFSET(0x6c) = fft_header->CpLen;
    FFT_REG_OFFSET(0x70) = fft_header->CpLen;
    FFT_REG_OFFSET(0x74) = 0;
    FFT_REG_OFFSET(0x78) = nfft + 0x100;
    FFT_REG_OFFSET(0x7c) = 0;
    FFT_REG_OFFSET(0x80) = 1;

    FFT_REG_OFFSET(0x84) = fft_header->fftStart;

    FFT_REG_OFFSET(0x88) = fft_header->fftStart;

    FFT_REG_OFFSET(0x8c) = 0;





    tmp2 = fft_header->fftStart + fft_header->timeOffset;
    if (tmp2 < 0)
    {
        DEBUG_PRINT("fail fft_header->fftStart 0x%x+ fft_header->timeOffset 0x%x < 0 \n", fft_header->fftStart, fft_header->timeOffset );

        return -5;
    }

    if (tmp2 > fft_header->CpLen)
    {
        DEBUG_PRINT("fail fft_header->fftStart 0x%x+ fft_header->timeOffset 0x%x < fft_header->CpLen 0x%x\n", fft_header->fftStart, fft_header->timeOffset, fft_header->CpLen);

        return -5;
    }

    if (fft_header->fftStart > fft_header->CpLen)
    {
        DEBUG_PRINT("fail fftStart > CpLen\n");

        return -12;
    }

    FFT_REG_OFFSET(0x90) = fft_header->fftStart;
    FFT_REG_OFFSET(0x94) = fft_header->fftStart;
    FFT_REG_OFFSET(0x98) = 0;

    *offset_to_write = 0x10;

    *num_of_regs = ((uint32_t) (&reg_buf[0x26 - lo_off] - top_reg_buf)) + 1 ;
    *num_of_data = (fft_header->nFFT + fft_header->CpLen) * sizeof(uint32_t);
    int file_data_size = matlab_header->length - sizeof(matlab_header_t) - fft_header->h_length;
    if ( file_data_size !=  ( *num_of_data )  )
        DEBUG_PRINT("Error fft payload size %d !=  nFFT+ CpLen %d\n", ( *num_of_data ), file_data_size);

    if (bytesSock < ( *num_of_data )  )
    {
        DEBUG_PRINT("Error fft payload size %d > socket data size\n", ( *num_of_data ), bytesSock);
        return -7;
    }

    *p_data_buf = buf + fft_header->h_length;



    return 0;


}



#define DDR_OFSET_FOR_UL 0x500000
#define SAMPLE_DDR_OFSET 0x130000


void SetFPGA_Regs(uint32_t *reg_virtual_addr)
{



    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;




//########## config symbol length

    PRE_REG_OFFSET_IN_FPGA(0x10060) = 0x00000000     ; //24 - ready mux in the oRAN 0 - no loop : 1 - loop
    PRE_REG_OFFSET_IN_FPGA(0x10050) = 0x00002240      ; //20 - short symbol length
    PRE_REG_OFFSET_IN_FPGA(0x10054) = 0x00002280      ; //21 - long symbol length

    PRE_REG_OFFSET_IN_FPGA(0x1004c) = 0x00000002 ; //PRE_REG_OFFSET_IN_FPGA(0x1004c)=0x00000001 ;// 0       ;//sniffer ---- bit 1:0 - downlink -- bbu --> antenna ////  1 - generatore /// 2 - /// uplink  --- anntenna --> bbu
    PRE_REG_OFFSET_IN_FPGA(0x10064) = 0x00000000      ; //25  - external fiber loopback
    PRE_REG_OFFSET_IN_FPGA(0x10068) = 0x00000001 ; // devmem 0xb0010068)=0x00000001 ;//0      ;//26  - up_loopback_dl_selector        /// 0 - down  /  1 up
    PRE_REG_OFFSET_IN_FPGA(0x1006c) = 0x00000000 ; // devmem 0xb001006c)=0x00000000 ;//1      ;//27  - select_generator_innowrap_uplink // 0 -generator / 1  innowrap uplink
    PRE_REG_OFFSET_IN_FPGA(0x10070) = 0x00000000      ; //28  - external fiber loopback          ///select_generator_oran_dl        = slv_reg28 ;/    / 0 oran dllink    /   1 -generator

//;// config the tic block

    PRE_REG_OFFSET_IN_FPGA(0x0002c) = 0x0000000e ; //e   ;// 14 0x0000000e
    PRE_REG_OFFSET_IN_FPGA(0x00030) = 0x00004500   ; // 17664
    PRE_REG_OFFSET_IN_FPGA(0x00034) = 0x00004480   ; // 17536
    PRE_REG_OFFSET_IN_FPGA(0x00038) = 0x0EA60000   ; // 245760000
    PRE_REG_OFFSET_IN_FPGA(0x0003c) = 0x0003C000   ; // 245760



    PRE_REG_OFFSET_IN_FPGA(0xa002c) = 0x0000000e ; //e   ;// 14
    PRE_REG_OFFSET_IN_FPGA(0xa0030) = 0x00004500   ; // 17664
    PRE_REG_OFFSET_IN_FPGA(0xa0034) = 0x00004480   ; // 17536
    PRE_REG_OFFSET_IN_FPGA(0xa0038) = 0x0EA60000   ; // 245760000
    PRE_REG_OFFSET_IN_FPGA(0xa003c) = 0x0003C000   ; // 245760

    PRE_REG_OFFSET_IN_FPGA(0xb002c) = 0x0000000e ; //e   ;// 14
    PRE_REG_OFFSET_IN_FPGA(0xb0030) = 0x00004500   ; // 17664
    PRE_REG_OFFSET_IN_FPGA(0xb0034) = 0x00004480   ; // 17536
    PRE_REG_OFFSET_IN_FPGA(0xb0038) = 0x0EA60000   ; // 245760000
    PRE_REG_OFFSET_IN_FPGA(0xb003c) = 0x0003C000   ; // 245760

    PRE_REG_OFFSET_IN_FPGA(0x2002c) = 0x0000000e ; //e   ;// 14
    PRE_REG_OFFSET_IN_FPGA(0x20030) = 0x00004500   ; // 17664
    PRE_REG_OFFSET_IN_FPGA(0x20034) = 0x00004480   ; // 17536
    PRE_REG_OFFSET_IN_FPGA(0x20038) = 0x0EA60000   ; // 245760000
    PRE_REG_OFFSET_IN_FPGA(0x2003c) = 0x0003C000   ; // 245760

    PRE_REG_OFFSET_IN_FPGA(0x5002c) = 0x0000000e ; //e   ;// 14
    PRE_REG_OFFSET_IN_FPGA(0x50030) = 0x00004500   ; // 17664
    PRE_REG_OFFSET_IN_FPGA(0x50034) = 0x00004480   ; // 17536
    PRE_REG_OFFSET_IN_FPGA(0x50038) = 0x0EA60000   ; // 245760000
    PRE_REG_OFFSET_IN_FPGA(0x5003c) = 0x0003C000   ; // 245760






}

int setFPGA_muxRegs(uint32_t *reg_virtual_addr, int mode)
{

    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;

    switch (mode)
    {
        case 8:
            // DL injector config registers :

            PRE_REG_OFFSET_IN_FPGA(0x1004c) = 0x00000000    ; //devmem 0xb001004c)=0x00000001 ;// 0       ;//sniffer ---- bit 1:0 - downlink -- bbu --> antenna ;//;//  1 - generatore ;/// 2 - ;/// uplink  --- anntenna --> bbu
            PRE_REG_OFFSET_IN_FPGA(0x10068) = 0x00000000    ; // devmem 0xb0010068 32  0x00000001 ;//0      ;//26  - up_loopback_dl_selector        ;/// 0 - down  /  1 up
            PRE_REG_OFFSET_IN_FPGA(0x1006c) = 0x00000001     ; // devmem 0xb001006c 32  0x00000000 ;//1      ;//27  - select_generator_innowrap_uplink ;// 0 -generator / 1  innowrap uplink
            PRE_REG_OFFSET_IN_FPGA(0x10070) = 0x00000001     ; //28      ;///select_generator_oran_dl       / 0 oran dllink    /   1 -generator
            PRE_REG_OFFSET_IN_FPGA(0x10060) = 0x00000000     ; //24 - ready mux in the oRAN 0 - no loop : 1 – loop
            break;
        case 11:
//UL injector config registers :

            PRE_REG_OFFSET_IN_FPGA(0x1004c) = 0x00000002    ; //devmem 0xb001004c 32  0x00000001 ;// 0       ;//sniffer ---- bit 1:0 - downlink -- bbu --> antenna ;//;//  1 - generatore ;/// 2 - ;/// uplink  --- anntenna --> bbu
            PRE_REG_OFFSET_IN_FPGA(0x10068) = 0x00000001    ; // devmem 0xb0010068 32  0x00000001 ;//0      ;//26  - up_loopback_dl_selector        ;/// 0 - down  /  1 up
            PRE_REG_OFFSET_IN_FPGA(0x1006c) = 0x00000000     ; // devmem 0xb001006c 32  0x00000000 ;//1      ;//27  - select_generator_innowrap_uplink ;// 0 -generator / 1  innowrap uplink
            PRE_REG_OFFSET_IN_FPGA(0x10070) = 0x00000000     ; //28      ;///select_generator_oran_dl       / 0 oran dllink    /   1 -generator
            PRE_REG_OFFSET_IN_FPGA(0x10060) = 0x00000000     ; //24 - ready mux in the oRAN 0 - no loop : 1 – loop
            break;
        case 10:
//DL sniffer config registers

            PRE_REG_OFFSET_IN_FPGA(0x1004c) = 0x00000000    ; //devmem 0xb001004c 32  0x00000001 ;// 0       ;//sniffer ---- bit 1:0 - downlink -- bbu --> antenna ;//;//  1 - generatore ;/// 2 - ;/// uplink  --- anntenna --> bbu
            PRE_REG_OFFSET_IN_FPGA(0x10068) = 0x00000000    ; // devmem 0xb0010068 32  0x00000001 ;//0      ;//26  - up_loopback_dl_selector        ;/// 0 - down  /  1 up
            PRE_REG_OFFSET_IN_FPGA(0x1006c) = 0x00000001     ; // devmem 0xb001006c 32  0x00000000 ;//1      ;//27  - select_generator_innowrap_uplink ;// 0 -generator / 1  innowrap uplink
            PRE_REG_OFFSET_IN_FPGA(0x10070) = 0x00000000     ; //28      ;///select_generator_oran_dl       / 0 oran dllink    /   1 -generator
            PRE_REG_OFFSET_IN_FPGA(0x10060 ) = 0x00000000     ; //24 - ready mux in the oRAN 0 - no loop : 1 – loop
            break;
        case 9:// – UL Time domain Sampling
//UL sniffer config registers :

            PRE_REG_OFFSET_IN_FPGA(0x1004c) = 0x00000002    ; //devmem 0xb001004c 32  0x00000001 ;// 0       ;//sniffer ---- bit 1:0 - downlink -- bbu --> antenna ;//;//  1 - generatore ;/// 2 - ;/// uplink  --- anntenna --> bbu
            PRE_REG_OFFSET_IN_FPGA(0x10068) = 0x00000001    ; // devmem 0xb0010068 32  0x00000001 ;//0      ;//26  - up_loopback_dl_selector        ;/// 0 - down  /  1 up
            PRE_REG_OFFSET_IN_FPGA(0x1006c) = 0x00000001     ; // devmem 0xb001006c 32  0x00000000 ;//1      ;//27  - select_generator_innowrap_uplink ;// 0 -generator / 1  innowrap uplink
            PRE_REG_OFFSET_IN_FPGA(0x10070) = 0x00000000     ; //28      ;///select_generator_oran_dl       / 0 oran dllink    /   1 -generator
            PRE_REG_OFFSET_IN_FPGA(0x10060) = 0x00000000     ; //24 - ready mux in the oRAN 0 - no loop : 1 – loop

            break;
        default:
            return -1;

    }

    return 0;












}




//injector
int GetIQsample__DL_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                         uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr)

{

    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    iq_sample_t *iq_sample_header;
    int i;
    uint32_t tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf;
    uint32_t lo_off, loop ;
    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;
    static uint32_t *local_ddr_virtual_addr[4];
    uint8_t  *pData;
    static int flag = 0; // 1 the address of ddr is set




// typedef struct
// {
//   uint32_t	h_length;//	Header length
//   uint16_t	ant_num;//	Index of antenna
//   uint16_t	TotalSymbol;//	Total number of symbols
//   uint16_t	CurrentSymbol;//	Current number of symbol
//   uint16_t FrameToRecord;
//
// } iq_sample_t;

//    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    iq_sample_header = (iq_sample_t *)buf;

    if (bytesSock < 100)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if ((iq_sample_header->h_length + sizeof(matlab_header_t)) > bytesSock)
    {
        DEBUG_PRINT(" iq_sample_header->h_length+sizeof(matlab_header_t) %d > bytesSock %d\n", \
                    (iq_sample_header->h_length + sizeof(matlab_header_t)), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);

    //  usleep(100);



    DUPPRINT(log_fd, "iq_sample length                 	 	= 0x%4.4x \n", iq_sample_header->h_length);
    DUPPRINT(log_fd, "iq_sample Total Index of antenna          = 0x%4.4x \n", iq_sample_header->total_ant_num);
    DUPPRINT(log_fd, "iq_sample Index of antenna              	= 0x%4.4x \n", iq_sample_header->ant_num);
    DUPPRINT(log_fd, "iq_sample Total number of symbols         = 0x%4.4x \n", iq_sample_header->TotalSymbol);
    DUPPRINT(log_fd, "iq_sample Current number of symbol        = 0x%4.4x \n", iq_sample_header->CurrentSymbol);
    DUPPRINT(log_fd, "iq_sample Frame To Record       		= 0x%4.4x \n", iq_sample_header->FrameToRecord);
    if ((iq_sample_header->CurrentSymbol < 0 ) || (iq_sample_header->TotalSymbol < 0))
    {
        return -10;
    }
    if ((iq_sample_header->ant_num < 0 ) || (iq_sample_header->ant_num > 3))
    {
        DUPPRINT(log_fd, "Bad iq_sample Index of antenna              	= 0x%4.4x \n", iq_sample_header->ant_num);
        return -11;

    }

    if ((iq_sample_header->CurrentSymbol == 0) && (iq_sample_header->ant_num == 0))
    {

        local_ddr_virtual_addr[0] = ddr_virtual_addr;
        local_ddr_virtual_addr[1] = &ddr_virtual_addr[(0x130000 ) / 4];
        local_ddr_virtual_addr[2] = &ddr_virtual_addr[(0x260000 ) / 4];
        local_ddr_virtual_addr[3] = &ddr_virtual_addr[(0x390000 ) / 4];
        DUPPRINT(log_fd, "start DDR at the begining address 0x%x , 0x%x , 0x%x , 0x%x \n", local_ddr_virtual_addr[0], local_ddr_virtual_addr[1], local_ddr_virtual_addr[2], local_ddr_virtual_addr[3]);
        PRE_REG_OFFSET_IN_FPGA(0x30014) = 0;
        flag = 1;
        DUPPRINT(log_fd, "Disabled componnet \n");

    }
    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));
    buf += sizeof(iq_sample_t);
    pData = buf;
    DEBUG_PRINT("iq_sample_t %d\n", sizeof(iq_sample_t));








    SetFPGA_Regs(reg_virtual_addr);
    setFPGA_muxRegs(reg_virtual_addr, (matlab_header->type));


    PRE_REG_OFFSET_IN_FPGA(0x30018) = 1;

    PRE_REG_OFFSET_IN_FPGA(0x3001c) = 1;//(iq_sample_header->FrameToRecord % 0x3ff);

    PRE_REG_OFFSET_IN_FPGA(0x30020) = 0x6f000000;
    PRE_REG_OFFSET_IN_FPGA(0x30024) = 0x6f130000;
    PRE_REG_OFFSET_IN_FPGA(0x30028) = 0x6f260000;
    PRE_REG_OFFSET_IN_FPGA(0x3002c) = 0x6f390000;

    PRE_REG_OFFSET_IN_FPGA(0x30038) = 0x6f000000 + DDR_OFSET_FOR_UL;
    PRE_REG_OFFSET_IN_FPGA(0x3003c) = 0x6f130000 + DDR_OFSET_FOR_UL;
    PRE_REG_OFFSET_IN_FPGA(0x30040) = 0x6f260000 + DDR_OFSET_FOR_UL;
    PRE_REG_OFFSET_IN_FPGA(0x30044) = 0x6f390000 + DDR_OFSET_FOR_UL;


    PRE_REG_OFFSET_IN_FPGA(0x30030) = 0x4b000;
    PRE_REG_OFFSET_IN_FPGA(0x30034) = 0x400;
    PRE_REG_OFFSET_IN_FPGA(0x30048) = 0x00000001; //18- FPGA_frame_to_record
    PRE_REG_OFFSET_IN_FPGA(0x3004C) = 0x0000012C; //19- chunck_number_from_pc
    PRE_REG_OFFSET_IN_FPGA(0x30050) = 0x00000000; //20- write_cycle_number_from_pc
    PRE_REG_OFFSET_IN_FPGA(0x30054) = 0x00000002; //21- write_cycle_selector
    PRE_REG_OFFSET_IN_FPGA(0x30058) = 0x00000000; //22- number_of_reading_selector
    PRE_REG_OFFSET_IN_FPGA(0x3005C) = 0x00000000; //23- load_number_of_reading_from_pc
    PRE_REG_OFFSET_IN_FPGA(0x30060) = 0x00000000; //24- pw_dl_gpio_header_selector
    PRE_REG_OFFSET_IN_FPGA(0x30064) = 0x00000000; //25- pw_dl_gpio_logic_selector
    PRE_REG_OFFSET_IN_FPGA(0x30068) = 0x00000000; //26- source_for_ul_design_selector



    loop = (matlab_header->length - iq_sample_header->h_length - sizeof(matlab_header_t));
    DUPPRINT(log_fd, "continue DDR at the begining address 0x%x\n", (uint32_t)local_ddr_virtual_addr[iq_sample_header->ant_num]);

    for (i = 0; i < loop; i += 4)
    {
        tmp1 = pData[0];
        tmp2 = pData[1];
        tmp1 |= (tmp2 << 8);
        tmp2 = pData[2];
        tmp1 |= (tmp2 << 16);
        tmp2 = pData[3];
        tmp1 |= (tmp2 << 24);
        if (tmp1 == 0)
            DUPPRINT(log_fd, "At addr 0x%x =0 \n", (uint32_t)local_ddr_virtual_addr[iq_sample_header->ant_num]);
        *(local_ddr_virtual_addr[iq_sample_header->ant_num]) = tmp1;
//         printf("pData= 0x%x  %x %x %x \n", pData[0], pData[1], pData[2], pData[3]);
//         printf("tmp1= 0x%x %x %x \n", tmp1, (uint32_t) * (local_ddr_virtual_addr[iq_sample_header->ant_num]), (uint32_t)local_ddr_virtual_addr[iq_sample_header->ant_num]);

        local_ddr_virtual_addr[iq_sample_header->ant_num]++;
        pData += 4;
    }
    DUPPRINT(log_fd, "after loop of %d DDR at the begining address 0x%x\n", loop, local_ddr_virtual_addr[iq_sample_header->ant_num]);

    if (((iq_sample_header->TotalSymbol - 1) == iq_sample_header->CurrentSymbol) &&
            ((iq_sample_header->total_ant_num - 1 ) == iq_sample_header->ant_num ))
    {
        PRE_REG_OFFSET_IN_FPGA(0x30014) = 0x1ff;
        flag = 0;
    }
    *offset_to_write = 0x10;

    *num_of_regs = -10;// send ack do not send to fifo
    *num_of_data = 0;// send ack do not send to fifo


    return 0;


}


//sniffer
int GetIQsample__UL_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                         uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr, uint32_t *ddr_virtual_addr)

{
    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    iq_sample_ul_t *iq_sample_ul_header;
    int i;
    uint32_t tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf;
    uint32_t lo_off, loop ;
    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;
    static uint32_t local_ddr_virtual_addr_offset[4];
    uint8_t  *pData;
    static int flag = 0; // 1 the address of ddr is set




// typedef struct
// {
//   uint32_t	h_length;//	Header length
//   uint16_t	ant_num;//	Index of antenna
//   uint16_t	TotalSymbol;//	Total number of symbols
//   uint16_t	CurrentSymbol;//	Current number of symbol
//   uint16_t FrameToRecord;
//
// } iq_sample_t;

//    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    iq_sample_ul_header = (iq_sample_ul_t *)buf;

    if (bytesSock < 30)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if ((iq_sample_ul_header->h_length + sizeof(matlab_header_t)) > bytesSock)
    {
        DEBUG_PRINT(" iq_sample_ul_header->h_length+sizeof(matlab_header_t) %d > bytesSock %d\n", \
                    (iq_sample_ul_header->h_length + sizeof(matlab_header_t)), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);

    //  usleep(100);



    DUPPRINT(log_fd, "iq_sampleUL length                 	 	= 0x%4.4x \n", iq_sample_ul_header->h_length);
    DUPPRINT(log_fd, "iq_sampleUL Total Index of antenna          = 0x%4.4x \n", iq_sample_ul_header->total_ant_num);
    DUPPRINT(log_fd, "iq_sampleUL Index of antenna              	= 0x%4.4x \n", iq_sample_ul_header->ant_num);
    DUPPRINT(log_fd, "iq_sampleUL Total number of symbols         = 0x%4.4x \n", iq_sample_ul_header->TotalSymbol);
    DUPPRINT(log_fd, "iq_sampleUL Current number of symbol        = 0x%4.4x \n", iq_sample_ul_header->CurrentSymbol);
    DUPPRINT(log_fd, "iq_sampleUL Frame To Record       		= 0x%4.4x \n", iq_sample_ul_header->FrameToRecord);
    DUPPRINT(log_fd, "iq_sampleUL SymbolLength      		= 0x%4.4x \n", iq_sample_ul_header->SymbolLength);
    if ((iq_sample_ul_header->CurrentSymbol < 0 ) || (iq_sample_ul_header->TotalSymbol < 0))
    {
        return -10;
    }
    if ((iq_sample_ul_header->ant_num < 0 ) || (iq_sample_ul_header->ant_num > 3))
    {
        DUPPRINT(log_fd, "Bad iq_sample Index of antenna              	= 0x%4.4x \n", iq_sample_ul_header->ant_num);
        return -11;

    }


    if ((iq_sample_ul_header->CurrentSymbol == 0) && (iq_sample_ul_header->ant_num == 0))
    {

    }
    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));
    buf += sizeof(iq_sample_ul_t);
    pData = buf;
    DEBUG_PRINT("iq_sample_ul_t %d\n", sizeof(iq_sample_ul_t));



    if (iq_sample_ul_header->FrameToRecord < 0xfff0)
    {
        SetFPGA_Regs(reg_virtual_addr);
        setFPGA_muxRegs(reg_virtual_addr, matlab_header->type);

        PRE_REG_OFFSET_IN_FPGA(0x14) = 0;
        //so some init
        local_ddr_virtual_addr_offset[0] = DDR_OFSET_FOR_UL;
        local_ddr_virtual_addr_offset[1] = 0x130000 + DDR_OFSET_FOR_UL;
        local_ddr_virtual_addr_offset[2] = 0x260000 + DDR_OFSET_FOR_UL;
        local_ddr_virtual_addr_offset[3] = 0x390000 + DDR_OFSET_FOR_UL;





        DUPPRINT(log_fd, "start DDR at the begining address 0x%x , 0x%x , 0x%x , 0x%x \n", local_ddr_virtual_addr_offset[0], local_ddr_virtual_addr_offset[1], local_ddr_virtual_addr_offset[2], local_ddr_virtual_addr_offset[3]);
        PRE_REG_OFFSET_IN_FPGA(0x40030) = 0x4b000;
        PRE_REG_OFFSET_IN_FPGA(0x40034) = 0x400;
        PRE_REG_OFFSET_IN_FPGA(0x4004C) = 0x4b000 / 0x400;

        flag = 1;
        DUPPRINT(log_fd, "Disabled componnet \n");
        PRE_REG_OFFSET_IN_FPGA(0x40018) = 1;
        PRE_REG_OFFSET_IN_FPGA(0x4001C) = 1;

        PRE_REG_OFFSET_IN_FPGA(0x40048) = 1;//(iq_sample_ul_header->FrameToRecord % 0x3ff);

        PRE_REG_OFFSET_IN_FPGA(0x40038) = 0x6f000000 + DDR_OFSET_FOR_UL;
        PRE_REG_OFFSET_IN_FPGA(0x4003c) = 0x6f130000 + DDR_OFSET_FOR_UL;
        PRE_REG_OFFSET_IN_FPGA(0x40040) = 0x6f260000 + DDR_OFSET_FOR_UL;
        PRE_REG_OFFSET_IN_FPGA(0x40044) = 0x6f390000 + DDR_OFSET_FOR_UL;

        PRE_REG_OFFSET_IN_FPGA(0x40020) = 0x6f000000;
        PRE_REG_OFFSET_IN_FPGA(0x40024) = 0x6f130000;
        PRE_REG_OFFSET_IN_FPGA(0x40028) = 0x6f260000;
        PRE_REG_OFFSET_IN_FPGA(0x4002c) = 0x6f390000;

        PRE_REG_OFFSET_IN_FPGA(0x40050) = 0x0;
        PRE_REG_OFFSET_IN_FPGA(0x40054) = 0x0;
        PRE_REG_OFFSET_IN_FPGA(0x40058) = 0x0;
        PRE_REG_OFFSET_IN_FPGA(0x4005c) = 0;
        PRE_REG_OFFSET_IN_FPGA(0x40060) = 0;
        PRE_REG_OFFSET_IN_FPGA(0x40064) = 0;
        PRE_REG_OFFSET_IN_FPGA(0x40068) = 0;



        {
            extern uint32_t *virt_addr_reserved_memory;
            uint32_t *pp = virt_addr_reserved_memory + DDR_OFSET_FOR_UL / 4;
            uint32_t a, b;
            *pp = 0x01010101;
            for (a = 1; a <= 200; a++)
            {
                b = *pp;
                pp++;
                *pp = b + 0x01010101;
                *pp = 0;
            }
        }



        PRE_REG_OFFSET_IN_FPGA(0x40014) = (0x1);
        // sleep(15);
//        PRE_REG_OFFSET_IN_FPGA(0x40014) = ((0xf<<5)|0x1);
        PRE_REG_OFFSET_IN_FPGA(0x40014) = 0x1ff;
        // set the HW to recored UL
        *num_of_regs = -10;//send ack ;
        *num_of_data = -1;// send ack do not send to FIFO


        return 0;

    }

    if (iq_sample_ul_header->FrameToRecord == 0xfffe)
    {
        static uint16_t busy = 0;

        if (busy == 0)
            *num_of_regs = -21;//send not ready;
        else
            *num_of_regs = -10;//	not busy
        return 0;
    }

    // copy to matlab
    //  loop = (matlab_header->length - iq_sample_ul_header->h_length - sizeof(matlab_header_t));
    DUPPRINT(log_fd, "continue DDR at the beginning address 0x%x\n", (uint32_t)local_ddr_virtual_addr_offset[iq_sample_ul_header->ant_num]);


    if (((iq_sample_ul_header->TotalSymbol - 1) == iq_sample_ul_header->CurrentSymbol) &&
            ((iq_sample_ul_header->total_ant_num - 1 ) == iq_sample_ul_header->ant_num ))
    {
        flag = 0;
    }
    PRE_REG_OFFSET_IN_FPGA(0x40014) = 0;
    *offset_to_write = 0x10;
    reg_buf[0] = local_ddr_virtual_addr_offset[iq_sample_ul_header->ant_num]; // ddr location to send
    reg_buf[1] = iq_sample_ul_header->SymbolLength; // size to be send
    {
        static int busssy = 0;
        if ((!busssy) && (flag == 1))
        {
            local_ddr_virtual_addr_offset[iq_sample_ul_header->ant_num] += iq_sample_ul_header->SymbolLength ;
            *num_of_regs = -22;//send  ready with data;
        }
        else
            *num_of_regs = -23;//send  not ready with data;
    }

    *num_of_data = -1;// send ack do not send to fifo


    return 0;


}


int GetPrecodingMatrix_regs(uint8_t *buf, int bytesSock, uint8_t **p_data_buf, int *num_of_data, uint32_t *top_reg_buf, uint32_t *num_of_regs,
                            uint32_t *offset_to_write, int log_fd, uint32_t *reg_virtual_addr)
{
    dut_header_t dut_header;
    matlab_header_t *matlab_header = (matlab_header_t *)buf;
    precoding_matrix_t *precoding_matrix_header;
    int i;
    int tmp1, tmp2, tmp3;
    uint32_t *reg_buf = top_reg_buf;
    uint32_t lo_off ;
    volatile uint32_t *preg_virtual_addr = reg_virtual_addr;
    interpolation_t	*InterpArray;

    uint16_t  *pData;
    fpga_data_rb_t fpga_data_rb;
    int iant, ilayer;
    int fpga_addr;
    uint32_t fpga_data;
    int loop, endloop;
    int16_t symbol;
    interpolation_t  tmp_data;





    DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)p_data_buf, (uint32_t)*p_data_buf);
    *num_of_regs = 0;
    buf += sizeof(matlab_header_t);
    precoding_matrix_header = (precoding_matrix_t *)buf;

    if (bytesSock < 100)
        return -1;

    if (matlab_header->length > bytesSock)
    {
        DEBUG_PRINT("matlab_header->length %d > bytesSock %d\n", matlab_header->length, bytesSock);
        return -2;
    }
    if ((precoding_matrix_header->h_length + sizeof(matlab_header_t)) > bytesSock)
    {
        DEBUG_PRINT(" precoding_matrix_header->h_length+sizeof(matlab_header_t) %d > bytesSock %d\n", \
                    (precoding_matrix_header->h_length + sizeof(matlab_header_t)), bytesSock);
        return -3;
    }



    DUPPRINT(log_fd, "matlab length = %4.4x \n", matlab_header->length);
    DUPPRINT(log_fd, "matlab Sequence number of the slot = %4.4x \n", matlab_header->slot_num);
    DUPPRINT(log_fd, "matlab Sequence number of the symbol = %4.4x \n", matlab_header->sym_num);
    DUPPRINT(log_fd, "matlab Sequence number of the layer = %4.4x \n", matlab_header->layer_num);

    // usleep(100);



    DUPPRINT(log_fd, "pre matrix length                 	 = 0x%4.4x \n", precoding_matrix_header->h_length);
    DUPPRINT(log_fd, "pre matrix first SFN              	 = 0x%4.4x \n", precoding_matrix_header->startSFN);
    DUPPRINT(log_fd, "pre matrix first Symbol          		 = 0x%4.4x \n", precoding_matrix_header->startSlot);
    DUPPRINT(log_fd, "pre matrix Number of layers        	 = 0x%4.4x \n", precoding_matrix_header->nLayers);
    DUPPRINT(log_fd, "pre matrix Number of Antennas          	 = 0x%4.4x \n", precoding_matrix_header->nAnt);
    DUPPRINT(log_fd, "pre matrix Number of SCSin               	 = 0x%4.4x \n", precoding_matrix_header->nSCSin);
    DUPPRINT(log_fd, "pre matrix Number of SCSout             	 = 0x%4.4x \n", precoding_matrix_header->nSCSout);
    DUPPRINT(log_fd, "pre matrix Number of RBs                	 = 0x%4.4x \n", precoding_matrix_header->nRbs);
    DUPPRINT(log_fd, "pre matrix Index of antenna to monitor	 = 0x%4.4x \n", precoding_matrix_header->iAnt);
    DUPPRINT(log_fd, "pre matrix Index of layer to monitor 	 = 0x%4.4x \n", precoding_matrix_header->iLayer);


    dut_header.control = 0;
    dut_header.status = 0;
    dut_header.Sequence_number_of_slot = matlab_header->slot_num;
    dut_header.Sequence_number_of_symbol = matlab_header->sym_num;
    dut_header.Sequence_number_of_layer = matlab_header->layer_num;
    dut_header.dut_error = 0;
    dut_header.dut_error_status = 0;

    memcpy(reg_buf, &dut_header, sizeof(dut_header_t));
    buf += sizeof(precoding_matrix_t);
    DEBUG_PRINT("precoding_matrix_t %d\n", sizeof(precoding_matrix_t));
    InterpArray = buf;
    // pInterp = buf;
    pData = (uint16_t *)(buf + sizeof(interpolation_t) * precoding_matrix_header->nRbs);
    DEBUG_PRINT("pData	%16.16x\n", pData );
    DEBUG_PRINT("buf 	%16.16x\n", buf );
    DEBUG_PRINT("interpolation_t %16.16x\n", sizeof(interpolation_t) );
    DEBUG_PRINT("offset %16.16x\n", sizeof(interpolation_t) * precoding_matrix_header->nRbs);


    //set num of entris = precoding_matrix_header->nScs;
    // set num of Antennas =precoding_matrix_header->nAnt*2
    // set num of layesrs = precoding_matrix_header->nLayers*2;
    // set num of RBs = precoding_matrix_header->nRbs
    //  precoding_matrix_header->nSCSin = 28;
    PRE_REG_OFFSET_IN_FPGA(0x5c) = 2 * precoding_matrix_header->nSCSin ;



    for (i = 0; i < precoding_matrix_header->nRbs; i++)
    {
        //InterpArray = (interpolation_t *) &pInterp[i];
        tmp_data.data = InterpArray[i].data;

//         tmp_data.data = 0;
//         tmp_data.bits_t.cont = 0;
//         if (i <= 3)
//             tmp_data.bits_t.interpolation = 2;
//         else
//             tmp_data.bits_t.interpolation = 12;
//         if ((i == 3) || (i == 7))
//             tmp_data.bits_t.cont = 0;





        DEBUG_PRINT("RB num %d data 0x%4.4x inter 0x%4.4x ,rep 0x%4.4x,cont 0x%4.4x,ssb 0x%4.4x \n", i,  tmp_data.data, \
                    tmp_data.bits_t.interpolation, tmp_data.bits_t.rep, tmp_data.bits_t.cont, tmp_data.bits_t.ssb );

        fpga_data_rb.bits.rept = tmp_data.bits_t.rep;
        fpga_data_rb.bits.cont = tmp_data.bits_t.cont;
        fpga_data_rb.bits.interp = 0;
        switch (tmp_data.bits_t.interpolation)
        {
            case 1:
                fpga_data_rb.bits.interp = 0;
                break;
            case 2:
                fpga_data_rb.bits.interp = 1;
                break;
            case 4:
                fpga_data_rb.bits.interp = 2;
                break;
            case 8:
                fpga_data_rb.bits.interp = 3;
                break;
            case 12:
                fpga_data_rb.bits.interp = 4;
                break;
            default:
                DEBUG_PRINT("interpolation is wrong value %d\n", tmp_data.bits_t.interpolation);
        }



        fpga_addr = i;
        DEBUG_PRINT("RB num %d interpolation 0x%4.4x,rept 0x%x,cont 0x%x,\n", i,  fpga_data_rb.bits.interp \
                    , fpga_data_rb.bits.rept, fpga_data_rb.bits.cont);

        PRE_REG_OFFSET_IN_FPGA(0x78) = fpga_addr;
        PRE_REG_OFFSET_IN_FPGA(0x7c) = fpga_data_rb.data;
        PRE_REG_OFFSET_IN_FPGA(0x80) = 0;
        //usleep(1);//by pass optomisation
        PRE_REG_OFFSET_IN_FPGA(0x80) = 1;
        //usleep(1);//by pass optomisation
        // PRE_REG_OFFSET_IN_FPGA(0x80) = 0;

    }

//     precoding_matrix_header->iLayer = 1; //	25	 li_mon_layer_to_capture_reg			iLayer(0..15)
//     precoding_matrix_header->iAnt = 1; //	26	li_mon_antenna_to_capture_reg			iAnt (0..15)


    {
// //         uint16_t *ffff = pData;
        for (symbol = 0; symbol < precoding_matrix_header->nSCSin ; symbol++)
        {

            //  DEBUG_PRINT(" symbol %3.3d,iant %3.3d,ilayer %3.3d,loop %3.3d \n", symbol, iant, ilayer, loop);
            //  for (loop = 0; loop < 2; loop++)
            {

                for (iant = 0; iant < precoding_matrix_header->nAnt ; iant++)
                {
                    for (ilayer = 0; ilayer < precoding_matrix_header->nLayers ; ilayer++)
                    {


                        fpga_data = ((*pData) & 0xfff);
                        pData++;
                        fpga_data |= (((*pData) & 0xfff) << 12);
                        pData++;
// DATA_0              = 24'hFA0FA0 ; // minus 96
// DATA_1              = 24'h060060 ; // plus 96
// DATA_2              = 24'h100200 ;
// DATA_3              = 24'h110210 ;
// DATA_4              = 24'h120220 ;
// DATA_5              = 24'h130230 ;
// DATA_6              = 24'h140240 ;
// DATA_7              = 24'h150250 ;
// DATA_8              = 24'h820820 ;
// DATA_9              = 24'h020020 ;
// DATA_10             = 24'h840840 ;
// DATA_11             = 24'h040040 ;
//
// DATA12 till DATA_23 is constant 24’hFFF_FFF
                        /*
                        			if (symbol >= 2)
                        			fpga_data =0x100200 + symbol*0x010010;
                        			if (symbol == 8)
                        			fpga_data =0x820820 ;
                        			if (symbol == 9)
                        			fpga_data =0x020020 ;
                        			if (symbol == 10)
                        			fpga_data =0x840840 ;
                        			if (symbol == 11)
                        			fpga_data =0x040040 ;

                        			if (symbol == 0)
                                                    fpga_data = 0xFA0FA0 ;
                                                if (symbol == 1)
                                                    fpga_data = 0x060060 ;*/

                        // DEBUG_PRINT(" symbol %d ,data 0x%8.8x\n", symbol, fpga_data);
                        fpga_addr = symbol + iant % 2 * precoding_matrix_header->nSCSin;
                        PRE_REG_OFFSET_IN_FPGA(0x60) = fpga_addr;
                        PRE_REG_OFFSET_IN_FPGA(0x64) = fpga_data;
                        PRE_REG_OFFSET_IN_FPGA(0x68) = iant / 2;
                        PRE_REG_OFFSET_IN_FPGA(0x6c) = ilayer;
                        //DEBUG_PRINT(" symbol %3.3d,iant %3.3d,ilayer %3.3d,loop %3.3d addr 0x%3.3x ,data 0x%3.3x\n", symbol, iant, ilayer, loop, fpga_addr,fpga_data);

                        PRE_REG_OFFSET_IN_FPGA(0x70) = 0;
                        //usleep(1);//by pass optomisation
                        PRE_REG_OFFSET_IN_FPGA(0x70) = 1;
                        //usleep(1);//by pass optomisation
                        // PRE_REG_OFFSET_IN_FPGA(0x70) = 0;



                    }
                }
            }
        }
        PRE_REG_OFFSET_IN_FPGA(0x74) = precoding_matrix_header->nRbs ;
    }




// afer all init and go set the monitor
// disable monitor
// precoding_matrix_header->iAnt
// precoding_matrix_header->iLayer
// tring the monitor





// reg_buf += sizeof(dut_header_t) / sizeof(uint32_t);


    lo_off = 0x10;//sizeof(dut_header_t) / sizeof(uint32_t);

    DEBUG_PRINT("virt_addr address %8.8x\n", (uint32_t)preg_virtual_addr);
    PRE_REG_OFFSET_IN_FPGA(0x84) = precoding_matrix_header->nSCSout ; //	21	 li_num_of_scs_cfg_reg			nSCS
    PRE_REG_OFFSET_IN_FPGA(0x88) = precoding_matrix_header->nLayers; //	22	 li_num_of_layers_cfg_reg			nLayers+1 ( 1..16 and not 0-15 )
    if ((!precoding_matrix_header->nAnt) || (precoding_matrix_header->nAnt % 2))
    {
        DEBUG_PRINT("Error Antana number %d\n", precoding_matrix_header->nAnt);
        return -6;
    }
    PRE_REG_OFFSET_IN_FPGA(0x8C) = precoding_matrix_header->nAnt	; //	23	 li_num_of_antennas_cfg_reg			nAnt+1 ( 2,4,…16 and not 0-15 )
//     PRE_REG_OFFSET_IN_FPGA(0x94) = precoding_matrix_header->iLayer; //	25	 li_mon_layer_to_capture_reg			iLayer(0..15)
//     PRE_REG_OFFSET_IN_FPGA(0x98) = precoding_matrix_header->iAnt; //	26	li_mon_antenna_to_capture_reg			iAnt (0..15)



    PRE_REG_OFFSET_IN_FPGA(0x90) = 0;
    PRE_REG_OFFSET_IN_FPGA(0x94) = precoding_matrix_header->iLayer; //	25	 li_mon_layer_to_capture_reg			iLayer(0..15)
    PRE_REG_OFFSET_IN_FPGA(0x98) = precoding_matrix_header->iAnt; //	26	li_mon_antenna_to_capture_reg			iAnt (0..15)
    PRE_REG_OFFSET_IN_FPGA(0x90) = 1;


//PRE_REG_OFFSET(0x90)=//	24	 li_mon_capture_cfg_reg			make pulse  ( 0 to 1 to 0)





    *offset_to_write = 0x10;

    *num_of_regs = -1;//((uint32_t) (&reg_buf[0x26 - lo_off] - top_reg_buf)) + 1 ;
    *num_of_data = 0;// no need to write to fifo

    *p_data_buf = buf + precoding_matrix_header->h_length;



    return 0;


}

