/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: route
 * History:
   <1> 04/03/2020 , create
************************************************************/
#include "struct.h"
#include "route.h"
#include "socket.h"
#include "arp.h"

/**************************************************************
*
*                         Route Talble
*
***************************************************************/
static inline Route* RouteTable ()
{
    static Route RoutList[M_ROUTE_SIZE];
    
    return RoutList;
}

static inline void PrintRoute ()
{
    Route *RHdr = RouteTable ();

    printf ("**********************************************************************************\r\n");
    printf ("%-16s\t%-16s\t%-16s\t%-8s\r\n", "Destination", "Mask", "Gate", "Interface");
    printf ("**********************************************************************************\r\n");
    for (int Index = 0; Index < M_ROUTE_SIZE; Index++)
    {
        if (RHdr->IsValid == 0)
        {
            break;
        }
        
        printf ("%-16s\t%-16s\t%-16s\t%-8s\r\n", RHdr->Dst, RHdr->Mask, RHdr->Gate, RHdr->Iface);
        RHdr++;
    }
    printf ("**********************************************************************************\r\n\r\n");

    return;
}

static inline void InitRoute (RawSockList *RsList)
{
    Route *RHdr = RouteTable ();
    RawSocket *Rs;

    
    for (int Index = 0; Index < RsList->RsNum; Index++)
    {
        Rs = RsList->Rss + Index;
        
        int Gate = inet_addr(Rs->Ipv4);
        int Mask = inet_addr(Rs->IpMask);
        int Dst  = Gate & Mask;

        /* add route */
        strncpy (RHdr->Dst,   inet_ntoa(*((struct in_addr*)&Dst)), sizeof(RHdr->Dst));
        strncpy (RHdr->Mask,  Rs->IpMask,     sizeof(RHdr->Mask));
        strncpy (RHdr->Gate,  Rs->Ipv4,       sizeof(RHdr->Gate));
        strncpy (RHdr->Iface, Rs->IfName,     sizeof(RHdr->Iface));
        RHdr->IsValid = 1;

        RHdr++;
    }

    PrintRoute ();

    return;
}

static inline char* GetRoute (char *Dst)
{
    Route *RHdr = RouteTable ();   

    int DstAddr = inet_addr(Dst);
    for (int Index = 0; Index < M_ROUTE_SIZE; Index++)
    {
        if (RHdr->IsValid == 0)
        {
            break;
        }
        
        int RDst = inet_addr(RHdr->Dst);
        int Mask = inet_addr(RHdr->Mask);

        if ((DstAddr & Mask) == RDst)
        {
            //printf ("Route match: %s -- %s, face:%s \r\n", Dst, R->Dist, R->Iface);
            return RHdr->Iface;
        }

        RHdr++;
    }
    
    return NULL;
}

static int IsGateAddr (char *Ipv4)
{  
    Route *RHdr = RouteTable ();   

    for (int Index = 0; Index < M_ROUTE_SIZE; Index++)
    {
        if (RHdr->IsValid == 0)
        {
            break;
        }
        
        if (strncmp (RHdr->Gate, Ipv4, sizeof(RHdr->Gate)) == 0)
        {
            return 1;
        }

        RHdr++;
    }
    
        
    return 0;
}



