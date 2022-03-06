/**
 * @file axis-fifo-eth-loop.c
 * @author Jason Gutel jason.gutel@gmail.com
 *
 * Sets up an echo ping-pong server over a TCP connection. Packets are sent
 * over sockets, sent to the AXI Stream FIFO core (assumed in loopback), and
 * then sent back out over the socket.
 *
 * Shows example of using poll() with the kernel module
 *
 * @bug No known bugs.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <fcntl.h>              // Flags for open()
#include <sys/stat.h>           // Open() system call
#include <sys/types.h>          // Types for open()
#include <unistd.h>             // Close() system call
#include <string.h>             // Memory setting and copying
#include <getopt.h>             // Option parsing
#include <errno.h>              // Error codes
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <sys/mman.h>
#include <byteswap.h>


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "axis-fifo.h"
#include "protocol.h"

#define DEBUG
int load_ddr(char *filename, uint32_t offset_addr, uint32_t size_wr, char as_hex);
void read_ddr(char *filename, uint32_t offset_addr, uint32_t size_wr, char as_hex);
#include "common.h"
/*----------------------------------------------------------------------------
 * Internal Definitions
 *----------------------------------------------------------------------------*/

#define AXIFIFO 1


#define TCP_PROTOCOL 1
#define UDP_PROTOCOL 0
#define DEF_MAX_BUF_SIZE_BYTES (1024*20)
#define DEF_MIN_BUF_SIZE_BYTES (1024*20)
#define DEF_MAX_REG_SIZE_LONG  0x200
#define DEF_PORT_NO    7777

// #define DEF_DEV_TX "/dev/axis_fifo_0x00000000a0010000"
// #define DEF_DEV_RX "/dev/axis_fifo_0x00000000a0010000"

#define DEF_DEV_TX "/dev/axis_fifo_0x00000000a0030000"
#define DEF_DEV_RX "/dev/axis_fifo_0x00000000a0030000"


// /proc/device-tree/reserved-memory/buffer\@0/reg
//# 0000000 0000 0000 006f 0000 0000 0000 0010 0000
#define RESERVED_MEMORY_FILE_ADDR "/proc/device-tree/reserved-memory/buffer@0/reg"

#define DEF_DEV_MEM "/dev/mem"
#define DEF_REG_ADDR 0xb0000000 //pw_system_config_regist
//#define DEF_REG_ADDR 0xa0000000

#define DEF_GPIO_ADDR 0xff0a0000
#define DEF_EMIO_OFFSET 0x6c




#define DEF_PROTOCOL TCP_PROTOCOL
#define MAP_SIZE 0x400000UL
#define MAP_MASK (MAP_SIZE - 1)
#define MAX_FIFOS 15





#define DDR_ADDDR  0x6f000000
#define MAP_SIZE_T 0x10000000UL
#define MAP_MASK_T (MAP_SIZE_T - 1)

struct thread_data
{
    int rc;
};

pthread_t eth_rx_thread;
pthread_t fifo_rx_thread;

static volatile bool running = true;
static volatile bool end_conection = false;

static int _opt_sock_port = DEF_PORT_NO;
static int _opt_use_protocol = DEF_PROTOCOL;
static int _opt_max_bytes = DEF_MAX_BUF_SIZE_BYTES;
static int _opt_max_bytes_eth = (1024 * 300);

static int _opt_min_bytes = DEF_MIN_BUF_SIZE_BYTES;
static int _max_reg_long = DEF_MAX_REG_SIZE_LONG;
static char _opt_dev_tx[255];
static char _opt_dev_rx[255];
static char _opt_dev_mem[255];

static int writeFifoFd = -1;
static int readFifoFd = -1;
static int memFd = -1;
static int memFd_ddr = -1;
static int serverFd = -1;
int tcpSocketFd = -1;
static int pipeFd[2];
static int shutdown_pipeFd[2];

static void *map_base;
static uint32_t *virt_addr;

static void *map_base_reserved_memory;
uint32_t *virt_addr_reserved_memory;

uint64_t tuser = 0;
int need_check_feedback = 0;


static uint32_t fpga_id = 0;
static uint32_t sw_fpga_id = 0;

/* for udp packets */
static struct sockaddr_in cliaddr;
static socklen_t cliaddrlen;
static int udpInit = 0;

static int socket_read_error = 0;
static int socket_write_error = 0;

static void signal_handler(int signal);
static void *fifo_rx_thread_fn(void *data);
static int process_options(int argc, char * argv[]);
static void print_opts();
static void display_help(char * progName);
static void *ethn_rx_thread_fn(void *data);
static void quit(void);
int main_loop(int argc, char **argv);
int get_file_name(char *buffer, int max);
int GetTempfileDescriptor(void);

typedef struct
{
    uint32_t addr;
    uint32_t size;
} dt_reg_t;

typedef union
{
    dt_reg_t dt_reg;
    char addr_as_string[20];
} component_info_t;

int get_component_info(char *comp_name, int req, component_info_t *comp_info);
int get_component_full_path(char *comp_name, char *return_name, int max_srt);
int find_driver(char *addr_as_string, char *filelocation, int max_str);
void reset_axi_fifo(void);

void reset_axi_fifo(void)
{
    uint32_t rc;
    /*****************************/
    /* initialize the fifo core  */
    /*****************************/
    rc = ioctl(readFifoFd, AXIS_FIFO_RESET_IP);
    if (rc)
    {
        FATAL_RETURN;
        perror("ioctl");
        return -1;
    }
    rc = ioctl(writeFifoFd, AXIS_FIFO_RESET_IP);
    if (rc)
    {
        FATAL_RETURN;
        perror("ioctl");
        return -1;
    }
    /* update rx_min_pkt so poll works as expected */
    uint32_t minWords = 2;// (_opt_min_bytes / 4);
    rc = ioctl(readFifoFd, AXIS_FIFO_SET_RX_MIN_PKT, &minWords);
    if (rc)
    {
        FATAL_RETURN;
        perror("ioctl");
        return -1;
    }

    /* update tx_max_pkt so poll works as expected */
    /* will only poll ready to write if there is enough buffer space */
    /* for a full packet */
    uint32_t maxWords = (_opt_max_bytes / 4);
    rc = ioctl(readFifoFd, AXIS_FIFO_SET_TX_MAX_PKT, &maxWords);
    if (rc)
    {
        FATAL_RETURN;
        perror("ioctl");
        return -1;
    }




}

int find_driver(char *addr_as_string, char *filelocation, int max_str)
{

    //  get from /proc/device-tree/__symbols__/ the device tree loaction
    // like /amba/axi_fifo_mm_s@a0030000 it is mean  /proc/device-tree/amba/axi_fifo_mm_s\@a0030000/
    // form this file we can read all the properties


    struct dirent *de;  // Pointer for directory entry
    char *catch = NULL;
    DIR *dr;
    char basepath[100] = "/dev";

    // opendir() returns a pointer of DIR type.
    printf("looking file with %s\n", addr_as_string);
    dr = opendir(basepath);
    DEBUG_PRINT("Open Dir %s\n", basepath);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        perror( "can't open \n");
        return -1;
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    // for readdir()
    while ((de = readdir(dr)) != NULL)
    {

        catch = strstr(de->d_name, addr_as_string);
        if (catch != NULL)
            {
                printf("catch %s\n", de->d_name);
                strcat(basepath, "/");
                strcat(basepath, de->d_name);
                printf("try %s\n", basepath);
                strncpy(filelocation, basepath, max_str);
                break;
            }
    }
    closedir(dr);
    if (catch == NULL)
        {
            printf("faild to find %s\n", addr_as_string);
            return -3;
        }


    return 0;



}

