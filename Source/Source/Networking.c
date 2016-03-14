#include <Networking.h>
#include <Log.h>
#include <ngdc.h>
#include <ngip/ethernet.h>
#include <ngos.h>
#include <ngtcp.h>
#include <ngudp.h>
#include <ngppp.h>
#include <ngappp.h>
#include <ngadns.h>
#include <ngeth.h>
#include <Nt_utl.h>

#define NET_MAX_BUFFERS		100			/* Message buffers */
#define NET_MAX_STACK_MEM	400*1024	/* 400KiB static memory */
#define NET_MAX_SOCKETS		10			/* Maximum number of sockets open */
#define NET_MAX_DEVCBS		2			/* Maximum simultaneous device control
										 * blocks */
#define NET_MAX_DEVIO_SIZE	2*1024		/* Maximum device I/O bufer size */

static NGsock NET_SocketTable[ NET_MAX_SOCKETS ];
/* Deviced control block strucutre */
static NGdevcb NET_DeviceIOControlBlock[ NET_MAX_DEVCBS ];
/* TCP control blocks */
static NET_TCPCB[ NET_MAX_SOCKETS ];
/* This should probably be in some kind of network device struct */
static NGdev NET_Device;
/* Network interface */
static NGifnet *NET_pInterface;
/* PPP interface */
static NGapppifnet NET_PPPIfnet;
/* LAN interface */
static NGethifnet NET_LANIfnet;
/* Device input buffer */
static NGubyte NET_DeviceInputBuffer[ NET_MAX_DEVIO_SIZE ];
/* Device output buffer */
static NGubyte NET_DeviceOutputBuffer[ NET_MAX_DEVIO_SIZE ];
/* Van Jacobson compression table (must be 32 elements) */
static NGpppvjent NET_VanJacobsonTable[ 32 ];
/* Adress Resolution Protocol Table */
static NGarpent NET_ARPTable[ 10 ];

static NET_STATUS NET_Status = NET_STATUS_ERROR;
static NET_DEVICE_TYPE NET_DeviceType = NET_DEVICE_TYPE_NONE;
static NET_DEVICE_HARDWARE NET_DeviceHardware = NET_DEVICE_HARDWARE_NONE;

static NGuint NET_DNS1 = 0, NET_DNS2 = 0;
static Uint8 NET_DNSWorkArea[ ADNS_WORKSIZE( 8 ) ];

static NetworkInfo3 NET_NetworkInfo;

/* Consider making this use the memory manager? */
static void *NET_StaticBufferAlloc( Uint32 p_Size, NGuint *p_pAddr );

static NGcfgent NET_InternalModemStack[ ] =
{
	NG_BUFO_MAX,				NG_CFG_INT( NET_MAX_BUFFERS ),
	NG_BUFO_ALLOC_F,			NG_CFG_FNC( NET_StaticBufferAlloc ),
	NG_BUFO_HEADER_SIZE,		NG_CFG_INT( sizeof( NGetherhdr ) ),
	NG_BUFO_DATA_SIZE,			NG_CFG_INT( ETHERMTU ),
	NG_SOCKO_MAX,				NG_CFG_INT( NET_MAX_SOCKETS ),
	NG_SOCKO_TABLE,				NG_CFG_PTR( &NET_SocketTable ),
	NG_RTO_CLOCK_FREQ,			NG_CFG_INT( NG_CLOCK_FREQ ),
	NG_DEVCBO_TABLE,			NG_CFG_PTR( NET_DeviceIOControlBlock ),
	NG_DEVCBO_MAX,				NG_CFG_INT( NET_MAX_DEVCBS ),
	/* Protocol */
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_TCP ),
	NG_TCPO_TCB_MAX,			NG_CFG_INT( NET_MAX_SOCKETS ),
	NG_TCPO_TCB_TABLE,			NG_CFG_PTR( &NET_TCPCB ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_UDP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_RAWIP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_IP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_PPP ),
	/* Device */
	NG_CFG_DEVADD,				NG_CFG_PTR( &NET_Device ),
	NG_CFG_DRIVER,				NG_CFG_PTR( &ngSerDrv_Trisignal ),
	NG_DEVO_BAUDS,				NG_CFG_LNG( 115200UL ),
	NG_DEVO_CLKBASE,			NG_CFG_LNG( SH4_PPHI_FREQ ),
	NG_DEVO_IBUF_PTR,			NG_CFG_PTR( &NET_DeviceInputBuffer ),
	NG_DEVO_OBUF_PTR,			NG_CFG_PTR( &NET_DeviceOutputBuffer ),
	NG_DEVO_IBUF_LEN,			NG_CFG_INT( sizeof( NET_DeviceInputBuffer ) ),
	NG_DEVO_OBUF_LEN,			NG_CFG_INT( sizeof( NET_DeviceOutputBuffer ) ),
	/* Interface */
	NG_CFG_IFADDWAIT,			NG_CFG_PTR( &NET_PPPIfnet ),
	NG_CFG_DRIVER,				NG_CFG_PTR( &ngNetDrv_AHDLC ),
	NG_IFO_NAME,				NG_CFG_PTR( "Internal Modem" ),
	/* PPP */
	NG_APPPIFO_DEVICE,			NG_CFG_PTR( &NET_Device ),
	NG_PPPIFO_DEFAULT_ROUTE,	NG_CFG_TRUE,
	NG_PPPIFO_LCP_ASYNC,		NG_CFG_FALSE,
	NG_PPPIFO_LCP_PAP,			NG_CFG_TRUE,
	NG_PPPIFO_LCP_CHAP,			NG_CFG_TRUE,
	NG_PPPIFO_IPCP_VJCOMP,		NG_CFG_TRUE,
	NG_PPPIFO_IPCP_VJTABLE,		NG_CFG_PTR( &NET_VanJacobsonTable ),
	NG_PPPIFO_IPCP_VJMAX,		NG_CFG_INT( sizeof( NET_VanJacobsonTable ) /
									sizeof( NET_VanJacobsonTable[ 0 ] ) ),
	NG_PPPIFO_IPCP_DNS1,		NG_CFG_TRUE,
	NG_PPPIFO_IPCP_DNS2,		NG_CFG_TRUE,
	NG_PPPIFO_MODEM,			NG_CFG_TRUE,
	NG_CFG_END
};

