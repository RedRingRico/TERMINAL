#include <NetworkSocket.h>

int NET_CreateSocketUDP( PSOCKET_UDP p_pSocket )
{
	p_pSocket->Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	if( p_pSocket->Socket == -1 )
	{
		return 1;
	}

	NET_SetSocketNonBlocking( p_pSocket->Socket, true );

	return 0;
}

void NET_DestroySocketUDP( PSOCKET_UDP p_pSocket )
{
	if( p_pSocket->Socket != INVALID_SOCKET )
	{
		closesocket( p_pSocket->Socket );
		p_pSocket->Socket = INVALID_SOCKET;
	}
}

int NET_BindSocketUDP( PSOCKET_UDP p_pSocket, PSOCKET_ADDRESS p_pAddress )
{
	if( bind( p_pSocket->Socket, &p_pAddress->SocketAddress,
		NET_GetSocketAddressSize( p_pAddress ) ) != 0 )
	{
		return 1;
	}

	return 0;
}

int NET_SocketSendTo( PSOCKET_UDP p_pSocket, const void *p_pData,
	const size_t p_Length, const PSOCKET_ADDRESS p_pAddress )
{
	int BytesSent = sendto( p_pSocket->Socket, ( const char * )p_pData,
		p_Length, 0, &p_pAddress->SocketAddress,
		NET_GetSocketAddressSize( p_pAddress ) );

	if( BytesSent >= 0 )
	{
		return BytesSent;
	}

	return -1;
}

int NET_SocketReceiveFrom( PSOCKET_UDP p_pSocket, void *p_pData,
	const size_t p_Length, PSOCKET_ADDRESS p_pAddress )
{
	int FromLength = NET_GetSocketAddressSize( p_pAddress );
	int ReadBytes = recvfrom( p_pSocket->Socket, p_pData, p_Length, 0,
		&p_pAddress->SocketAddress, &FromLength );

	if( ReadBytes >= 0 )
	{
		return ReadBytes;
	}
	else
	{
		return 0;
	}
}

int NET_SetSocketNonBlocking( int p_Socket, bool p_NonBlocking )
{
	int NonBlocking = p_NonBlocking ? 1 : 0;

	return ioctlsocket( p_Socket, FIONBIO, &NonBlocking );
}