int get_component_info(char *comp_name, int req, component_info_t *comp_info)
{
    uint64_t address_re, size_re;

    char filename[100] = "";
    FILE *addr_fd ;
    int err;

    switch (req)
    {
        case 3:

        case 0:
        case 1:

            strcat(filename, comp_name);
            strcat(filename, "/reg");

            addr_fd = fopen(filename, "rb");
            if (addr_fd < 0)
            {

                printf("Open %s  failed with error: %s\n", filename, strerror(errno));
                return -1;
            }

            fseek(addr_fd, 0, SEEK_SET);

            err = fread(&address_re, sizeof(address_re), 1, addr_fd);
            if ( err < 0 )
            {
                perror("read");
                //FATAL;
                quit();
            }

            if ((fread(&size_re, sizeof(size_re), 1, addr_fd)) < 0 )
            {
                perror("read");
                //FATAL;
                quit();
            }



            fclose(addr_fd);

            address_re = bswap_64(address_re);
            size_re    = bswap_64(size_re);
            printf("Address %x\n", address_re);
            printf("size %x\n", size_re);
            if (req == 1)
            {
                comp_info->dt_reg.addr = address_re;
                comp_info->dt_reg.size = size_re;
            }
            else
                sprintf(comp_info->addr_as_string, "%x", address_re);
            break;
        default:
            break;
    }
    return 0;

}

int get_component_full_path(char *comp_name, char *return_name, int max_srt)
{
    //  get from /proc/device-tree/__symbols__/ the device tree loaction
    // like /amba/axi_fifo_mm_s@a0030000 it is mean  /proc/device-tree/amba/axi_fifo_mm_s\@a0030000/
    // form this file we can read all the properties


    struct dirent *de;  // Pointer for directory entry
    char *catch = NULL;
    FILE *fp;
    DIR *dr;
    char basepath[100] = "/proc/device-tree/__symbols__";

    // opendir() returns a pointer of DIR type.
    dr = opendir(basepath);
    DEBUG_PRINT("Open Dir\n %s", basepath);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        perror( "can't open __LINE__\n");
        return -1;
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    // for readdir()
    while ((de = readdir(dr)) != NULL)
    {
        catch = strstr(de->d_name, comp_name);
        if (catch != NULL)
            {
                printf("catch %s\n", de->d_name);
                strcat(basepath, "/");
                strcat(basepath, de->d_name);
                printf("try %s\n", basepath);
                fp = fopen(basepath, "r");
                if (fp == NULL)
                {
                    perror("Error opening file");
                    return (-2);
                }

                if ( fgets (return_name, max_srt, fp) == NULL )
                {
                    printf("Error read file %s\n", de->d_name);
                }
                fclose(fp);
                break ;
            }
    }



    closedir(dr);


    if (catch == NULL)
        {
            printf("faild to find %s\n", comp_name);
            return -3;
        }


    return 0;



}
int get_file_name(char *buffer, int max)
{

    time_t rawtime;

    time (&rawtime);
    snprintf(buffer, max, "/TEST_%s", ctime(&rawtime) );
// Lets convert space to _ in

    char *p = buffer;
    for (; *p; ++p)
    {
        if ((*p == ' ') || (*p == ':'))
            *p = '_';
    }


    buffer[strlen( buffer) - 1] = 0;
    printf("file name %s\n", buffer);
    return 0;
}
/*----------------------------------------------------------------------------
 * Main
 *----------------------------------------------------------------------------*/
int main_loop(int argc, char **argv)
{







    process_options(argc, argv);
    sleep(1);
    printf("Begin...\n");

    int rc;
    struct sockaddr_in servAddr;
    int opt;
    int addrlen;
    void        *tret;
    int  bytes, err;
    FILE *addr_fd;
    uint64_t address_re, size_re;
    component_info_t component_info;
    char filelocation[100];
    char filelocation_full[100];



    // Listen to ctrl+c and assert
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    {

#if AXIFIFO
        //    if (get_component_full_path("pw_axi_fifo", filelocation, sizeof(filelocation)) < 0)
        if (get_component_full_path("axi_fifo_mm_s_0", filelocation, sizeof(filelocation)) < 0)
        {
            printf("failed to get_component_full_path\n");
            exit (1);
        }
        printf("full path %s\n", filelocation);

        sprintf(filelocation_full, "/proc/device-tree%s", filelocation);

        get_component_info(filelocation_full, 0, &component_info);

        if (find_driver(component_info.addr_as_string, filelocation, sizeof(filelocation)) < 0)
        {
            printf("failed to find_driver\n");
            exit (1);

        }

        strncpy(_opt_dev_tx, filelocation, sizeof(_opt_dev_tx));
        strncpy(_opt_dev_rx, filelocation, sizeof(_opt_dev_rx));
        printf("driver %s\n", filelocation);

#endif

// check reserved memory





    }
    printf("xxxxxxxxxxxxxxxxxxxxxxx\n");

    /*****************/
    /* read FPGA ID  */
    /*****************/

    memFd = open(_opt_dev_mem, O_RDWR | O_SYNC);
    if (memFd < 0)
    {
        FATAL_RETURN;
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        return -1;
    }


    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DEF_GPIO_ADDR & ~MAP_MASK);
    if (map_base == (void *) -1)
    {
        close(memFd);
        FATAL_RETURN;
        printf("mmap failed with error: %s\n", strerror(errno));
    }
    virt_addr = map_base + (DEF_GPIO_ADDR & MAP_MASK);



    fpga_id = virt_addr[DEF_EMIO_OFFSET  / sizeof(uint32_t)];
    //  fpga_id = 6;
    printf("PL block Number %d\n", fpga_id);

    if (sw_fpga_id)
    {
        fpga_id = sw_fpga_id;
        printf("override fpga_id = %d \n", sw_fpga_id);

    }
    switch (fpga_id)
    {
        case 1:
            printf("PL block IFFT 1\n");
            break;
        case 2:
            printf("PL block FFT 2\n");
            break;
        case 6:
            printf("PL block PrecodingMat 6\n");
            break;
        case 8:
            printf("PL block DL Time domain Injectiont 8\n");
            break;
        case 9:
            printf("PL block UL Time domain Injectiont 9\n");
            break;
        case 10:
            printf("PL block DL+UL Time domain Injectiont 10\n");
            break;
        case 12:
            printf("PL block up sampler 12\n");
            break;

        default:
        {
            printf("PL block unknown\n");
            goto ret;
        }
    }

    if (munmap(map_base, MAP_SIZE) == -1) FATAL;
    close(memFd);




    /* get reserved memory address */

    memFd_ddr = open(_opt_dev_mem, O_RDWR | O_SYNC);
    if (memFd_ddr < 0)
    {
        FATAL_RETURN;
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        return -1;
    }