static NGcfgent NET_LANStack[ ] =
{
	NG_BUFO_MAX,				NG_CFG_INT( NET_MAX_BUFFERS ),
	NG_BUFO_ALLOC_F,			NG_CFG_FNC( NET_StaticBufferAlloc ),
	/* Add padding for DMA */
	NG_BUFO_HEADER_SIZE,		NG_CFG_INT( sizeof( NGetherhdr ) + 30 ),
	NG_BUFO_DATA_SIZE,			NG_CFG_INT( ETHERMTU + 31 ),
	NG_BUFO_INPQ_MAX,			NG_CFG_INT( 8 ),
	NG_SOCKO_MAX,				NG_CFG_INT( NET_MAX_SOCKETS ),
	NG_SOCKO_TABLE,				NG_CFG_PTR( &NET_SocketTable ),
	NG_RTO_CLOCK_FREQ,			NG_CFG_INT( NG_CLOCK_FREQ ),
	/* Protocol */
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_TCP ),
	NG_TCPO_TCB_MAX,			NG_CFG_INT( NET_MAX_SOCKETS ),
	NG_TCPO_TCB_TABLE,			NG_CFG_PTR( &NET_TCPCB ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_UDP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_RAWIP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_IP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_PPP ),
	NG_CFG_PROTOADD,			NG_CFG_PTR( &ngProto_ARP ),
	NG_ARPO_MAX,				NG_CFG_INT( sizeof( NET_ARPTable ) /
									sizeof( NET_ARPTable[ 0 ] ) ),
	NG_ARPO_TABLE,				NG_CFG_PTR( &NET_ARPTable ),
	NG_ARPO_EXPIRE,				NG_CFG_INT( 600 ),
	NG_CFG_IFADD,				NG_CFG_PTR( &NET_LANIfnet ),
	NG_CFG_DRIVER,				NG_CFG_PTR( &ngNetDrv_DCLAN ),
	NG_IFO_NAME,				NG_CFG_PTR( "LAN" ),
	NG_ETHIFO_DEV1,				NG_CFG_INT( DCLAN_AUTO ),
	/* Need to add support for DHCP and PPPoE */
	NG_CFG_END
};

