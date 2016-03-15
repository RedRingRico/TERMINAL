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
#include <ngmdm.h>
#include <Nt_utl.h>

#define NET_MAX_BUFFERS		100			/* Message buffers */
#define NET_MAX_STACK_MEM	400*1024	/* 400KiB static memory */
#define NET_MAX_SOCKETS		10			/* Maximum number of sockets open */
#define NET_MAX_DEVCBS		2			/* Maximum simultaneous device control
										 * blocks */
#define NET_MAX_DEVIO_SIZE	2*1024		/* Maximum device I/O bufer size */

#define NET_MAX_COUNTRY_INIT_STR	8
#define NET_MAX_MODEM_FLAGS_STR		13
#define NET_MAX_USER_INIT_STR		51
#define NET_MAX_DIAL_STR			137

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
/* AT command for the country code */
static char NET_CountryInit[ NET_MAX_COUNTRY_INIT_STR ];
/* Modem initialisation AT command */
static char NET_ModemFlags[ NET_MAX_MODEM_FLAGS_STR ];
/* User-defined AT string */
static char NET_UserInit[ NET_MAX_USER_INIT_STR ];
/* AT modem dial command string */
static char NET_Dial[ NET_MAX_DIAL_STR ];
/* DNS server addresses */
static struct in_addr NET_DNSAddress[ 2 ];

static bool NET_Init = false;

static NGmdmstate NET_ModemState;
static char NET_ModemSpeed[ 64 ] = "";

static NGmdmscript NET_ModemDialScript[ ] =
{
	/* Action			Answer			Output	Goto	Return		Timeout */
	{ "ATE0",			"OK",			NULL,	1,		NG_EOK,		1 },
	{ NET_CountryInit,	"OK",			NULL,	2,		NG_EOK,		2 },
	{ NET_ModemFlags,	"OK",			NULL,	3,		NG_EOK,		1 },
	{ NET_UserInit,		"OK",			NULL,	4,		NG_EOK,		1 },
	{ NET_Dial,			"CONNECT",		NULL,	-1,		0x0100,		45 },
	{ NULL,				"NO CARRIER",	NULL,	-1,		0x000B,		0 },
	{ NULL,				"NO ANSWER",	NULL,	-1,		0x000C,		0 },
	{ NULL,				"NO DIALTONE",	NULL,	-1,		0x000D,		0 },
	{ NULL,				"BUSY",			NULL,	-1,		0x000E,		0 },
	{ NULL,				"ERROR",		NULL,	-1,		NG_EINVAL,	0 },
	{ NULL,				NULL,			NULL,	-1,		NG_EOK,		0 }
};

static NET_STATUS NET_Status = NET_STATUS_ERROR;
static NET_DEVICE_TYPE NET_DeviceType = NET_DEVICE_TYPE_NONE;
static NET_DEVICE_HARDWARE NET_DeviceHardware = NET_DEVICE_HARDWARE_NONE;
static NET_INTERNAL_MODEM_RESET NET_InternalModemReset =
	NET_INTERNAL_MODEM_RESET_INIT;

static NGuint NET_DNS1 = 0, NET_DNS2 = 0;
static Uint8 NET_DNSWorkArea[ ADNS_WORKSIZE( 8 ) ];

static NetworkInfo3 NET_NetworkInfo;

/* Consider making this use the memory manager? */
static void *NET_StaticBufferAlloc( Uint32 p_Size, NGuint *p_pAddr );
void NET_ChangeDeviceParams( int p_Set, int p_Clear );

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
	}

	NET_Status = NET_STATUS_DISCONNECTED;

	/* Initialise the asynchronous DNS work area */
	ngADnsInit( 8, ( void * )&NET_DNSWorkArea );

	ngYield( );

	LOG_Debug( "Initialsed NexGen: %s", ngGetVersionString( ) );

	NET_Init = true;

	return 0;
}

void NET_Terminate( void )
{
	if( NET_Status != NET_STATUS_ERROR || NET_Status != NET_STATUS_NODEVICE )
	{
		ngExit( 0 );
	}
}