/**************************************************************
*
*                         Forwarding
*
***************************************************************/
/**************************************************************
//struct ip {
//    u_char  ip_v:4,         /* version */
//           ip_hl:4;         /* header length */
//
//    u_char  ip_tos;         /* type of service */
//    short   ip_len;         /* total length */
//    u_short ip_id;          /* identification */
//    short   ip_off;         /* fragment offset field */
//#define IP_DF 0x4000        /* dont fragment flag */
//#define IP_MF 0x2000        /* more fragments flag */
//    u_char  ip_ttl;         /* time to live */
//    u_char  ip_p;           /* protocol */
//    u_short ip_sum;         /* checksum */
//    struct  in_addr ip_src,
//              ip_dst;  /* source and dest address */
//};
//************************************************************/
static inline int RecvPacket (int Socket, Packet *Pkt)
{
    struct sockaddr_in SockAddr;
    socklen_t SockLen = sizeof(SockAddr);
    
    memset(&SockAddr, 0, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    Pkt->PacketLen = recvfrom(Socket, Pkt->Buffer, sizeof(Pkt->Buffer), 0, 
                              (struct sockaddr *)&SockAddr, (socklen_t *)&SockLen);
    if (Pkt->PacketLen <= 0)
    {
        return -1;
    }


    struct ip *IpHdr = (struct ip *)Pkt->Buffer;
    strncpy (Pkt->SrcIp, inet_ntoa(IpHdr->ip_src), sizeof(Pkt->SrcIp));
    strncpy (Pkt->DstIp, inet_ntoa(IpHdr->ip_dst), sizeof(Pkt->DstIp));

    //printf ("[%d]SrcIp:%s, DstIp:%s, Size = %d, Proto = %d\r\n", 
    //        IpHdr->ip_id, Pkt->SrcIp, Pkt->DstIp, Pkt->PacketLen, (int)IpHdr->ip_p);
    
    return 0;
}

static inline unsigned short IphdrCheckSum(struct ip *IpHdr)   
{ 
    int HdrLen = (IpHdr->ip_hl)<<2;    
    unsigned short *Codeing = (unsigned short *)IpHdr;
    unsigned int CheckSum = 0;

    IpHdr->ip_sum = 0;
    while(HdrLen > 0)
    {
        CheckSum += *Codeing;
        Codeing++;
        
        HdrLen -= 2;
    }   
  
    CheckSum  = (CheckSum>>16) + (CheckSum & 0xffff);
    CheckSum += (CheckSum>>16);
    CheckSum  = ~CheckSum;   
    //printf("CheckSum=%04x\n",CheckSum);

    return CheckSum;
} 

static inline void UpdateTTL (Packet *Pkt)
{
    struct ip *IpHdr = (struct ip *)Pkt->Buffer;
    
    /* update TTL */
    if (IpHdr->ip_ttl == 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return;
    }
    IpHdr->ip_ttl -= 1;

    /* Update checksum */
    IpHdr->ip_sum = IphdrCheckSum (IpHdr);
    
    return;
}

static inline void SendPakcet (Packet *Pkt)
{
    /*1. route */
    char *Iface = GetRoute (Pkt->DstIp);
    if (Iface == NULL)
    {
        Log ("[%s:%d] Pkt->DstIp = %s\r\n", __FUNCTION__, __LINE__, Pkt->DstIp);
        return;
    }
    
    /*2. arp */
    char *DstMac = Ipv42Mac (Pkt->DstIp);
    if (DstMac == NULL)
    {
        Log ("[%s:%d] (%s -> %s)\r\n", __FUNCTION__, __LINE__, Pkt->SrcIp, Pkt->DstIp);
        return;
    }

    /*3. IF socket */
    RawSocket* Rs = GetIfSocket (Iface);
    if (Rs == NULL)
    {
        Log ("[%s:%d] Iface = %s\r\n", __FUNCTION__, __LINE__, Iface);
        return;
    }

    /*4. send */
    struct sockaddr_ll Addrll;
    memset(&Addrll, 0, sizeof(struct sockaddr_ll));
    
    Addrll.sll_family = AF_PACKET;
    Addrll.sll_ifindex = Rs->IfIndex;
    Addrll.sll_halen = ETHER_ADDR_LEN;
    Addrll.sll_protocol = htons(0x0800);
    Addrll.sll_addr[0] = DstMac[0];
    Addrll.sll_addr[1] = DstMac[1];
    Addrll.sll_addr[2] = DstMac[2];
    Addrll.sll_addr[3] = DstMac[3];
    Addrll.sll_addr[4] = DstMac[4];
    Addrll.sll_addr[5] = DstMac[5];

    //printf("DstIface: %s, DstMac: %x:%x:%x:%x:%x:%x, Socket: %d, IfIndex:%d \r\n", 
    //       Iface, DstMac[0], DstMac[1], DstMac[2], DstMac[3], DstMac[4], DstMac[5], Rs->Socket, Rs->IfIndex);

    int SendSize = sendto(Rs->Socket, Pkt->Buffer, Pkt->PacketLen, 0, (struct sockaddr*)&Addrll,  sizeof(Addrll));
    if(SendSize < Pkt->PacketLen)
    {
        Log ("[%s:%d] Pkt->DstIp = %s\r\n", __FUNCTION__, __LINE__, Pkt->DstIp);
    }
    
    return;
}


static inline void Forwarding (RawSocket *Rs)
{
    Packet Pkt;
    memset (&Pkt, 0, sizeof(Pkt));
    
    if (RecvPacket (Rs->Socket, &Pkt) < 0)
    {
        return;
    }
    
    if (IsGateAddr (Pkt.DstIp))
    {
        return;
    } 

    UpdateTTL (&Pkt);

    SendPakcet (&Pkt);
    
    return;
}

/**************************************************************
*
*                         Route Core
*
***************************************************************/
static inline int SetFd (RawSockList *RsList, fd_set *MasterFd)
{
    RawSocket *Rs;
    int MaxFd = 0;

    for (int Index = 0; Index < RsList->RsNum; Index++)
    {
        Rs = RsList->Rss + Index;
        
        FD_SET(Rs->Socket, MasterFd);
        if (Rs->Socket > MaxFd)
        {
            MaxFd = Rs->Socket;
        }
    }

    return (MaxFd+1);
}


int RouteCore ()
{
    int MaxFd;
    RawSockList *RsList;
    RawSocket *Rs;
    
    fd_set ReadFds;
    fd_set MasterFds;
    
    FD_ZERO(&MasterFds);
    FD_ZERO(&ReadFds);

    int Ret = InitSocket (&RsList);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    InitRoute (RsList);

    MaxFd = SetFd (RsList, &MasterFds);
    if (MaxFd <= 1)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    while (1)
    {
        ReadFds = MasterFds;

        if (select(MaxFd, &ReadFds , NULL , NULL , NULL) == -1)
        {
            exit(1);
        }

        for (int Index = 0; Index < RsList->RsNum; Index++)
        {
            Rs = RsList->Rss + Index;

            if (!FD_ISSET(Rs->Socket, &ReadFds))
            {
                continue;            
            }

            Forwarding (Rs);
        }     
        
    }
    
    return 0;
}



