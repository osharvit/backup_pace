/**
 * Copyright (C) 2021 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/* DMA Proxy Test Application
 *
 * This application is intended to be used with the DMA Proxy device driver. It provides
 * an example application showing how to use the device driver to do user space DMA
 * operations.
 *
 * The driver allocates coherent memory which is non-cached in a s/w coherent system
 * or cached in a h/w coherent system.
 *
 * Transmit and receive buffers in that memory are mapped to user space such that the
 * application can send and receive data using DMA channels (transmit and receive).
 *
 * It has been tested with an AXI DMA system with transmit looped back to receive.
 * Since the AXI DMA transmit is a stream without any buffering it is throttled until
 * the receive channel is running.
 *
 * Build information: The pthread library is required for linking. Compiler optimization
 * makes a very big difference in performance with -O3 being good performance and
 * -O0 being very low performance.
 *
 * More complete documentation is contained in the device driver (dma-proxy.c).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sched.h>
#include <semaphore.h>
#include <errno.h>
#include "dma-proxy.h"
#include "paceipc/ipclib.h"
#include "pacecli/cli_ipc_ctrl_inf.h"

#define MAX_SINGLE_BW	37  //Gbit/s
#define MIN_AVAILABLE_FREE_SPACE (1024000000 / 8)
#define NUM_CHANNELS	2
#define DEFAULT_CHUNK_SIZE	128

static const char *TX_DEV_NAME[NUM_CHANNELS] = {"/dev/dma_proxy_tx", "/dev/dma_proxy_tx2"};
static const char *RX_DEV_NAME[NUM_CHANNELS] = {"/dev/dma_proxy_rx", "/dev/dma_proxy_rx2"};

typedef struct _x_context {
	int x_proxy_fd;
	int x_size;
	pthread_t x_tid;
	int LAST_BUFFER_ID;
	FILE *inpstream;
    FILE *outstream;
	int interleaved_fd;
	int session_id;
	int num_transfers;
	int stop_time_ms;
	int stop_size_kb;
	int need_2_store_file;
	sem_t endless_hw_sem;
	void *(*start_routine) (void *);
	struct channel_buffer *x_proxy_buffer_p;
} x_context;

// TX thread context 
static x_context tx[NUM_CHANNELS];
// Rx thread context
static x_context rx[NUM_CHANNELS];

static int rx_size = 0;
static volatile int tx_wait =0;
static volatile int stop = 0;
static volatile int rx_counter = 0;
static int LAST_BUFFER_ID = 0;
void *h_ipc_client = NULL;
uint64_t start_time, end_time;
static int num_cycles = 1;
static int chunk_size = DEFAULT_CHUNK_SIZE;
static int single_injection = 1;
char* inject_record_file;
char* dst_record_file;
static int verbose = 0;
/*******************************************************************************************************************/
/* Handle a control C or kill, maybe the actual signal number coming in has to be more filtered?
 * The stop should cause a graceful shutdown of all the transfers so that the application can
 * be started again afterwards.
 */
void sigint(int a)
{
    int i;
    printf("Received STOP, exiting\n");
	stop = 1;
	for (i = 0; i < NUM_CHANNELS; i++) {
		if (tx[i].x_tid > 0)
			sem_post(&tx[i].endless_hw_sem);
	}
}

/*******************************************************************************************************************/
/* Get the clock time in usecs to allow performance testing
 */
static uint64_t get_posix_clock_time_usec ()
{
    struct timespec ts;

    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
    else
        return 0;
}

static int check_available_space(char *exec_file) {
  struct statvfs st;
  statvfs(exec_file, &st);
  unsigned long free_space = st.f_bfree * st.f_frsize;
  if (free_space < MIN_AVAILABLE_FREE_SPACE) {
    printf("Not enough space on disk: %lu bytes; required %d bytes\n",free_space, MIN_AVAILABLE_FREE_SPACE);
    return 0;
  } 
  return 1;
}

static int open_record_file(const char *filename, int channel_index) {
 // check disk size for 50 MB free space
 char long_name[1024] = {'\0'};
 snprintf(long_name, 1024, "%s_%d", filename, channel_index);
 rx[channel_index].outstream = fopen(long_name, "wb");
 if (!rx[channel_index].outstream) {
    printf("Error open file: %s; error: %d\n", long_name, errno);
    return -1;
 }
 return 0; 
}