#if 1

    if (get_component_full_path("reserved", filelocation, sizeof(filelocation)) < 0)
    {
        printf("failed to get_component_full_path \n");
        exit (1);
    }
    sprintf(filelocation_full, "/proc/device-tree%s", filelocation);
    printf("full path %s\n", filelocation_full);
    get_component_info(filelocation_full, 0, &component_info);
    address_re = component_info.dt_reg.addr;
    size_re = component_info.dt_reg.size;
    DEBUG_PRINT("RESERVED_MEMORY addr  0x%x size  0x%x \n", address_re, size_re );
    DEBUG_PRINT("RESERVED_MEMORY addr  0x%16.16x size  0x%16.16x , sizeof %d\n", address_re, size_re, sizeof(address_re) );

    DEBUG_PRINT("mask 0x%lx 0x%x mask 0x%x 0x%x \n", ((size_re - 1)), (~(size_re - 1)), MAP_MASK_T, ( ~MAP_MASK_T));

    DEBUG_PRINT("mask  0x%x mask  0x%x \n", (address_re & (~(size_re - 1))), (DDR_ADDDR  & ~MAP_MASK_T));



    map_base_reserved_memory = mmap(0, MAP_SIZE_T, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DDR_ADDDR  & ~MAP_MASK_T);
    if (map_base_reserved_memory == (void *) -1)
    {
        close(memFd);
        printf("mmap failed with error: %s\n", strerror(errno));
        FATAL_RETURN;
    }
    virt_addr_reserved_memory = map_base_reserved_memory + (DDR_ADDDR & MAP_MASK_T);
    printf("virt_addr_reserved_memory 0x%x ,0x%x", map_base_reserved_memory, virt_addr_reserved_memory );





#endif

    /*************/
    /* open regs */
    /*************/
    DEBUG_PRINT("dev mem %s\n", _opt_dev_mem);

    memFd = open(_opt_dev_mem, O_RDWR | O_SYNC);
    if (memFd < 0)
    {
        FATAL_RETURN;
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        return -1;
    }
    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DEF_REG_ADDR & ~MAP_MASK);
    if (map_base == (void *) -1)
    {
        close(memFd);
        FATAL_RETURN;
        printf("mmap failed with error: %s\n", strerror(errno));
    }
    virt_addr = map_base + (DEF_REG_ADDR & MAP_MASK);
    printf("regs address base addr=0x%16.16x  virt_addr = 0x%16.16x\n", map_base, virt_addr);






    //CheckFpgaStatus
    if (CheckFpgaStatus(virt_addr))
    {
        if (munmap(map_base, MAP_SIZE) == -1) FATAL;
        close(memFd);
        return -1;

    }





    /*************/
    /* open FIFO */
    /*************/
#if AXIFIFO
    readFifoFd = open(_opt_dev_rx, O_RDONLY);
    writeFifoFd = open(_opt_dev_tx, O_WRONLY);

    if (readFifoFd < 0)
    {
        printf("Open %s read failed with error: %s\n", _opt_dev_rx, strerror(errno));
        FATAL_RETURN;
        return -1;
    }
    if (writeFifoFd < 0)
    {
        printf("Open %s write failed with error: %s\n", _opt_dev_tx, strerror(errno));
        FATAL_RETURN;
        return -1;
    }

#endif
    if (pipe(pipeFd) == -1)
    {
        printf("pipe failed with error: %s\n",  strerror(errno));
        FATAL_RETURN;
        return -1;
    }
    if (pipe(shutdown_pipeFd) == -1)
    {
        printf("shutdown pipe failed with error: %s\n",  strerror(errno));
        FATAL_RETURN;
        return -1;
    }


    //  printf("PL2 block Number %d\n", fpga_id);

    /**********************************************/
    /* Start TCP Server and wait for a connection */
    /**********************************************/
    opt = 1;
    addrlen = sizeof(servAddr);
    if (_opt_use_protocol == TCP_PROTOCOL)
    {
        if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            DEBUG_PRINT("socket failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if ((serverFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        cliaddrlen = sizeof(cliaddr);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        DEBUG_PRINT("setsockopt");
        exit(EXIT_FAILURE);
    }

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(_opt_sock_port);

    if (bind(serverFd, (struct sockaddr *)&servAddr,
             sizeof(servAddr)) < 0)
    {
        DEBUG_PRINT("bind failed");
        exit(EXIT_FAILURE);
    }


    if (_opt_use_protocol == TCP_PROTOCOL)
    {
        if (listen(serverFd, 3) < 0)
        {
            DEBUG_PRINT("listen");
            exit(EXIT_FAILURE);
        }

    }



    while (running)
    {

        if (_opt_use_protocol == TCP_PROTOCOL)
        {

            if ((tcpSocketFd = accept(serverFd, (struct sockaddr *)&servAddr,
                                      (socklen_t*)&addrlen)) < 0)
            {
                DEBUG_PRINT("accept");
                exit(EXIT_FAILURE);
            }
            else
            {
                DEBUG_PRINT("Accepted client Connection accepted from %s:%d\n", inet_ntoa(servAddr.sin_addr), ntohs(servAddr.sin_port));
                end_conection = false;

            }

        }

#if AXIFIFO
        reset_axi_fifo();




#endif
        /*****************/
        /* start threads */
        /*****************/

        /* start thread listening for ethernet packets */
        rc = pthread_create(&eth_rx_thread, NULL, ethn_rx_thread_fn,
                            (void *)NULL);

        /* start thread listening for fifo receive packets */
        rc = pthread_create(&fifo_rx_thread, NULL, fifo_rx_thread_fn,
                            (void *)NULL);



        /* perform noops */






//         if (socket_read_error || socket_write_error)
//         {
//             DEBUG_PRINT("Error %s socket...\n", socket_read_error ? "reading" : "writing");
//             goto ret;
//         }
//         sleep(1);

        rc = pthread_join(eth_rx_thread, &tret);
        if (rc != 0)
            DEBUG_PRINT( "can′t join with eth_rx_thread\n");
        rc = (long)tret;
        if (rc == 0)
        {
            quit();
        }
        if (rc == 1)
        {
            DEBUG_PRINT("End eth_rx_thread with %d\n", rc);
        }

        rc = pthread_join(fifo_rx_thread, &tret);
        if (rc != 0)
            DEBUG_PRINT("can′t join with fifo_rx_thread\n");
        rc = (long)tret;
        if (rc == 0)
        {
            quit();
        }
        DEBUG_PRINT("End fifo_rx_thread with %d\n", rc);

    }

ret:
    printf("SHUTTING DOWN\n");
#if AXIFIFO
    close(writeFifoFd);
    close(readFifoFd);
#endif
    if (munmap(map_base, MAP_SIZE) == -1) FATAL;
    close(memFd);
    if (munmap(map_base_reserved_memory, MAP_SIZE) == -1) FATAL;
    close(memFd_ddr);

    close(pipeFd[0]);
    close(pipeFd[1]);
    close(shutdown_pipeFd[0]);
    close(shutdown_pipeFd[1]);
    return rc;
}