int NET_ResetInternalModem( int p_DropTime, int p_TimeOut )
{
	static Uint32 TimeStart;
	int DeviceFlags, Size, Error;

	switch( NET_InternalModemReset )
	{
		case NET_INTERNAL_MODEM_RESET_INIT:
		{
			Error = ngDevioOpen( &NET_Device, 0, &NET_DeviceIOControlBlock );

			if( Error != 0 )
			{
				return Error;
			}

			/* Clear DTR (hardware line reset) */
			NET_ChangeDeviceParams( 0, NG_DEVCF_SER_DTR );
			TimeStart = syTmrGetCount( );
			NET_InternalModemReset = NET_INTERNAL_MODEM_RESET_DROP_LINES;
			return NG_EWOULDBLOCK;
		}
		case NET_INTERNAL_MODEM_RESET_DROP_LINES:
		{
			if( syTmrCountToMicro(
					syTmrDiffCount( TimeStart, syTmrGetCount( ) ) ) >=
				( p_DropTime * 1000000 ) )
			{
				/* Set DTR */
				NET_ChangeDeviceParams( NG_DEVCF_SER_DTR, 0 );
				TimeStart = syTmrGetCount( );
				NET_InternalModemReset = NET_INTERNAL_MODEM_RESET_WAIT_FOR_DSR;
			}

			return NG_EWOULDBLOCK;
		}
		case NET_INTERNAL_MODEM_RESET_WAIT_FOR_DSR:
		{
			Size = sizeof( DeviceFlags );
			ngDevioIoctl( NET_DeviceIOControlBlock, NG_IOCTL_DEVCTL,
				NG_DEVCTL_GCFLAGS, &DeviceFlags, &Size );

			if( DeviceFlags & NG_DEVCF_SER_DSR )
			{
				/* Reset successful */
				Error = NG_EOK;
			}
			else if( syTmrCountToMicro(
					syTmrDiffCount( TimeStart, syTmrGetCount( ) ) ) >=
				( p_TimeOut * 1000000 ) )
			{
				Error = NG_ETIMEDOUT;
			}
			else
			{
				return NG_EWOULDBLOCK;
			}

			ngDevioClose( NET_DeviceIOControlBlock );
			NET_InternalModemReset = NET_INTERNAL_MODEM_RESET_INIT;

			return Error;
		}
	}
}

