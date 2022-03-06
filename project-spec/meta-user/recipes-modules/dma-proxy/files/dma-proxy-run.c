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
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include "dma-proxy.h"

static int verify;
static struct channel_buffer *tx_proxy_buffer_p;
static int tx_proxy_fd;
static int test_size;
static volatile int tx_wait = 1;
static volatile int stop = 0;
static volatile int rx_counter = 0;
static volatile int start_pattern = 0;
static pthread_t tx_tid;
static int LAST_BUFFER_ID = 0;

/*******************************************************************************************************************/
/* Handle a control C or kill, maybe the actual signal number coming in has to be more filtered?
 * The stop should cause a graceful shutdown of all the transfers so that the application can
 * be started again afterwards.
 */
void sigint(int a)
{
	stop = 1;
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

static int fill_inject_data(const char* filename) {
  FILE * stream;
  int count, buffer_id;
  test_size = 0;
  stream = fopen(filename, "rb");
  for (buffer_id = 0; buffer_id < TX_BUFFER_COUNT; buffer_id += BUFFER_INCREMENT) {
    count = fread(tx_proxy_buffer_p[buffer_id].buffer, sizeof(char), BUFFER_SIZE, stream);
    if (count < 0) {
      printf("read failed; error = %d\n", errno);
      fclose(stream);
      return -1;
    }
    printf("read %d bytes into buffer_id = %d\n", count, buffer_id);
    test_size += count;
    tx_proxy_buffer_p[buffer_id].length = count;
    if (count < BUFFER_SIZE)
      break;
  }
  printf("Test_size = %d\n", test_size);
  fclose(stream);
  return buffer_id;
}
/*******************************************************************************************************************/
/*
 * The following function is the transmit thread to allow the transmit and the receive channels to be
 * operating simultaneously. Some of the ioctl calls are blocking so that multiple threads are required.
 */
void *tx_thread(int *num_transfers_input)
{
	int i, counter = 0, buffer_id, in_progress_count = 0;
	int num_transfers = (*num_transfers_input);
	int stop_in_progress = 0;
	id_t pid;

	/* Wait until the receive processing has some transfers in the queue to start sending to prevent
	 * a loss of data
	 */
	while (tx_wait);

	// Start all buffers being sent

	for (buffer_id = 0; buffer_id < LAST_BUFFER_ID ; buffer_id += BUFFER_INCREMENT) {

		/* Start the DMA transfer and this call is non-blocking
		 *
		 */
		ioctl(tx_proxy_fd, START_XFER, &buffer_id);
		in_progress_count++;
                printf("Set TX buffer: %d\n", buffer_id);
	}

	/* Start finishing up the DMA transfers that were started beginning with the 1st channel buffer.
	 */
	buffer_id = 0;

	while (1) {

		/* Perform the DMA transfer and check the status after it completes
		 * as the call blocks til the transfer is done.
		 */
		ioctl(tx_proxy_fd, FINISH_XFER, &buffer_id);
		if (tx_proxy_buffer_p[buffer_id].status != PROXY_NO_ERROR)
			printf("Proxy tx transfer error for buffer_id %d\n",buffer_id);
		else
			printf("FINISH_XFER for TX buffer: %d\n", buffer_id);
                /* Keep track of how many transfers are in progress and how many completed
                 */
                in_progress_count--;
                counter++;

		/* If all the transfers are done then exit */

		if ((num_transfers > 0) && (counter >= num_transfers))
			break;

		/* If an early stop (control c or kill) has happened then exit gracefully
		 * letting all transfers queued up be completed, but it's trickier because
		 * the number of transmit vs receive channel buffers can be very different
		 * which means another X transfers need to be done gracefully shutdown the
		 * receive without leaving transfers in progress which is unrecoverable
		 */
		if (stop & !stop_in_progress) {
			stop_in_progress = 1;
			num_transfers = counter + LAST_BUFFER_ID;
			printf("Tx detected stop condition, number of transfers: %d\n", num_transfers);
		}

		/* If the ones in progress will complete the count then don't start more */

		if ((counter + in_progress_count) >= num_transfers)
			goto end_tx_loop;

		/* Restart the completed channel buffer to start another transfer and keep
		 * track of the number of transfers in progress
		 */
		ioctl(tx_proxy_fd, START_XFER, &buffer_id);
		printf("Set START_XFER for buffer %d\n", buffer_id);
		in_progress_count++;

end_tx_loop:

		/* Flip to next buffer and wait for it treating them as a circular list
		 */
		buffer_id += BUFFER_INCREMENT;
		buffer_id %= LAST_BUFFER_ID;
	}
}

/*******************************************************************************************************************/
/*
 * Setup the transmit and receive threads so that the transmit thread is low priority to help prevent it from
 * overrunning the receive since most testing is done without any backpressure to the transmit channel.
 */
void setup_threads(int *num_transfers)
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
	pthread_create(&tx_tid, &tattr, tx_thread, num_transfers);

	/* Set the calling thread priority to the maximum as it should be the receive processing
	 */
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
}

