#ifndef __TERMINAL_NETWORKING_H__
#define __TERMINAL_NETWORKING_H__

#include <shinobi.h>
#include <ngdc.h>
#include <ngip/ethernet.h>
#include <ngos.h>
#include <ngtcp.h>
#include <ngudp.h>
#include <ngppp.h>
#include <ngappp.h>
#include <ngadns.h>
#include <ngeth.h>
#include <Memory.h>

typedef enum
{
	NET_STATUS_NODEVICE,
	NET_STATUS_CONNECTED,
	NET_STATUS_NEGOTIATING,
	NET_STATUS_PPP_POLL,
	NET_STATUS_INTERFACE_CLOSE,
	NET_STATUS_DISCONNECTED,
	NET_STATUS_RESET,
	NET_STATUS_ERROR
}NET_STATUS;

typedef enum
{
	NET_INTERNAL_MODEM_RESET_INIT,
	NET_INTERNAL_MODEM_RESET_DROP_LINES,
	NET_INTERNAL_MODEM_RESET_WAIT_FOR_DSR,
	NET_INTERNAL_MODEM_RESET_UNKNOWN	
}NET_INTERNAL_MODEM_RESET;

typedef enum
{
	NET_DEVICE_TYPE_LAN_10,
	NET_DEVICE_TYPE_LAN_100,
	NET_DEVICE_TYPE_LAN_UNKNOWN,
	NET_DEVICE_TYPE_BBA,
	NET_DEVICE_TYPE_EXTMODEM,
	NET_DEVICE_TYPE_SERIALPPP,
	NET_DEVICE_TYPE_INTMODEM,
	NET_DEVICE_TYPE_NONE
}NET_DEVICE_TYPE;

typedef enum
{
	NET_DEVICE_HARDWARE_LAN,
	NET_DEVICE_HARDWARE_MODEM,
	NET_DEVICE_HARDWARE_SERIAL,
	NET_DEVICE_HARDWARE_NONE
}NET_DEVICE_HARDWARE;

typedef enum
{
	NET_CONNECTION_STATE_NEGOTIATING,
	NET_CONNECTION_STATE_CONNECTED,
	NET_CONNECTION_STATE_UNKNOWN
}NET_CONNECTION_STATE;

typedef enum
{
	NET_DISCONNECT_STATUS_USER_CANCELLED,
	NET_DISCONNECT_STATUS_USER_DISCONNECTED
}NET_DISCONNECT_STATUS;

typedef struct _tagNETWORK_CONFIGURATION
{
	PMEMORY_BLOCK	pMemoryBlock;
}NETWORK_CONFIGURATION,*PNETWORK_CONFIGURATION;

int NET_Initialise( PNETWORK_CONFIGURATION p_pNetworkConfiguration );
void NET_Terminate( void );

void NET_Update( void );

int NET_ConnectToISP( void );
int NET_DisconnectFromISP( void );

NET_STATUS NET_GetStatus( void );
NET_DEVICE_TYPE NET_GetDeviceType( void );

int NET_GetDevOpen( void );
int NET_GetIfaceOpen( void );

#define DNS_REQUEST_FAILED -1
#define DNS_REQUEST_RESOLVED 0
#define DNS_REQUEST_POLLING 1

/* DNS query and resolution */
typedef struct _tagDNS_REQUEST
{
#if defined ( DEBUG )
	/* There is a 64-character limit on names */
	char			Name[ NG_DNS_NAME_MAX + 1 ];
	char			IPAddress[ 16 ];
#endif /* DEBUG*/
	ngADnsTicket	Ticket;
	Sint32			Status;
	Uint32			IP;
}DNS_REQUEST,*PDNS_REQUEST;

/* The DNS is set up with NET_Initialise, shut down with NET_Terminate */
int NET_DNSRequest( PDNS_REQUEST p_pQuery, const char *p_pDomain );
/* Remove should be called when the resolution succeeds */
void NET_DNSRemoveRequest( PDNS_REQUEST p_pQuery );
void NET_DNSPoll( void );

#endif /* __TERMINAL_NETWORKING_H__ */

