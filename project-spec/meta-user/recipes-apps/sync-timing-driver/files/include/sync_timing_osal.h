/****************************************************************************************/
/**
 *  \defgroup porting SYNC TIMING DRIVER PORTING GUIDE
 *  @{
 *  \defgroup osal Operating System (OS) Abstraction Layer Porting
 *  @brief    This section defines the various OS Abstracted APIs that is used by the 
 *            Timing  Chipset driver. It is expected that the OEM will implement these 
 *            functions for their development platform as needed. A generic version for 
 *            Linux is made available by Sync.
 *  @{
 *  \defgroup osal_api OS Abstraction Layer APIs
 *  @{
 *
 *  \file          sync_timing_osal.h
 *
 *  \details       This file contains OSAL associated functions and data structures.
 *
 *  \date          Created: 06/27/2018
 *
 ****************************************************************************************/
 
/****************************************************************************************/
/**                  Copyright (c) 2018, 2021 Skyworks Solution Inc.                   **/
/****************************************************************************************/
/** This software is provided 'as-is', without any express or implied warranty.        **/
/** In no event will the authors be held liable for any damages arising from the use   **/
/** of this software.                                                                  **/
/** Permission is granted to anyone to use this software for any purpose, including    **/
/** commercial applications, and to alter it and redistribute it freely, subject to    **/
/** the following restrictions:                                                        **/
/** 1. The origin of this software must not be misrepresented; you must not claim that **/
/**    you wrote the original software. If you use this software in a product,         **/
/**    an acknowledgment in the product documentation would be appreciated but is not  **/
/**    required.                                                                       **/
/** 2. Altered source versions must be plainly marked as such, and must not be         **/
/**    misrepresented as being the original software.                                  **/
/** 3. This notice may not be removed or altered from any source distribution.         **/
/****************************************************************************************/

#ifndef _SYNC_TIMING_OSAL_H_
#define _SYNC_TIMING_OSAL_H_

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/
#if OS_LINUX
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <semaphore.h>
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <math.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/resource.h>
#include <sys/timerfd.h>
#include <assert.h>
#include <ifaddrs.h>
#include <termios.h>
#include <float.h>

#include <netinet/ether.h>

#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/filter.h>

#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/types.h>

#else
#error "No OS has been defined"
#endif

/*****************************************************************************************
    Macros
*****************************************************************************************/
#ifdef _DOXYGEN_
/* for internal Doygen generation */
#define OS_LINUX 1
#endif

#if OS_LINUX
/**
 *  \defgroup osal_ds OSAL Generic Definitions and Data Structures
 *  @brief  OSAL Generic Definitions and Data Structures for Linux OS
 *  @{
 */

/*! OSAL API Success - Returned by OSAL APIs when call is successful */
#define SYNC_TIMING_OSAL_SUCCESS                                0
/*! OSAL API Timeout - Returned by OSAL APIs when call times out */
#define SYNC_TIMING_OSAL_TIMEOUT                                1
/*! OSAL API Failure - Returned by OSAL APIs when call fails */
#define SYNC_TIMING_OSAL_ERR_NODATA                             2
/*! OSAL API undefined operation - Returned by OSAL APIs when call fails */
#define SYNC_TIMING_OSAL_ERR_CONTINUE                           3
/*! OSAL API Error Continue - Returned by OSAL APIs when call fails */
#define SYNC_TIMING_OSAL_ERR_INVALID_PARAM                      4
/*! OSAL API Error Invalid Parameter - Returned by OSAL APIs when call fails */

#define SYNC_TIMING_OSAL_FAIL                                   0xFFFFFFFF
/*! OSAL API undefined operation - Returned by OSAL APIs when call fails */
#define SYNC_TIMING_OSAL_UNDEFINED_OPERATION                    0xFFFFFFFE

/*! OSAL WAIT_FOREVER Definition for timeout based APIs */
#define SYNC_TIMING_OSAL_WAIT_FOREVER                           0xFFFFFFFF
/*! OSAL Millisecond per TICK - OS specific information */
#define SYNC_TIMING_OSAL_MS_PER_TICK                            (10)

/*! OSAL Read Only Flag */
#define SYNC_TIMING_OSAL_FLAGS_RD_ONLY                          O_RDONLY
/*! OSAL Write Only Flag */
#define SYNC_TIMING_OSAL_FLAGS_WR_ONLY                          O_WRONLY 
/*! OSAL Read-Write Flag */
#define SYNC_TIMING_OSAL_FLAGS_RDWR                             O_RDWR
/*! OSAL Create Flag */
#define SYNC_TIMING_OSAL_FLAGS_CREAT                            O_CREAT 
/*! OSAL Non-blocking Flag */
#define SYNC_TIMING_OSAL_FLAGS_NONBLOCK                         O_NONBLOCK
/*! OSAL Close on Exec Flag */
#define SYNC_TIMING_OSAL_FLAGS_CLOEXEC                          O_CLOEXEC


/*! Largest string that can be passed to Sync_Timing_OSAL_WrapperStartProgram */
#define SYNC_TIMING_OSAL_MAX_PROGSTRING                         256

/*! OSAL Socket Domain AF_PACKET */
#define SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET                  AF_PACKET

/*! OSAL Socket Domain PF_PACKET */
//#define SYNC_TIMING_OSAL_SOCK_DOMAIN_PF_PACKET                  PF_PACKET

/*! OSAL Socket Domain AF_INET */
#define SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET                    AF_INET

/*! OSAL Socket Domain AF_INET6 */
#define SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6                   AF_INET6




/*! OSAL Socket Type SOCK_DGRAM */
#define SYNC_TIMING_OSAL_SOCK_TYPE_DGRAM                        SOCK_DGRAM

/*! OSAL Socket Type SOCK_RAW */
#define SYNC_TIMING_OSAL_SOCK_TYPE_RAW                          SOCK_RAW




/*! OSAL Socket Protocol - Ethernet Slow Protocol */
#define SYNC_TIMING_OSAL_SOCK_PROTO_ETH_P_SLOW                  ETH_P_SLOW

/*! OSAL Socket Protocol - Ethernet All Protocol */
#define SYNC_TIMING_OSAL_SOCK_PROTO_ETH_P_ALL                   ETH_P_ALL

/*! OSAL Socket Protocol - Ethernet 1588 Protocol */
#define SYNC_TIMING_OSAL_SOCK_PROTO_ETH_P_1588                  ETH_P_1588




/*! OSAL Socket Option Level - IOCTL Call */
#define SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_IOCTL              -1

/*! OSAL Socket Option Level - SOL_SOCKET */
#define SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_SOCKET             SOL_SOCKET

