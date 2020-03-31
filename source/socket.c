/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: socket implementation
 * History:
   <1> 04/03/2020 , create
************************************************************/
#include <linux/if_ether.h>
#include "struct.h"

static RawSockList g_RsList;

static inline int IsIfFiltered (int Fd, struct ifreq *IfReq)
{
    struct ifreq If = *IfReq;
    int Ret = ioctl(Fd, SIOCGIFFLAGS, &If);
    if (Ret < 0)
    {
        return 1;
    }

    if ((IFF_LOOPBACK & If.ifr_flags) || !(IFF_UP & If.ifr_flags))
    {
        return 1;
    }

    return 0;
}

static inline int GetIfIndex (int Fd, struct ifreq *IfReq, int *IfIndex)
{
    struct ifreq If = *IfReq;
    
    int Ret = ioctl(Fd, SIOCGIFINDEX, &If);
    if (Ret < 0)
    {
        return -1;
    }

    *IfIndex = If.ifr_ifindex;
    return 0;
}


static inline int GetIfMac (int Fd, struct ifreq *IfReq, char *Mac)
{
    int Ret = ioctl(Fd, SIOCGIFHWADDR, (char *)IfReq);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    snprintf(Mac, M_MAC, "%02x%02x%02x%02x%02x%02x",
             (unsigned char)IfReq->ifr_hwaddr.sa_data[0],
             (unsigned char)IfReq->ifr_hwaddr.sa_data[1],
             (unsigned char)IfReq->ifr_hwaddr.sa_data[2],
             (unsigned char)IfReq->ifr_hwaddr.sa_data[3],
             (unsigned char)IfReq->ifr_hwaddr.sa_data[4],
             (unsigned char)IfReq->ifr_hwaddr.sa_data[5]);
    return 0;
}

static inline int GetIfIpv4 (int Fd, struct ifreq *IfReq, char *Ipv4)
{
    int Ret = ioctl(Fd, SIOCGIFADDR, (char *)IfReq);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    snprintf(Ipv4, M_IPV4, "%s",
             (char *)inet_ntoa(((struct sockaddr_in *)&(IfReq->ifr_addr))->sin_addr));
    return 0;                
}

static inline int GetIfIpv4Mask (int Fd, struct ifreq *IfReq, char *Mask)
{
    int Ret = ioctl(Fd, SIOCGIFNETMASK, (char *)IfReq);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    snprintf(Mask, M_IPV4, "%s",
             (char *)inet_ntoa(((struct sockaddr_in *)&(IfReq->ifr_netmask))->sin_addr));
    
    return 0;
}


static int GetIfInfo(RawSockList *RsList) 
{
    int Ret = 0;
    RawSocket Rs;

    struct ifreq  IfReqList[M_IF_SUPPORT];
    struct ifconf IfConf;   
    struct ifreq  *IfReq;

    int Fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Fd <= 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    IfConf.ifc_len = sizeof(IfReqList);
    IfConf.ifc_buf = (caddr_t)IfReqList;
    Ret = ioctl(Fd, SIOCGIFCONF, (char *)&IfConf);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        close(Fd);
        return -1;
    }
    
    int IfNum = IfConf.ifc_len / sizeof(struct ifreq);
    for (int IfIndex = 0; IfIndex < IfNum; IfIndex++)
    {
        IfReq = IfReqList + IfIndex;

        memset (&Rs, 0, sizeof(Rs));
        strncpy (Rs.IfName, IfReq->ifr_name, M_IF_NAME);

        Ret = GetIfIndex (Fd, IfReq, &Rs.IfIndex);
        if (Ret < 0)
        {
            Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
            continue;
        }
        
        if (IsIfFiltered (Fd, IfReq))
        {
            continue;
        }
 
        Ret = GetIfMac (Fd, IfReq, Rs.Mac);
        if (Ret < 0)
        {
            Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
            continue;
        }
        
        Ret = GetIfIpv4 (Fd, IfReq, Rs.Ipv4);
        if (Ret < 0)
        {
            Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
            continue;
        }

        Ret = GetIfIpv4Mask (Fd, IfReq, Rs.IpMask);
        if (Ret < 0)
        {
            Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
            continue;
        }

        memcpy (RsList->Rss+RsList->RsNum, &Rs, sizeof(Rs));
        RsList->RsNum++;

        //printf ("[%d]device name:%-8s,  DeviceMac:%s, DeviceIp:%-16s, Mask:%s\r\n", Rs.IfIndex, Rs.IfName, Rs.Mac, Rs.Ipv4, Rs.IpMask);
    }


    close(Fd);

    return 0;
}

static inline int InitRawSocket (RawSocket *Rs)
{
    int Socket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
    if(Socket < 0) 
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    struct sockaddr_ll Addrll;
    memset(&Addrll, 0, sizeof(struct sockaddr_ll));
    Addrll.sll_ifindex = Rs->IfIndex;
    Addrll.sll_family = AF_PACKET;
    if (bind(Socket, (struct sockaddr*)&Addrll, sizeof(Addrll)) < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    Rs->Socket = Socket;
    //printf ("Bind Socket:%d to %s \r\n", Socket, Rs->IfName);

    return 0;
}

int InitSocket (RawSockList **RawSkList)
{
    int Ret = 0;
    RawSockList *RsList = &g_RsList;
    
	Ret = GetIfInfo (RsList);
    if (Ret < 0)
    {
        Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    //printf ("===> Get %d Interface information \r\n", RsList->RsNum);
    for (int RsIndex = 0; RsIndex < RsList->RsNum; RsIndex++)
    {
        Ret = InitRawSocket (RsList->Rss+RsIndex);
        if (Ret < 0)
        {
            Log ("[%s:%d] fail\r\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }

    *RawSkList = RsList;
    
    return 0;
}


RawSocket* GetIfSocket (char *Iface)
{   
    RawSockList *RsList = &g_RsList;        
    RawSocket* Rs = RsList->Rss;
    
    for (int RsIndex = 0; RsIndex < RsList->RsNum; RsIndex++)
    {    
        if (strncmp (Rs->IfName, Iface, sizeof(Rs->IfName)) == 0)
        {
            return Rs;
        }

        Rs++;
    }

    return NULL;
}