static void quit(void)
{
    running = false;
}





int GetTempfileDescriptor(void)
{
    // buffer to hold the temporary file name
    char nameBuff[32];
    int filedes = -1, count = 0;

    // memset the buffers to 0
    memset(nameBuff, 0, sizeof(nameBuff));

    // Copy the relevant information in the buffers
    strncpy(nameBuff, "/tmp/myTmpFile-XXXXXX", 21);

    errno = 0;
    // Create the temporary file, this function will replace the 'X's
    filedes = mkostemp(nameBuff, O_RDWR);

    // Call unlink so that whenever the file is closed or the program exits
    // the temporary file is deleted
    unlink(nameBuff);

    if (filedes < 1)
    {
        DEBUG_PRINT("\n Creation of temp file failed with error [%s]\n", strerror(errno));
        return -1;
    }
    else
    {
        DEBUG_PRINT("\n Temporary file [%s] created\n", nameBuff);
    }

//     errno = 0;
    // Write some data to the temporary file
//     if(-1 == write(filedes,buffer,sizeof(buffer)))
//     {
//         printf("\n write failed with error [%s]\n",strerror(errno));
//         return 1;
//     }
//
//     printf("\n Data written to temporary file is [%s]\n",buffer);



    /*
        errno = 0;
        // rewind the stream pointer to the start of temporary file
        if(-1 == lseek(filedes,0,SEEK_SET))
        {
            printf("\n lseek failed with error [%s]\n",strerror(errno));
            return 1;
        }

        errno=0;
        // read the data from temporary file
        if( (count =read(filedes,buffer,11)) < 11 )
        {
            printf("\n read failed with error [%s]\n",strerror(errno));
            return 1;
        }

        // Show whatever is read
        printf("\n Data read back from temporary file is [%s]\n",buffer);*/

    return filedes;
}

