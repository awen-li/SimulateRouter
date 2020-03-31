/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: internal data struct definition 
 * History:
   <1> 04/03/2020 , create
************************************************************/

#ifndef _STRUCT_H_
#define _STRUCT_H_
#include "macro.h"

typedef struct tag_Arp
{
    char Ipv4[M_IPV4]; 
    char Mac[M_MAC];      
}Arp;

typedef struct tag_Packet
{
    char SrcIp[M_IPV4];
    char DstIp[M_IPV4];

    char Buffer[M_BUFFER];
    int  PacketLen;
    
}Packet;


typedef struct tag_RawSocket
{
    int Socket;
    
    char IfName[M_IF_NAME];
    int  IfIndex;
    
    char Mac[M_MAC];
    
    char Ipv4[M_IPV4];
    char IpMask[M_IPV4];

}RawSocket;

typedef struct tag_RawSockList
{
    RawSocket Rss[M_IF_SUPPORT];
    int RsNum;
}RawSockList;


typedef struct tag_Route
{
    char Dst[M_IPV4];
    char Mask[M_IPV4];
    char Gate[M_IPV4];
    char Iface[M_IF_NAME];
    int  IsValid;
}Route;


#endif
