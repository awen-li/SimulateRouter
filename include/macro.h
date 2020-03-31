
#ifndef _MACRO_H_
#define _MACRO_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>




#define M_IF_NAME       (32)
#define M_MAC           (16)
#define M_IPV4          (16)

#define M_IF_SUPPORT    (16)
#define M_BUFFER        (2048)
#define M_ARP_SIZE      (32)

#define M_ROUTE_SIZE    (32)


void Log(const char* Format, ...);

#endif
