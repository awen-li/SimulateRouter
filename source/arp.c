/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: simulate arp
 * History:
   <1> 04/03/2020 , create
************************************************************/
#include "struct.h"

char* Ipv42Mac (char *Ipv4)
{
    static Arp ArpList[] = 
    {
        {"192.168.1.100", {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
        {"172.16.0.100",  {0x00, 0x00, 0x00, 0x00, 0x00, 0x02}},
        {"10.0.0.100",    {0x00, 0x00, 0x00, 0x00, 0x00, 0x03}}
    };

    Arp *ArpItem = ArpList;
    for (int Index = 0; Index < sizeof(ArpList)/sizeof(Arp); Index++)
    {
        if (strncmp(ArpItem->Ipv4, Ipv4, sizeof(ArpItem->Ipv4)) == 0)
        {
            return ArpItem->Mac;
        }

        ArpItem++;
    }

    return NULL;
}




