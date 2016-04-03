#ifndef __TERMINAL_NETWORKCLIENT_H__
#define __TERMINAL_NETWORKCLIENT_H__

#include <NetworkSocket.h>
#include <NetworkSocketAddress.h>
#include <NetworkMessage.h>
#include <shinobi.h>

typedef struct _tagNETWORK_CLIENT
{
	Uint32			ID;
	PSOCKET_UDP		Socket;
	PSOCKET_ADDRESS	SocketAddress;
}NETWORK_CLIENT,*PNETWORK_CLIENT;

/* The address passed in must be an IPv4 address in its dotted format */
/* Use NET_GetIPFromDomain to convert a domain name to an IPv4 address */
int NCL_Initialise( PNETWORK_CLIENT p_pClient, const char *p_pServerIP,
	const Uint16 p_ServerPort );

void NCL_Terminate( PNETWORK_CLIENT p_pClient );

void NCL_SendMessage( PNETWORK_CLIENT p_pClient, PNETWORK_MESSAGE p_pMessage );

#endif /* __TERMINAL_NETWORKCLIENT_H__ */