void NET_Update( void )
{
	static bool WaitForDSR = false;

	if( NET_Init )
	{
	ngYield( );

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
	else if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
	{
		switch( NET_Status )
		{
			case NET_STATUS_NEGOTIATING:
			{
				int ModemState = ngModemPoll( &NET_ModemState );

				switch( ModemState )
				{
					case NG_EWOULDBLOCK:
					{
						break;
					}
					case 0x100:
					{
						char *pCharPtr;
						int i = 1;
						int Char;

						LOG_Debug( "Polling PPP" );
						strcpy( NET_ModemSpeed, NET_ModemState.mdst_buf );
						i = strlen( NET_ModemSpeed );
						pCharPtr = NET_ModemSpeed + i;

						LOG_Debug( "Modem speed (before): %s",
							NET_ModemSpeed );
						LOG_Debug( "strlen: %d", i );

						while( ++i < sizeof( NET_ModemSpeed ) )
						{
							Char = ngDevioReadByte( NET_DeviceIOControlBlock,
								0 );

							if( Char >= 0 )
							{
								if( Char < ' ' )
								{
									break;
								}
								else
								{
									*pCharPtr++ = Char;
								}
							}
							else if( Char != NG_EWOULDBLOCK )
							{
								pCharPtr = 0;
								break;
							}
						}

						if( pCharPtr )
						{
							*pCharPtr = '\0';
							LOG_Debug( "Connection speed: %s",
								NET_ModemSpeed );
						}
						else
						{
							LOG_Debug( "Failed to retreive the connection "
								"speed" );
							NET_Status = NET_STATUS_DISCONNECTED;
							break;
						}

						ngDevioClose( NET_DeviceIOControlBlock );

						ngIfOpen( NET_pInterface );
						ngPppStart( NET_pInterface );
						NET_Status = NET_STATUS_PPP_POLL;
						break;
					}
					case NG_ETIMEDOUT:
					{
						LOG_Debug( "Connection timed out" );
						NET_Status = NET_STATUS_DISCONNECTED;
						break;
					}
					default:
					{
						LOG_Debug( "Unknown modem state" );
						NET_Status = NET_STATUS_DISCONNECTED;
					}
				}
				break;
			}
			case NET_STATUS_PPP_POLL:
			{
				int PPPState = ngPppGetState( NET_pInterface );

				switch( PPPState )
				{
					case NG_PIFS_DEAD:
					{
						LOG_Debug( "PPP dead" );
						ngIfClose( NET_pInterface );
						NET_Status = NET_STATUS_DISCONNECTED;
						break;
					}
					case NG_PIFS_IFUP:
					{
						ngIfGetOption( NET_pInterface,
							NG_PPPIFO_IPCP_DNS1_ADDR, &NET_DNS1 );
						ngIfGetOption( NET_pInterface,
							NG_PPPIFO_IPCP_DNS2_ADDR, &NET_DNS2 );

						/* Initialise the DNS IPs */
						ngDnsInit( NULL, NET_DNS1, NET_DNS2 );
						NET_Status = NET_STATUS_CONNECTED;
						LOG_Debug( "Interface up" );
						break;
					}
				}
				break;
			}
			case NET_STATUS_CONNECTED:
			{
				NET_Status = NET_STATUS_CONNECTED;
				if( ngPppGetState( NET_pInterface ) == NG_PIFS_DEAD )
				{
					ngIfClose( NET_pInterface );
					LOG_Debug( "PPP dead" );
					NET_Status = NET_STATUS_RESET;
				}
				break;
			}
			case NET_STATUS_RESET:
			{
				int ModemState = NET_ResetInternalModem( 2, 5 );

				switch( ModemState )
				{
					case NG_EWOULDBLOCK:
					{
						/* Not finished, keep going */
						return;
					}
					case NG_ETIMEDOUT:
					{
						LOG_Debug( "Reset timed out" );
						break;
					}
					case NG_EOK:
					{
						/* Normally, this should check for another number to
						 * dial, but the infrastrucutre isn't present to
						 * support this */
						NET_Status = NET_STATUS_DISCONNECTED;
						LOG_Debug( "Modem reset" );
						ngExit( 0 );
						NET_Init = false;
						break;
					}
					default:
					{
						LOG_Debug( "Failed to reset the internal modem: %d",
							ModemState );
						break;
					}
				}
				
				break;
			}
		}
	}
	}
}