/*! OSAL Socket Option Level - SOL_PACKET */
#define SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_PACKET             SOL_PACKET

/*! OSAL Socket Option Level - IPPROTO_IP */
#define SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_IPPROTO_IP         IPPROTO_IP

/*! OSAL Socket Option Level - IPPROTO_IPV6 */
#define SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_IPPROTO_IPV6       IPPROTO_IPV6

/*! OSAL Socket Option Enumerations */
typedef enum
{
    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_RCVTIMEO,
    //<! OSAL Socket Option Name - SO_RCVTIMEO

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_REUSEADDR,
    //<! OSAL Socket Option Name - SO_REUSEADDR    

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_REUSEPORT,
    //<! OSAL Socket Option Name - SO_REUSEPORT

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_BINDTODEVICE,
    //<! OSAL Socket Option Name - SO_BINDTODEVICE    

    SYNC_TIMING_OSAL_SOCKET_OPTION_PKT_ADD_MEMBERSHIP,
    //<! OSAL Socket Option Name - PACKET_ADD_MEMBERSHIP

    SYNC_TIMING_OSAL_SOCKET_OPTION_IP_ADD_MEMBERSHIP,
    //<! OSAL Socket Option Name - IP_ADD_MEMBERSHIP

    SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_LOOP,
    //<! OSAL Socket Option Name - IP_MULTICAST_LOOP

    SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_TTL,
    //<! OSAL Socket Option Name - IP_MULTICAST_TTL

    SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_IF,          
    //<! OSAL Socket Option Name - IP_MULTICAST_IF

    SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_ADD_MEMBERSHIP,      
    //<! OSAL Socket Option Name - IPV6_ADD_MEMBERSHIP

    SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_LOOP,      
    //<! OSAL Socket Option Name - IPV6_MULTICAST_LOOP

    SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_HOPS,      
    //<! OSAL Socket Option Name - IPV6_MULTICAST_HOPS

    SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_IF,        
    //<! OSAL Socket Option Name - IPV6_MULTICAST_HOPS

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_NO_CHECK,              
    //<! OSAL Socket Option Name - SO_NO_CHECK

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_ATTACH_FILTER,         
    //<! OSAL Socket Option Name - SO_ATTACH_FILTER

    SYNC_TIMING_OSAL_SOCKET_SIOCSHWTSTAMP,                   
    //<! OSAL Socket Option Name - SIOCSHWTSTAMP

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_TIMESTAMPING,          
    //<! OSAL Socket Option Name - SO_TIMESTAMPING

    SYNC_TIMING_OSAL_SOCKET_OPTION_SO_SELECT_ERR_QUEUE,
    //<! OSAL Socket Option Name - SO_SELECT_ERR_QUEUE

    SYNC_TIMING_OSAL_SOCKET_OPTION_MAX
    //<! OSAL Socket Option Max - used for comparison - donot use as input


} SYNC_TIMING_OSAL_SOCKET_OPTION_E;


/*! OSAL Socket Option Type - PACKET_MR_MULTICAST */
#define SYNC_TIMING_OSAL_SOCKET_MR_TYPE_PACKET_MR_MULTICAST     PACKET_MR_MULTICAST

/*! OSAL Socket Option Type - PACKET_MR_ALLMULTI */
#define SYNC_TIMING_OSAL_SOCKET_MR_TYPE_PACKET_MR_ALLMULTI      PACKET_MR_ALLMULTI

/*! OSAL Socket Option Type - PACKET_MR_PROMISC */
#define SYNC_TIMING_OSAL_SOCKET_MR_TYPE_PACKET_MR_PROMISC       PACKET_MR_PROMISC




/*! OSAL Socket Option Timestamp type - SOF_TIMESTAMPING_TX_HARDWARE */
#define SYNC_TIMING_OSAL_SOCKET_SOF_TIMESTAMPING_TX_HARDWARE    SOF_TIMESTAMPING_TX_HARDWARE

/*! OSAL Socket Option Timestamp type - SOF_TIMESTAMPING_RX_HARDWARE */
#define SYNC_TIMING_OSAL_SOCKET_SOF_TIMESTAMPING_RX_HARDWARE    SOF_TIMESTAMPING_RX_HARDWARE

/*! OSAL Socket Option Timestamp type - SOF_TIMESTAMPING_RAW_HARDWARE */
#define SYNC_TIMING_OSAL_SOCKET_SOF_TIMESTAMPING_RAW_HARDWARE   SOF_TIMESTAMPING_RAW_HARDWARE

/*! OSAL Socket Option Timestamp type - HWTSTAMP_TX_ONESTEP_SYNC */
#define SYNC_TIMING_OSAL_SOCKET_HWTSTAMP_TX_ONESTEP_SYNC        HWTSTAMP_TX_ONESTEP_SYNC

/*! OSAL Socket Option Timestamp type - HWTSTAMP_TX_ON */
#define SYNC_TIMING_OSAL_SOCKET_HWTSTAMP_TX_ON                  HWTSTAMP_TX_ON




/*! OSAL Socket Recv Flag - MSG_ERRQUEUE */
#define SYNC_TIMING_OSAL_SOCKET_MSG_ERRQUEUE                    MSG_ERRQUEUE

/*! OSAL Socket Recv Flag - MSG_DONTWAIT */
#define SYNC_TIMING_OSAL_SOCKET_MSG_DONTWAIT                    MSG_DONTWAIT




/*! OSAL Max Network Address (Eth/IPv4/IPv6) Length - set to 16 */
#define SYNC_TIMING_OSAL_MAX_ADDR_LENGTH                        16

/*! OSAL Ethernet Header Length - set to ETH_HLEN */
#define SYNC_TIMING_OSAL_ETH_HLEN                               ETH_HLEN

/*! OSAL Ethernet Address Length - set to ETH_ALEN */
#define SYNC_TIMING_OSAL_ETH_ALEN                               ETH_ALEN

/*! OSAL IPv4 Address Length - set to 4 */
#define SYNC_TIMING_OSAL_IPV4_ADDR_LEN                          4

/*! OSAL IPv6 Address Length - set to 16 */
#define SYNC_TIMING_OSAL_IPV6_ADDR_LEN                          16




/*! OSAL Clock Type REALTIME */
#define SYNC_TIMING_OSAL_CLOCK_REALTIME                         CLOCK_REALTIME

/*! OSAL Clock Type MONOTONIC */
#define SYNC_TIMING_OSAL_CLOCK_MONOTONIC                        CLOCK_MONOTONIC

/*! OSAL Max Token Length - set to 128 */
#define SYNC_TIMING_OSAL_MAX_TOKEN_LENGTH                       128