static void *ethn_rx_thread_fn(void *data)
{
    uint8_t buf[_opt_max_bytes_eth + 10], *p_data_buf;
    int rc;
    int32_t bytesSock = 0, frag_bytesSock, total_bytesSock = 0;
    struct pollfd fds[2];
    int packetRead = 0;
    int fifoID, num_of_regs, num_of_data;
    //ssize_t bytesFifo;
    int32_t bytesFifo;
    int packets_rx, packets_tx;
    uint32_t reg_buf[_max_reg_long + 10];
    uint32_t offset_to_write;
    int log_fd = -1;
    int data_length = -1, need_to_get = _opt_max_bytes;

    /* shup up compiler */
    (void)data;

    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(fds, 0, sizeof(fds));
    if (_opt_use_protocol == TCP_PROTOCOL)
        fds[0].fd = tcpSocketFd;
    else
        fds[0].fd = serverFd;
    fds[0].events = POLLIN;
    fds[1].fd = shutdown_pipeFd[0];
    fds[1].events = POLLIN;
#if AXIFIFO
//    fds[2].fd = writeFifoFd;
//    fds[2].events = 0;//POLLOUT;
#endif

    packets_rx = 0;
    packets_tx = 0;
    fifoID = -1;
    while (running && (!end_conection))
    {
        rc = poll(fds, 2, -1);
//         DEBUG_PRINT("poll fds[1] = 0x%x fds[0] = 0x%x fds[2] = 0x%x rc= %d\n", fds[1].revents, fds[0].revents, fds[2].revents, rc);
        if (fds[1].revents & POLLIN)
        {
            continue;
        }

        if (rc > 0)
        {
            if (packetRead == 0 && (fds[0].revents & POLLIN))
            {

                if (_opt_use_protocol == TCP_PROTOCOL)
                {
                    frag_bytesSock = read(tcpSocketFd, &buf[total_bytesSock], _opt_max_bytes > need_to_get ? need_to_get : _opt_max_bytes);
                }
                else
                {
                    frag_bytesSock = recvfrom(serverFd, &buf[total_bytesSock], _opt_max_bytes,
                                              MSG_WAITALL, (struct sockaddr *)&cliaddr,
                                              &cliaddrlen);
                    udpInit = 1;
                }
                if (frag_bytesSock > 0)
                {

                    packets_rx++;
                    DEBUG_PRINT("bytes from socket %d\n", frag_bytesSock );
                }
                else if (frag_bytesSock == 0)
                {
                    DEBUG_PRINT("Connection lost\n");
                    socket_read_error = 1;
                    end_conection = true;
                    continue;

                }
                else
                {
                    total_bytesSock = 0;
                    perror("read");
                    socket_read_error = 1;
                    //FATAL;
                    quit();
                }
                total_bytesSock += frag_bytesSock;
                DEBUG_PRINT("total_bytesSock  %d %d\n", total_bytesSock, frag_bytesSock);
                frag_bytesSock = 0;
                if ((total_bytesSock > 10000) || (total_bytesSock < 0))
                    DEBUG_PRINT("total_bytesSock \n");
                rc = ProtocolCheck(buf, total_bytesSock, &fifoID, fpga_id, &data_length);
                if (rc  < 0)
                {
                    total_bytesSock = 0;
                    DEBUG_PRINT("Error protocol header %d \n", rc);
//                     for (uint16_t *z = (uint16_t *)buf, rc = 0; rc < total_bytesSock / 4;)
//                     {
//                         printf(" buf[rc] %2.2x %2.2x %2.2x %2.2x %2.2d - ", buf[rc++], buf[rc++], buf[rc++], buf[rc++], rc);
//                         printf(" Z %4.4x %4.4x\n", *z++, *z++);
//                     }
                    continue;
                }
                if (rc == 1)
                {
                    // DEBUG_PRINT("continue for more data %d \n", rc);
                    continue;
                }
                if (rc == 2)
                {
                    need_to_get = data_length - total_bytesSock;
                    DEBUG_PRINT("will try to get %d \n", need_to_get );
                    if (need_to_get < 0)
                        DEBUG_PRINT("over read %d \n", need_to_get );
                    if (need_to_get > 0)
                        continue;
                }
                // got all data
                packetRead = 1;
                bytesSock = total_bytesSock;
                total_bytesSock = 0;
                need_to_get = data_length = _opt_max_bytes;
            }
            //  DEBUG_PRINT("fifo ID %d\n",fifoID);
            //check protocol ? whitch fifo and regs we need to write
            if ((fifoID > MAX_FIFOS ) || ( fifoID < 0))
            {
                packetRead = 0;
                //DEBUG_PRINT("Get wrong fifo id %d num of Bytes %d\n", fifoID, bytesSock);
                fifoID = 0x7ffffff;
                //	    DEBUG_PRINT("fds[0].revents & POLLIN = %d & fds[1].revents & POLLOUT=%d \n",fds[0].revents & POLLIN,fds[1].revents & POLLOUT);

                continue;
            }
            if (packetRead == 1 && fifoID == 0)
            {
                DEBUG_PRINT("control command TBD \nn", fifoID);
                if (log_fd != -1 )
                {

                    if (-1 == write(pipeFd[1], &log_fd, sizeof(log_fd)))
                    {
                        DEBUG_PRINT("\n write failed with error [%s]\n", strerror(errno));
                        //         return 1;
                    }

                    DEBUG_PRINT(" Data written to pipe %d\n", log_fd);
                    log_fd = -1;

                }
                packetRead = 0;
            }
            /*
                        if (fifoID == 3 )
                        {
                            fds[1].events = POLLOUT;
                        }*/

            // need to check if fifo is ready ?????
//             if (packetRead == 1 && (!(fds[fifoID].revents & POLLOUT)) )
//             {
//                 DEBUG_PRINT("fifo is not ready \n", fifoID);
//                 virt_addr[0x40 / sizeof(uint32_t)] = 0;
//                 virt_addr[0x40 / sizeof(uint32_t)] = 1;
//
//                 DEBUG_PRINT("command addr 0x%8.8x= 0x%8.8x status 0x%8.8x\n", &virt_addr[0x40 / sizeof(uint32_t)], virt_addr[0x40 / sizeof(uint32_t)], virt_addr[0x44 / sizeof(uint32_t)]);
//                 packetRead = 0;buf
//
//             }
            if (packetRead == 1 )
            {

                if (log_fd == -1 )
                {
                    log_fd = GetTempfileDescriptor();
                    if (log_fd == -1)
                        DEBUG_PRINT("failed to get Temp file Descriptor\n");
                }
                else
                {
                    errno = 0;
                    // rewind the stream pointer to the start of temporary file
                    if (-1 == lseek(log_fd, 0, SEEK_SET))
                    {
                        DEBUG_PRINT("\n lseek failed with error [%s]\n", strerror(errno));

                    }

                }

                DEBUG_PRINT("handle Packet len %d\n", bytesSock);
                //get register from the data
                // DEBUG_PRINT("buf %x p_data_buf=%x *p_data_buf=%x\n", (uint32_t)buf, (uint32_t)&p_data_buf, (uint32_t)p_data_buf);
                packetRead = 0;
                packets_tx++;
                printf("PL block Number %d\n", fpga_id);

                memset(reg_buf, 0, sizeof(reg_buf));
                if (fifoID != 12)
                    virt_addr[0x10] = 0; // stop FPGA works
                // DEBUG_PRINT("virt_addr address %8.8x\n", (uint32_t)virt_addr);
                rc = GetGegister(fifoID, buf, bytesSock, &p_data_buf, &num_of_data, reg_buf, &num_of_regs, &offset_to_write, log_fd, virt_addr, virt_addr_reserved_memory);
                if (rc < 0)
                {
                    DEBUG_PRINT("GetGegister return %d\n", rc);
                    packetRead = 0;
                    continue;
                }

                //DEBUG_PRINT("write to address 0x%8.8x \n", (virt_addr + offset_to_write ));
                //DEBUG_PRINT("write to address 0x%8.8x num_of_regs= 0x%x\n", &(virt_addr[offset_to_write ]), num_of_regs);
                //DEBUG_PRINT("read command 0x%8.8x \n", virt_addr[0x40 / sizeof(uint32_t)]);
                if (num_of_regs >= 0)
                {
                    if (fifoID != 12)
                    {

                        DEBUG_PRINT("set to zero \n");
                        usleep(100);

                        virt_addr[0x10] = 0;
                    }
                    //write regs
                    DUPPRINT(log_fd, "copyed all regs from 0x%8.8x number 0x%8.8x 0x%8.8x \n", offset_to_write, num_of_regs, num_of_regs * sizeof(uint32_t));
                    for (int xx = 0 ; xx < num_of_regs; xx++)
                    {
                        virt_addr[offset_to_write + xx] = reg_buf[xx];
                        DUPPRINT(log_fd, "Regs virt_addr[0x%4.4x] = 0x%8.8x = %10.10d\n", (offset_to_write + xx)*sizeof(uint32_t), reg_buf[xx], reg_buf[xx]);
                    }


                    if (fifoID != 12)
                    {
                        usleep(100);
                        DEBUG_PRINT("set command 0x%8.8x \n", virt_addr[0x10]);
                        virt_addr[0x10] = 0;
                        usleep(100);
                        // write command go
                        DEBUG_PRINT("virt_addr address %8.8x\n", (uint32_t)virt_addr);
                        virt_addr[0x10 ] = 1;
                        DEBUG_PRINT("set command 0x%8.8x \n", (virt_addr[0x10]));
                    }
                }



                DEBUG_PRINT("GetGegister  num_of_data %d p_data_buf = 0x%x\n", num_of_data, p_data_buf);
                //write data to fifo
                if (num_of_regs <= -10)
                {
                    int send_matlab_fd = GetTempfileDescriptor();
                    int bytes_to_send = 24; //bytes for general header

                    if (num_of_regs == (-21))
                    {
                        //send ack with with reg 0 = 0xffff ps is not ready
                        rc = AddHeaderToData(fifoID, buf, bytes_to_send, &(virt_addr[0x10 ]), 9, -10);
                    }
                    if (num_of_regs <= (-22))
                    {
                        uint32_t *p;
                        uint32_t cc;
                        p = (virt_addr_reserved_memory );
                        p += (reg_buf[0] / 4); //convert to uint32 pointer
                        {
                            for (cc = 0; cc < reg_buf[1]; cc += 4, bytes_to_send += 4)
                            {
                                *(uint32_t *)(&buf[bytes_to_send]) = *p;
                                if (cc < 20)
                                    printf ("DDR data 0x%8.8x\n", *p);
                                p++;
                            }

                        }
                        //memcpy(&buf[bytes_to_send], p, reg_buf[1]);
                        //bytes_to_send+=reg_buf[1];
                        //send ack with data from the buffer
                        rc = AddHeaderToData(fifoID, buf, bytes_to_send, reg_buf, (num_of_regs == (-22)) ? 8 : 9, -11);


                    }

                    if (num_of_data == -10)
                    {
                        //send only ack  reg0=0
                        // bytes_to_send=
                        rc = AddHeaderToData(fifoID, buf, bytes_to_send, &(virt_addr[0x10 ]), 8, -12);


                    }
                    if (rc < 0)
                    {
                        DEBUG_PRINT("Error add the header  %d\n", rc);
                        continue;
                    }

                    if (-1 == write(send_matlab_fd, buf, bytes_to_send))
                    {
                        DEBUG_PRINT("\n write failed with error [%s]\n", strerror(errno));
                        //continue;
                        //         return 1;
                    }

                    //send ack to the matlab
                    if (-1 == write(pipeFd[1], &send_matlab_fd, sizeof(send_matlab_fd)))
                    {
                        DEBUG_PRINT("\n write failed with error [%s]\n", strerror(errno));
                        continue;
                        //         return 1;
                    }

                    continue;
                }
                if (num_of_data >  0)
                {

                    if (num_of_data % 4)
                    {
                        DEBUG_PRINT("number bytes to fifo is not divided by 4 without remainder %d\n", num_of_data);
                        DEBUG_PRINT("Skipe %d bytes\n", num_of_data);
                        continue;
                    }
#if AXIFIFO
                    need_check_feedback = 0;
                    if (fifoID == 12)
                    {
                        memcpy(&tuser, p_data_buf, sizeof(uint64_t));
                        need_check_feedback = 1;
                    }

                    bytesFifo = write(writeFifoFd, p_data_buf, num_of_data);
#endif
                    if (bytesFifo > 0)
                    {
                        DEBUG_PRINT("bytes to fifo %d\n", bytesFifo);
// 			usleep(2000);
//                         DEBUG_PRINT("Re send \n");
// 			bytesFifo = write(writeFifoFd, p_data_buf, num_of_data);
// 			DEBUG_PRINT("bytes to fifo %d\n", bytesFifo);


                    }
                    else
                    {
                        num_of_regs = (uint32_t)reg_buf;
                        DEBUG_PRINT("write to axi fifo return 0x%x\n", bytesFifo);
                        perror("write");
                        FATAL;
                        quit();
                    }
                }



            }
        }
    }

    printf("ethernet packets rx : %d, fifo packets tx     : %d\n", packets_rx, packets_tx);
    if  (end_conection == true)
        return ((void *)1);

    return (void *)0;
}