/*******************************************************************************************************************/
/*
 * The main program starts the transmit thread and then does the receive processing to do a number of DMA transfers.
 */
int main(int argc, char *argv[])
{
	struct channel_buffer *rx_proxy_buffer_p;
	int rx_proxy_fd, i;
	int in_progress_count = 0, buffer_id = 0;
	uint64_t start_time, end_time, time_diff;
	int mb_sec, num_buffers, num_cycles = 1;
        int module_id = -1;
        int stop_time_ms = 0;
        int stop_size_kb = 0;
	int num_transfers = 0;
        int record = 0;
        int inject = 0;
        char* inject_file;
	printf("injection/recording\n");

	signal(SIGINT, sigint);

	if (argc < 2) {
		printf("Usage: dma-proxy-test < file to inject>  [# of DMA transfers to perform, if omitted - infinite loop]\n");
		exit(EXIT_FAILURE);
	}
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i],"-f",2)==0) {
	  inject_file = argv[++i];
        }
        else if (strncmp(argv[i],"-m", 2)==0) {
          module_id = atoi(argv[++i]);
        }
        else if (strncmp(argv[i],"-s", 2)==0 || strncmp(argv[i],"--stop-size", 11) ==0) {
	  stop_size_kb = atoi(argv[++i]);
        }
        else if (strncmp(argv[i],"-t", 2)==0 || strncmp(argv[i],"--stop-time", 11) ==0) {
          stop_time_ms = atoi(argv[++i]);    
        }
        else if (0 == strncmp(argv[i], "inject", 6)) {
          inject = 1;
        }
        else if (0 == strncmp(argv[i], "record", 6)) {
          record = 1;
        }
        else if (strncmp(argv[i],"-n", 2)==0 || strncmp(argv[i],"--num", 5) ==0) {
          num_cycles = atoi(argv[++i]);
        }
        else
          printf("Invalid option: %s\n", argv[i]);
    }

	printf("File %s will be injected %d time(s)\n", inject_file, num_cycles); 
	/* Open the DMA proxy device for the transmit and receive channels, the proxy driver is a character device
	 * that creates these device nodes
 	 */
		tx_proxy_fd = open("/dev/dma_proxy_tx", O_RDWR);

		if (tx_proxy_fd < 1) {
			printf("Unable to open DMA proxy device file");
			exit(EXIT_FAILURE);
		}
		rx_proxy_fd = open("/dev/dma_proxy_rx", O_RDWR);
		if (tx_proxy_fd < 1) {
			printf("Unable to open DMA proxy device file");
			exit(EXIT_FAILURE);
		}
	/* Map the transmit and receive channels memory into user space so it's accessible. Note that each channel
	 * has a set of channel buffers which are offsets from the start of the mapped channel memory.
 	 */
	  tx_proxy_buffer_p = (struct channel_buffer *)mmap(NULL, sizeof(struct channel_buffer) * TX_BUFFER_COUNT,
									PROT_READ | PROT_WRITE, MAP_SHARED, tx_proxy_fd, 0);
          if (tx_proxy_buffer_p == MAP_FAILED) {
                printf("Failed to mmap\n");
                exit(EXIT_FAILURE);
          }

        if (inject) {
          num_buffers = fill_inject_data((const char*)inject_file);
          LAST_BUFFER_ID = num_buffers +1;
          num_transfers = num_cycles *LAST_BUFFER_ID;
          setup_threads(&num_transfers);

	}

	start_time = get_posix_clock_time_usec();
	  rx_proxy_buffer_p = (struct channel_buffer *)mmap(NULL, sizeof(struct channel_buffer) * RX_BUFFER_COUNT,
									PROT_READ | PROT_WRITE, MAP_SHARED, rx_proxy_fd, 0);
	  if (rx_proxy_buffer_p == MAP_FAILED) {
		printf("Failed to mmap\n");
		exit(EXIT_FAILURE);
	  }



	// Start all buffers being received

        if (record) {
 	  for (buffer_id = 0; buffer_id < LAST_BUFFER_ID; buffer_id += BUFFER_INCREMENT) {

		/* Don't worry about initializing the receive buffers as the pattern used in the
		 * transmit buffers is unique across every transfer so it should catch errors.
		 */
		rx_proxy_buffer_p[buffer_id].length = BUFFER_SIZE ;

		ioctl(rx_proxy_fd, START_XFER, &buffer_id);

		/* Handle the case of a specified number of transfers that is less than the number
		 * of buffers
		 */
		if ((num_transfers > 0) && (++in_progress_count >= num_transfers))
			break;
	  }
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
	while (record) {

		ioctl(rx_proxy_fd, FINISH_XFER, &buffer_id);

		if (rx_proxy_buffer_p[buffer_id].status != PROXY_NO_ERROR) {
			printf("Proxy rx transfer error for buffer_id %d, # transfers %d, # completed %d, # in progress %d\n",
						buffer_id, num_transfers, rx_counter, in_progress_count);
			exit(-1);
		}

		/* Verify the data received matches what was sent (tx is looped back to tx)
		 * A unique value in the buffers is used across all transfers
		 */
		
		/* Keep track how many transfers are in progress so that only the specified number
		 * of transfers are attempted
		 */
		in_progress_count--;

		/* If all the transfers are done then exit */

		if (++rx_counter >= num_transfers)
			break;

		/* If the ones in progress will complete the number of transfers then don't start more
		 * but finish the ones that are already started
		 */
		if ((rx_counter + in_progress_count) >= num_transfers)
			goto end_rx_loop;

		/* Start the next buffer again with another transfer keeping track of
		 * the number in progress but not finished
		 */
		ioctl(rx_proxy_fd, START_XFER, &buffer_id);

		in_progress_count++;

end_rx_loop:

		/* Flip to next buffer treating them as a circular list, and possibly skipping some
		 * to show the results when prefetching is not happening
		 */
		buffer_id += BUFFER_INCREMENT;
		buffer_id %= LAST_BUFFER_ID;
	}

	end_time = get_posix_clock_time_usec();
	time_diff = end_time - start_time;
	mb_sec = ((1000000 / (double)time_diff) * (num_cycles * (double)test_size)) / 1000;

	printf("Time: %d microseconds\n", time_diff);
	printf("Transfer size: %d KB\n", (long long)(num_cycles) * (test_size / 1024));
	printf("Throughput: %d KB / sec \n", mb_sec);

	/* Wait for the transmit thread to finish
	 */
	pthread_join(tx_tid, NULL);

	/* Unmap the proxy channel interface memory and close the device files before leaving
	 */
	munmap(tx_proxy_buffer_p, sizeof(struct channel_buffer));
	munmap(rx_proxy_buffer_p, sizeof(struct channel_buffer));

	close(tx_proxy_fd);
	close(rx_proxy_fd);

	printf("DMA proxy test complete\n");

	return 0;
}