#else
#error "No OS has been defined"
#endif

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/
#if OS_LINUX
/*! OSAL Mutual Exclusion Object definition */
typedef pthread_mutex_t         SYNC_TIMING_OSAL_MUTEX_T;

/*! OSAL Semaphore Object definition */
typedef sem_t                   SYNC_TIMING_OSAL_SEM_T;

/*! OSAL Structure to hold thread information  */
typedef struct
{
    pthread_t  threadId;  //!< Thread Identifier
} SYNC_TIMING_OSAL_THREAD_T;

/*! OSAL Structure to hold Msg Queue Object information  */
typedef struct
{
    mqd_t                   MsgQId;        
    //!< Message Queue Identifier; Output when open is called; Input for all other APIs
    uint32_t                uMaxMsgSize;   
    //!< Maximum message size used in open call
    uint32_t                uMaxMsgs;      
    //!< Maximum number of messages in the queue used in open call
    uint32_t                uMode;         
    //!< Message Queue FD mode used in open call
    int32_t                 flags;         
    //!< Message Queue flags used in open call
} SYNC_TIMING_OSAL_MSG_QUEUE_T;

/*! OSAL Enumeration to define the SOCKET FILTER TYPE - used for RAW sockets  */
typedef enum
{
    SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_PTP_EVENT,
    //!< Filter Type PTP Event messages
    SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_PTP_GENERAL,
    //!< Filter Type PTP General messages
    SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_MAX
    //!< Filte type max - used for boundary checks - donot use as input
    
} SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_E;

/*! OSAL Ethernet Address Structure */
typedef struct
{
    uint8_t     addr[SYNC_TIMING_OSAL_ETH_ALEN];
} SYNC_TIMING_OSAL_ETHER_ADDR_T;

//typedef struct
//{
//    uint32_t    s_addr;
//} SYNC_TIMING_OSAL_IN_ADDR_T;

/*! OSAL IPV4 Address Structure */
typedef struct in_addr SYNC_TIMING_OSAL_IN_ADDR_T;

/*! OSAL IPV6 Address Structure */
typedef struct in6_addr SYNC_TIMING_OSAL_IN6_ADDR_T;

/*! OSAL Structure to hold Socket Object information  */
typedef struct
{
    int32_t                     sockId;        
    //!< Socket Identifier; Output when open is called; Input for all other APIs
    int32_t                     sockDomain;
    //!< Socket Family or domain - Input when open is called
    int32_t                     sockType;
    //!< Socket Type - Input when open is called
    int32_t                     sockProto;
    //!< Socket protocol - Input when open is called
    uint16_t                    sockPort;
    //!< Socket port if applicable - Input when open is called
    int8_t                      *pIfName;
    //!< Interface Name - Input when open is called
    uint8_t                     enableTS;
    //!< Enable/Disable TS - Input when open is called
    uint8_t                     bMulticast;
    //!< Multicast socket - Input when open is called
    uint32_t                    tsInfo;
    //!< TS enabling Information - Input when open is called
    uint32_t                    uIfIdx;
    //!< Interface Index;  Output when open is called; Input for all other APIs
    uint8_t                     uIfAddress[SYNC_TIMING_OSAL_ETH_ALEN]; 
    //!< Interface Address for RAW mode;  Output when open is called; Input for all other APIs
    SYNC_TIMING_OSAL_IN_ADDR_T  uInetAddress;
    //!< Interface Address for IPv4 mode;  Output when open is called; Input for all other APIs    
    SYNC_TIMING_OSAL_IN6_ADDR_T uInet6Address;
    //!< Interface Address for IPv6 mode;  Output when open is called; Input for all other APIs        
    void *                      pLastMsgRecd;
    //!< Storage to a pointer to the last message received.

} SYNC_TIMING_OSAL_SOCKET_T;

/*! OSAL Structure to hold Socket Bind information  */
typedef struct
{
    uint16_t            sockPort;           //!< Socket port to bind to
    uint32_t            bMulticastEnabled;  //!< multicast enabled or not

} SYNC_TIMING_OSAL_SOCKET_BIND_INFO_T;

/*! OSAL TIMEVAL Structure  */
typedef struct
{
    time_t              sec;        //!< seconds value
    suseconds_t         uSec;       //!< micro-seconds value

} SYNC_TIMING_OSAL_TIMEVAL_T;

/*! OSAL TIMEZONE Structure  */
typedef struct
{
    int32_t             minuteswestsec;         //!< minutes west of Greenwich
    int32_t             dsttime;                //!< type of DST correction

} SYNC_TIMING_OSAL_TIMEZONE_T;

/*! OSAL Socket Option Structure for adding Membership  */
typedef struct
{
    uint8_t             address[SYNC_TIMING_OSAL_MAX_ADDR_LENGTH];  
    //!< multicast address - ethernet (first 6 bytes) or ipv4 (first 4 bytes) or ipv6 (all 16 bytes)
    uint16_t            action;
    //!< action (if any - MR_MULTICAST or MR_ALLMULTI or MR_PROMISC or none)
    
} SYNC_TIMING_OSAL_SOCKET_OPTION_ADD_MEMBERSHIP_T;

/*! OSAL TIMESPEC Structure  */
typedef struct
{
    time_t              seconds;        //!< seconds value
    uint64_t            nanoseconds;    //!< nanoseconds value

} SYNC_TIMING_OSAL_TIMESPEC_T;

/*! OSAL STRING TOKENIZER Structure */
typedef struct 
{
    char*               str;
    //!< String to be tokenized
    char*               current_pos;
    //!< current position in the tokenization process
    char*               end_pos;
    //!< end position
    char*               delim;
    //!< Token delimiter
    bool                trim_leading;
    //!< Trim leading delimiters before tokenization
    char                token[SYNC_TIMING_OSAL_MAX_TOKEN_LENGTH + 1];
    //!< Token extracted last

} SYNC_TIMING_OSAL_STR_TOKENIZER_T;

/*! OSAL Interface Protocol type enumeration */
typedef enum
{
    SYNC_TIMING_OSAL_IF_PROTOCOL_RESERVED = 0,
    //!< Interface Protocol reserved    
    SYNC_TIMING_OSAL_IF_PROTOCOL_UDP_IPV4 = 1,
    //!< Interface Protocol is UDP/IPV4
    SYNC_TIMING_OSAL_IF_PROTOCOL_UDP_IPV6 = 2, 
    //!< Interface Protocol is UDP/IPV6
    SYNC_TIMING_OSAL_IF_PROTOCOL_IEEE802_3 = 3,
    //!< Interface Protocol is IEEE 802.3

} SYNC_TIMING_OSAL_IF_PROTOCOL_TYPE_E;

