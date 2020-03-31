/***********************************************************
 * Author: Wen Li
 * Date  : 04/03/2020
 * Describe: socket definition
 * History:
   <1> 04/03/2020 , create
************************************************************/

#ifndef _SOCKET_H_
#define _SOCKET_H_

int InitSocket (RawSockList **RawSkList);

RawSocket* GetIfSocket (char *Iface);


#endif