static void *fifo_rx_thread_fn(void *data)
{
    int rc = 0, rc2 = 0;
    ssize_t bytesSock;
    ssize_t bytesFifo;
    int packets_rx, packets_tx;
    uint8_t buf[_opt_max_bytes + 20], *p_data_buf;
    struct pollfd fds[4];
    int packetRead = 0;
    int header_place_saver = 24; //bytes for general header
    int FifoID, num_of_regs, num_of_data;
    uint32_t reg_buf[_max_reg_long + 10];
    int timeout_msecs = 500;
    int64_t echo_test = -1;
    int save_location;


    /* shup up compiler */
    (void)data;

    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(fds, 0, sizeof(fds));
    if (_opt_use_protocol == TCP_PROTOCOL)
        fds[0].fd = tcpSocketFd;
    else
        fds[0].fd = serverFd;
    fds[0].events = 0; // POLLOUT;
#if AXIFIFO
    fds[1].fd =  readFifoFd; //was writeFifoFd look like a bug change to  readFifoFd
    fds[1].events = POLLIN;
#endif
    fds[2].fd =  pipeFd[0];
    fds[2].events = POLLIN;
    fds[3].fd =  shutdown_pipeFd[0];
    fds[3].events = POLLIN;

    packets_rx = 0;
    packets_tx = 0;

    DEBUG_PRINT("fifo_rx_thread_fn start\n");
    while (running && (!end_conection))
//    while (running )
    {
        FifoID = 0xffff;
        if (rc2)
            DEBUG_PRINT("waiting to fifo RX\n");
        rc = poll(fds, 4, timeout_msecs);
        //   rc = poll(fds, 4, -1);
        rc2 = rc;
        if (rc < 0)
            DEBUG_PRINT("poll exit %d\n", rc);
        if (fds[3].revents & POLLIN)
        {
            continue;
        }
        if (rc > 0)
        {
#if AXIFIFO            // DEBUG_PRINT("rc > 0\n");
            if (packetRead == 0 && (fds[1].revents & POLLIN))
            {

                save_location = header_place_saver;
                if (need_check_feedback)
                {
                    save_location -= sizeof(uint64_t);
                }
                FifoID = 1;
                DEBUG_PRINT("try to read \n");


                bytesFifo = read(readFifoFd, &buf[save_location], _opt_max_bytes);
                //  DEBUG_PRINT("Error read %d %d,0x%8.8x %d\n", bytesFifo, readFifoFd, &buf[header_place_saver], _opt_max_bytes);
                if (bytesFifo > 0)
                {
                    packetRead = 1;
                    DEBUG_PRINT("bytes from fifo %d\n", bytesFifo);
                    packets_rx++;
                    if (need_check_feedback)
                    {

                        memcpy(&echo_test, &buf[save_location], sizeof(uint64_t));
                        if (echo_test != tuser)
                            DEBUG_PRINT("Error Tuser send %x get %x", tuser, echo_test );
                        bytesFifo -= sizeof(uint64_t);
                        echo_test >>= 24;

                    }
                }
                else
                {
                    DEBUG_PRINT("Got from Read %d", bytesFifo);
                    perror("read");
                    //FATAL;
                    quit();
                }
            }
#endif
            if (packetRead == 0 && (fds[2].events & POLLIN))
            {
                // got msg from other thread
                int log_fd_piped;
                bytesFifo = read(pipeFd[0], &log_fd_piped, sizeof(log_fd_piped));
                if (bytesFifo > 0)
                {
                    packetRead = 2;
                    DEBUG_PRINT("bytes from pipe %d\n", bytesFifo);

                    errno = 0;
                    if (log_fd_piped > 0)
                    {
                        // rewind the stream pointer to the start of temporary file
                        if (-1 == lseek(log_fd_piped, 0, SEEK_SET))
                        {
                            DEBUG_PRINT("\n lseek failed with error [%s]\n", strerror(errno));
                            return 1;
                        }

                        errno = 0;
                        // read the data from temporary file
                        bytesFifo = read(log_fd_piped, &buf[0], _opt_max_bytes);

                        if (bytesFifo >= 0)
                        {
                            // Show whatever is read
                            DEBUG_PRINT("\n Data read back from temporary file is %d bytes\n", bytesFifo);

                            // check if need to read more
                            if (bytesFifo == _opt_max_bytes)
                            {
                                //retringer
                                write(pipeFd[1], &log_fd_piped, sizeof(log_fd_piped));
                            }
                            else
                                close(log_fd_piped);
                        }
                        else
                        {
                            perror("read");
                            //FATAL;
                            quit();
                        }


                        //packets_rx++;
                    }
                    else
                    {
                        //not a file
                        DEBUG_PRINT("Error wrong file  %d\n", log_fd_piped);
//                         bytesFifo = header_place_saver;
//                         rc = AddHeaderToData(FifoID, buf, bytesFifo, &(virt_addr[0x10 ]), fpga_id);
//                         if (rc < 0)
//                         {
//                             DEBUG_PRINT("Error add the header  %d\n", rc);
//                             continue;
//                         }

                    }
                }
            }

            if (packetRead == 1 /*&& (fds[0].revents & POLLOUT)*/)
            {
                bytesFifo += header_place_saver;
                packetRead = 1;
                // DEBUG_PRINT("reg 0xf8  0x%8.8x \n", virt_addr[(0xf8) / sizeof(uint32_t)]);
                if (fpga_id == 12)
                    rc = AddHeaderToData(FifoID, buf, bytesFifo, &(virt_addr[0x20 ]), fpga_id, (int32_t)echo_test);

                else
                    rc = AddHeaderToData(FifoID, buf, bytesFifo, &(virt_addr[0x10 ]), fpga_id, (int32_t)echo_test);

                if (rc < 0)
                {
                    DEBUG_PRINT("Error add the header  %d\n", rc);
                    continue;
                }


//                 {
// #define PATH_MAX 300
//                     char file_name[PATH_MAX + 1];
//                     int tfd;
//                     get_file_name(file_name, PATH_MAX);
//
//                     if ((tfd = open(file_name, O_WRONLY | O_CREAT, 0644)) == -1)
//                     {
//                         perror("Cannot open output file\n"); FATAL;
//                     }
//                     write(tfd, buf, bytesFifo);
//                     close(tfd);
//
//
//                 }
            }
            //sprintf(buf, "Get a  msg number %d\n", packets_tx);
            if ((packetRead == 1) || (packetRead == 2 ))
            {
                packetRead = 0;
                if (_opt_use_protocol == TCP_PROTOCOL)
                {

                    bytesSock = write(tcpSocketFd, buf, bytesFifo);
                    //bytesSock = write(tcpSocketFd, buf, strlen(buf));

                    if (bytesSock > 0)
                    {
                        DEBUG_PRINT("bytes to socket %d\n", bytesSock);
                        packets_tx++;

                    }
                    else
                    {
                        perror("write");
                        socket_write_error = 1;
                        //FATAL;
                        return ((void *)1);
                    }
                }
                else if (_opt_use_protocol == UDP_PROTOCOL && udpInit)
                {
                    bytesSock = sendto(serverFd, buf, bytesFifo,
                                       MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
                                       cliaddrlen);
                    if (bytesSock > 0)
                    {
                        DEBUG_PRINT("bytes to socket %d\n", bytesSock);
                        packets_tx++;

                    }
                    else
                    {
                        perror("write");
                        socket_write_error = 1;
                        //FATAL;
                        quit();
                    }
                }
            }
        }
    }

    printf("fifo packets rx     : %d, ethernet packets tx : %d\n", packets_rx, packets_tx);

    if (end_conection)
        return ((void *)1);
    return (void *)0;
}

