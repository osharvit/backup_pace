/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_osal.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 06/27/2018
 *
 * DESCRIPTION        : OSAL Wrapper Implementation
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

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_osal.h"
#include "sync_timing_log.h"

/*****************************************************************************************
    Macros
*****************************************************************************************/

#define OSAL_DBG_OUT 0
#define OSAL_INFO_OUT 1

#define  OSAL_DEBUG_PRINT(...) do\
{ \
if (OSAL_DBG_OUT) SYNC_TIMING_DEBUG(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  OSAL_ERROR_PRINT(...) do\
{ \
SYNC_TIMING_ERROR(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  OSAL_ALWAYS_PRINT(...) do\
{ \
SYNC_TIMING_ALWAYS(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)

#define  OSAL_INFO_PRINT(...) do\
{ \
if (OSAL_INFO_OUT) SYNC_TIMING_INFO3(SYNC_TIMING_LOG_DEFAULT_HANDLE,  __VA_ARGS__); \
}while(0)


/*****************************************************************************************
* Static global variables
*****************************************************************************************/

extern char *program_invocation_name;
extern char *program_invocation_short_name;

#if OS_LINUX
uint32_t    uMsgQ_rlimit_set = 0;

static struct sock_filter raw_filter[N_RAW_FILTER] = {
    {OP_LDH,  0, 0, OFF_ETYPE               },
    {OP_JEQ,  0, 4, ETH_P_8021Q             }, /*f goto non-vlan block*/
    {OP_LDH,  0, 0, OFF_ETYPE + 4           },
    {OP_JEQ,  0, 7, ETH_P_1588              }, /*f goto reject*/
    {OP_LDB,  0, 0, SYNC_TIMING_OSAL_ETH_HLEN + VLAN_HLEN    },
    {OP_JUN,  0, 0, 2                       }, /*goto test general bit*/
    {OP_JEQ,  0, 4, ETH_P_1588              }, /*f goto reject*/
    {OP_LDB,  0, 0, SYNC_TIMING_OSAL_ETH_HLEN                },
    {OP_AND,  0, 0, PTP_GEN_BIT             }, /*test general bit*/
    {OP_JEQ,  0, 1, 0                       }, /*0,1=accept event; 1,0=accept general*/
    {OP_RETK, 0, 0, 1500                    }, /*accept*/
    {OP_RETK, 0, 0, 0                       }, /*reject*/
};

static SYNC_TIMING_OSAL_INTERFACE_T *pInterfaceList = NULL;

static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_HWTS_Supported(char    *interface_name, 
                                                                          int32_t family);
static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_Valid(struct ifaddrs *ifa);
static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_Supported(struct ifaddrs *ifa, 
                                                                     uint32_t       uNumInterfaces, 
                                                                     char           **pInterfaces);
#endif

/*****************************************************************************************
* Functions
*****************************************************************************************/

/******************************************************************************
 *      Socket Services
 ******************************************************************************/

uint32_t Sync_Timing_OSAL_Wrapper_Socket_Open(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket)
{
#if OS_LINUX
    struct ifreq        ifr;
    int8_t              ret_value = 0;
    struct ifaddrs      *ifaddr, *ifa;

    if (!pOsalSocket)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET: 
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:            
            /** create socket */
            OSAL_ALWAYS_PRINT("%s: %s\n", pOsalSocket->pIfName,
                   (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET) ? \
                            "SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET" : "SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6");
            pOsalSocket->sockId = socket(pOsalSocket->sockDomain, pOsalSocket->sockType, 
                                         0);
            if(pOsalSocket->sockId < 0)
            {
                OSAL_ERROR_PRINT("IPv4:IPv6 - Cannot create AF_PACKET socket !!! Error = %s\n",
                        strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            
            /* Retrieve network interface index */
            memset(&ifr, 0, sizeof(ifr));
            memcpy(ifr.ifr_name, (char *)pOsalSocket->pIfName, IFNAMSIZ);
            ret_value = ioctl(pOsalSocket->sockId, SIOCGIFINDEX, &ifr);
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("IPv4:IPv6 - Cannot get interface index for %s: %s\n",
                        pOsalSocket->pIfName, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            pOsalSocket->uIfIdx = ifr.ifr_ifindex;
            
            /* Retrieve hardware address */
            if (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET)
            {
                memset(&ifr, 0, sizeof(ifr));
                memcpy(ifr.ifr_name, (char *)pOsalSocket->pIfName, IFNAMSIZ);
                ret_value = ioctl(pOsalSocket->sockId, SIOCGIFADDR, &ifr);
                if( ret_value < 0 )
                {
                    OSAL_ERROR_PRINT("IPV4: Cannot get interface address for %s: %s\n",
                        pOsalSocket->pIfName, strerror(errno));
                    return SYNC_TIMING_OSAL_FAIL;
                }

                pOsalSocket->uInetAddress.s_addr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
            }
            else
            {
                // Get list of all interfaces on the system
                if (getifaddrs(&ifaddr) < 0) 
                {
                    OSAL_ERROR_PRINT("IPV6: Cannot get all the interface addresses (%s)\n", strerror(errno));
                    return SYNC_TIMING_OSAL_FAIL;
                }
                
                for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
                 {
                    if (strcmp(ifa->ifa_name, (char *)pOsalSocket->pIfName) == 0 &&
                        ifa->ifa_addr->sa_family == AF_INET6) 
                    {
                        memcpy(&pOsalSocket->uInet6Address,
                               ((struct sockaddr_in6*) ifa->ifa_addr)->sin6_addr.s6_addr, 
                               SYNC_TIMING_OSAL_IPV6_ADDR_LEN);
                        break;
                    }
                }
                
                freeifaddrs(ifaddr);
                if (ifa == NULL)
                {
                     OSAL_ERROR_PRINT("IPV6: Cannot get interface address for %s: %s\n",
                         pOsalSocket->pIfName, strerror(errno));
                     return SYNC_TIMING_OSAL_FAIL;
                }

            }
            break;
            
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
        //case SYNC_TIMING_OSAL_SOCK_DOMAIN_PF_PACKET:
            /** create socket */
            pOsalSocket->sockId = socket(pOsalSocket->sockDomain, pOsalSocket->sockType, 
                                         htons(pOsalSocket->sockProto));
            if(pOsalSocket->sockId < 0)
            {
                OSAL_ERROR_PRINT("Cannot create AF_PACKET socket !!! Error = %s\n",
                        strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            
            /* Retrieve network interface index */
            memset(&ifr, 0, sizeof(ifr));
            memcpy(ifr.ifr_name, (char *)pOsalSocket->pIfName, IFNAMSIZ);
            ret_value = ioctl(pOsalSocket->sockId, SIOCGIFINDEX, &ifr);
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("Cannot get interface index for %s: %s\n",
                        pOsalSocket->pIfName, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            pOsalSocket->uIfIdx = ifr.ifr_ifindex;
            
            /* Retrieve hardware address */
            memset(&ifr, 0, sizeof(ifr));
            memcpy(ifr.ifr_name, (char *)pOsalSocket->pIfName, IFNAMSIZ);
            ret_value = ioctl(pOsalSocket->sockId, SIOCGIFHWADDR, &ifr);
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("RAW: Cannot get interface address for %s: %s\n",
                    pOsalSocket->pIfName, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            memcpy(&pOsalSocket->uIfAddress[0], ifr.ifr_hwaddr.sa_data, ETH_ALEN);
            break;

        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_Socket_Bind(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket,
                                              SYNC_TIMING_OSAL_SOCKET_BIND_INFO_T *pOsalSocketBindInfo)
{
#if OS_LINUX
    struct sockaddr_ll  src_addr_ll;
    int8_t              ret_value = 0;
    struct sockaddr_in  src_addr_in;
    struct sockaddr_in6 src_addr_in6;

    if (!pOsalSocket)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
        //case SYNC_TIMING_OSAL_SOCK_DOMAIN_PF_PACKET:
            /** Bind to specified interface */
            src_addr_ll.sll_family = SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET;
            src_addr_ll.sll_protocol = htons(pOsalSocket->sockProto);
            src_addr_ll.sll_ifindex = pOsalSocket->uIfIdx;
            
            ret_value = bind(pOsalSocket->sockId, (struct sockaddr *) &src_addr_ll, sizeof(src_addr_ll) );
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("RAW: Cannot bind socket to interface %s: %s\n",
                        pOsalSocket->pIfName, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET:
            /** bind source address/port */
            src_addr_in.sin_family      = AF_INET;
            src_addr_in.sin_port        = htons( pOsalSocketBindInfo->sockPort );

            if( pOsalSocketBindInfo->bMulticastEnabled ) 
            {
                src_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
            } 
            else 
            {
                src_addr_in.sin_addr.s_addr = pOsalSocket->uInetAddress.s_addr;
            }

            ret_value = bind( pOsalSocket->sockId, (struct sockaddr *) &src_addr_in, 
                              sizeof(src_addr_in) );
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("MC: Cannot bind AF_INET socket to port %d: %s\n",
                        pOsalSocketBindInfo->sockPort, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }            
            break;
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:
            /** bind source address/port */
            src_addr_in6.sin6_family        = AF_INET6;
            src_addr_in6.sin6_port          = htons( pOsalSocketBindInfo->sockPort );

            #if 1
            if( pOsalSocketBindInfo->bMulticastEnabled ) 
            {
                src_addr_in6.sin6_addr          = in6addr_any;
                src_addr_in6.sin6_scope_id      = 0;
            } 
            else 
            {
                OSAL_INFO_PRINT("IPV6: in6addr_any: %s\n", sync_timing_osal_inet_ntoa6(&in6addr_any));
                OSAL_INFO_PRINT("IPV6: Bind to address: %s\n", sync_timing_osal_inet_ntoa6(&pOsalSocket->uInet6Address));
                inet_pton(AF_INET6, sync_timing_osal_inet_ntoa6(&pOsalSocket->uInet6Address), 
                          (void *)&src_addr_in6.sin6_addr);
                //src_addr_in6.sin6_addr = pOsalSocket->uInet6Address;
                //memcpy(&src_addr_in6.sin6_addr, &pOsalSocket->uInet6Address, 
                //       sizeof(struct in6_addr));
                src_addr_in6.sin6_scope_id      = pOsalSocket->uIfIdx;
            }
            #endif

            src_addr_in6.sin6_flowinfo      = 0;
            

            ret_value = bind( pOsalSocket->sockId, (struct sockaddr *) &src_addr_in6, 
                              sizeof(src_addr_in6) );
            if( ret_value < 0 )
            {
                OSAL_ERROR_PRINT("IPV6: Cannot bind AF_INET6 socket to port %d: %s\n",
                        pOsalSocketBindInfo->sockPort, strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }            
            break;

        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_Socket_SetOpt(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket,
                                                uint32_t uOptionLevel,
                                                uint32_t uOptionName,
                                                void *pOptionVal)
{
#if OS_LINUX
    struct packet_mreq                                  pkt_mreq;
    struct ip_mreqn                                     ip_mreq;
    struct ipv6_mreq                                    ipv6_mreq;
    int8_t                                              ret_value = 0;
    SYNC_TIMING_OSAL_TIMEVAL_T                          *rto;
    SYNC_TIMING_OSAL_SOCKET_OPTION_ADD_MEMBERSHIP_T     *pmreq;
    SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_E               socketFilterType;
    int32_t                                             filter_test;
    struct sock_fprog                                   prg = { N_RAW_FILTER, raw_filter };
    struct ifreq                                        req;
    struct hwtstamp_config                              cfg;
    int32_t                                             flags;    
    struct timeval                                      recvTO;
    int32_t                                             optVal = 1;
    struct in_addr                                      mcast_addr, if_addr;
    struct in6_addr                                     mcastv6_addr;
    
    if (!pOsalSocket)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET:
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:    
            switch(uOptionName)
            {
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_RCVTIMEO:
                    rto = (SYNC_TIMING_OSAL_TIMEVAL_T *)pOptionVal;
                    recvTO.tv_sec = rto->sec;
                    recvTO.tv_usec = rto->uSec;

                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, SO_RCVTIMEO, 
                                           (char *) &recvTO, 
                                           sizeof(recvTO));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot set receive timeout on socket !!! (%s) \n",
                                strerror(errno)); 
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_PKT_ADD_MEMBERSHIP:
                    pmreq = (SYNC_TIMING_OSAL_SOCKET_OPTION_ADD_MEMBERSHIP_T *)pOptionVal;
                    memset(&pkt_mreq, 0, sizeof(pkt_mreq));
                
                    pkt_mreq.mr_ifindex = pOsalSocket->uIfIdx;
                    pkt_mreq.mr_type = pmreq->action;
                    if (pmreq->action == SYNC_TIMING_OSAL_SOCKET_MR_TYPE_PACKET_MR_MULTICAST)
                    {   
                        pkt_mreq.mr_alen = ETH_ALEN;
                        memcpy(pkt_mreq.mr_address, pmreq->address, ETH_ALEN);
                    }
                    else
                    {
                        pkt_mreq.mr_alen = 0;
                    }
                    
                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, PACKET_ADD_MEMBERSHIP, 
                                           (char *) &pkt_mreq, sizeof(pkt_mreq));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot join multicast group on socket !!! %s\n", strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_ATTACH_FILTER:
                    socketFilterType = *((SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_E *)(pOptionVal));
                    
                    filter_test = RAW_FILTER_TEST;
                    if (socketFilterType == SYNC_TIMING_OSAL_SOCKET_FILTER_TYPE_PTP_EVENT) 
                    {
                        raw_filter[filter_test].jt = 0;
                        raw_filter[filter_test].jf = 1;
                    } 
                    else 
                    {
                        raw_filter[filter_test].jt = 1;
                        raw_filter[filter_test].jf = 0;
                    }

                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, SO_ATTACH_FILTER, 
                                           &prg, sizeof(prg));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot attach packet filter on the socket !!! %s\n", strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_SIOCSHWTSTAMP:
                    if (uOptionLevel == SYNC_TIMING_OSAL_SOCKET_OPTION_LEVEL_IOCTL)
                    {
                        memset(&req, 0, sizeof(req));
                        memset(&cfg, 0, sizeof(cfg));

                        memcpy(req.ifr_name, (char *)pOsalSocket->pIfName, IFNAMSIZ);
                        req.ifr_data    = (void *) &cfg;
                        cfg.rx_filter   = HWTSTAMP_FILTER_ALL;
                        cfg.tx_type     = *((int32_t *)(pOptionVal));

                        
                        if (ioctl(pOsalSocket->sockId, SIOCSHWTSTAMP, &req) < 0)
                        {
                            OSAL_ERROR_PRINT("Cannot enable hardware timestamping on the socket !!! %s\n", 
                                    strerror(errno));
                            return SYNC_TIMING_OSAL_FAIL;
                        }
                    }
                    else
                    {
                        OSAL_ERROR_PRINT("Undefined OSAL operation on socket !!! \n");
                        return SYNC_TIMING_OSAL_UNDEFINED_OPERATION;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_TIMESTAMPING:
                    flags = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, SO_TIMESTAMPING, 
                                   &flags, sizeof(flags)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot enable timestamping on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_SELECT_ERR_QUEUE:
                    flags = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, SO_SELECT_ERR_QUEUE, 
                                   &flags, sizeof(flags)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot enable error queue on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_REUSEADDR:
                    optVal = *((int32_t *)(pOptionVal));
                    OSAL_INFO_PRINT("Setsockopt SO_REUSEADDR\n");
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, SO_REUSEADDR, 
                                   &optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot reuse address on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }                    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_REUSEPORT:
                    optVal = *((int32_t *)(pOptionVal));
                    OSAL_INFO_PRINT("Setsockopt SO_REUSEPORT\n");
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, SO_REUSEPORT, 
                                   &optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot reuse port on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_BINDTODEVICE:
                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, SO_BINDTODEVICE, 
                                           (char *)pOsalSocket->pIfName, 
                                           strlen((char *)pOsalSocket->pIfName));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot bind socket to device %s: %s\n",
                                pOsalSocket->pIfName, strerror(errno) );
                        return SYNC_TIMING_OSAL_FAIL;
                    }                    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IP_ADD_MEMBERSHIP:
                    pmreq = (SYNC_TIMING_OSAL_SOCKET_OPTION_ADD_MEMBERSHIP_T *)pOptionVal;
                    memset(&ip_mreq, 0, sizeof(ip_mreq));

                    memcpy(&mcast_addr, &(pmreq->address[0]), SYNC_TIMING_OSAL_IPV4_ADDR_LEN);

                    ip_mreq.imr_multiaddr.s_addr = mcast_addr.s_addr;
                    ip_mreq.imr_address.s_addr = pOsalSocket->uInetAddress.s_addr;
                    ip_mreq.imr_ifindex = pOsalSocket->uIfIdx;
                    
                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, IP_ADD_MEMBERSHIP, 
                                           (char *) &ip_mreq, sizeof(ip_mreq));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot join IPV4 multicast group on socket !!! %s\n", strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_LOOP:
                    optVal = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IP_MULTICAST_LOOP, 
                                   (char *)&optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set ip multicast loop setting on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_TTL:
                    optVal = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IP_MULTICAST_TTL, 
                                   (char *)&optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set ip multicast ttl on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IP_MULTICAST_IF:
                    if_addr.s_addr = pOsalSocket->uInetAddress.s_addr;
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IP_MULTICAST_IF, 
                                   (char *)&if_addr, sizeof(if_addr)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set ip multicast if on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }                       
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_SO_NO_CHECK:
                    optVal = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, SO_NO_CHECK, 
                                   &optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot disable outgoing UDP checksum on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_ADD_MEMBERSHIP:
                    pmreq = (SYNC_TIMING_OSAL_SOCKET_OPTION_ADD_MEMBERSHIP_T *)pOptionVal;
                    memset(&ipv6_mreq, 0, sizeof(ipv6_mreq));
                
                    memcpy(mcastv6_addr.s6_addr, &(pmreq->address[0]), SYNC_TIMING_OSAL_IPV6_ADDR_LEN);
                
                    ipv6_mreq.ipv6mr_multiaddr = mcastv6_addr;
                    ipv6_mreq.ipv6mr_interface = pOsalSocket->uIfIdx;
                
                    ret_value = setsockopt(pOsalSocket->sockId, uOptionLevel, IPV6_ADD_MEMBERSHIP, 
                                           (char *) &ipv6_mreq, sizeof(ipv6_mreq));
                    if( ret_value < 0 )
                    {
                        OSAL_ERROR_PRINT("Cannot join IPv6 multicast group on socket !!! %s\n", 
                               strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_LOOP:
                    optVal = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IPV6_MULTICAST_LOOP, 
                                   (char *)&optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set IPv6 multicast loop setting on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_HOPS:
                    optVal = *((int32_t *)(pOptionVal));
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IPV6_MULTICAST_HOPS, 
                                   (char *)&optVal, sizeof(optVal)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set IPv6 multicast ttl on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }    
                    break;
                case SYNC_TIMING_OSAL_SOCKET_OPTION_IPV6_MULTICAST_IF:
                    if (setsockopt(pOsalSocket->sockId, uOptionLevel, IPV6_MULTICAST_IF, 
                                   (char *)&pOsalSocket->uIfIdx, sizeof(pOsalSocket->uIfIdx)) < 0)
                    {
                        OSAL_ERROR_PRINT("Cannot set IPv6 multicast if on socket : %s\n",
                                strerror(errno));
                        return SYNC_TIMING_OSAL_FAIL;
                    }                       
                    break;

                default:
                    OSAL_ERROR_PRINT("Unsupported SOCKET option name %u\n", uOptionName);
                    return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Socket_RecvMsg(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket, 
                                                 uint8_t *pBuff, uint32_t uBuffSize,
                                                 uint32_t minRecvDataLen,
                                                 uint8_t *pSrcAddr, uint32_t uSrcAddrLen,
                                                 uint32_t *pActualRecvDataLen,
                                                 uint32_t flags,
                                                 SYNC_TIMING_OSAL_TIMESPEC_T *pTs,
                                                 uint32_t *pbTsPresent)
{
#if OS_LINUX
    struct msghdr       msg;
    struct iovec        iov                 = {pBuff, uBuffSize};
    struct sockaddr_ll  src_ll;
    struct sockaddr_in  src_in;
    struct sockaddr_in6 src_in6;
    uint8_t             *pControl           = NULL;
    int8_t              ret_value           = 0;
    struct cmsghdr      *cm                 = NULL;
    struct timespec     *ts                 = NULL;
    uint8_t             splRecvLenHandling  = 0;

    if (!pOsalSocket || !pActualRecvDataLen || !pBuff)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET:
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:    
            if ((flags & SYNC_TIMING_OSAL_SOCKET_MSG_ERRQUEUE) != 0)
            {
                memset(&msg, 0, sizeof(msg));

                iov.iov_base = pBuff;
                iov.iov_len = ALL_MTU;
        
                msg.msg_iov         = &iov;
                msg.msg_iovlen      = 1;
                msg.msg_control     = pBuff + ALL_MTU;
                msg.msg_controllen  = AUX_LEN;

                splRecvLenHandling = 1;
            }
            else
            {
                if (pOsalSocket->enableTS)
                {
                    pControl = malloc(AUX_LEN);  // AUX_LEN = 512 - TBD
                }

                memset(&src_ll, 0, sizeof(src_ll));
                memset(&src_in, 0, sizeof(src_in));
                memset(&msg, 0, sizeof(msg));
                if (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET)
                {
                    msg.msg_name        = &src_ll;
                    msg.msg_namelen     = sizeof(src_ll);
                }
                else if (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET)
                {
                    msg.msg_name        = &src_in;
                    msg.msg_namelen     = sizeof(src_in);
                }
                else // AF_INET6
                {
                    msg.msg_name        = &src_in6;
                    msg.msg_namelen     = sizeof(src_in6);
                }
                msg.msg_iov         = &iov;
                msg.msg_iovlen      = 1;
                msg.msg_flags       = 0;
                msg.msg_control     = pControl;
                msg.msg_controllen  = msg.msg_control ? AUX_LEN : 0;
            }

            ret_value = recvmsg(pOsalSocket->sockId, &msg, flags);
            if( ret_value < 0 )
            {
                //printf("Cannot receive data (%s)\n", strerror(errno));
                ret_value = errno;
                *pActualRecvDataLen = 0;
                if ( ret_value == ENOMSG || ret_value == EAGAIN || ret_value == EWOULDBLOCK)
                {
                     if (pControl)
                     {
                        free(pControl);
                     }
                }
                if (ret_value == EAGAIN)
                {
                    return SYNC_TIMING_OSAL_ERR_NODATA;
                }
                else
                {
                    return SYNC_TIMING_OSAL_FAIL;
                }
            }

            *pActualRecvDataLen = ret_value;

            if (ret_value < minRecvDataLen && splRecvLenHandling == 0)
            {
                *pActualRecvDataLen = 0;
                if (pControl)
                {
                    free(pControl);
                }
                return SYNC_TIMING_OSAL_FAIL;
            }

            if (ret_value < minRecvDataLen && splRecvLenHandling == 1)
            {
                *pActualRecvDataLen = ret_value;
                if (pControl)
                {
                    free(pControl);
                }
                return SYNC_TIMING_OSAL_ERR_CONTINUE;
            }

            if (pSrcAddr)
            {
                if (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET)
                {
                    memcpy(pSrcAddr, &src_ll.sll_addr, uSrcAddrLen);
                }
                else if (pOsalSocket->sockDomain == SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET)
                {
                    memcpy(pSrcAddr, &src_in.sin_addr.s_addr, uSrcAddrLen);
                }
                else // AF_INET6
                {
                    memcpy(pSrcAddr, &src_in6.sin6_addr.s6_addr, uSrcAddrLen);
                }
            }
            
            // Handle TIMESTAMP extraction 
            if (pbTsPresent)
            {
                *pbTsPresent = 0;
            }

            if (pOsalSocket->enableTS && pTs && pbTsPresent)
            {
                for( cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm) )
                {
                    if( cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING )
                    {
                        ts = (struct timespec *) CMSG_DATA(cm);
                        pTs->seconds = ts[2].tv_sec;
                        pTs->nanoseconds = ts[2].tv_nsec;
                        *pbTsPresent = 1;
                        break;
                    }
                }
            }

            if (pControl)
            {
                free(pControl);
            }

            break;
        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_Socket_SendTo(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket, 
                                                uint8_t *pBuff, uint32_t uBuffSize,
                                                uint32_t flags,
                                                uint8_t *pDestAddr, uint32_t uDestAddrLen)
{
#if OS_LINUX
    struct sockaddr_ll  receiver_ll;
    struct sockaddr_in  receiver_in;
    struct sockaddr_in6 receiver_in6;

    int8_t              ret_value = 0;

    if (!pOsalSocket || !pDestAddr)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
            receiver_ll.sll_family = pOsalSocket->sockDomain;
            receiver_ll.sll_protocol = htons(pOsalSocket->sockProto);
            receiver_ll.sll_ifindex = pOsalSocket->uIfIdx;
            receiver_ll.sll_hatype = 0;
            receiver_ll.sll_pkttype = 0;
            receiver_ll.sll_halen = ETH_ALEN;
            
            memcpy(&receiver_ll.sll_addr, pDestAddr, ETH_ALEN);
            
            ret_value = sendto(pOsalSocket->sockId, pBuff, uBuffSize, 0, 
                               (struct sockaddr *) &receiver_ll, sizeof(receiver_ll));
            if( ret_value <= 0 )
            {
                OSAL_ERROR_PRINT("Cannot send data on raw socket (%s)\n", strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET:
            receiver_in.sin_family = AF_INET;
            receiver_in.sin_port = htons( pOsalSocket->sockPort );
        
            memcpy(&receiver_in.sin_addr.s_addr, &pDestAddr[0], SYNC_TIMING_OSAL_IPV4_ADDR_LEN);

            ret_value = sendto( pOsalSocket->sockId, pBuff, uBuffSize, 0, 
                                (struct sockaddr *) &receiver_in, sizeof( receiver_in ));
            if( ret_value <= 0 )
            {
                OSAL_ERROR_PRINT("Cannot send data on ipv4 socket (%s)\n", strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:
            receiver_in6.sin6_family = AF_INET6;
            receiver_in6.sin6_port = htons( pOsalSocket->sockPort );
            receiver_in6.sin6_flowinfo = 0;
            receiver_in6.sin6_scope_id = 0;

            memcpy(&receiver_in6.sin6_addr.s6_addr, &pDestAddr[0], SYNC_TIMING_OSAL_IPV6_ADDR_LEN);

            ret_value = sendto( pOsalSocket->sockId, pBuff, uBuffSize, 0, 
                                (struct sockaddr *) &receiver_in6, sizeof( receiver_in6 ));
            if( ret_value <= 0 )
            {
                OSAL_ERROR_PRINT("Cannot send data on ipv6 socket (%s)\n", strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_Socket_Send(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket, 
                                                uint8_t *pBuff, uint32_t uBuffSize,
                                                uint32_t flags)
{
#if OS_LINUX
    int8_t              ret_value = 0;

    if (!pOsalSocket)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET:             
            ret_value = send(pOsalSocket->sockId, pBuff, uBuffSize, flags);
            if( ret_value <= 0 )
            {
                OSAL_ERROR_PRINT("Cannot send data (%s)\n", strerror(errno));
                return SYNC_TIMING_OSAL_FAIL;
            }
            break;
        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Socket_Close(SYNC_TIMING_OSAL_SOCKET_T *pOsalSocket)
{
#if OS_LINUX
    if (!pOsalSocket)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    switch(pOsalSocket->sockDomain)
    {
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_PACKET: 
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET:
        case SYNC_TIMING_OSAL_SOCK_DOMAIN_AF_INET6:  
            //if (pOsalSocket->sockId != 0 && pOsalSocket->sockId != -1)
            {
                shutdown(pOsalSocket->sockId, SHUT_RDWR);
                close(pOsalSocket->sockId);
                pOsalSocket->sockId = -1;
            }
            break;
        default:
            OSAL_ERROR_PRINT("Unsupported SOCKET Domain %u\n", pOsalSocket->sockDomain);
            return SYNC_TIMING_OSAL_FAIL;
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

char *sync_timing_osal_inet_ntoa6(const void *addr)
{
    static char buf[40];
    unsigned short *p;

    p = (unsigned short *) addr;
    sprintf(buf, "%x:%x:%x:%x:%x:%x:%x:%x",
        SYNC_TIMING_OSAL_NTOHS(p[0]), SYNC_TIMING_OSAL_NTOHS(p[1]), 
        SYNC_TIMING_OSAL_NTOHS(p[2]), SYNC_TIMING_OSAL_NTOHS(p[3]),
        SYNC_TIMING_OSAL_NTOHS(p[4]), SYNC_TIMING_OSAL_NTOHS(p[5]), 
        SYNC_TIMING_OSAL_NTOHS(p[6]), SYNC_TIMING_OSAL_NTOHS(p[7]));
    return((char *) buf);
}

/******************************************************************************
 *      Interface access Services
 ******************************************************************************/

static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_HWTS_Supported(char    *interface_name, 
                                                                          int32_t family)
{
#if OS_LINUX
    struct ifreq            req;
    struct hwtstamp_config  cfg;
    int32_t                 sock;
    int32_t                 res;

    sock = socket(family, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) 
    {
        return 0;
    }

    memset(&req, 0, sizeof(req));
    memset(&cfg, 0, sizeof(cfg));
    req.ifr_data    = (void*)&cfg;
    strcpy(req.ifr_name, interface_name);

    res = (ioctl(sock, SIOCSHWTSTAMP, &req) < 0) ? 0 : 1;

    close(sock);

    return res;
    
#else
#error "No OS has been defined"
#endif    
}

static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_Valid(struct ifaddrs *ifa)
{
#if OS_LINUX
    if (ifa->ifa_addr == NULL) 
    {
        return 0;
    }
    
    if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) 
    {
        return 0;
    }
    
    if ((ifa->ifa_flags & IFF_UP) == 0) 
    {
        return 0;
    }
    
    if ((ifa->ifa_flags & IFF_RUNNING) == 0) 
    {
        return 0;
    }
    
    if (ifa->ifa_addr->sa_family != AF_INET &&
        ifa->ifa_addr->sa_family != AF_INET6 &&
        ifa->ifa_addr->sa_family != AF_PACKET) 
    {
        return 0;
    }
        
    return 1;
#else
#error "No OS has been defined"
#endif

}

static int32_t Sync_Timing_OSAL_Wrapper_Internal_Interface_Supported(struct ifaddrs *ifa, 
                                                                     uint32_t       uNumInterfaces, 
                                                                     char           **pInterfaces)
{
#if OS_LINUX
    if (Sync_Timing_OSAL_Wrapper_Internal_Interface_Valid(ifa) == 0)
    {
        return 0;
    }

    // If command line interfaces are used, filter accordingly
    if (uNumInterfaces > 0) 
    {
        int32_t i;
        int32_t result = 0;

        for (i = 0; i < uNumInterfaces; i++) 
        {
            if (!strcmp(ifa->ifa_name, pInterfaces[i])) 
            {
                result = 1;
                break;
            }
        }

        if (result == 0)
        {
            return 0;
        }
    }

    return Sync_Timing_OSAL_Wrapper_Internal_Interface_HWTS_Supported(ifa->ifa_name, 
                                                                      ifa->ifa_addr->sa_family);
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Init(uint32_t uNumInterfaces, char **pInterfaces,
                                                  uint32_t *pInterfaceCount)
{
#if OS_LINUX
    struct ifaddrs                  *ifaddr, 
                                    *ifa, 
                                    *ifa_inner;
    struct ifreq                    ifr;
    SYNC_TIMING_OSAL_INTERFACE_T    *pInterface;
    int32_t                         count, 
                                    sock, 
                                    index;
    int32_t                         my_ifa;
    SYNC_TIMING_OSAL_INTERFACE_T   **ppInterfaceList = &pInterfaceList;

    // Get list of all interfaces on the system
    if (getifaddrs(&ifaddr) < 0) 
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    // If the interfaces are given, apply them in order
    if (uNumInterfaces > 0) 
    {
        // Allocate memory for our interfaces
        *ppInterfaceList = malloc(uNumInterfaces * sizeof(SYNC_TIMING_OSAL_INTERFACE_T));
        if (*ppInterfaceList == NULL) 
        {
            freeifaddrs(ifaddr);
            return SYNC_TIMING_OSAL_FAIL;
        }

        // Initialize the pointer
        index = 0;
        pInterface = *ppInterfaceList;

        // Loop through the user list of interfaces
        for (my_ifa=0; my_ifa < uNumInterfaces; my_ifa++) 
        {
            // Loop through the system interfaces
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
            {
                // Does it match the next on the list.
                if (!strcmp(ifa->ifa_name, pInterfaces[my_ifa])) 
                {

                    if (Sync_Timing_OSAL_Wrapper_Internal_Interface_Valid (ifa) &&
                        Sync_Timing_OSAL_Wrapper_Internal_Interface_HWTS_Supported (ifa->ifa_name, 
                                                                         ifa->ifa_addr->sa_family)) 
                    {
                        // All looks good. Add it!
                        memset(pInterface, 0, sizeof(SYNC_TIMING_OSAL_INTERFACE_T));
                        strcpy(pInterface->name, ifa->ifa_name);
                        pInterface->index = index;
                        pInterface->channel = index + 1;
                        pInterface->activated = 0;

                        memset(&ifr, 0, sizeof(ifr));
                        strcpy(ifr.ifr_name, ifa->ifa_name);

                        sock = socket(ifa->ifa_addr->sa_family, SOCK_DGRAM, IPPROTO_IP);
                        if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) 
                        {
                            OSAL_INFO_PRINT("Unable to get MAC address for interface %s: %s\n",
                                ifa->ifa_name, strerror(errno));
                        }
                        else 
                        {
                            memcpy(pInterface->mac_addr.ether_addr_octet,
                                    ifr.ifr_hwaddr.sa_data, SYNC_TIMING_OSAL_ETH_ALEN);
                        }
                        close(sock);

                        OSAL_INFO_PRINT("Configured index %d to port %s\n",
                                    index+1, ifa->ifa_name);

                        pInterface++;
                        index++;
                    }

                    // Move out of searching loop to next in list.
                    break;
                }
            }
        }

        count = index;
    } 
    else 
    {
        // Count number of interfaces that support PTP by checking HW
        // timestamping support. Loop through list of all interfaces on the
        // system. Filter out multiple interfaces with the same name.
        count = 0;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
        {

            if (!Sync_Timing_OSAL_Wrapper_Internal_Interface_Supported(ifa, uNumInterfaces, 
                                                                       pInterfaces)) 
            {
                continue;
            }

            for (ifa_inner = ifa->ifa_next; ifa_inner != NULL; ifa_inner = ifa_inner->ifa_next) 
            {
                if (strcmp(ifa_inner->ifa_name, ifa->ifa_name) == 0) 
                {
                    break;
                }
            }

            if (ifa_inner != NULL) 
            {
                continue;
            }

            count++;
        }

        // Allocate memory for our interfaces
        *ppInterfaceList = malloc(count * sizeof(SYNC_TIMING_OSAL_INTERFACE_T));
        if (*ppInterfaceList == NULL) 
        {
            freeifaddrs(ifaddr);
            return SYNC_TIMING_OSAL_FAIL;
        }

        pInterface = *ppInterfaceList;
        index = 0;

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
        {
            if (!Sync_Timing_OSAL_Wrapper_Internal_Interface_Supported(ifa, uNumInterfaces, pInterfaces)) 
            {
                continue;
            }

            for (ifa_inner = ifa->ifa_next; ifa_inner != NULL; ifa_inner = ifa_inner->ifa_next) 
            {
                if (strcmp(ifa_inner->ifa_name, ifa->ifa_name) == 0) 
                {
                    break;
                }
            }

            if (ifa_inner != NULL) 
            {
                continue;
            }

            memset(pInterface, 0, sizeof(SYNC_TIMING_OSAL_INTERFACE_T));
            strcpy(pInterface->name, ifa->ifa_name);
            pInterface->index = index;
            pInterface->channel = index + 1;
            pInterface->activated = 0;

            memset(&ifr, 0, sizeof(ifr));
            strcpy(ifr.ifr_name, ifa->ifa_name);

            sock = socket(ifa->ifa_addr->sa_family, SOCK_DGRAM, IPPROTO_IP);
            if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) 
            {
                OSAL_INFO_PRINT("Unable to get MAC address for interface %s: %s\n",
                            ifa->ifa_name, strerror(errno));
            }
            else 
            {
                memcpy(pInterface->mac_addr.ether_addr_octet, ifr.ifr_hwaddr.sa_data, 
                       SYNC_TIMING_OSAL_ETH_ALEN);
            }
            close(sock);

            pInterface++;
            index++;
        }

    }

    freeifaddrs(ifaddr);
    *pInterfaceCount = count;

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_IsActive(uint32_t uIfIdx, uint32_t *pActive)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    *pActive = (pInterface->activated);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Activate(uint32_t uIfIdx)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    pInterface->activated = 1;

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv4Address(uint32_t uIfIdx, uint32_t *pTrue)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];
    struct ifreq ifr;
    int sock, res;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pInterface->name, IFNAMSIZ);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    res = (ioctl(sock, SIOCGIFADDR, &ifr) < 0) ? 0 : 1;
    if (res) 
    {
        pInterface->ipv4_addr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
    }
    
    close(sock);

    *pTrue = res;

    //printf("HasIpv4Address = %u\n", *pTrue);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_HasIpv6Address(uint32_t uIfIdx, uint32_t *pTrue)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];
    struct ifaddrs *ifaddr, *ifa;

    // Get list of all interfaces on the system
    if (getifaddrs(&ifaddr) < 0) 
    {
        *pTrue = 0;
        return SYNC_TIMING_OSAL_SUCCESS;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (strcmp(ifa->ifa_name, pInterface->name) == 0 &&
            ifa->ifa_addr->sa_family == AF_INET6) 
        {
            pInterface->ipv6_addr = ((struct sockaddr_in6*) ifa->ifa_addr)->sin6_addr;
            break;
        }
    }

    freeifaddrs(ifaddr);
    
    *pTrue = (ifa) ? 1 : 0;

    //printf("HasIpv6Address = %u\n", *pTrue);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetName(uint32_t uIfIdx, char **pName)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    *pName = &(pInterface->name[0]);

    //printf("Ifname = %s\n", *pName);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetChannel(uint32_t uIfIdx, int32_t *pChannel)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    *pChannel = pInterface->channel;

    //printf("channel = %d\n", *pChannel);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetPhysicalAddr(uint32_t uIfIdx,                                                              
                                                             uint8_t *pPhysicalAddr,
                                                             uint16_t *pAddrSize)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    if (!pPhysicalAddr || !pAddrSize)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    *pAddrSize = sizeof(pInterface->mac_addr.ether_addr_octet);

    memcpy((char *)pPhysicalAddr, pInterface->mac_addr.ether_addr_octet, 
           sizeof(pInterface->mac_addr.ether_addr_octet));

    //printf("pPhysicalAddr = %s, pAddrSize = %u\n", SYNC_TIMING_OSAL_ETHER_NTOA(&(pInterface->mac_addr)), 
    //       *pAddrSize);

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}                                                            

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_GetProtocolAddr(uint32_t uIfIdx, 
                                                 SYNC_TIMING_OSAL_IF_PROTOCOL_TYPE_E protoAddrType, 
                                                 uint8_t *pProtocolAddr,
                                                 uint16_t *pProtocolAddrSize)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_INTERFACE_T *pInterface = &pInterfaceList[uIfIdx];

    if (!pProtocolAddr || !pProtocolAddrSize)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    switch(protoAddrType)
    {
        case SYNC_TIMING_OSAL_IF_PROTOCOL_UDP_IPV4:
            *pProtocolAddrSize = sizeof(pInterface->ipv4_addr.s_addr);
            memcpy((char *)pProtocolAddr, (uint8_t *) &(pInterface->ipv4_addr.s_addr), 
                   sizeof(pInterface->ipv4_addr.s_addr));
            break;
        case SYNC_TIMING_OSAL_IF_PROTOCOL_UDP_IPV6:
            *pProtocolAddrSize = sizeof(pInterface->ipv6_addr.s6_addr);
            memcpy((char *)pProtocolAddr, pInterface->ipv6_addr.s6_addr, 
                   sizeof(pInterface->ipv6_addr.s6_addr));
            break;
        case SYNC_TIMING_OSAL_IF_PROTOCOL_IEEE802_3:
            *pProtocolAddrSize = sizeof(pInterface->mac_addr.ether_addr_octet);
            memcpy((char *)pProtocolAddr, pInterface->mac_addr.ether_addr_octet, 
                   sizeof(pInterface->mac_addr.ether_addr_octet));
            //printf("pProtocolAddr = %s, pProtocolAddrSize = %u\n", pProtocolAddr, *pProtocolAddrSize);

            break;
        default:
            return SYNC_TIMING_OSAL_FAIL;
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}                                                             

uint32_t Sync_Timing_OSAL_Wrapper_Interfaces_Term()
{
#if OS_LINUX
    free(pInterfaceList);
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}



/******************************************************************************
 *      Queue Services
 ******************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Open(const char *pName, 
                                            SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue)
{
#if OS_LINUX
    struct mq_attr attr;
    struct rlimit  rlim;

    //printf("clock ticks = %lu\n", sysconf(_SC_CLK_TCK));

    if (uMsgQ_rlimit_set == 0)
    {
        if (getrlimit(RLIMIT_MSGQUEUE , &rlim) != -1)
        {
            //printf("soft: %lu; hard: %lu\n", rlim.rlim_cur, rlim.rlim_max);
            rlim.rlim_cur = 3276800;
            rlim.rlim_max = 3276800;
            if (setrlimit(RLIMIT_MSGQUEUE , &rlim) != -1)
            {
                ;
                //printf("Set rlimit\n");
                //rlim.rlim_cur = 0;
                //rlim.rlim_max = 0;
                //getrlimit(RLIMIT_MSGQUEUE , &rlim);
                //printf("soft: %lu; hard: %lu\n", rlim.rlim_cur, rlim.rlim_max);
            }
            else
            {
                OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with setrlimit()! %s\n", 
                       strerror(errno));
            }
        }
        else
        {
            OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with getrlimit()! %s\n", strerror(errno));
        }
        uMsgQ_rlimit_set = 1;
    }

    attr.mq_flags = 0;
    attr.mq_maxmsg = pOsalQueue->uMaxMsgs;
    attr.mq_msgsize = pOsalQueue->uMaxMsgSize;
    attr.mq_curmsgs = 0;

    if (!pOsalQueue || !pName)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    if (pOsalQueue->uMaxMsgs && pOsalQueue->uMaxMsgSize)
    {
        pOsalQueue->MsgQId = mq_open(pName, pOsalQueue->flags, pOsalQueue->uMode, &attr);
    }
    else
    {
        pOsalQueue->MsgQId = mq_open(pName, pOsalQueue->flags);
    }

    if (pOsalQueue->MsgQId == -1)
    {
        OSAL_ERROR_PRINT("OSAL Failure... something went wrong with mq_open()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    else
    {
        return SYNC_TIMING_OSAL_SUCCESS;
    }
#else
#error "No OS has been defined"
#endif

}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Close(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue)
{
#if OS_LINUX
    int32_t ret = 0;
    if (!pOsalQueue)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    ret =mq_close(pOsalQueue->MsgQId);
    if (ret == -1)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with mq_close()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    else
    {
        return SYNC_TIMING_OSAL_SUCCESS;
    }
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Destroy(const char *pName)
{
#if OS_LINUX
    int32_t ret = 0;
    if (!pName)
        return SYNC_TIMING_OSAL_FAIL;
    ret = mq_unlink(pName);
    if (ret == -1)
    {
        // Ignore this for now 
        //printf("OSAL Failure ... something went wrong with mq_unlink()! %s\n", strerror(errno));
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Send(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue, 
                                            void *pMsg, uint32_t uMsgLen, 
                                            uint32_t uMsgPrio, uint32_t uMsgTimeoutMs)
{
#if OS_LINUX
    int32_t ret = 0;
    if (!pOsalQueue || !pMsg || (uMsgLen == 0))
        return SYNC_TIMING_OSAL_FAIL;
    ret = mq_send(pOsalQueue->MsgQId, pMsg, uMsgLen, uMsgPrio);
    // TODO - handling uMsgTimeoutMs for mq_timedsend
    if (ret == -1)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with mq_send()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    else
        return SYNC_TIMING_OSAL_SUCCESS;
    
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_Recv(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue, 
                                            void *pMsg, uint32_t uMsgLenRecv,
                                            uint32_t *pActualMsgLen, uint32_t *pMsgPrio, 
                                            uint32_t uMsgTimeoutMs)
{
#if OS_LINUX
    int status;
    struct timespec timeout;
    int32_t ret = 0;
    uint32_t ticks = SYNC_TIMING_OSAL_WAIT_FOREVER;
    //uint32_t sec = 0;
    //uint64_t ns = 0;

    if (!pOsalQueue || !pMsg || (uMsgLenRecv == 0))
        return SYNC_TIMING_OSAL_FAIL;

    if ((uMsgTimeoutMs > 0) && (uMsgTimeoutMs != SYNC_TIMING_OSAL_WAIT_FOREVER))
    {
        // Round off the ticks to the nearest
        ticks = (uMsgTimeoutMs + SYNC_TIMING_OSAL_MS_PER_TICK - 1)/SYNC_TIMING_OSAL_MS_PER_TICK;
    }

    if (ticks != SYNC_TIMING_OSAL_WAIT_FOREVER)
    {
        status = clock_gettime(CLOCK_REALTIME, &timeout);
        if (status != 0)
        {
            return status;
        }
        //printf("OLD: timeout.tv_sec = %lu, timeout.tv_nsec = %lu\n", timeout.tv_sec, timeout.tv_nsec);
        timeout.tv_sec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK)/1000;
        timeout.tv_nsec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK % 1000)*1000000;

        //sec = (uMsgTimeoutMs)/1000;
        //timeout.tv_sec += sec;
        //ns = (uMsgTimeoutMs - (sec*1000)) * 1000000;
        //timeout.tv_nsec += ns;
        
        if (timeout.tv_nsec >= 1000000000)
        {
            // Carry over to the seconds
            timeout.tv_sec += 1;
            timeout.tv_nsec -= 1000000000;
        }
        //printf("NEW: timeout.tv_sec = %lu, timeout.tv_nsec = %lu\n", timeout.tv_sec, timeout.tv_nsec);
        ret = mq_timedreceive(pOsalQueue->MsgQId, pMsg, uMsgLenRecv, pMsgPrio, &timeout);
    }
    else
    {
        ret = mq_receive(pOsalQueue->MsgQId, pMsg, uMsgLenRecv, pMsgPrio);
    }

    if (ret == -1)
    {
        if (errno == 110 || errno == 11)
        {
            return SYNC_TIMING_OSAL_TIMEOUT;
        }
        else
        {
            OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with mq_receive()! errno: %d (%s)\n", 
                    errno, strerror(errno));
            return SYNC_TIMING_OSAL_FAIL;
        }
    }
    else
    {
        *pActualMsgLen = ret;
        return SYNC_TIMING_OSAL_SUCCESS;
    }

#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQGetAvail(SYNC_TIMING_OSAL_MSG_QUEUE_T *pOsalQueue,
                                                uint32_t *pAvail)
{
#if OS_LINUX
    int32_t ret;
    struct mq_attr attr;
    ret = mq_getattr(pOsalQueue->MsgQId, &attr);
    if (ret == -1)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with mq_getattr()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    else
    {
        *pAvail = (attr.mq_maxmsg - attr.mq_curmsgs);
        return SYNC_TIMING_OSAL_SUCCESS;
    }
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_MsgQ_CleanupUnused(const char *pPrgmName)
{
#if OS_LINUX
    DIR             *pDir;
    struct dirent   *pEnt;
    char            buf[256];
    uint32_t        uPid            = 0;
    char            msgQName[512]   = {0};
    char            *pSavePtr       = NULL;
    char            *token          = NULL, 
                    *prevToken      = NULL;
    FILE            *fp             = NULL; 
    
    if (!(pDir = opendir("/dev/mqueue"))) 
    {
        OSAL_ERROR_PRINT("Cannot open /dev/mqueue");
        return SYNC_TIMING_OSAL_FAIL;
    }

    while((pEnt = readdir(pDir)) != NULL) 
    {
        if (NULL != strstr(pEnt->d_name, pPrgmName))
        {
            sprintf(msgQName, "/%s", pEnt->d_name);
            //printf("d_name = %s\n", pEnt->d_name);
            token = strtok_r(pEnt->d_name, "_", &pSavePtr);
            while(token != NULL)
            {
                //printf("token = %s\n", token);
                prevToken = token;
                token = strtok_r(NULL, "_", &pSavePtr);
            }
            
            if (prevToken != NULL)
            {
                uPid = atoi(prevToken);
                OSAL_INFO_PRINT("uPid = %u\n", uPid);
                snprintf(buf, sizeof(buf), "/proc/%u/stat", uPid);
                fp = fopen(buf, "r");

                if (fp) 
                {
                    // Active Process using this message Queue - continue on to the next one
                    OSAL_INFO_PRINT("Active msg q = %s\n", msgQName);
                    fclose(fp);
                }
                else
                {
                    // Stale - unlink this message queue
                    OSAL_INFO_PRINT("Stale msg q = %s\n", msgQName);
                    mq_unlink(msgQName);
                }
           }
       }
    }

    closedir(pDir);
    return SYNC_TIMING_OSAL_SUCCESS;
    
#else
#error "No OS has been defined"
#endif
}

/******************************************************************************
 *      Mutex Services
 ******************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Create(char *pMutexName, void **ppMutex)
{
#if OS_LINUX
    SYNC_TIMING_OSAL_MUTEX_T *pMutex = NULL;
    pMutex = malloc(sizeof(SYNC_TIMING_OSAL_MUTEX_T));
    if (pMutex && (0 == pthread_mutex_init(pMutex, 0)))
    {
        *ppMutex = pMutex;
        return SYNC_TIMING_OSAL_SUCCESS;
    }
    else
    {
        if (pMutex)
        {
            free(pMutex);
        }
        return SYNC_TIMING_OSAL_FAIL;
    }
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Delete(void *pMutex)
{
#if OS_LINUX
    pthread_mutex_destroy(pMutex);
    if (pMutex)
    {
        free(pMutex);
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Get(void *pMutex, uint32_t timeoutMS)
{
#if OS_LINUX
    uint32_t        ticks = SYNC_TIMING_OSAL_WAIT_FOREVER;
    int             status;
    struct timespec timeout;

    if ((timeoutMS > 0) && (timeoutMS != SYNC_TIMING_OSAL_WAIT_FOREVER))
    {
        // Round off the ticks to the nearest
        ticks = (timeoutMS + SYNC_TIMING_OSAL_MS_PER_TICK - 1)/SYNC_TIMING_OSAL_MS_PER_TICK;
    }
    
    if (ticks != SYNC_TIMING_OSAL_WAIT_FOREVER)
    {
        status = clock_gettime(CLOCK_REALTIME, &timeout);
        if (status != 0)
        {
            return status;
        }
        timeout.tv_sec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK)/1000;
        timeout.tv_nsec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK % 1000)*1000000;
        if (timeout.tv_nsec >= 1000000000)
        {
            // Carry over to the seconds
            timeout.tv_sec += 1;
            timeout.tv_nsec -= 1000000000;
        }
        return pthread_mutex_timedlock(pMutex, &timeout);
    }
    else
    {
        return pthread_mutex_lock(pMutex);
    }  
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Mutex_Put(void *pMutex)
{
#if OS_LINUX
    return pthread_mutex_unlock(pMutex);
#else
#error "No OS has been defined"
#endif
}

/******************************************************************************
 *      Semaphore Services
 ******************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Sem_Init(SYNC_TIMING_OSAL_SEM_T *pSem, uint32_t initialCount)
{
#if OS_LINUX
    if (pSem)
    {
        return (sem_init(pSem, 0, initialCount));
    }
    else
    {
        return SYNC_TIMING_OSAL_ERR_INVALID_PARAM;
    }
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Sem_Destroy(SYNC_TIMING_OSAL_SEM_T *pSem)
{
#if OS_LINUX
    return (sem_destroy(pSem));
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_Sem_Get(void *pSem, uint32_t timeoutMS)
{
#if OS_LINUX
    uint32_t        ticks = SYNC_TIMING_OSAL_WAIT_FOREVER;
    int             status;
    struct timespec timeout;

    if ((timeoutMS > 0) && (timeoutMS != SYNC_TIMING_OSAL_WAIT_FOREVER))
    {
        // Round off the ticks to the nearest
        ticks = (timeoutMS + SYNC_TIMING_OSAL_MS_PER_TICK - 1)/SYNC_TIMING_OSAL_MS_PER_TICK;
    }
    
    if (ticks != SYNC_TIMING_OSAL_WAIT_FOREVER)
    {
        status = clock_gettime(CLOCK_REALTIME, &timeout);
        if (status != 0)
        {
            return status;
        }
        timeout.tv_sec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK)/1000;
        timeout.tv_nsec += (ticks*SYNC_TIMING_OSAL_MS_PER_TICK % 1000)*1000000;
        if (timeout.tv_nsec >= 1000000000)
        {
            // Carry over to the seconds
            timeout.tv_sec += 1;
            timeout.tv_nsec -= 1000000000;
        }
        return sem_timedwait(pSem, &timeout);
    }
    else
    {
        return sem_wait(pSem);
    }
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Sem_Put(void *pSem)
{
#if OS_LINUX
    return sem_post(pSem);
#else
#error "No OS has been defined"
#endif
}

/******************************************************************************
 *      Thread Services
 ******************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Thread_Create(SYNC_TIMING_OSAL_THREAD_T *pThread, char *pThreadName,
                                       void (*func)(uint32_t), void *param,
                                       void *pStack, uint32_t stackSize,
                                       uint32_t uPriority)
{
#if OS_LINUX
    int                 status;
    pthread_attr_t      attr;
    struct sched_param  schedparam;    
    
    status = pthread_attr_init(&attr);
    if (status == 0)
    {
        if (pStack && stackSize > 0)
          status = pthread_attr_setstack(&attr, pStack, stackSize);
        else if(stackSize > 0)
          status = pthread_attr_setstacksize(&attr, stackSize);

        if (uPriority > 0)
        {
            schedparam.sched_priority = uPriority;
            pthread_attr_setschedparam(&attr, &schedparam);
        }
        
        if (status == 0)
        {
            status = pthread_create(&pThread->threadId, &attr, (void *((*)(void *)))func, (void *)param);
#if 0
#if(ULONG_MAX > 4294967295)
            uint64_t param64 = param;
            status = pthread_create(&pThread->threadId, &attr, (void *((*)(void *)))func, (void *)param64);
#else
            status = pthread_create(&pThread->threadId, &attr, (void *((*)(void *)))func, (void *)param);
#endif
#endif
            if (status == 0 && pThreadName != NULL)
            {
                pthread_setname_np(pThread->threadId, pThreadName);
            }
        }
    }
    pthread_attr_destroy(&attr);
    return status;
#else
#error "No OS has been defined"
#endif

}

uint32_t Sync_Timing_OSAL_Wrapper_Thread_Terminate(SYNC_TIMING_OSAL_THREAD_T *pThread)
{
#if OS_LINUX
    return pthread_cancel(pThread->threadId);
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_Thread_WaitForTerm(SYNC_TIMING_OSAL_THREAD_T *pThread)
{
#if OS_LINUX
    return pthread_join(pThread->threadId, NULL);
#endif
}


/******************************************************************************
 *      Time Services
 ******************************************************************************/
int32_t Sync_Timing_OSAL_Wrapper_SleepMS(uint32_t milliSeconds)
{
#if OS_LINUX
    return usleep(milliSeconds*1000);
#else
#error "No OS has been defined"
#endif
}

int32_t Sync_Timing_OSAL_Wrapper_USleep(uint32_t useconds)
{
#if OS_LINUX
      return usleep(useconds);
#else
#error "No OS has been defined"
#endif
}

int32_t Sync_Timing_OSAL_Wrapper_NanoSleep(SYNC_TIMING_OSAL_TIMESPEC_T *pTimeout, 
                                           SYNC_TIMING_OSAL_TIMESPEC_T *rem)
{
    return nanosleep((struct timespec *)pTimeout, (struct timespec *)rem);
}

uint32_t Sync_Timing_OSAL_Wrapper_ClockGetTime(uint32_t clockId, SYNC_TIMING_OSAL_TIMESPEC_T *pTime)
{
#if OS_LINUX
    int retVal = 0;

    retVal = clock_gettime(clockId, (struct timespec *)pTime);
    if (retVal < 0)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with clock_gettime()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif

}
uint32_t Sync_Timing_OSAL_Wrapper_ClockSetTime(uint32_t clockId, SYNC_TIMING_OSAL_TIMESPEC_T *pTime)
{
#if OS_LINUX
    int retVal = 0;

    retVal = clock_settime(clockId, (struct timespec *)pTime);
    if (retVal < 0)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with clock_settime()! %s\n", strerror(errno));
        return SYNC_TIMING_OSAL_FAIL;
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_GetTimeOfDay(SYNC_TIMING_OSAL_TIMEVAL_T *pTV, 
                                               SYNC_TIMING_OSAL_TIMEZONE_T *pTZ)
{
#if OS_LINUX
    int retVal = 0;
    struct timeval tv;
    struct timezone tz;

    if (pTV == NULL)
    {
        return SYNC_TIMING_OSAL_ERR_INVALID_PARAM;
    }

    retVal = gettimeofday(&tv, &tz);
    if (retVal < 0)
    {
        OSAL_ERROR_PRINT("OSAL Failure ... something went wrong with gettimeofday()! %s\n", strerror(errno));
        pTV->sec = 0;
        pTV->uSec = 0;
        return SYNC_TIMING_OSAL_FAIL;
    }
    pTV->sec = tv.tv_sec;
    pTV->uSec = tv.tv_usec;

    if (pTZ)
    {
        pTZ->dsttime = tz.tz_dsttime;
        pTZ->minuteswestsec = tz.tz_minuteswest;
    }
    
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_TimerCreate(SYNC_TIMING_OSAL_TIMER_T *pTimer, uint32_t uClockId)
{
#if OS_LINUX
    if (pTimer == NULL)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }
    
    pTimer->fd = timerfd_create(uClockId, 0);
    if (pTimer->fd < 0) 
    {
        OSAL_ERROR_PRINT("Failed to create timer for timer, errno %d\n", errno);
        return SYNC_TIMING_OSAL_FAIL;
    }
  
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}


uint32_t Sync_Timing_OSAL_Wrapper_TimerSetTime(SYNC_TIMING_OSAL_TIMER_T *pTimer, 
                                               uint32_t bAbsolute,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pNewVal,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pOldVal)
{
#if OS_LINUX
    int retVal = 0;
    struct itimerspec ts;
    struct itimerspec oldtsVal;

    if (pTimer == NULL || pNewVal == NULL)
    {
      return SYNC_TIMING_OSAL_FAIL;
    }

    // Start the timer, use intervals
    ts.it_interval.tv_sec   = pNewVal->timerInterval.seconds;
    ts.it_interval.tv_nsec  = pNewVal->timerInterval.nanoseconds;
    ts.it_value.tv_sec      = pNewVal->initValue.seconds;
    ts.it_value.tv_nsec     = pNewVal->initValue.nanoseconds;
    retVal = timerfd_settime(pTimer->fd, bAbsolute, &ts, (pOldVal == NULL)?NULL:&oldtsVal);
    if (retVal < 0) 
    {
        OSAL_ERROR_PRINT("Failed to start timer for timer, errno %d\n", errno);
        return SYNC_TIMING_OSAL_FAIL;
    }

    if (pOldVal != NULL)
    {
        pOldVal->initValue.seconds = oldtsVal.it_value.tv_sec;
        pOldVal->initValue.nanoseconds = oldtsVal.it_value.tv_nsec;
        pOldVal->timerInterval.seconds = oldtsVal.it_interval.tv_sec;
        pOldVal->timerInterval.nanoseconds = oldtsVal.it_interval.tv_nsec;   
    }
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_TimerGetTime(SYNC_TIMING_OSAL_TIMER_T *pTimer,
                                               SYNC_TIMING_OSAL_TIMERSPEC_T *pCurrVal)
                                               
{
#if OS_LINUX
    int retVal = 0;
    struct itimerspec oldtsVal;

    if (pTimer == NULL || pCurrVal == NULL)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    retVal = timerfd_gettime(pTimer->fd, &oldtsVal);
    if (retVal < 0) 
    {
        OSAL_ERROR_PRINT("Failed to get current timer settings value from timer, errno %d\n", errno);
        return SYNC_TIMING_OSAL_FAIL;
    }

    if (pCurrVal != NULL)
    {
        pCurrVal->initValue.seconds = oldtsVal.it_value.tv_sec;
        pCurrVal->initValue.nanoseconds = oldtsVal.it_value.tv_nsec;
        pCurrVal->timerInterval.seconds = oldtsVal.it_interval.tv_sec;
        pCurrVal->timerInterval.nanoseconds = oldtsVal.it_interval.tv_nsec;   
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_TimerWaitForExpiry(SYNC_TIMING_OSAL_TIMER_T *pTimer, 
                                                     uint64_t *pNumExpirations)
                                                     
{
#if OS_LINUX
    int retVal = 0;
    char buf[8] = {0};

    if (pTimer == NULL)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    retVal = read(pTimer->fd, buf, 8);

    if (retVal == 8)
    {
        if (pNumExpirations != NULL)
        {
            *pNumExpirations = (uint64_t)buf;
        }
    }

    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_TimerDestroy(SYNC_TIMING_OSAL_TIMER_T *pTimer)
{
#if OS_LINUX

    if (pTimer == NULL)
    {
        return SYNC_TIMING_OSAL_FAIL;
    }

    close(pTimer->fd);
    return SYNC_TIMING_OSAL_SUCCESS;
#else
#error "No OS has been defined"
#endif
}



/******************************************************************************
 *      StdC lib Services
 ******************************************************************************/
void* Sync_Timing_OSAL_Wrapper_Malloc(uint32_t size)
{
    return malloc(size);
}

void* Sync_Timing_OSAL_Wrapper_Calloc(uint32_t size)
{
    return calloc(size, sizeof(uint8_t));
}

void  Sync_Timing_OSAL_Wrapper_Free(void *pBuf)
{
    free(pBuf);
}

void* Sync_Timing_OSAL_Wrapper_Memset(void *pSrc, uint8_t initValue, uint32_t len)
{
    return memset(pSrc, initValue, len);
}

void* Sync_Timing_OSAL_Wrapper_Memcpy(void *pDest, const void *pSrc, uint32_t len)
{
    return memcpy(pDest, pSrc, len);
}

int32_t Sync_Timing_OSAL_Wrapper_Memcmp(const void *pSrc1, const void *pSrc2, uint32_t len)
{
    return memcmp(pSrc1, pSrc2, len);
}

size_t Sync_Timing_OSAL_Wrapper_GetLine(char **pStr, size_t *n, FILE *stream)
{
    return getline(pStr, n, stream);
}

size_t Sync_Timing_OSAL_Wrapper_Getopt_Long(int argc, char * const argv[],
           const char *optstring, const struct option *longopts, int *longindex)
{
    return getopt_long(argc, argv, optstring, longopts, longindex);
}

char* Sync_Timing_OSAL_Wrapper_Getcwd(char *buf, size_t size)
{
    return getcwd(buf, size);
}

int Sync_Timing_OSAL_Wrapper_Access(const char *pFilePath, int mode)
{
    return access(pFilePath, mode);
}


/******************************************************************************
 *      String Operations
 ******************************************************************************/
uint32_t Sync_Timing_OSAL_Wrapper_Strlen(const char *pSrc)
{
    return (uint32_t)strlen(pSrc);
}

uint32_t Sync_Timing_OSAL_Wrapper_Strtol(const char *pSrc, char **pRet, int base)
{
    return (uint32_t)strtol(pSrc, pRet, base);
}

uint32_t Sync_Timing_OSAL_Wrapper_Atoi(const char *pSrc)
{
    return (int32_t)atoi(pSrc);
}

char* Sync_Timing_OSAL_Wrapper_Strcpy(char *pDest, const char *pSrc)
{
    return strcpy(pDest, pSrc);
}

char* Sync_Timing_OSAL_Wrapper_Strdup(const char *pSrc)
{
    return strdup(pSrc);
}

char* Sync_Timing_OSAL_Wrapper_Strcat(char *pDest, const char *pSrc)
{
    return strcat(pDest, pSrc);
}

char* Sync_Timing_OSAL_Wrapper_Strstr(const char *pSuperStr, const char *pStrToFind)
{
    return strstr(pSuperStr, pStrToFind);
}

int32_t Sync_Timing_OSAL_Wrapper_Strcmp(const char *pStr1, const char *pStr2)
{
    return (int32_t)strcmp(pStr1, pStr2);
}

void Sync_Timing_OSAL_Wrapper_StrTokenizer_Init(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me, 
                                                       char* str, char* delim,
                                                       bool trim_leading)
{
    me->str = str;
    me->current_pos = str;
    me->end_pos = me->str + strlen(str);
    me->delim = delim;
    me->trim_leading = trim_leading;
}

static void trimLeadingDelimiters(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me)
{
    char* cur_pos;

    cur_pos = me->current_pos;

    // If the delimiter string matches the beginning of the string, skip it.
    while (strstr (cur_pos, me->delim) == cur_pos)
    {
        cur_pos += strlen(me->delim);
    }

    me->current_pos = cur_pos;
}

int32_t Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me)
{
    if (me->trim_leading)
    {
        trimLeadingDelimiters(me);
    }


    if(me->current_pos < me->end_pos )  /* no token anymore, but rest */
        return 1;
    else
        return 0;
}

char* Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(SYNC_TIMING_OSAL_STR_TOKENIZER_T* me)
{
    char* cur_pos;
    char* new_pos;
    int length;

    if (me->trim_leading)
    {
        trimLeadingDelimiters(me);
    }


    cur_pos = me->current_pos;
    while( (new_pos = strstr( cur_pos, me->delim )) != NULL )
    {
        cur_pos = strstr( cur_pos, "\"" );
        if( cur_pos == NULL || cur_pos > new_pos ) break;

        cur_pos = strstr( new_pos + 1, "\"" );
        if( cur_pos == NULL ) break;

        cur_pos = cur_pos + 1;
    }

    if( new_pos == NULL )
    {
        if( me->current_pos < me->end_pos )
        {
            new_pos = me->end_pos; /* no token anymore, return the rest */
            length = (int)(new_pos - me->current_pos);
            strncpy( me->token, me->current_pos, length );
            me->token[length] = '\0';
            me->current_pos = me->end_pos;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        length = (int)(new_pos - me->current_pos);
        strncpy( me->token, me->current_pos, length );
        me->token[length] = '\0';
        me->current_pos = new_pos + 1;
    }
    return me->token;
}

char* Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces( char* str )
{
    unsigned int i=0;

    while(i < strlen(str))
    {
        if( str[i] == ' ' )
        {
            i++;
        }
        else
        {
            return &str[i];
        }
    }

    return str;
}

/******************************************************************************
 *      Program Specific Operations
 ******************************************************************************/
const char* Sync_Timing_OSAL_Wrapper_GetShortProgramName()
{
#if OS_LINUX
    return program_invocation_short_name;
#else
#error "No OS has been defined"
#endif
}

const char* Sync_Timing_OSAL_Wrapper_GetProgramName()
{
#if OS_LINUX
    return program_invocation_name;
#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_GetProgramId()
{
#if OS_LINUX
    return getpid();
#else
#error "No OS has been defined"
#endif
}

uint32_t  Sync_Timing_OSAL_Wrapper_GetProgramStatus(const char *pPrgmName)
{
#if OS_LINUX
    char cmd[SYNC_TIMING_OSAL_MAX_PROGSTRING] = {0};

    sprintf(cmd, "pidof -x %s > /dev/null", pPrgmName);
    
    if(0 == system(cmd)) 
    {
        //Process is running.
        return 1;
    }
    else
    {
        //Process is not running.
        return 0;
    }
#else
#error "No OS has been defined"
#endif
}

int32_t Sync_Timing_OSAL_Wrapper_StartProgram(const char *pPrgmName, const char *pArgs)
{
#if OS_LINUX
    char startCommand[(SYNC_TIMING_OSAL_MAX_PROGSTRING*2) + 32] = {0};
    sprintf(startCommand, 
        "nohup %s %s >/dev/null 2>&1 &", pPrgmName, pArgs);
    return system(startCommand);

#else
#error "No OS has been defined"
#endif
}

int32_t Sync_Timing_OSAL_Wrapper_StopProgram(const char *pPrgmName)
{
#if OS_LINUX
    char startCommand[(SYNC_TIMING_OSAL_MAX_PROGSTRING) + 32] = {0};
    sprintf(startCommand, 
        "cat %s | xargs kill -9", pPrgmName);

    return system(startCommand);

#else
#error "No OS has been defined"
#endif
}


uint32_t  Sync_Timing_OSAL_Wrapper_GetProgramStatusFromId(uint32_t ProgramId)
{
#if OS_LINUX
    char buf[256]   = {0};
    FILE *fp        = NULL;

    snprintf(buf, sizeof(buf), "/proc/%u/stat", ProgramId);
    fp = fopen(buf, "r");
    
    if (fp) 
    {
        fclose(fp);
        return 1;
    }
    else
    {
        return 0;
    }

#else
#error "No OS has been defined"
#endif
}

uint32_t Sync_Timing_OSAL_Wrapper_GetUnusedProgramId(const char *pPrgmName)
{
#if OS_LINUX
    DIR             *pDir;
    struct dirent   *pEnt;
    char            buf[256];
    uint32_t        uPid            = 0;
    char            msgQName[512]   = {0};
    char            *pSavePtr       = NULL;
    char            *token          = NULL, 
                    *prevToken      = NULL;
    FILE            *fp             = NULL; 
    
    if (!(pDir = opendir("/dev/mqueue"))) 
    {
        OSAL_ERROR_PRINT("Cannot open /dev/mqueue");
        return SYNC_TIMING_OSAL_FAIL;
    }

    while((pEnt = readdir(pDir)) != NULL) 
    {
        if (NULL != strstr(pEnt->d_name, pPrgmName))
        {
            sprintf(msgQName, "/%s", pEnt->d_name);
            //printf("d_name = %s\n", pEnt->d_name);
            token = strtok_r(pEnt->d_name, "_", &pSavePtr);
            while(token != NULL)
            {
                //printf("token = %s\n", token);
                prevToken = token;
                token = strtok_r(NULL, "_", &pSavePtr);
            }
            
            if (prevToken != NULL)
            {
                uPid = atoi(prevToken);
                OSAL_INFO_PRINT("uPid = %u\n", uPid);
                snprintf(buf, sizeof(buf), "/proc/%u/stat", uPid);
                fp = fopen(buf, "r");

                if (fp) 
                {
                    // Active Process using this message Queue - continue on to the next one
                    OSAL_INFO_PRINT("Active msg q = %s\n", msgQName);
                    fclose(fp);
                }
                else
                {
                    // Stale - unlink this message queue
                    OSAL_INFO_PRINT("Stale msg q = %s\n", msgQName);
                    closedir(pDir);                    
                    return uPid;
                }
           }
       }
    }

    closedir(pDir);
    return 0;
#else
#error "No OS has been defined"
#endif
}