typedef struct
{
    SYNC_TIMING_OSAL_TIMESPEC_T timerInterval;
    SYNC_TIMING_OSAL_TIMESPEC_T initValue;
    
} SYNC_TIMING_OSAL_TIMERSPEC_T;

typedef struct
{
    int32_t                     fd;
  
} SYNC_TIMING_OSAL_TIMER_T;

#else
#error "No OS has been defined"
#endif

/** @} osal_ds */

#if OS_LINUX

#define OP_AND                                                  (BPF_ALU | BPF_AND | BPF_K)
#define OP_JEQ                                                  (BPF_JMP | BPF_JEQ | BPF_K)
#define OP_JUN                                                  (BPF_JMP | BPF_JA)
#define OP_LDB                                                  (BPF_LD  | BPF_B   | BPF_ABS)
#define OP_LDH                                                  (BPF_LD  | BPF_H   | BPF_ABS)
#define OP_RETK                                                 (BPF_RET | BPF_K)

#define ALL_MTU                                                 1536
#define AUX_LEN                                                 512

#define PTP_GEN_BIT                                             0x08 
                                            /* indicates general message, if set in message type */

#define N_RAW_FILTER                                            12
#define RAW_FILTER_TEST                                         9
    
#define EUI48                                                   6

#define MAC_LEN                                                 EUI48

#define VLAN_HLEN                                               4

typedef uint8_t eth_addr[MAC_LEN];

typedef struct sync_osal_eth_hdr 
{
    eth_addr dst;
    eth_addr src;
    uint16_t type;
} __attribute__((packed)) sync_osal_eth_hdr_t;

struct sync_osal_vlan_hdr 
{
    eth_addr dst;
    eth_addr src;
    uint16_t tpid;
    uint16_t tci;
    uint16_t type;
} __attribute__((packed));

typedef struct {
    char                name[SYNC_TIMING_OSAL_MAX_ADDR_LENGTH];
    uint32_t            index;
    uint32_t            channel;
    uint32_t            activated;
    struct ether_addr   mac_addr;
    struct in_addr      ipv4_addr;
    struct in6_addr     ipv6_addr;
    
} SYNC_TIMING_OSAL_INTERFACE_T;


#define SYNC_TIMING_OSAL_POLLFD             pollfd

#define SYNC_TIMING_OSAL_POLLPRI            POLLPRI
#define SYNC_TIMING_OSAL_POLLERR            POLLERR
#define SYNC_TIMING_OSAL_POLLHUP            POLLHUP
#define SYNC_TIMING_OSAL_POLLNVAL           POLLNVAL
#define SYNC_TIMING_OSAL_POLLIN             POLLIN

#define SYNC_TIMING_OSAL_FD_SET_T           fd_set
#define SYNC_TIMING_OSAL_FD_SET             FD_SET
#define SYNC_TIMING_OSAL_FD_CLR             FD_CLR
#define SYNC_TIMING_OSAL_FD_ZERO            FD_ZERO
#define SYNC_TIMING_OSAL_FD_ISSET           FD_ISSET

#define Sync_Timing_OSAL_Wrapper_Select     select

#define Sync_Timing_OSAL_Wrapper_Poll       poll

#define SYNC_TIMING_OSAL_HTONS              htons

#define SYNC_TIMING_OSAL_NTOHS              ntohs

#define SYNC_TIMING_OSAL_ETHER_NTOA(x)      ether_ntoa((struct ether_addr *)x)

#define SYNC_TIMING_OSAL_INET_NTOA          inet_ntoa

//#define SYNC_TIMING_OSAL_INET_NTOA6         sync_timing_osal_inet_ntoa6

#define SYNC_TIMING_OSAL_INET_ADDR          inet_addr

#define OFF_ETYPE                           (2 * sizeof(eth_addr))

#else
#error "No OS has been defined"
#endif


/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