static void signal_handler(int signal)
{
    int s = 1;
    switch (signal)
    {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            running = false;
            if (-1 == write(shutdown_pipeFd[1], &s, sizeof(s)))
            {
                DEBUG_PRINT("\n write failed with error [%s]\n", strerror(errno));
                //         return 1;
            }

            shutdown(serverFd, SHUT_RD);
            break;

        default:
            break;
    }
}

static void display_help(char * progName)
{
    printf("Ver %s ,%s\n", __DATE__, __TIME__ );
    printf("Usage : %s [OPTIONS]\n"
           "\n"
           "  -h, --help     Print this menu\n"
           "  -d, --default  Use default sw \n"
           "  -t, --devTx    Device to use ... %s\n"
           "  -r, --devRx    Device to use ... %s\n"
           "  -b, --maxbytes Maximum number of bytes to expect in a packet\n"
           "  -c, --minbytes Minimum number of bytes to expect in a packet\n"
           "  -p, --port     Port number to bind to\n"
           "  -s, --save     save DDR to a file\n"
           "  -l, --load     load a text file to DDR\n"
           "  -i, --id       set FPGA id\n"
           "  -S, --size     size of DDR to load/save\n"
           "  -O, --offset   offset from the DDR  \n"
           "  -H, --hex      use file as text but Hex  \n"

           ,
           progName, _opt_dev_tx, _opt_dev_rx
          );
    printf(
        "  -f, --devMem   Device to use ... %s\n"
        "  -x, --protocol 0 for udp, 1 for tcp\n"
        , _opt_dev_mem);
    fflush(stdout);
}

static void print_opts()
{
    printf("Ver %s ,%s\n", __DATE__, __TIME__ );
    printf("Options : \n"
           "Port           : %d\n"
           "Max Bytes      : %d\n"
           "Min Bytes      : %d\n"
           "DevTX          : %s\n"
           "DevRx          : %s\n"
           ,
           _opt_sock_port,
           _opt_max_bytes,
           _opt_min_bytes,
           _opt_dev_tx,
           _opt_dev_rx
          );


    printf("DevMem         : %s\n"
           "Protocol       : %s\n"
           ,
           _opt_dev_mem,
           _opt_use_protocol ? "TCP" : "UDP"
          );

    fflush(stdout);
}

static int process_options(int argc, char * argv[])
{



    strncpy(_opt_dev_tx, DEF_DEV_TX, sizeof(_opt_dev_tx));
    strncpy(_opt_dev_rx, DEF_DEV_RX, sizeof(_opt_dev_rx));
    strncpy(_opt_dev_mem, DEF_DEV_MEM, sizeof(_opt_dev_mem));
    char filename_for_dump[50] = {NULL}, flga_as_file = 0;
    char as_hex = 0;
    uint32_t offset_add = 0, size_dump = 0;


    for (;;)
    {
        int option_index = 0;
        static const char *short_options = "hdr:t:c:b:p:x:f:l:s:i:S:O:H:";
        static const struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"devRx", required_argument, 0, 'r'},
            {"protocol", required_argument, 0, 'x'},
            {"devMem", required_argument, 0, 'f'},
            {"devTx", required_argument, 0, 't'},
            {"maxbytes", required_argument, 0, 'b'},
            {"minbytes", required_argument, 0, 'c'},
            {"port", required_argument, 0, 'p'},
            {"default", required_argument, 0, 'd'},
            {"save", required_argument, 2, 's'},
            {"load", required_argument, 1, 'l'},
            {"id", required_argument, 1, 'i'},
            {"size", required_argument, 1, 'S'},
            {"offset", required_argument, 1, 'O'},
            {"hex", no_argument, 0, 'H'},
            {0, 0, 0, 0},
        };

        int c = getopt_long(argc, argv, short_options,
                            long_options, &option_index);

        if (c == EOF)
        {
            break;
        }

        switch (c)
        {
            case 'i':
                sw_fpga_id = atoi(optarg);
                break;
            case 'H':
                as_hex = 1;
                break;
            case 'l':
                //load_ddr("IQ_DL.txt", offset_add, size_dump);
                //exit(0);
                sscanf(optarg, "%s", filename_for_dump);
                flga_as_file = 'l';

                break;
            case 'S':
            {
                sscanf(optarg, "%x", &size_dump);
                if ((flga_as_file == 0) || (flga_as_file == 'l') )
                    load_ddr(filename_for_dump, offset_add, size_dump, as_hex);
                else
                    read_ddr(filename_for_dump, offset_add, size_dump, as_hex);

                exit(0);
            }
            case 'O':
            {
                sscanf(optarg, "%x", &offset_add);
            }
            break;
            case 's':
            {
                sscanf(optarg, "%s", filename_for_dump);
                flga_as_file = 1;

            }

            break;
            case 't':
                strncpy(_opt_dev_tx, optarg, sizeof(_opt_dev_tx));
                break;

            case 'r':
                strncpy(_opt_dev_rx, optarg, sizeof(_opt_dev_rx));
                break;
            case 'f':
                strncpy(_opt_dev_mem, optarg, sizeof(_opt_dev_mem));
                break;
            case 'x':
                _opt_use_protocol = atoi(optarg);
                _opt_use_protocol = !!_opt_use_protocol;
                break;

            case 'c':
                _opt_min_bytes = atoi(optarg);
                break;

            case 'b':
                _opt_max_bytes = atoi(optarg);
                break;

            case 'p':
                _opt_sock_port = atoi(optarg);
                break;

            case 'd':
                break;

            default:
            case 'h':
                display_help(argv[0]);
                exit(0);
                break;
        }
    }

    print_opts();
    return 0;
}



#define wr_ddr(x)  (*mem_ddr_add)=x; \
		    mem_ddr_add++;



