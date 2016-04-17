#ifndef __TERMINAL_NETWORKSOCKET_H__
#define __TERMINAL_NETWORKSOCKET_H__

#include <shinobi.h>
#include <ngsocket.h>
#include <NetworkSocketAddress.h>

#define INVALID_SOCKET	-1

typedef struct _tagSOCKET_UDP
{
	int	Socket;
}SOCKET_UDP,*PSOCKET_UDP;

int NET_CreateSocketUDP( PSOCKET_UDP p_pSocket );
void NET_DestroySocketUDP( PSOCKET_UDP p_pSocket );

int NET_BindSocketUDP( PSOCKET_UDP p_pSocket, PSOCKET_ADDRESS p_pAddress );
int NET_SocketSendTo( PSOCKET_UDP p_pSocket, const void *p_pData,
	const size_t p_Length, const PSOCKET_ADDRESS p_pAddress );
int NET_SocketReceiveFrom( PSOCKET_UDP p_pSocket, void *p_pData,
	const size_t p_Length, PSOCKET_ADDRESS p_pAddress );

int NET_SetSocketNonBlocking( int p_Socket, bool p_NonBlocking );

#endif /* __TERMINAL_NETWORKSOCKET_H__ */