/*****************************************************************************************
    Prototypes
*****************************************************************************************/
/**
 *  \defgroup osal_socket_api OSAL Socket
 *  @brief OSAL Socket Communication APIs
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_Open
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function opens a socket for the provided input parameters
 *
 * \param[in,out]   pOsalSocket  Pointer to the Socket object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_Open(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_Bind
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function binds the socket to a given port/address based on the parameters
 *
 * \param[in]       pOsalSocket          Pointer to the Socket object
 * \param[in]       pOsalSocketBindInfo  Socket Bind parameters to use
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_Bind(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket,
                                              SYNC_TIMING_OSAL_SOCKET_BIND_INFO_T *pOsalSocketBindInfo);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_SetOpt
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function sets a particular socket option to the given value
 *
 * \param[in]       pOsalSocket          Pointer to the Socket object
 * \param[in]       uOptionName          Socket option name enumeration
 * \param[in]       uOptionLevel         Socket option level
 * \param[in]       pOptionVal           Socket option value
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_SetOpt(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket,
                                                uint32_t uOptionName,
                                                uint32_t uOptionLevel,
                                                void *pOptionVal);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_RecvMsg
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to receive a message and timestamp (if enabled) on a 
 *                  given socket
 *
 * \param[in]       pOsalSocket          Pointer to the Socket object
 * \param[out]      pBuff                Pointer to buffer to store the msg in
 * \param[out]      uBuffSize            Size of the buffer provided
 * \param[out]      minRecvDataLen       Minimum data to be received on the socket
 * \param[out]      pSrcAddr             Pointer to return the Msg Src address
 * \param[in]       uSrcAddrLen          Length of the expected src address 
 * \param[out]      pActualRecvDataLen   Actual length of message received and put into the buffer
 * \param[in]       flags                Receive flags
 * \param[out]      pTs                  Pointer to TIMESPEC_T structure to return retrieved
 *                                       timestamp (if requested)
 * \param[out]      pbTsPresent          Indicates whether timestamp was present and extracted
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_RecvMsg(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket, 
                                                 uint8_t *pBuff, uint32_t uBuffSize,
                                                 uint32_t minRecvDataLen,
                                                 uint8_t *pSrcAddr, uint32_t uSrcAddrLen,
                                                 uint32_t *pActualRecvDataLen,
                                                 uint32_t flags,
                                                 SYNC_TIMING_OSAL_TIMESPEC_T *pTs,
                                                 uint32_t *pbTsPresent);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_Send
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function sends a raw message (with ethernet header) on a socket
 *
 * \param[in]       pOsalSocket         Pointer to the Socket object
 * \param[in]       pBuff               Buffer containing data to be sent
 * \param[in]       uBuffSize           Size of data to send
 * \param[in]       flags               Send flags
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_Send(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket,
                                                uint8_t *pBuff, uint32_t uBuffSize,
                                                uint32_t flags);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_SendTo
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function sends a message on a rawlink socket
 *
 * \param[in]       pOsalSocket         Pointer to the Socket object
 * \param[in]       pBuff               Buffer containing data to be sent
 * \param[in]       uBuffSize           Size of data to send
 * \param[in]       flags               Send flags
 * \param[in]       pDestAddr           Destination address to send the message
 * \param[in]       uDestAddrLen        Length of the destination address
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_SendTo(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket, 
                                                uint8_t *pBuff, uint32_t uBuffSize,
                                                uint32_t flags,
                                                uint8_t *pDestAddr, uint32_t uDestAddrLen);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Socket_Close
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function close an existing open socket
 *
 * \param[in]       pOsalSocket  Pointer to the Socket object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Socket_Close(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_Init
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function collects informations about the provided interface names or the 
 *                  existing interfaces when nothing is provided
 *
 * \param[in]       uNumInterfaces  Number of interfaces
 * \param[in]       pInterfaces     List of interface names
 * \param[out]      pInterfaceCount Enumerated interface count (valid ones only)
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Init(uint32_t uNumInterfaces, char **pInterfaces, 
                                                  uint32_t *pInterfaceCount);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_IsActive
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to determine if a particular interface is active
 *
 * \param[in]       uIfIdx          Interface index
 * \param[out]      pActive         Pointer to integer variable to say active (1) or not (0)
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_IsActive(uint32_t uIfIdx, uint32_t *pActive);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_Activate
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to activate a particular interface
 *
 * \param[in]       uIfIdx          Interface index
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Activate(uint32_t uIfIdx);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv4Address
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to determine if a particular interface has an IPv4 address
 *
 * \param[in]       uIfIdx          Interface index
 * \param[out]      pTrue           Pointer to integer variable to say TRUE (1) or FALSE (0)
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv4Address(uint32_t uIfIdx, uint32_t *pTrue);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv6Address
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to determine if a particular interface has an IPv6 address
 *
 * \param[in]       uIfIdx          Interface index
 * \param[out]      pTrue           Pointer to integer variable to say TRUE (1) or FALSE (0)
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv6Address(uint32_t uIfIdx, uint32_t *pTrue);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_GetName
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to get the interface name
 *
 * \param[in]       uIfIdx          Interface index
 * \param[out]      pName           Interface name
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetName(uint32_t uIfIdx, char **pName);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_GetChannel
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to get a particular interfaces' virtual channel number 
 *                  (typically index + 1)
 *
 * \param[in]       uIfIdx          Interface index
 * \param[out]      pChannel        Pointer to integer variable to return channel
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetChannel(uint32_t uIfIdx, int32_t *pChannel);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_GetPhysicalAddr
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to get a particular interfaces' physical address
 *
 * \param[in]       uIfIdx              Interface index
 * \param[out]      pPhysicalAddr       Physical address value
 * \param[out]      pAddrSize           Physical address size
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetPhysicalAddr(uint32_t uIfIdx,                                                              
                                                             uint8_t *pPhysicalAddr,
                                                             uint16_t *pAddrSize);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_GetProtocolAddr
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to get a particular interfaces' protocol address
 *
 * \param[in]       uIfIdx              Interface index
 * \param[in]       protoAddrType       Protocol address type; 
 *                                      Refer to SYNC_TIMING_OSAL_IF_PROTOCOL_TYPE_E
 * \param[out]      pProtocolAddr       Protocol address value
 * \param[out]      pProtocolAddrSize   Protocol address size
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetProtocolAddr(uint32_t uIfIdx, 
                                                 SYNC_TIMING_OSAL_IF_PROTOCOL_TYPE_E protoAddrType, 
                                                 uint8_t *pProtocolAddr,
                                                 uint16_t *pProtocolAddrSize);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Interfaces_Term
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/10/2019
 *//**
 * 
 * \brief           This function is used to terminate the interface enumeration data
 *
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Term();


/** @} osal_socket_api */

char *sync_timing_osal_inet_ntoa6(const void *addr);

/**
 *  \defgroup osal_msgq_api OSAL Message Queue
 *  @brief OSAL Message Queue handling APIs
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_Open
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function opens a message queue for the provided input parameters
 *
 * \param[in]       pName       The Message Queue Name
 * \param[in,out]   pOsalQueue  Pointer to the Message Queue object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Open(const char *pName, 
                                            SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_Close
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function closes a message queue for the provided input parameters
 *
 * \param[in]       pOsalQueue  Pointer to the Message Queue object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Close(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_Destroy
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     08/06/2018
 *//**
 * 
 * \brief           This function destroys a message queue for the provided input parameters
 *
 * \param[in]       pName       The Message Queue Name
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Destroy(const char *pName);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_Send
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function is used for sending a message 
 *
 * \param[in]       pOsalQueue    Pointer to the Message Queue object
 * \param[in]       pMsg          Pointer to the message that needs to be sent
 * \param[in]       uMsgLen       Length of the message that needs to be sent
 * \param[in]       uMsgPrio      Priority of the message
 * \param[in]       uMsgTimeoutMs Message Send Timeout (NOT SUPPORTED CURRENTLY)
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Send(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue, 
                                            void *pMsg, uint32_t uMsgLen, 
                                            uint32_t uMsgPrio, uint32_t uMsgTimeoutMs);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_Recv
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function receives a message from the queue
 *
 * \param[in]       pOsalQueue    Pointer to the Message Queue object
 * \param[in]       pMsg          Pointer to a buffer in which the message needs to be stored
 * \param[in]       uMsgLenRecv   Length of the message buffer
 * \param[out]      pActualMsgLen Length of the message received
 * \param[out]      pMsgPrio      Priority of the message received
 * \param[in]       uMsgTimeoutMs Message Recv Timeout 
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Recv(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue, 
                                            void *pMsg, uint32_t uMsgLenRecv,
                                            uint32_t *pActualMsgLen, uint32_t *pMsgPrio, 
                                            uint32_t uMsgTimeoutMs);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_MsgQ_CleanupUnused
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     08/29/2018
 *//**
 * 
 * \brief           This function cleans up unused message queues
 *
 * \param[in]       pPrgmName    Program Name

 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_CleanupUnused(const char *pPrgmName);

/** @} osal_msgq_api */

/**
 *  \defgroup osal_mutex_api OSAL Mutex
 *  @brief OSAL Mutex handling APIs
 *  @{
 */
