#include <NetworkClient.h>
#include <Log.h>

int NCL_Initialise( PNETWORK_CLIENT p_pClient, const char *p_pServerIP,
	const Uint16 p_ServerPort )
{
	p_pClient->ID = 0;

	p_pClient->Socket = malloc( sizeof( SOCKET_UDP ) );
	p_pClient->SocketAddress = malloc( sizeof( SOCKET_ADDRESS ) );
	
	if( NET_CreateSocketUDP( p_pClient->Socket ) != 0 )
	{
		LOG_Debug( "Failed to create a UDP socket" );

		return 1;
	}

	NET_CreateSocketAddress( p_pClient->SocketAddress,
		inet_addr( p_pServerIP ), htons( p_ServerPort ) );

	return 0;
}

void NCL_Terminate( PNETWORK_CLIENT p_pClient )
{
	free( p_pClient->SocketAddress );
	free( p_pClient->Socket );
}

void NCL_SendMessage( PNETWORK_CLIENT p_pClient, PNETWORK_MESSAGE p_pMessage )
{
	NET_SocketSendTo( p_pClient->Socket, p_pMessage->pData, p_pMessage->Size,
		p_pClient->SocketAddress );
	LOG_Debug( "Sending message of %d bytes", p_pMessage->Size );
}

