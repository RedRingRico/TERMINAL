#include <NetworkSocketAddress.h>

int NET_CreateSocketAddress( PSOCKET_ADDRESS p_pSocketAddress,
	Uint32 p_Address, Uint16 p_Port )
{
	struct sockaddr_in *pSockAddr =
		( struct sockaddr_in * )&p_pSocketAddress->SocketAddress;

	pSockAddr->sin_family = AF_INET;
	pSockAddr->sin_port = p_Port;

	if( p_Address != 0 )
	{
		pSockAddr->sin_addr.s_addr = p_Address;
	}
	else
	{
		pSockAddr->sin_addr.s_addr = INADDR_ANY;
	}

	return 0;
}

size_t NET_GetSocketAddressSize( PSOCKET_ADDRESS p_pSocketAddress )
{
	return sizeof( p_pSocketAddress->SocketAddress );
}