/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Mutex_Create
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function creates a mutex object
 *
 * \param[in]       pMutexName       The Mutex Name
 * \param[out]      ppMutex          Pointer to the Mutex Object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Create(char *pMutexName, void **ppMutex);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Mutex_Delete
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function deletes the mutex object
 *
 * \param[in]       pMutex          Pointer to the Mutex Object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Delete(void *pMutex);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Mutex_Get
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function obtains a lock on the mutex object
 *
 * \param[in]       pMutex          Pointer to the Mutex Object
 * \param[in]       timeoutMS       Timeout for obtaining the mutex lock
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Get(void *pMutex, uint32_t timeoutMS);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Mutex_Put
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function releases the lock on the mutex object
 *
 * \param[in]       pMutex          Pointer to the Mutex Object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Put(void *pMutex);
/** @} osal_mutex_api */

/**
 *  \defgroup osal_semaphore_api OSAL Semaphores
 *  @brief OSAL Semaphore handling APIs
 *  @{
 */

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Sem_Init
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/30/2019
 *//**
 * 
 * \brief           This function initializes an un-named semaphore object
 *
 * \param[in]       pSem           Pointer to the semaphore Object
 * \param[in]       initialCount   Initial value of the semaphore 
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Sem_Init(SYNC_TIMING_OSAL_SEM_T *pSem, uint32_t initialCount);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Sem_Destroy
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     07/30/2019
 *//**
 * 
 * \brief           This function destroys a semaphore object
 *
 * \param[in]       pSem          Pointer to the Semaphore Object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Sem_Destroy(SYNC_TIMING_OSAL_SEM_T *pSem);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Sem_Get
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function obtains a lock on the semaphore object
 *
 * \param[in]       pSem            Pointer to the Semaphore Object
 * \param[in]       timeoutMS       Timeout for obtaining the semaphore lock
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Sem_Get(void *pSem, uint32_t timeoutMS);

/***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Sem_Put
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function releases the lock on the semaphore object
 *
 * \param[in]       pSem          Pointer to the Semaphore Object
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Sem_Put(void *pSem);
/** @} osal_semaphore_api */

/**
 *  \defgroup osal_thread_api OSAL Thread
 *  @brief OSAL Thread handling APIs
 *  @{
 */
 
 /***************************************************************************************
 * FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Thread_Create
 *
 * AUTHOR           Srini Venkataraman
 *
 * DATE CREATED     06/29/2018
 *//**
 * 
 * \brief           This function instantiates and starts a thread
 *
 * \param[out]      pThread       Pointer to the thread object SYNC_TIMING_OSAL_THREAD_T
 * \param[in]       pThreadName   Thread Name 
 * \param[in]       func          Starting function of the thread
 * \param[in]       param         Input param for the thread's starting function
 * \param[in]       pStack        Pointer to the thread's stack (NOT SUPPORTED CURRENTLY)
 * \param[in]       stackSize     Stack Size
 * \param[in]       uPriority     Priority of the thread
 *
 * \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
 *
 * ___
 ***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Thread_Create(SYNC_TIMING_OSAL_THREAD_T *pThread, char *pThreadName,
                                       void (*func)(uint32_t), void *param,
                                       void *pStack, uint32_t stackSize,
                                       uint32_t uPriority);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Thread_Terminate
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function terminates a thread;
*
* \param[in]       pThread       Pointer to the thread object SYNC_TIMING_OSAL_THREAD_T
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Thread_Terminate(SYNC_TIMING_OSAL_THREAD_T *pThread);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Thread_WaitForTerm
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     07/16/2019
*//**
* 
* \brief           This function waits for a thread to terminate;
*
* \param[in]       pThread       Pointer to the thread object SYNC_TIMING_OSAL_THREAD_T
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Thread_WaitForTerm(SYNC_TIMING_OSAL_THREAD_T *pThread);

/** @} osal_thread_api */

/**
 *  \defgroup osal_std_c_api OSAL Standard C Library 
 *  @brief OSAL Standard C Library APIs
 *  @{
 */

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Malloc
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function allocates memory of the given size and returns a pointer 
*                  to the memory
*
* \param[in]       size       Size of the memory desired
*
* \returns         Pointer to the allocated memory
*
* ___
***************************************************************************************/
void * Sync_Timing_OSAL_Wrapper_Malloc(uint32_t size);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Calloc
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/08/2018
*//**
* 
* \brief           This function allocates memory of the given size, sets it all to 0 
*                  and returns a pointer to the memory
*
* \param[in]       size       Size of the memory desired
*
* \returns         Pointer to the allocated memory
*
* ___
***************************************************************************************/
void * Sync_Timing_OSAL_Wrapper_Calloc(uint32_t size);


/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Free
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function frees the allocated memory
*
* \param[in]       pBuf       Pointer to the allocated memory
*
* \returns         None
*
* ___
***************************************************************************************/
void   Sync_Timing_OSAL_Wrapper_Free(void *pBuf);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Memset
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function initializes the specifies memory to value specified
*
* \param[in]       pSrc       Pointer to the allocated memory
* \param[in]       setVal     Value to use for initializing the memory
* \param[in]       len        Length of the memory to initialize
*
* \returns         None
*
* ___
***************************************************************************************/
void * Sync_Timing_OSAL_Wrapper_Memset(void *pSrc, uint8_t setVal, uint32_t len);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Memcpy
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function is used to copy data from one memory region to another
*
* \param[in]       pDest      Pointer to the destination memory
* \param[in]       pSrc      Pointer to the destination memory
* \param[in]       len        Length of the memory to copy
*
* \returns         Pointer to the pDest memory
*
* ___
***************************************************************************************/
void * Sync_Timing_OSAL_Wrapper_Memcpy(void *pDest, const void *pSrc, uint32_t len);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Memcmp
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function is used to compare data from one memory region to another
*
* \param[in]       pSrc1      Pointer to the source memory 1
* \param[in]       pSrc2      Pointer to the source memory 2
* \param[in]       len        Length of the memory to compare
*
* \returns         An integer less than, equal to, or greater than zero if the first len
*                  bytes of pSrc1 is found, respectively, to be less than, to match, 
*                  or be greater than the first n bytes of pSrc2
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_Memcmp(const void *pSrc1, const void *pSrc2, uint32_t len);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetLine
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/05/2021
*//**
*
* \brief           This function reads an entire line from a stream, up to and
*                  including the next newline character.
*
* \param[in]       pStr      Pointer to the line read by the function
* \param[in]       n         Size in bytes of the memory, pointed by pStr.
* \param[in]       stream    The input stream, from where to read the line. Ex: stdin
*
* \returns         The number of characters read, excluding terminating NULL character.
*
* ___
***************************************************************************************/
size_t Sync_Timing_OSAL_Wrapper_GetLine(char **pStr, size_t *n, FILE *stream);