static int open_interleaved_data_file(x_context *tx, const char *filename, size_t chunk_size) {
    off_t cur_offset = tx->num_transfers * chunk_size;
    ssize_t num_read;
    int buffer_id = 0;
	tx->x_proxy_buffer_p[buffer_id].length = 0;
	tx->interleaved_fd = open(filename, O_RDONLY);
	if (tx->interleaved_fd == -1) {
		printf("Failed [%d] to open %s, exiting\n", tx->session_id, filename);
		return -1;
    }

    for (; (stop == 0) ; ) {
        num_read = pread(tx->interleaved_fd, tx->x_proxy_buffer_p[buffer_id].buffer, chunk_size, cur_offset);
        printf("read [%d] %ld bytes from offset %ld\n", tx->session_id, num_read, cur_offset);
        if (num_read <= 0)
            break;
        cur_offset += 2*chunk_size;
        tx->x_proxy_buffer_p[buffer_id].length += num_read;
	}
	printf("read [%d] %ld bytes from file\n", tx->session_id, tx->x_proxy_buffer_p[buffer_id].length);
    return 0;
}


static int fill_inject_data(x_context* tx, const char* filename) {
  long count; 
  int buffer_id;
  tx->x_size = 0;
  tx->inpstream = fopen(filename, "rb");
  if (!tx->inpstream) {
    printf("Error open file: %s; error: %d\n", filename, errno);
    return -1;
  }
  for (buffer_id = 0; buffer_id < TX_BUFFER_COUNT; buffer_id += BUFFER_INCREMENT) {
    count = fread(tx->x_proxy_buffer_p[buffer_id].buffer, sizeof(char), BUFFER_SIZE, tx->inpstream);
    if (count < 0) {
      printf("read failed; error = %d\n", errno);
      fclose(tx->inpstream);
      return -1;
    }
    //printf("read %ld bytes into buffer_id = %d\n", count, buffer_id);
    tx->x_size += count;
    tx->x_proxy_buffer_p[buffer_id].length = count;
    if (count < BUFFER_SIZE) {
      // read last chunk from file
      buffer_id += 1;
      break;
    }
  }
  //printf("Ready for transfer = %d\n", test_size);
 
  return buffer_id;
}

static int store_record_file(unsigned char *buffer, int store, FILE *outstream) {
    int  count = fwrite(buffer, sizeof(char), store, outstream);
    if (count < 0) {
      printf("write failed; error = %d\n", errno);
      fclose(outstream);
      return -1;
    }
    //printf("wrote %d bytes into file\n", count);
    return count;
}

static int scan_ints(char *p) {
     int c[4], i;
     int num = 0;
     while (*p != ':')
       p--;
     p++;
     while (isblank(*(p)))
       p++;
     sscanf(p, "%d          %d          %d          %d",&(c[0]),&(c[1]),&(c[2]),&(c[3]));
     for (i = 0; i < 4; i++) {
       if (c[i] > 0) {
         num = c[i];
         break;
       }
     }     
     return num;
}


static int read_interrupts(int is_inject) {
        int source, n;
        char *p, *p2;
        int c1,c2,c3,c4;
        unsigned char buffer[9000];
        int ret = -1;
        source = open("/proc/interrupts", O_RDONLY);
        n=read(source, buffer, 9000);
        if (n <= strlen("xilinx-dma-controller")) {
          printf("Failed to read interrupts\n");
          return -1;
        }
        buffer[n] = 0;
        if ((p = strstr(buffer, "xilinx-dma-controller")) != NULL) {
          if (is_inject == 0) {
            // read Rx interrupts
            p2 = p + strlen("xilinx-dma-controller");
            if ((p = strstr(p2, "xilinx-dma-controller")) != NULL) {
              ret = scan_ints(p);
	    }
          } else {
	    ret = scan_ints(p);
	  }
        }

        close(source);
        return ret;    
}

static int open_ipc(int argc, char** argv) {
    void *ipc_cfg_client;
    int rc;
    int major = 0, minor = 0;
/*
    for (int i = 0; i < argc; i++)
    {
        printf("  param[%d] == %s\n", i, argv[i]);
    }
    printf("\n");
*/
    const void* pinfo = ipc_scan_server_info(argc, argv);
    if (pinfo != NULL)
    {
        printf("ipc-cli-ip: %s\n", (const char*)ipc_get_server_param(pinfo, IPC_SERV_INFO_IP));
        printf("ipc-cli-port: %d\n", *(unsigned int*)ipc_get_server_param(pinfo, IPC_SERV_INFO_PORT));
        ipc_destroy_server_info(pinfo);
    }
    ipc_cfg_client = ipc_create_cfg(NULL);
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_PORT, IPC_CFG_VAL(50007));
    ipc_set_cfg(ipc_cfg_client, IPC_CFG_IP, IPC_CFG_VAL("127.0.0.1"));
    rc = ipc_open(ipc_cfg_client, &h_ipc_client);
    printf("cli ipc_open rc:%d\n", rc);
    ipc_destroy_cfg(ipc_cfg_client);

    if (rc < 0)
        return rc;

    IPC_CLI_DEF_hLIB(h_ipc_client);