int load_ddr(char *filename, uint32_t offset_addr, uint32_t size_wr, char as_hex)
{

#if 0
    int symbol[14] = {2208, 2192, 2192, 2192, 2192, 2192, 2192, 2208, 2192, 2192, 2192, 2192, 2192, 2192};

    int frame_num = 0;
    int subfram_num = 0;
//    int slot;
    int symbol_num;
    int sample_num;
    uint32_t data_counter = 0;
    uint32_t memFd;
    uint32_t *mem_ddr_add;
    //test

    memFd = open(DEF_DEV_MEM, O_RDWR | O_SYNC);
    if (memFd < 0)
    {
        FATAL_RETURN;
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        return -1;
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE_T, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DDR_ADDDR  & ~MAP_MASK_T);
    if (map_base == (void *) -1)
    {
        close(memFd);
        printf("mmap failed with error: %s\n", strerror(errno));
        FATAL_RETURN;
    }
    virt_addr = map_base + (DDR_ADDDR & MAP_MASK_T);
    mem_ddr_add = virt_addr;

#endif


    uint32_t data_counter = 0;
    uint32_t memFd, i;
    uint32_t *mem_ddr_add;
    FILE *matlab;
    int err, b, data, scandata;

    //test

    memFd = open(DEF_DEV_MEM, O_RDWR | O_SYNC);
    if (memFd < 0)
    {
        FATAL_RETURN;
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        return -1;
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE_T, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DDR_ADDDR  & ~MAP_MASK_T);
    if (map_base == (void *) -1)
    {
        close(memFd);
        printf("mmap failed with error: %s\n", strerror(errno));
        FATAL_RETURN;
    }
    virt_addr = map_base + (DDR_ADDDR & MAP_MASK_T);
    mem_ddr_add = virt_addr;
    printf("virt_addr_reserved_memory 0x%x ,0x%x", map_base, virt_addr );

    if (filename[0] == NULL)
    {
        for (i = 0; i < size_wr; i++)
        {
            virt_addr[(offset_addr / sizeof(uint32_t)) + i] = 0;



        }
        goto exit_fill;
    }




#if 0
    {
        uint32_t symbol[14] = {2208, 2192, 2192, 2192, 2192, 2192, 2192, 2208, 2192, 2192, 2192, 2192, 2192, 2192};
        int subfram_num, symbol_num, sample_num, ant;
        data_counter = 0;
        for (ant = 0; ant < 4; ant++)
        {
            mem_ddr_add = virt_addr + ant * 0x130000;
            for (subfram_num = 0 ; subfram_num < 10 ; subfram_num ++)
            {
                for (symbol_num = 0 ; symbol_num < 14 ; symbol_num ++)
                {
                    wr_ddr(symbol[symbol_num]);
                    wr_ddr((subfram_num << 6) | symbol_num);
                    for (sample_num = 0 ; sample_num < symbol[symbol_num] ; sample_num = sample_num + 1)
                    {
                        wr_ddr(data_counter++);
                    }
                }
            }
        }
    }
#else



    printf("Start data\n\r");

    printf("open %s\n", filename);
    matlab = fopen(filename, "r");



    if (matlab < 0)
    {

        printf("Open %s failed with error: %s\n", filename, strerror(errno));
        return -1;
    }
    i = 0;
    while (1)
    {
        data = 0;
        //printf("scanf %d\n",i++);

        if (as_hex)
        {
            err = fscanf(matlab, "%x ", &scandata);
        }
        else
            err = fscanf(matlab, "%d ", &scandata);

        data = scandata;
        if ( err < 0 )
        {

            perror("fscanf");
            //FATAL;
            quit();
        }
        if ( feof(matlab) )
            break ;
        //printf("scanf %d\n",i++);
        if (as_hex)
        {
            err = fscanf(matlab, "%x ", &scandata);
        }
        else
            err = fscanf(matlab, "%d ", &scandata);

        scandata <<= 16;
        data = scandata + data;
        if ( err < 0 )
        {

            perror("fscanf");
            //FATAL;
            quit();
        }

        wr_ddr(data);
        i += 4;
        if ( feof(matlab) || i >= size_wr )
            break ;

    }
    fclose(matlab);
#endif
    printf("End data size 0x%x\n\r", i);
    close(memFd);
exit_fill:

    if (munmap(map_base, MAP_SIZE) == -1) FATAL;
    return 0;

}


void read_ddr(char *filename, uint32_t offset_addr, uint32_t size_wr, char as_hex)
{
    uint32_t data_counter = 0;
    uint32_t memFd;
    uint32_t *mem_ddr_add;
    int fileStream;
    int err;
    FILE *matlab;

    //test

    memFd = open(DEF_DEV_MEM, O_RDWR | O_SYNC);
    if (memFd < 0)
    {
        printf("Open %s mem failed with error: %s\n", _opt_dev_mem, strerror(errno));
        FATAL_RETURN;
        return -1;
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE_T, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, DDR_ADDDR  & ~MAP_MASK_T);
    if (map_base == (void *) -1)
    {
        close(memFd);
        printf("mmap failed with error: %s\n", strerror(errno));
        FATAL_RETURN;
    }
    virt_addr = map_base + (DDR_ADDDR & MAP_MASK_T);
    mem_ddr_add = virt_addr;

    //
    {
        char matlab_file[80];
        int loooop, lp;
        uint32_t mdata;
        strcpy(matlab_file, filename);
        strcat(matlab_file, ".txt");
        printf("dump to %s", matlab_file);

        matlab = fopen(matlab_file, "w");

        if (matlab < 0)
        {

            printf("Open %s failed with error: %s\n", matlab_file, strerror(errno));
            return -8;
        }
        for (loooop = 0; loooop < (size_wr / sizeof(uint32_t)); loooop++)
        {
            mdata = mem_ddr_add[((offset_addr / sizeof(uint32_t)) + loooop)];
            if (as_hex)
                fprintf(matlab, "%4.4x %4.4x\r\n", (mdata & 0xffff), ((mdata >> 16) & 0xffff));
            else
                fprintf(matlab, "%d\r\n%d\r\n", (mdata & 0xffff), ((mdata >> 16) & 0xffff));

        }
        fclose(matlab);
    }


    fileStream = open(filename,  O_RDWR | O_CREAT, 0666);

    if (fileStream != -1)
    {

        printf("writing to %s a size 0x%x\n", filename, size_wr);

        printf(" base %x offset_addr %x phy addr: %x size %x\n", DDR_ADDDR, offset_addr, (DDR_ADDDR + offset_addr), size_wr);
        printf(" mem_ddr_add %x offset_addr %x size %x\n", mem_ddr_add, &mem_ddr_add[offset_addr / sizeof(uint32_t)], size_wr);

        err = write(fileStream, &mem_ddr_add[offset_addr / sizeof(uint32_t)], size_wr);
        if (err <= 0)
        {
            printf("Open %s failed with error: %s\n", filename, strerror(errno));
            perror("write error");

        }
        close(fileStream);


    }
    else
    {
        printf("Open %s failed with error: %s\n", filename, strerror(errno));

    }

    return 0;

}