int NET_Initialise( void )
{
	/* For now, just look for the standard modem, later this will also include
	 * the LAN/BBA */
	/* One potential issue is that the device may either be disconnected or the
	 * cable connecting it may not be present (BBA showed this behaviour),
	 * therfore requiring polling the device rather than initialising it and
	 * forgetting about it */
	int ReturnStatus;
#if defined ( DEBUG )
	/* 5 - Errors
	 * 2 - Normal debug information
	 * 1 - More verbose debug information
	 * 0 - USE ONLY IF COMPLETELY NECESSARY
	 */
	ngDebugSetLevel( 1 );
	/* Allow DHCP debugging */
	ngDebugSetModule( NG_DBG_DHCP, 1 );
#endif /* DEBUG */
	
	/* In the future, probe for more devices if NG_EINIT_DEVOPEN is returned
	 * until there are no more devices left to try, then return a no device
	 * found error code (should not cause the game to fail) */

	/* Get the Flash settings for the network */
	if( ntInfInit( &NET_NetworkInfo, NULL ) != NTD_OK )
	{
		LOG_Debug( "Failed to retreive the network information from flash" );
		return -2;
	}

	/* First, acquire the network device, either an internal modem or BB/LAN
	 * adapter */
	ReturnStatus = ngInit( NET_InternalModemStack );

	if( ReturnStatus == NG_EINIT_NOERROR )
	{
		NET_DeviceType = NET_DEVICE_TYPE_INTMODEM;
		NET_DeviceHardware = NET_DEVICE_HARDWARE_MODEM;

		NET_pInterface = ngIfGetPtr( "Internal Modem" );
	}
	else /* Okay, maybe there's a LAN/BBA? */
	{
		ngExit( 0 );
		ReturnStatus = ngInit( NET_LANStack );
		NET_DeviceHardware = NET_DEVICE_HARDWARE_LAN;
	}

	if( ReturnStatus != NG_EINIT_NOERROR )
	{
		/* Minus codes are non-fatal */
		int ErrorCode = 1;

		NET_Status = NET_STATUS_ERROR;
		LOG_Debug( "Failed to initialise the NexGen network stack" );

		switch( ReturnStatus )
		{
			case NG_EINIT_DEVOPEN:
			{
				ErrorCode = -1;
				LOG_Debug( "\tCould not open the device" );
				NET_Status = NET_STATUS_NODEVICE;
				break;
			}
			default:
			{
			}
		}

		ngExit( 0 );

		return 0;//ErrorCode;
	}

	/* Must be the BBA/LAN */
	if( NET_DeviceHardware == NET_DEVICE_HARDWARE_LAN )
	{
		int NetSpeed;

		NET_pInterface = ngIfGetPtr( "LAN" );

		ngIfGetOption( NET_pInterface, NG_IFO_DCLAN_CURRENT_SPEED, &NetSpeed );
		
		switch( NetSpeed )
		{
			case DCLAN_10BaseT:
			case DCLAN_10BaseTX:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_10;
				break;
			}
			case DCLAN_100BaseT:
			case DCLAN_100BaseTX:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_100;
				break;
			}
			default:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_UNKNOWN;
			}
		}
	}
	else if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
	{
		ngIfGetOption( NET_pInterface, NG_PPPIFO_IPCP_DNS1_ADDR, &NET_DNS1 );
		ngIfGetOption( NET_pInterface, NG_PPPIFO_IPCP_DNS2_ADDR, &NET_DNS2 );

		/* Initialise the DNS IPs */
		ngDnsInit( NULL, NET_DNS1, NET_DNS2 );
	}

	NET_Status = NET_STATUS_DISCONNECTED;

	/* Initialise the asynchronous DNS work area */
	ngADnsInit( 8, ( void * )&NET_DNSWorkArea );

	ngYield( );

	LOG_Debug( "Initialsed NexGen: %s", ngGetVersionString( ) );

	return 0;
}

void NET_Terminate( void )
{
	if( NET_Status != NET_STATUS_ERROR || NET_Status != NET_STATUS_NODEVICE )
	{
		ngExit( 0 );
	}
}

void NET_Update( void )
{
	if( NET_DeviceHardware == NET_DEVICE_HARDWARE_LAN )
	{
		int NetSpeed;

		ngIfGetOption( NET_pInterface, NG_IFO_DCLAN_CURRENT_SPEED, &NetSpeed );
		
		switch( NetSpeed )
		{
			case DCLAN_10BaseT:
			case DCLAN_10BaseTX:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_10;
				break;
			}
			case DCLAN_100BaseT:
			case DCLAN_100BaseTX:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_100;
				break;
			}
			default:
			{
				NET_DeviceType = NET_DEVICE_TYPE_LAN_UNKNOWN;
			}
		}
	}

	ngYield( );
}

static void *NET_StaticBufferAlloc( Uint32 p_Size, NGuint *p_pAddr )
{
	static unsigned char StaticBuffer[ NET_MAX_STACK_MEM ];

	if( p_Size > NET_MAX_STACK_MEM )
	{
		LOG_Debug( "[NET_StaticBufferAlloc] <ERROR> Too much memory was "
			"requested, maximum of %d, requested %d", NET_MAX_STACK_MEM,
			p_Size );

		return NULL;
	}

	return StaticBuffer;
}

NET_STATUS NET_GetStatus( void )
{
	return NET_Status;
}

NET_DEVICE_TYPE NET_GetDeviceType( void )
{
	return NET_DeviceType;
}

/* Required for the debug version of NexGen */
void ngStdOutChar( int p_Char, void *p_pData )
{
#if defined ( DEBUG )
	static char PrintBuffer[ 128 ];
	static int PrintBufferLength;

	if( p_Char >= 0 )
	{
		if( p_Char == '\n' )
		{
			if( PrintBufferLength == 0 )
			{
				return;
			}
			else
			{
				PrintBuffer[ PrintBufferLength++ ] = p_Char;
				if( PrintBufferLength < sizeof( PrintBuffer ) - 1 )
				{
					return;
				}
			}
			PrintBuffer[ PrintBufferLength ] = '\0';
			LOG_Debug( PrintBuffer );
			PrintBufferLength = 0;
		}
	}
#endif /* DEBUG */
}