/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Getopt_Long
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/026/2021
*//**
*
* \brief           This function parses the command-line arguments.
*
* \param[in]       argc         argument count
* \param[in]       argv         arguments array
* \param[in]       optstring    string containing the legitimate option characters
* \param[in]       longopts     pointer to the first element of an array of "struct option" declared
                                    in <getopt.h>
* \param[in]       longindex    points to a variable which is set to the index of the long option 
                                    relative to longopts.
*
* \returns         If an option was successfully found, then getopt() returns the option character.
*
* ___
***************************************************************************************/
size_t Sync_Timing_OSAL_Wrapper_Getopt_Long(int argc, char * const argv[],
           const char *optstring, const struct option *longopts, int *longindex);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Getcwd
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/026/2021
*//**
*
* \brief           This function gets current working directory.
*
* \param[out]       buf     Pointer to array containing absolute path of the current working directory.
* \param[in]        size    length of absolute pathname.
*
* \returns         pointer to a string containing the pathname of the current working directory.
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_Getcwd(char *buf, size_t size);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Access
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/026/2021
*//**
*
* \brief           This function checks whether the calling process can access the file with 
                    permissions listed in mode parameter.
*
* \param[out]       pFilePath    Pointer containing file path.
* \param[in]        mode         Access modes of the file.
*
* \returns         0 on success, -1 for failure.
*
* ___
***************************************************************************************/
int Sync_Timing_OSAL_Wrapper_Access(const char *pFilePath, int mode);


/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Strlen
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     07/25/2018
*//**
* 
* \brief           This function is used to determine length of a given NULL terminated string
*
* \param[in]       pSrc      Pointer to the source string
*
* \returns         An unsigned integer equal to the length of pSrc excluding the
*                  terminating null byte 
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Strlen(const char *pSrc);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strtol
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/05/2021
*//**
* 
* \brief           This function is used to convert a string to a long integer
*
* \param[in]       pSrc      Pointer to the input string
* \param[out]      pRet      Can be passed as NULL by caller.
*                               - If not NULL, this function stores the address of the
*                                   first invalid character in *pSrc.
*                               - If there were no valid digits at all in pSrc, this function
*                                   stores the original value of pSrc in *pRet (and returns 0).
*                               - If *pSrc is not '\0' but **pRet is '\0' on return, 
*                                   then the entire string is valid. 
*
* \param[in]       base      Number base to be used for conversion (10 for dec & 16 for Hex)
*
* \returns         A long integer converted from input string.
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Strtol(const char *pSrc, char **pRet, int base);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Atoi
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/26/2021
*//**
* 
* \brief           This function is used to convert a string to an integer.
                        Almost equals to Sync_Timing_OSAL_Wrapper_Strtol(pSrc, NULL, 10);
*
* \param[in]       pSrc      Pointer to the input string
*
* \returns         An integer converted from input string.
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Atoi(const char *pSrc);


/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strcpy
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/13/2018
*//**
* 
* \brief           This function is used to copy from a source string into a destination string
*
* \param[in]       pDest     Pointer to the destination string
* \param[in]       pSrc      Pointer to the source string
*
* \returns         Pointer to the resulting string 
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_Strcpy(char *pDest, const char *pSrc);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strdup
*
* AUTHOR           Venu Byravarasu
*
* DATE CREATED     03/26/2021
*//**
* 
* \brief           This function creates a duplicate string
*
* \param[in]       pSrc      Pointer to the input string
*
* \returns         Pointer to the duplicate string created.
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_Strdup(const char *pSrc);


/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strcat
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/13/2018
*//**
* 
* \brief           This function is used to append a source string into a destination string
*
* \param[in]       pDest     Pointer to the destination string
* \param[in]       pSrc      Pointer to the source string
*
* \returns         Pointer to the resulting string 
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_Strcat(char *pDest, const char *pSrc);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strstr
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/14/2018
*//**
* 
* \brief           This function finds  the  first  occurrence of the substring pStrToFind 
*                  in the string pSuperStr.
*
* \param[in]       pSuperStr     Pointer to the big super string
* \param[in]       pStrToFind    Pointer to the string to the find
*
* \returns         Pointer to the resulting string 
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_Strstr(const char *pSuperStr, const char *pStrToFind);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_Strcmp
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/16/2018
*//**
* 
* \brief           This function compares the 2 strings and returns an integer less than, equal to, 
*                  or greater than zero, respectively, if s1 is found to be less than, to match, 
*                  or be greater than s2
*
* \param[in]       pStr1    Pointer to the String 1
* \param[in]       pStr2    Pointer to the String 2
*
* \returns         Integer value indicating appropriate str comparison match result.
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_Strcmp(const char *pStr1, const char *pStr2);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StrTokenizer_Init
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/21/2019
*//**
* 
* \brief           This initializes the string tokenizer with the given values
*
* \param[in]       me            String Tokenizer instance to initialize
* \param[in]       str           Pointer to the String to be tokenized
* \param[in]       delim         string delimiter
* \param[in]       trim_leading  set to true to trim leading delimiters
*
* \returns         None
*
* ___
***************************************************************************************/
void Sync_Timing_OSAL_Wrapper_StrTokenizer_Init(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me, 
                                                       char* str, char* delim,
                                                       bool trim_leading );
