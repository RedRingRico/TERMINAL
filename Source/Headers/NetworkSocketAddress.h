#ifndef __TERMINAL_NETWORKSOCKETADDRESS_H__
#define __TERMINAL_NETWORKSOCKETADDRESS_H__

#include <shinobi.h>
#include <ngsocket.h>

typedef struct _tagSOCKET_ADDRESS
{
	struct sockaddr	SocketAddress;
}SOCKET_ADDRESS,*PSOCKET_ADDRESS;

/* Both the address and port should be in network byte order */
int NET_CreateSocketAddress( PSOCKET_ADDRESS p_pSocketAddress,
	Uint32 p_Address, Uint16 p_Port );
size_t NET_GetSocketAddressSize( PSOCKET_ADDRESS p_pSocketAddress );

#endif /* __TERMINAL_NETWORKSOCKETADDRESS_H__ */

