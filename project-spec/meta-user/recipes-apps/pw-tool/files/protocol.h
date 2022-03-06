




#pragma once 

int CheckFpgaStatus(long *virt_addr);

int ProtocolCheck (uint8_t *buf, int bytesSock, int *type, uint32_t fpga_id,int *data_length);

int GetGegister(int fifoID,uint8_t *buf,int bytesSock,uint8_t **p_data_buf,int *num_of_data,uint32_t *reg_buf,uint32_t *num_of_regs,\
		uint32_t *offset_to_write,int log_fd,uint32_t *reg_virtual_addr, uint32_t *virt_addr_reserved_memory);

int AddHeaderToData(int FifoID, uint8_t *buf, uint32_t total_size, uint32_t *reg_address, uint32_t fpga_id,int32_t over_symbol);