/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/21/2019
*//**
* 
* \brief           This functions indicates if there are any more tokens left
*
* \param[in]       me       String Tokenizer instance to use
*
* \returns         1 if Tokens remain; 0 if there are no more tokens left
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/21/2019
*//**
* 
* \brief           This function provides next string token in the tokenizer instance
*
* \param[in]       me       String Tokenizer instance to use
*
* \returns         Pointer to the next String token
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/21/2019
*//**
* 
* \brief           This function is used to remove leading spaces from a given string
*
* \param[in]       str      Pointer to the String
*
* \returns         Pointer to the string with leading spaces removed
*
* ___
***************************************************************************************/
char* Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces( char* str );

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetProgramName
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     07/25/2018
*//**
* 
* \brief           This function is used to get the program name
*
* \returns         Program invocation name 
*
* ___
***************************************************************************************/
const char* Sync_Timing_OSAL_Wrapper_GetProgramName();

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetShortProgramName
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     07/25/2018
*//**
* 
* \brief           This function is used to get the short program name
*
* \returns         Program invocation name 
*
* ___
***************************************************************************************/
const char* Sync_Timing_OSAL_Wrapper_GetShortProgramName();

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetProgramId
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/14/2018
*//**
* 
* \brief           This function is used to get the program ID
*
* \returns         Program ID 
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_GetProgramId();

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetProgramStatus
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/27/2018
*//**
* 
* \brief           This function is used to determine if a program is still running or not
*
* \param[in]       pPrgmName    Program Name
*
* \returns         1 = running or 0 = not running
*
* ___
***************************************************************************************/
uint32_t  Sync_Timing_OSAL_Wrapper_GetProgramStatus(const char *pPrgmName);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetProgramStatusFromId
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     08/27/2018
*//**
* 
* \brief           This function is used to determine if a program is still running or not
*
* \param[in]       ProgramId    Program ID
*
* \returns         1 = running or 0 = not running
*
* ___
***************************************************************************************/
uint32_t  Sync_Timing_OSAL_Wrapper_GetProgramStatusFromId(uint32_t ProgramId);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetUnusedProgramId
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     09/06/2018
*//**
* 
* \brief           This function is used to get any unused program Id for a particular program
*
* \param[in]       pPrgmName    Program Name
*
* \returns         inactive Program Id or 0 if nothing left
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_GetUnusedProgramId(const char *pPrgmName);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StartProgram
*
* AUTHOR           Nick Zajerko-McKee
*
* DATE CREATED     05/04/2021
*//**
* 
* \brief           This function executes a program to run in a sperate process.
*
* \param[in]       pPrgmName    Program Name
* \param[in]       pArgs        Program Arguments
*
* \returns         0 = OK, otherwise the program return status
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_StartProgram(const char *pPrgmName, const char *pArgs);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_StopProgram
*
* AUTHOR           Nick Zajerko-McKee
*
* DATE CREATED     05/04/2021
*//**
* 
* \brief           This function terminates a given process
*
* \param[in]       pPrgmName    Program Name
*
* \returns         0 = OK, otherwise the program return status
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_StopProgram(const char *pPrgmName);

/** @} osal_std_c_api */

/**
 *  \defgroup osal_time_api OSAL Standard Time APIs
 *  @brief OSAL Standard Time APIs
 *  @{
 */
/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_SleepMS
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function is used to make the calling thread sleep for the 
*                  specified milliseconds
*
* \param[in]       milliSeconds      Time to sleep in milliseconds
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_SleepMS(uint32_t milliSeconds);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_USleep
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/29/2018
*//**
* 
* \brief           This function is used to make the calling thread sleep for the 
*                  specified microseconds
*
* \param[in]       useconds      Time to sleep in microseconds
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_USleep(uint32_t useconds);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_NanoSleep
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     06/26/2019
*//**
* 
* \brief           This function is used to make the calling thread sleep for the 
*                  specified nanoseconds
*
* \param[in]       pTimeout      Time to sleep in seconds and nanoseconds
* \param[in]       rem           Remaining time (when signal interrupted)
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_NanoSleep(SYNC_TIMING_OSAL_TIMESPEC_T *pTimeout, 
                                           SYNC_TIMING_OSAL_TIMESPEC_T *rem);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_ClockGetTime
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/06/2019
*//**
* 
* \brief           This function is used to get the time for specified clock Id
*
* \param[in]       clockId      Clock ID (REALTIME, MONOTONIC, etc.)
* \param[out]      pTime        Pointer to the TIMESPEC object that contains the time
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_ClockGetTime(uint32_t clockId, SYNC_TIMING_OSAL_TIMESPEC_T *pTime);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_ClockSetTime
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/06/2019
*//**
* 
* \brief           This function is used to set the time for specified clock Id
*
* \param[in]       clockId      Clock ID (REALTIME, MONOTONIC, etc.)
* \param[in]       pTime        Pointer to the TIMESPEC object that contains the time
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_ClockSetTime(uint32_t clockId, SYNC_TIMING_OSAL_TIMESPEC_T *pTime);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_GetTimeOfDay
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     02/06/2019
*//**
* 
* \brief           This function is used to current get the time of day value
*
* \param[in]       pTV      Current Time Value
* \param[in]       pTZ      Current Time Zone
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_GetTimeOfDay(SYNC_TIMING_OSAL_TIMEVAL_T *pTV, 
                                               SYNC_TIMING_OSAL_TIMEZONE_T *pTZ);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_TimerCreate
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/18/2019
*//**
* 
* \brief           This function is used to create a timer object
*
* \param[in]       pTimer      Timer object handle; Memory provided by the user. 
*                              Used to store timer information used by the OSAL layer
* \param[in]       clockId    Clock Id based on which timer is desired 
*                              (SYNC_TIMING_OSAL_CLOCK_MONOTONIC or SYNC_TIMING_OSAL_CLOCK_REALTIME) 
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_TimerCreate(SYNC_TIMING_OSAL_TIMER_T *pTimer, uint32_t clockId);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_TimerSetTime
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/18/2019
*//**
* 
* \brief           This function is used to arm or disarm the timer object
*
* \param[in]       pTimer      Timer object handle;
* \param[in]       bAbsolute   Absolute timer (1) or relative timer (0 - default)
* \param[in]       pNewVal     New Timer value to use; Set all fields to 0 to disarm timer.
* \param[out]      pOldVal     If not NULL, then current timer setting value is returned
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_TimerSetTime(SYNC_TIMING_OSAL_TIMER_T     *pTimer, 
                                               uint32_t                     bAbsolute,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pNewVal,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pOldVal);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_TimerGetTime
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/18/2019
*//**
* 
* \brief           This function is used to get the current timer setting
*
* \param[in]       pTimer      Timer object handle;
* \param[out]      pCurrVal    current timer setting value is returned in this field.
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_TimerGetTime(SYNC_TIMING_OSAL_TIMER_T *pTimer,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pCurrVal);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_TimerWaitForExpiry
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/18/2019
*//**
* 
* \brief           This function is used to wait for timer expiry; 
*                  Blocks until next expiry if no expiry has happened. 
*                  Else returns the number of expirations.
*
* \param[in]       pTimer           Timer object handle;
* \param[out]      pNumExpirations  Number of expirations
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_TimerWaitForExpiry(SYNC_TIMING_OSAL_TIMER_T *pTimer, 
                                                     uint64_t *pNumExpirations);

/***************************************************************************************
* FUNCTION NAME    Sync_Timing_OSAL_Wrapper_TimerDestroy
*
* AUTHOR           Srini Venkataraman
*
* DATE CREATED     10/18/2019
*//**
* 
* \brief           This function is used to disarm and destroy the timer object
*
* \param[in]       pTimer           Timer object handle;
*
* \returns         SYNC_TIMING_OSAL_SUCCESS, SYNC_TIMING_OSAL_FAILURE
*
* ___
***************************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_TimerDestroy(SYNC_TIMING_OSAL_TIMER_T *pTimer);


/** @} osal_time_api */
/** @} osal_api */
/** @} osal */
/** @} porting */

#endif // _SYNC_TIMING_OSAL_H_