/*
    void* ret;
    printf("ipc_open rc:%d\n", rc);
    ret = ipc_cli_api_command("print Hello JOPPA CLI;\n");
    if (ret)
        printf("CLI-API - rc:%d\n", *(int*)ipc_return_get_data(ret));
    ipc_call_free_return(ret);
*/    return 0;
}

static int send2log(char *buff, int buf_size) {
    int err_code;
    return 0;
    void* ret = ipc_call(h_ipc_client, 1, 11, 22, 33, buff);
    if (ret != NULL)
    {
        err_code = ipc_return_get_error_code(ret);
        printf("API call error code:%d\n", err_code);
        printf("API call ret type: %d\n", ipc_return_get_data_type(ret));
        printf("API call ret size: %d\n", ipc_return_get_data_size(ret));

        if (ipc_return_get_data_type(ret) == IPC_APT_U32)
            printf("API call ret data: %d\n", *(uint32_t*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_STR)
            printf("API call ret data: '%s'\n", (char*)ipc_return_get_data(ret));
        else if (ipc_return_get_data_type(ret) == IPC_APT_VOID)
            printf("API call ret data: VOID\n");
    }
    ipc_call_free_return(ret);
    return err_code;
}

// Configure num of DMA channels to use and their direction
static int configure_operation_mode(int inject, int record, int single_injection, int calibration_mode) {
	//use single channel
	uint32_t reg = 0x1;
	if (single_injection == 0) {
		//use both channels
		reg = 0x3;
		printf(" Using 2 port mode\n");
	} else {
		printf(" Using single port mode\n");
	}
	if (inject && calibration_mode) {
		// inject + record together
		printf(" Using injection via %s and recording via single port\n", single_injection ? "single port" : "2 ports");
	}
	if (record && calibration_mode) {
		printf(" Using injection via single port and recording from %s\n", single_injection ? "single port" : "2 ports");
	}
	// write to HW dma channels selection
}

static int hw_cycle_inject(x_context *tx) {
	int buffer_id = 0;
	int cycle_status = 0;

	if (verbose)
		printf("hw_cycle_inject [%d] for TX buffer: %d\n", tx->session_id, buffer_id);
	ioctl(tx->x_proxy_fd, XFER_CYCLE, &buffer_id);
	printf("Proxy [%d] tx transfer error for buffer_id %d; status = %d\n", tx->session_id, buffer_id, tx->x_proxy_buffer_p[buffer_id].status);
	if ((tx->x_proxy_buffer_p[buffer_id].status != PROXY_NO_ERROR) && (tx->x_proxy_buffer_p[buffer_id].status != PROXY_TIMEOUT)) {
		cycle_status = -1;
	}
	else 
	{
		if (verbose)
			printf("FINISH_XFER [%d] for TX buffer: %d\n", tx->session_id, buffer_id);
	}
	return cycle_status;
}

static void stop_cyclic_dma(x_context *tx) {
	int dummy = 0;
	ioctl(tx->x_proxy_fd, STOP_DMA, &dummy);
	printf("stop_cyclic_dma: stopped\n");
}

static int single_inject(x_context *tx) {
	int i, counter = 0, buffer_id, in_progress_count = 0;
	int stop_in_progress = 0;
	id_t pid;
	int num_transfers = tx->num_transfers;
	long count;
	int cycle_status = 0;
//    printf("single_inject: num_transfers  %d\n", tx->num_transfers);
	for (buffer_id = 0; buffer_id < tx->LAST_BUFFER_ID ; buffer_id += BUFFER_INCREMENT) {

        /* Start the DMA transfer and this call is non-blocking
         * For blocking call XFER must be passed instead of START_XFER.
         */
		ioctl(tx->x_proxy_fd, START_XFER, &buffer_id);
		in_progress_count++;
		if (verbose)
			printf("Set [%d] TX buffer: %d\n", tx->session_id, buffer_id);
	}

    /* Start finishing up the DMA transfers that were started beginning with the 1st channel buffer.
     */
	buffer_id = 0;
	while (1) {
        /* Perform the DMA transfer and check the status after it completes
         * as the call blocks til the transfer is done.
         */
        ioctl(tx->x_proxy_fd, FINISH_XFER, &buffer_id);
        if (tx->x_proxy_buffer_p[buffer_id].status != PROXY_NO_ERROR) {
            printf("Proxy [%d] tx transfer error for buffer_id %d\n", tx->session_id, buffer_id);
            cycle_status = -1;
        }
        else {
			if (verbose)
            	printf("FINISH_XFER [%d] for TX buffer: %d\n", tx->session_id, buffer_id);
		}
        /* Keep track of how many transfers are in progress and how many completed
        */
        in_progress_count--;
        counter++;

        /* If all the transfers are done then exit */

        if ((num_transfers > 0) && (counter >= num_transfers)) {
			if (verbose)
            	printf("[%d] counter = %d; num_transfers = %d - break\n", tx->session_id, counter, num_transfers);
            break;
        }

        /* If an early stop (control c or kill) has happened then exit gracefully
         * letting all transfers queued up be completed, but it's trickier because
         * the number of transmit vs receive channel buffers can be very different
         * which means another X transfers need to be done gracefully shutdown the
         * receive without leaving transfers in progress which is unrecoverable
         */
        if (stop & !stop_in_progress) {
            stop_in_progress = 1;
            num_transfers = counter + tx->LAST_BUFFER_ID;
            printf("Tx [%d] detected stop condition, number of transfers: %d\n", tx->session_id, num_transfers);
        }

        /* If the ones in progress will complete the count then don't start more */

        if ((counter + in_progress_count) >= num_transfers) {
            printf("[%d] in_progress_count = %d; counter = %d; num_transfers = %d - end_tx_loop\n", tx->session_id, in_progress_count, counter, num_transfers);
            goto end_tx_loop;
        }
        /* Restart the completed channel buffer to start another transfer and keep
         * track of the number of transfers in progress
         */
        count = fread(tx->x_proxy_buffer_p[buffer_id].buffer, sizeof(char), BUFFER_SIZE, tx->inpstream);
        if (count < 0) { // must be <= 0, now this helps testing with loopback 
          printf("read [%d] failed for buffer_id = %d; error = %d\n", tx->session_id, buffer_id, errno);
          stop = 1;
          break;
        }
        printf("read [%d] %ld bytes into buffer_id = %d\n", tx->session_id, count, buffer_id);
        tx->x_proxy_buffer_p[buffer_id].length = (count > 0 ? count : BUFFER_SIZE);
        tx->x_size += count;
        ioctl(tx->x_proxy_fd, START_XFER, &buffer_id);
        printf("Set [%d]  START_XFER for buffer %d\n", tx->session_id, buffer_id);
        in_progress_count++;

end_tx_loop:

        /* Flip to next buffer and wait for it treating them as a circular list
         */
        buffer_id += BUFFER_INCREMENT;
        buffer_id %= TX_BUFFER_COUNT;
    }
	return cycle_status;
}

static int single_receive(x_context *rx) {
    int total_record;
    int in_progress_count = 0;
	int rx_cycle_status = 0;
    int buffer_id;
    int transferred_bytes;
    int num_transfers = rx->num_transfers;
	int stop_time_ms = rx->stop_time_ms;
	int stop_size_kb = rx->stop_size_kb;
	int need_2_store_file = rx->need_2_store_file;
	if (verbose)
		printf("single_receive: num_transfers = %d; buff = %08X\n", num_transfers, rx->x_proxy_buffer_p);
	printf("single_receive: num_transfers = %d; buff = %08X; rx->LAST_BUFFER_ID = %d\n", num_transfers, rx->x_proxy_buffer_p, rx->LAST_BUFFER_ID);
// set up free rx buffers
    for (buffer_id = 0; buffer_id < rx->LAST_BUFFER_ID; buffer_id += BUFFER_INCREMENT) {

       rx->x_proxy_buffer_p[buffer_id].length = BUFFER_SIZE;

		printf("single_receive: set buffer %d\n", buffer_id);
       ioctl(rx->x_proxy_fd, START_XFER, &buffer_id);

        /* Handle the case of a specified number of transfers that is less than the number
         * of buffers
         */
        if (++in_progress_count >= num_transfers)
            break;
     }
     /* Start the transmit thread now that receive buffers are queued up and started
     * and finish receiving the data in the 1st buffer. If the transmit starts before
     * the receive is ready there will be verify errors.
     */
     tx_wait = 0;
     buffer_id = 0;

    /* Finish each queued up receive buffer and keep starting the buffer over again
     * until all the transfers are done
     */
     while (stop == 0) {
        int store, stored;
        ioctl(rx->x_proxy_fd, FINISH_XFER, &buffer_id);

        if (rx->x_proxy_buffer_p[buffer_id].status != PROXY_NO_ERROR) {
            printf("Proxy [%d] single_receive transfer error for buffer_id %d, # completed %d, # in progress %d, #num_transfers = %d\n",
					rx->session_id, buffer_id, rx_counter, in_progress_count, num_transfers);
            rx_cycle_status = -1;
            break;
        } else {
          rx->x_size += BUFFER_SIZE;
		  if (verbose)
			printf("Received RX buffer_id %d, rx_counter = %d, in_progress_count = %d, #num_transfers = %d\n", buffer_id, rx_counter,in_progress_count, num_transfers);
        }
#ifndef STORE
// In loopback hw, need to receive all transmitted buffers
		if (need_2_store_file && (stop_time_ms > 0 || stop_size_kb > 0)) {
			store = (total_record > BUFFER_SIZE ? BUFFER_SIZE : total_record);
          	stored = store_record_file(rx->x_proxy_buffer_p[buffer_id].buffer, store, rx->outstream);
            printf("Stored [%d] %d bytes from %d,; attempted %d bytes\n", rx->session_id, stored, total_record, store);
            if (stored != store) {
            	printf("Error [%d] to store to disk; exiting\n", rx->session_id);
                rx_cycle_status = -1;
                break;
			}
			if (stop_time_ms > 0) {
            // stop time condition is valid
            	end_time = get_posix_clock_time_usec();
                if ( (end_time - start_time) >= (stop_time_ms * 1000)) {
					printf("Finished [%d] to record  on time\n", rx->session_id);
					need_2_store_file = 0;
					break;
				}
			}
			if (stop_size_kb > 0) {
            // stop size condition is valid
				total_record -= stored;
				if (total_record <= 0) {
					printf("Finished [%d] to record required amount of bytes\n", rx->session_id);
					need_2_store_file = 0;
					break;
				}
			}
		}
#endif
        /* Keep track how many transfers are in progress so that only the specified number
         * of transfers are attempted
         */
        in_progress_count--;
        /* If all the transfers are done then exit
        */
/*
        if (++rx_counter >= num_transfers)
            break;
			*/
        /* If the ones in progress will complete the number of transfers then don't start more
         * but finish the ones that are already started
         */
/*        if ((rx_counter + in_progress_count) >= num_transfers)
            goto end_rx_loop;
*/
        /* Start the next buffer again with another transfer keeping track of
         * the number in progress but not finished
         */
        rx->x_proxy_buffer_p[buffer_id].length = BUFFER_SIZE;
        ioctl(rx->x_proxy_fd, START_XFER, &buffer_id);
   //     printf("Start the next RX buffer: %d\n", buffer_id);
        in_progress_count++;

end_rx_loop:

        /* Flip to next buffer treating them as a circular list, and possibly skipping some
         * to show the results when prefetching is not happening
         */
        buffer_id += BUFFER_INCREMENT;
        buffer_id %= RX_BUFFER_COUNT;
    }
    return rx_cycle_status;
}

/*******************************************************************************************************************/
/*
 * The following function is the transmit thread to allow the transmit and the receive channels to be
 * operating simultaneously. Some of the ioctl calls are blocking so that multiple threads are required.
 */
void *tx_thread(x_context *tx)
{
    int i, cycle_status = 0;
	/* Wait until the receive processing has some transfers in the queue to start sending to prevent
	 * a loss of data
	 */
	if (single_injection) {
		int num_buffers = fill_inject_data(tx, (const char*)inject_record_file);
		tx->LAST_BUFFER_ID = num_buffers;
		if (verbose)
			printf("Ready [%d] for transfer %d buffers\n", tx->session_id, tx->LAST_BUFFER_ID);
	} else {
		if (0 > open_interleaved_data_file(tx, (const char*)inject_record_file, chunk_size)) {
			printf("[%d] failed to open file %s\n", tx->session_id, inject_record_file);
			return NULL;
		}
	}
	while (tx_wait);

	// Transfer all buffers that were already filled with data
	start_time = get_posix_clock_time_usec();
	if ((num_cycles > 1) || (num_cycles == 0))

    {
	//hw controlled cycle injection
		cycle_status = hw_cycle_inject(tx);
		printf("hw_cycle_inject  status = %d\n", cycle_status);
		if (cycle_status != -1)
			sem_wait(&tx->endless_hw_sem);
		stop_cyclic_dma(tx);
	} else {
		// sw controlled cycle (will be obsolete)
    	for (i = 1; (stop == 0) && (cycle_status >= 0) && (( i <= num_cycles) || (num_cycles == 0)); i++) {
			cycle_status = single_inject(tx);
			if (verbose)
				printf("Passed in [%d]  %d cycles of %d; status = %d\n", tx->session_id, i , num_cycles, cycle_status);
		}
	}	
	end_time = get_posix_clock_time_usec();
}

void *rx_thread(x_context *rx)
{
    int i, cycle_status = 0;
	printf("rx_thread  [%d] with  %d cycles\n", rx->session_id, num_cycles);
	if (verbose)
		printf("rx_thread  [%d] with  %d cycles\n", rx->session_id, num_cycles);

	for (i = 1; ((i <= num_cycles) || (num_cycles == 0))  && (cycle_status >= 0) && (stop == 0); i++) {
		cycle_status = single_receive(rx);
		printf("Received [%d] RX cycle %d from %d; RX status = %d\n", rx->session_id, i, num_cycles, cycle_status);
	}
}

/*******************************************************************************************************************/
/*
 * Setup the transmit and receive threads so that the transmit thread is low priority to help prevent it from
 * overrunning the receive since most testing is done without any backpressure to the transmit channel.
 */
void setup_threads(x_context *th)
{
	pthread_attr_t tattr;
	int newprio = 20;
	struct sched_param param;

	/* The transmit thread should be lower priority than the receive
	 * Get the default attributes and scheduling param
	 */
	pthread_attr_init (&tattr);
	pthread_attr_getschedparam (&tattr, &param);

	/* Set the transmit priority to the lowest
	 */
	param.sched_priority = newprio;
	pthread_attr_setschedparam (&tattr, &param);

	/* Create the thread for the transmit processing passing the number of transactions to it, start it
	 * before the receive processing is started so that the total time taken does not include the
	 * creation of a thread
	 */
	if (0 != pthread_create(&(th->x_tid), &tattr, th->start_routine, th)) {
		printf("Thread creation failed\n");
		th->x_tid = -1;
        return;
	}

	/* Set the calling thread priority to the maximum as it should be the receive processing
	 */
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	sem_init(&th->endless_hw_sem, 0, 0);
	if (verbose)
		printf("setup_threads: finished for stop-size: %d\n", th->stop_size_kb);
}

static int setup_channel(x_context *x, int session_id, const char *dev_name, int stop_size, int stop_time, int store_file) {
	x->session_id = session_id;
	x->x_proxy_buffer_p = MAP_FAILED;
	x->x_tid = -1;
	printf("Opening DMA proxy device file: %s\n", dev_name);
	x->x_proxy_fd = open(dev_name, O_RDWR);
	if (x->x_proxy_fd < 1) {
		printf("Unable to open DMA proxy device file: %s\n", dev_name);
		return -1;
	}
	x->need_2_store_file = store_file;
    x->stop_size_kb = stop_size;
	x->stop_time_ms = stop_time;
	x->interleaved_fd = -1;
	return 0;
}

static int init_x_channel(x_context *x, int num_transfers, int last_buff, void * thread_func) {
	x->inpstream = NULL;
	x->outstream = NULL;
	x->interleaved_fd = -1;
    /* Map the transmit and receive channels memory into user space so it's accessible. Note that each channel
     * has a set of channel buffers which are offsets from the start of the mapped channel memory.
     */
	x->x_proxy_buffer_p = (struct channel_buffer *)mmap(NULL, sizeof(struct channel_buffer) * TX_BUFFER_COUNT,
                                    PROT_READ | PROT_WRITE, MAP_SHARED, x->x_proxy_fd, 0);
	if (x->x_proxy_buffer_p == MAP_FAILED) {
		printf("Failed [%d] to mmap\n", x->session_id);
		return -1;
	}
   
	x->LAST_BUFFER_ID = last_buff;
	x->num_transfers = num_transfers;
	x->start_routine = thread_func;
	if (thread_func)
		setup_threads(x);
	return 0;
}

static int check_dual_injection_file_size(const char *src_file, int chunk_size) {
	struct stat st;
	if (0 > stat((const char*)inject_record_file, &st)) {
		printf("Error fstat file: %s; error: %d\n", inject_record_file, errno);
		return -1;
	}
	if (st.st_size % (2*chunk_size)) {
		printf("INjection file has a wrong size: %lu\n", st.st_size);
		return -1;
	}
	return 0;
}

static void unload(x_context *x) {
    /* Unmap the proxy channel interface memory and close the device files before leaving
     */
	if (x->x_proxy_buffer_p && (x->x_proxy_buffer_p != MAP_FAILED))
		munmap(x->x_proxy_buffer_p, sizeof(struct channel_buffer));
	if (x->outstream)
		fclose(x->outstream);
    if (x->inpstream)
		fclose(x->inpstream);
    if (x->x_proxy_fd > 0)
	    close(x->x_proxy_fd);
    if (x->interleaved_fd > 0)
		close(x->interleaved_fd);
    if (x->x_tid > 0)
        sem_destroy(&x->endless_hw_sem);
}
/*******************************************************************************************************************/
/*
 * The main program starts the transmit thread and then does the receive processing to do a number of DMA transfers.
 */
int main(int argc, char *argv[])
{
	int i, total_record;
	int in_progress_count = 0, buffer_id = 0;
	uint64_t time_diff;
	int mb_sec, num_buffers;
    int module_id = -1;
    int stop_time_ms = -1;
    int stop_size_kb = -1;
	int num_transfers = 0;
    int record = 0;
    int inject = 0;
    int need_2_store_file = 0;
    int ret, ints;
	struct stat st;
    int session_id = 0;
    int transferred_bytes;
    int bandwidth = 0;
	int num_devices = 1; // in each direction
	int calibration_mode = 0;

	printf("injection/recording: BUFFER_COUNT = %d; BUFFER_SIZE = %d\n", BUFFER_COUNT, BUFFER_SIZE);

	signal(SIGINT, sigint);

	if (argc < 2) {
		printf("Usage: inject-record < -f file to inject>  [ -m module_id] [-b bandwidth] [ --id session_id] [-c | --cycles num_cycles]\n");
		exit(EXIT_FAILURE);
	}
    for (i = 1; i < argc; i++) {
		if (strncmp(argv[i],"-f",2)==0 || strncmp(argv[i],"--file", 6) ==0) {
			inject_record_file = argv[++i];
        }
		else if (strncmp(argv[i],"-m", 2)==0) {
			module_id = atoi(argv[++i]);
		}
		else if (strncmp(argv[i],"-s", 2)==0 || strncmp(argv[i],"--stop-size", 11) ==0) {
			stop_size_kb = atoi(argv[++i]);
			need_2_store_file = 1;
		}
		else if (strncmp(argv[i],"-t", 2)==0 || strncmp(argv[i],"--stop-time", 11) ==0) {
			stop_time_ms = atoi(argv[++i]);    
			need_2_store_file = 1;
		}
		else if (strncmp(argv[i],"-b", 2)==0) {
			bandwidth = atoi(argv[++i]);    
		}
		else if (strncmp(argv[i],"-cs", 3)==0) {
			chunk_size = atoi(argv[++i]);
		}
		else if (strncmp(argv[i],"--src", 5)==0) {
			inject_record_file = argv[++i];
			printf("src file = %s\n", inject_record_file);
		}
		else if (strncmp(argv[i],"--dst", 5)==0) {
			dst_record_file = argv[++i];
			printf("dst_record_file = %s\n", dst_record_file);
		}
		else if (strncmp(argv[i],"-v", 2)==0) {
			verbose = 1;
		}
		else if (strncmp(argv[i],"--id", 4)==0) {
			session_id = atoi(argv[++i]);
		}
		else if (strncmp(argv[i],"-c", 2)==0 || strncmp(argv[i],"--cycles", 8) ==0) {
			num_cycles = atoi(argv[++i]);
			if (num_cycles < 0) {
				printf("Invalid number for %s: %d\n", argv[i], num_cycles);
				return -1;
			}
		}
		else if (0 == strncmp(argv[i], "inject", 6)) {
			inject = 1;
		}
		else if (0 == strncmp(argv[i], "record", 6)) {
			record = 1;
		}
	}
	calibration_mode = (dst_record_file && inject_record_file) ? 1 : 0;
	total_record = BUFFER_SIZE +1;
    // preliminary set the following: 
   // LAST_BUFFER_ID = 1;
    // decide  whether there will be dual (interleaved injection or recording)
    if (bandwidth > MAX_SINGLE_BW) {
        num_devices = NUM_CHANNELS;
        single_injection = 0;
        tx_wait = 0;
		if (inject && (-1 == check_dual_injection_file_size(inject_record_file, chunk_size))) {
			exit(EXIT_FAILURE);
		}
    }
	if (stop_size_kb > 0) {
		total_record = stop_size_kb *1024;
		printf("stop size to write %d KB\n", stop_size_kb);
		need_2_store_file = 1;
        if (! check_available_space(argv[0])) {
            printf("Not enough space\n");
            exit(EXIT_FAILURE);
        }
		if (calibration_mode) {
			// open file(s) for record specified by -dst option
			if (0 > open_record_file(dst_record_file, 0)) {
				printf("failed to open file for recording channel 0\n");
				exit(EXIT_FAILURE);
			}
			if (single_injection == 0) {
				if (0 > open_record_file(dst_record_file, 1)) {
					printf("failed to open file for recording channel 1\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		else { 
			if (0 > open_record_file(inject_record_file, 0)) {
				printf("failed to open file for recording single channel\n");
				exit(EXIT_FAILURE);
			}
		}
	}
    configure_operation_mode(inject, record, single_injection, calibration_mode);
    // Below is useful ONLY when using loopback fpga
//	if (inject)
//		record = 1;
	if (calibration_mode)
		tx_wait = 1;
	ret = open_ipc(argc, argv);
	if (0 != ret) {
		printf("Failed to open IPC\n");
	}
    if (single_injection && inject) {
		if (0 > stat((const char*)inject_record_file, &st)) {
			printf("Error fstat file: %s; error: %d\n", inject_record_file, errno);
			exit(EXIT_FAILURE);
		} else {
			printf("File size to inject is %lu\n", st.st_size);
			if (st.st_size > (BUFFER_SIZE * TX_BUFFER_COUNT)) {
				num_transfers = st.st_size / BUFFER_SIZE + ((st.st_size % BUFFER_SIZE) ? 1 : 0);
				printf("Need to read more bytes from file; num_transfers = %d\n", num_transfers);
			} else {
				num_transfers = st.st_size / BUFFER_SIZE + ((st.st_size % BUFFER_SIZE) ? 1 : 0);
				printf("File size id less than buffer(s); num_transfers = %d\n", num_transfers);
			}
		}
	}
    /* Open the DMA proxy device for the transmit and receive channels, the proxy driver is a character device
     * that creates these device nodes
     */
	// Usually, there will be only receive or only transmit (when inject or record is set).
	for (i = 0; i < num_devices; i++) {
		if (0 > setup_channel(inject ? &(tx[i]) : &(rx[i]), session_id +i, inject ? TX_DEV_NAME[i] : RX_DEV_NAME[i], stop_size_kb, stop_time_ms, need_2_store_file)) {
			printf("Unable to open DMA proxy device file: %s\n", inject ? TX_DEV_NAME[i] : RX_DEV_NAME[i]);
			goto release;
		}
		if (0 > init_x_channel(inject ? &(tx[i]) : &(rx[i]), inject ? num_transfers :RX_BUFFER_COUNT , inject ? num_transfers : RX_BUFFER_COUNT, inject ? tx_thread: rx_thread)) {
			printf("init_x_channel failed for %s\n", inject ? TX_DEV_NAME[i] : RX_DEV_NAME[i]);
			goto release;
		}
	}
/***********************************************************/
// When inject and calibration mode, need to init single rx
    if (inject && calibration_mode) {
	// Init Rx single channel in addition to the above initialized Tx.
		if (0 > setup_channel(&rx[0], session_id, RX_DEV_NAME[0], stop_size_kb, stop_time_ms, need_2_store_file)) {
			printf("Unable to open DMA proxy Rx device file: %s\n",RX_DEV_NAME[0]);
			goto release;
		}
		if (0 > init_x_channel(&(rx[0]), RX_BUFFER_COUNT , RX_BUFFER_COUNT, rx_thread)) {
			printf("init_x_channel rx failed\n");
			goto release;
		}
	}
// When record calibration mode, need to init single tx
	if (record && calibration_mode) {
	// Init Tx single channel in addition to the above initialized Rx.
		if (0 > setup_channel(&tx[0], session_id, TX_DEV_NAME[0], stop_size_kb, stop_time_ms, need_2_store_file)) {
			printf("Unable to open DMA proxy Tx device file: %s\n",TX_DEV_NAME[0]);
			goto release;
		}
		if (0 > init_x_channel(&(tx[0]), num_transfers , num_transfers, tx_thread)) {
			printf("init_x_channel tx failed\n");
			goto release;
		}
	}
/***********************************************************/
	/* Wait for the transmit thread to finish*/
    printf("Before thread join\n");
    for (i = 0; i < num_devices; i++) {
		if (tx[i].x_tid > 0) {
			if (0 != pthread_join(tx[i].x_tid, NULL))
				printf("pthread_join failed for tx[%d]\n", i);
		}
		if (record && (rx[i].x_tid > 0)) {
			if (0 != pthread_join(rx[i].x_tid, NULL))
				printf("pthread_join failed for rx[%d]\n", i);
		}
	}
	time_diff = end_time - start_time;
    if (0 <= (ints = read_interrupts(inject)))
          printf("Number of %s interrupts: %d\n", (inject ? "Tx" : "Rx"), ints);
    transferred_bytes = inject ? tx[0].x_size : rx[0].x_size;
	mb_sec = ((1000000 / (double)time_diff) * (num_cycles * (double)transferred_bytes));
	printf("Time: %ld microseconds\n", time_diff);
	printf("Transfer size: %lld KB\n", (long long)(num_cycles) * (transferred_bytes / 1024));
	printf("Throughput: %d Bytes / sec \n", mb_sec);
	
release:
	/* Unmap the proxy channel interface memory and close the device files before leaving
	 */
	stop = 1;
	for (i = 0; i < num_devices; i++) {
		unload(&tx[i]);
		unload(&rx[i]);
	}
#ifdef IPC_T
	ipc_close(h_ipc_client);
#endif
	return 0;
}