int NET_ConnectToISP( void )
{
	Uint32 Flag;
	int Value;

	if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
	{
		char DialCmd[ 256 ];
		char User[ 49 ];
		char Password[ 17 ];
		char *pUser, *pPassword;
		struct in_addr IPAddress;
		int OpenReturn;

		LOG_Debug( "Starting the modem ISP connection" );

		/* For now, only try the first number */
		ntInfGetDialString( 0, 0, DialCmd, sizeof( DialCmd ) );
		strncpy( NET_CountryInit, "AT", sizeof( NET_CountryInit ) );
		ntInfBuildFlagString( NET_ModemFlags, sizeof( NET_ModemFlags ) );
		ntInfGetModemInit( 0, NET_UserInit, sizeof( NET_UserInit ) );
		strncpy( NET_Dial, DialCmd, sizeof( DialCmd ) );
		ntInfGetLoginId( 0, User, sizeof( User ) );
		ntInfGetLoginPasswd( 0, Password, sizeof( Password ) );
		pUser = User;
		pPassword = Password;
		ngIfSetOption( NET_pInterface, NG_PPPIFO_AUTH_USER, ( void * )&pUser );
		ngIfSetOption( NET_pInterface, NG_PPPIFO_AUTH_SECRET,
			( void * )&pPassword );

		LOG_Debug( "Connecting credentials:  Username: %s | Password: %s",
			pUser, pPassword );

		if( ntInfGetNetworkAccessInfo2Flag( 0, &Flag ) == 0 )
		{
			ntInfGetIPAddr( 0, &IPAddress );

			if( IPAddress.s_addr && ( Flag & NA2_FIXED_ADDRESS ) )
			{
				Value = NG_CFG_FALSE;
				ngIfSetOption( NET_pInterface, NG_PPPIFO_IPCP_ACCEPT_LOCAL,
					( void * )&Value );
				ngIfSetOption( NET_pInterface, NG_IFO_ADDR,
					( void * )&IPAddress.s_addr );
			}

			if( Flag & NA2_NO_HEADERCOMP )
			{
				Value = NG_CFG_FALSE;
				ngIfSetOption( NET_pInterface, NG_PPPIFO_IPCP_VJCOMP,
					( void * )&Value );
			}
		}

		ntInfGetPrimaryDnsAddress( 0, &NET_DNSAddress[ 0 ] );
		ntInfGetSecondaryDnsAddress( 0, &NET_DNSAddress[ 1 ] );

		OpenReturn = ngDevioOpen( &NET_Device, 0, &NET_DeviceIOControlBlock );
		if( OpenReturn == 0 )
		{
			int DevFlags, DeviceFlags;
			int Size;

			LOG_Debug( "Device opened" );

			DeviceFlags = ( NG_DEVCF_SER_DTR | NG_DEVCF_SER_RTS |
				NG_DEVCF_SER_NOHUPCL );
			Size = sizeof( DevFlags );
			ngDevioIoctl( NET_DeviceIOControlBlock, NG_IOCTL_DEVCTL,
				NG_DEVCTL_GCFLAGS, &DeviceFlags, &Size );
			DevFlags &= ~( 0 );
			DevFlags |= DeviceFlags;
			ngDevioIoctl( NET_DeviceIOControlBlock, NG_IOCTL_DEVCTL,
				NG_DEVCTL_SCFLAGS, &DevFlags, &Size );
			
			LOG_Debug( "Country initialisation" );
			LOG_Debug( NET_CountryInit );
			LOG_Debug( "Modem flags" );
			LOG_Debug( NET_ModemFlags );
			LOG_Debug( "User initialisation" );
			LOG_Debug( NET_UserInit );
			LOG_Debug( "Dial" );
			LOG_Debug( NET_Dial );

			LOG_Debug( "Attempting to start the modem initialisation" );
			ngModemInit( &NET_ModemState, NET_DeviceIOControlBlock,
				NET_ModemDialScript );
			NET_Status = NET_STATUS_NEGOTIATING;
			LOG_Debug( "Finished modem initialisation" );

			return 0;
		}
		else
		{
			LOG_Debug( "Failed to open device: %d", OpenReturn );
		}
	}

	return 1;
}

int NET_DisconnectFromISP( void )
{
	if( NET_Status != NET_STATUS_DISCONNECTED )
	{
		LOG_Debug( "Disconnecting from ISP" );
		ngPppStop( NET_pInterface );
		//ngIfClose( NET_pInterface );
		NET_Status = NET_STATUS_RESET;
	}

	return 0;
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

void NET_ChangeDeviceParams( int p_Set, int p_Clear )
{
	int Size, DeviceFlags;

	Size = sizeof( DeviceFlags );

	ngDevioIoctl( NET_DeviceIOControlBlock, NG_IOCTL_DEVCTL,
		NG_DEVCTL_GCFLAGS, &DeviceFlags, &Size );

	DeviceFlags &= ~p_Clear;
	DeviceFlags |= p_Set;

	ngDevioIoctl( NET_DeviceIOControlBlock, NG_IOCTL_DEVCTL,
		NG_DEVCTL_SCFLAGS, &DeviceFlags, &Size );
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

