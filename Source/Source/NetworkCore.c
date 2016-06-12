#include <NetworkCore.h>
#include <Memory.h>
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
#include <Array.h>

#define NET_MAX_BUFFERS		100			/* Message buffers */
#define NET_MAX_SOCKETS		10			/* Maximum number of sockets open */
#define NET_MAX_DEVCBS		2			/* Maximum simultaneous device control
										 * blocks */
#define NET_MAX_DEVIO_SIZE	2*1024		/* Maximum device I/O bufer size */

#define NET_MAX_COUNTRY_INIT_STR	8
#define NET_MAX_MODEM_FLAGS_STR		13
#define NET_MAX_USER_INIT_STR		51
#define NET_MAX_DIAL_STR			137
#define NET_VAN_JACOBSON_TABLE_SIZE	32
#define NET_MAX_ARP					10

static NGsock *NET_pSocketTable = NULL;
/* Pointer to the device control block */
NGdevcb *NET_DeviceControlBlock;
/* Deviced control block strucutre */
static NGdevcb *NET_pDeviceIOControlBlock = NULL;
/* TCP control block */
static NGtcpcb *NET_pTCPControlBlock = NULL;
/* This should probably be in some kind of network device struct */
NGdev NET_Device;
/* Network interface */
NGifnet *NET_pInterface;
/* PPP interface */
NGapppifnet NET_PPPIfnet;
/* Ethernet interface */
NGethifnet NET_EthernetIfnet;
/* Device input buffer */
static NGubyte *NET_pDeviceInputBuffer = NULL;
/* Device output buffer */
static NGubyte *NET_pDeviceOutputBuffer = NULL;
/* Van Jacobson compression table (must be 32 elements) */
static NGpppvjent *NET_pVanJacobsonTable = NULL;
/* Adress Resolution Protocol Table */
static NGarpent *NET_pARPTable = NULL;
/* Modem script */
static NGmdmscript *NET_pModemScript = NULL;
/* AT command for the country code */
static char *NET_pCountryInit = NULL;
/* Modem initialisation AT command */
static char *NET_pModemFlags = NULL;
/* User-defined AT string */
static char *NET_pUserInit = NULL;
/* AT modem dial command string */
static char *NET_pDial = NULL;
/* DNS server addresses */
struct in_addr NET_DNSAddress[ 2 ];
/* Answer from polling */
static ngADnsAnswer NET_DNSAnswer;
/* The array of DNS requests */
static ARRAY NET_DNSRequests;
/* Memory block for any of NexGen needs */
static PMEMORY_BLOCK NET_pNetMemoryBlock = NULL;

static bool NET_Initialised = false;

static int NET_DevOpen = 0;
static int NET_IfaceOpen = 0;

NGmdmstate NET_ModemState;

static NET_STATUS NET_Status = NET_STATUS_NODEVICE;
static NET_DEVICE_TYPE NET_DeviceType = NET_DEVICE_TYPE_NONE;
static NET_DEVICE_HARDWARE NET_DeviceHardware = NET_DEVICE_HARDWARE_NONE;

static NGuint NET_DNS1 = 0, NET_DNS2 = 0;
static Uint8 NET_DNSWorkArea[ ADNS_WORKSIZE( 8 ) ];

static NetworkInfo3 NET_NetworkInfo;

/* Consider making this use the memory manager? */
static void *NET_MemoryBufferAlloc( Uint32 p_Size, NGuint *p_pAddress );
static void NET_MemoryBufferFree( NGuint *p_pAddress );
void NET_ChangeDeviceParams( int p_Set, int p_Clear );

void NET_SetConfigElement( NGcfgent *p_pConfig, size_t *p_pIndex,
	u_int p_Option, NGcfgarg p_Arg )
{
	p_pConfig[ ( *p_pIndex ) ].cfg_option = p_Option;
	p_pConfig[ ( *p_pIndex ) ].cfg_arg = p_Arg;

	++( *p_pIndex );
}

static NGcfgent *NET_InitInternalModemStack( void )
{
	NGcfgent *pInternalModemStack = NULL;
	Uint32 ElementCount =  40UL;
	size_t Index = 0;

#if defined( DEBUG )
	ElementCount = 43UL;
#endif /* DEBUG */

	pInternalModemStack = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		sizeof( NGcfgent ) * ElementCount, "NET: Internal modem stack" );

	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_BUFO_MAX, NET_MAX_BUFFERS );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_BUFO_ALLOC_F, NG_CFG_FNC( NET_MemoryBufferAlloc ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_BUFO_FREE_F, NG_CFG_FNC( NET_MemoryBufferFree ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_BUFO_HEADER_SIZE, NG_CFG_INT( sizeof( NGetherhdr ) ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_BUFO_DATA_SIZE, NG_CFG_INT( ETHERMTU ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_SOCKO_MAX, NG_CFG_INT( NET_MAX_SOCKETS ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_SOCKO_TABLE, NG_CFG_PTR( NET_pSocketTable ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_RTO_CLOCK_FREQ, NG_CFG_INT( NG_CLOCK_FREQ ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVCBO_TABLE, NG_CFG_PTR( NET_pDeviceIOControlBlock ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVCBO_MAX, NG_CFG_INT( NET_MAX_DEVCBS ) );
#if defined ( DEBUG )
	NET_SetConfigElement( pInternalModemStack, &Index, NG_DEBUGO_LEVEL,
		NG_CFG_INT( 1 ) );
	NET_SetConfigElement( pInternalModemStack, &Index, NG_DEBUGO_MODULE,
		NG_CFG_INT( NG_DBG_PPP | NG_DBG_DRV | NG_DBG_CORE | NG_DBG_APP ) );
#endif /* DEBUG */
	/* Protocol */
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_TCP ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_TCPO_TCB_MAX, NG_CFG_INT( NET_MAX_SOCKETS ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_TCPO_TCB_TABLE, NG_CFG_PTR( NET_pTCPControlBlock ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_UDP ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_RAWIP ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_IP ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_PPP ) );
	/* Device */ 
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_DEVADD, NG_CFG_PTR( &NET_Device ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_DRIVER, NG_CFG_PTR( &ngSerDrv_Trisignal ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_BAUDS, NG_CFG_LNG( 115200UL ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_CLKBASE, NG_CFG_LNG( SH4_PPHI_FREQ ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_IBUF_PTR, NG_CFG_PTR( NET_pDeviceInputBuffer ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_OBUF_PTR, NG_CFG_PTR( NET_pDeviceOutputBuffer ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_IBUF_LEN, NG_CFG_INT( NET_MAX_DEVIO_SIZE ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_DEVO_OBUF_LEN, NG_CFG_INT( NET_MAX_DEVIO_SIZE ) );
	/* Interface */
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_IFADDWAIT, NG_CFG_PTR( &NET_PPPIfnet ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_DRIVER, NG_CFG_PTR( &ngNetDrv_AHDLC ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_IFO_NAME, NG_CFG_PTR( "Internal Modem" ) );
#if defined ( DEBUG )
	NET_SetConfigElement( pInternalModemStack, &Index, NG_IFO_FLAGS,
		NG_CFG_INT( NG_IFF_DEBUG ) );
#endif /* DEBUG */
	/* PPP */
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_APPPIFO_DEVICE, NG_CFG_PTR( &NET_Device ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_DEFAULT_ROUTE, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_LCP_ASYNC, NG_CFG_FALSE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_LCP_PAP, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_LCP_CHAP, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_IPCP_VJCOMP, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_IPCP_VJTABLE, NG_CFG_PTR( NET_pVanJacobsonTable ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_IPCP_VJMAX, NG_CFG_INT( NET_VAN_JACOBSON_TABLE_SIZE ) );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_IPCP_DNS1, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_IPCP_DNS2, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_PPPIFO_MODEM, NG_CFG_TRUE );
	NET_SetConfigElement( pInternalModemStack, &Index,
		NG_CFG_END, 0 );

	return pInternalModemStack;
}

static NGcfgent *NET_InitEthernetStack( void )
{
	NGcfgent *pEthernetStack = NULL;
	Uint32 ElementCount =  24UL;
	size_t Index = 0;

	pEthernetStack = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		sizeof( NGcfgent ) * 43, "NET: Internal modem stack" );

	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_MAX, NG_CFG_INT( NET_MAX_BUFFERS ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_ALLOC_F, NG_CFG_FNC( NET_MemoryBufferAlloc ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_FREE_F, NG_CFG_FNC( NET_MemoryBufferFree ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_HEADER_SIZE, NG_CFG_INT( sizeof( NGetherhdr ) + 30 ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_DATA_SIZE, ETHERMTU + 31 );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_BUFO_INPQ_MAX, NG_CFG_INT( 8 ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_SOCKO_MAX, NG_CFG_INT( NET_MAX_SOCKETS ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_SOCKO_TABLE, NG_CFG_PTR( NET_pSocketTable ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_RTO_CLOCK_FREQ, NG_CFG_INT( NG_CLOCK_FREQ ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_TCP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_TCPO_TCB_MAX, NG_CFG_INT( NET_MAX_SOCKETS ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_TCPO_TCB_TABLE, NG_CFG_PTR( NET_pTCPControlBlock ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_UDP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_RAWIP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_IP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_PPP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_PROTOADD, NG_CFG_PTR( &ngProto_ARP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_ARPO_MAX, NG_CFG_INT( NET_MAX_ARP ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_ARPO_TABLE, NG_CFG_PTR( NET_pARPTable ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_ARPO_EXPIRE, 600 );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_IFADD, NG_CFG_PTR( &NET_EthernetIfnet ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_DRIVER, NG_CFG_PTR( &ngNetDrv_DCLAN ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_IFO_NAME, NG_CFG_PTR( "Ethernet" ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_ETHIFO_DEV1, NG_CFG_INT( DCLAN_AUTO ) );
	NET_SetConfigElement( pEthernetStack, &Index,
		NG_CFG_END, 0 );

	return pEthernetStack;
}

int NET_Initialise( PNETWORK_CONFIGURATION p_pNetworkConfiguration )
{
	/* For now, just look for the standard modem, later this will also include
	 * the LAN/BBA */
	/* One potential issue is that the device may either be disconnected or the
	 * cable connecting it may not be present (BBA showed this behaviour),
	 * therfore requiring polling the device rather than initialising it and
	 * forgetting about it */
	int ReturnStatus = 0;
	NGcfgent *pInternalStack = NULL;

	/* Make sure that the network is off to a sane start */
	NET_Terminate( );

	if( p_pNetworkConfiguration->pMemoryBlock == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Memory block is null\n" );

		return -1;
	}

	/* Allocate all memory required by NexGen */
	NET_pNetMemoryBlock = p_pNetworkConfiguration->pMemoryBlock;

	if( ( NET_pSocketTable = MEM_AllocateFromBlock( NET_pNetMemoryBlock, 
		sizeof( NGsock ) * NET_MAX_SOCKETS, "NET: Socket table" ) ) == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the socket table\n" );

		return -1;
	}

	if( ( NET_pDeviceIOControlBlock = MEM_AllocateFromBlock(
		NET_pNetMemoryBlock, sizeof( NGdevcb ) * NET_MAX_DEVCBS,
		"NET: Device I/O control block" ) ) == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the device I/O control block\n" );

		return -1;
	}

	if( ( NET_pDeviceInputBuffer = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		NET_MAX_DEVIO_SIZE, "NET: Device input buffer" ) ) == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the device input buffer\n" );

		return -1;
	}

	if( ( NET_pDeviceOutputBuffer = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		NET_MAX_DEVIO_SIZE, "NET: Device output buffer" ) ) == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the device output buffer\n" );

		return -1;
	}

	if( ( NET_pTCPControlBlock = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		sizeof( NGtcpcb ) * NET_MAX_SOCKETS, "NET: TCP control block" ) ) ==
		NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the TCP control block\n" );

		return -1;
	}

	if( ( NET_pVanJacobsonTable = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
		sizeof( NGpppvjent ) * NET_VAN_JACOBSON_TABLE_SIZE,
		"NET: Van Jacobson compression table" ) ) == NULL )
	{
		LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory for "
			"the Van Jacobson compression table\n" );

		return -1;
	}
	
	/* In the future, probe for more devices if NG_EINIT_DEVOPEN is returned
	 * until there are no more devices left to try, then return a no device
	 * found error code (should not cause the game to fail) */

	/* Get the Flash settings for the network */
	if( ntInfInit( &NET_NetworkInfo, NULL ) != NTD_OK )
	{
		LOG_Debug( "Failed to retreive the network information from flash" );
		return -2;
	}

	pInternalStack = NET_InitInternalModemStack( );

	/* First, acquire the network device, either an internal modem or BB/LAN
	 * adapter */
	ReturnStatus = ngInit( pInternalStack );

	/* Done with the temporary stack */
	MEM_FreeFromBlock( NET_pNetMemoryBlock, pInternalStack );

	if( ReturnStatus == NG_EINIT_NOERROR )
	{
		NET_DeviceType = NET_DEVICE_TYPE_INTMODEM;
		NET_DeviceHardware = NET_DEVICE_HARDWARE_MODEM;

#if defined ( DEBUG )
	/* 5 - Errors
	 * 2 - Normal debug information
	 * 1 - More verbose debug information
	 * 0 - USE ONLY IF COMPLETELY NECESSARY
	 */
	ngDebugSetLevel( 2 );
	/* Allow DHCP debugging */
	ngDebugSetModule( NG_DBG_DHCP, 1 );
	ngDebugSetModule( NG_DBG_PPP, 1 );
	ngDebugSetModule( NG_DBG_APP, 1 );
	ngDebugSetModule( NG_DBG_DRV, 1 );
	ngDebugSetModule( NG_DBG_CORE, 1 );
#endif /* DEBUG */

		NET_pInterface = ngIfGetPtr( "Internal Modem" );
	}
	else /* Okay, maybe there's a LAN/BBA? */
	{
		ngExit( 0 );

		/* Deallocate the modem-specific memory */
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pVanJacobsonTable );
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceIOControlBlock );
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceInputBuffer );
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceOutputBuffer );
		NET_pVanJacobsonTable = NULL;
		NET_pDeviceIOControlBlock = NULL;
		NET_pDeviceInputBuffer = NULL;
		NET_pDeviceOutputBuffer = NULL;

		MEM_GarbageCollectMemoryBlock( NET_pNetMemoryBlock );

		/* Allocate the Ethernet-specific memory */
		if( ( NET_pARPTable = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			sizeof( NGarpent ) * NET_MAX_ARP, "NET: ARP table" ) ) == NULL )
		{
			LOG_Debug( "[NET_Initialise] <ERROR> Failed to allocate memory "
				"for the TCP ARP table\n" );

			return -1;
		}
		pInternalStack = NET_InitEthernetStack( );
		ReturnStatus = ngInit( pInternalStack );
		MEM_FreeFromBlock( NET_pNetMemoryBlock, pInternalStack );
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

		ntInfExit( );
		ngExit( 0 );

		return 0;//ErrorCode;
	}

	/* Must be the BBA/LAN */
	if( NET_DeviceHardware == NET_DEVICE_HARDWARE_LAN )
	{
		int NetSpeed;

		NET_pInterface = ngIfGetPtr( "Ethernet" );

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

		ngIfSetOption( NET_pInterface, NG_ETHIFO_DEV1, ( void * )&NetSpeed );
	}
	else if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
	{
	}

	NET_Status = NET_STATUS_DISCONNECTED;

	/* Initialise the asynchronous DNS work area */
	ngADnsInit( 8, ( void * )&NET_DNSWorkArea );

	ARY_Initialise( &NET_DNSRequests, p_pNetworkConfiguration->pMemoryBlock, 8,
		sizeof( DNS_REQUEST ), 0, "DNS Request Array" );

	LOG_Debug( "Initialsed NexGen: %s", ngGetVersionString( ) );

	ngYield( );

	NET_Initialised = true;

	return 0;
}

void NET_Terminate( void )
{
	if( NET_Initialised == true )
	{
		LOG_Debug( "NET_Terminate <INFO> Disconnecting from ISP\n" );
		/* Safely disconnect */
		NET_DisconnectFromISP( );
		NET_Update( );

		while( ( NET_Status != NET_STATUS_DISCONNECTED ) &&
			( NET_Status != NET_STATUS_NODEVICE ) )
		{
			NET_Update( );
		}

		LOG_Debug( "\tDONE!\n" );

		if( NET_Status != NET_STATUS_NODEVICE )
		{
			NET_Status = NET_STATUS_NODEVICE;
			ntInfExit( );
			ngExit( 0 );
		}
	}

	if( NET_pSocketTable != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pSocketTable );
		NET_pSocketTable = NULL;
	}

	if( NET_pDeviceIOControlBlock != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceIOControlBlock );
		NET_pDeviceIOControlBlock = NULL;
	}

	if( NET_pDeviceInputBuffer != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceInputBuffer );
		NET_pDeviceInputBuffer = NULL;
	}

	if( NET_pDeviceOutputBuffer != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDeviceOutputBuffer );
		NET_pDeviceOutputBuffer = NULL;
	}

	if( NET_pTCPControlBlock != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pTCPControlBlock ); 
		NET_pTCPControlBlock = NULL;
	}

	if( NET_pVanJacobsonTable != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pVanJacobsonTable );
		NET_pVanJacobsonTable = NULL;
	}

	if( NET_pModemScript != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pModemScript );
		NET_pModemScript = NULL;
	}

	if( NET_pCountryInit != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pCountryInit );
		NET_pCountryInit = NULL;
	}

	if( NET_pModemFlags != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pModemFlags );
		NET_pModemFlags = NULL;
	}

	if( NET_pUserInit != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pUserInit );
		NET_pUserInit = NULL;
	}

	if( NET_pDial != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pDial );
		NET_pDial = NULL;
	}

	if( NET_pARPTable != NULL )
	{
		MEM_FreeFromBlock( NET_pNetMemoryBlock, NET_pARPTable );
		NET_pARPTable = NULL;
	}

	ARY_Terminate( &NET_DNSRequests );

	if( NET_pNetMemoryBlock != NULL)
	{
		MEM_GarbageCollectMemoryBlock( NET_pNetMemoryBlock );
	}
}

int NET_ResetInternalModem( int p_DropTime, int p_TimeOut )
{
	static Uint32 TimeStart;
	static NET_INTERNAL_MODEM_RESET ModemResetState =
		NET_INTERNAL_MODEM_RESET_INIT;
	int DeviceFlags, Size, Error;

	switch( ModemResetState )
	{
		case NET_INTERNAL_MODEM_RESET_INIT:
		{
			Error = ngDevioOpen( &NET_Device, 0, &NET_DeviceControlBlock );

			if( Error != 0 )
			{
				LOG_Debug( "Modem Reset: ailed to open network device" );
				return Error;
			}

			++NET_DevOpen;

			/* Clear DTR (hardware line reset) */
			NET_ChangeDeviceParams( 0, NG_DEVCF_SER_DTR );
			TimeStart = syTmrGetCount( );
			ModemResetState = NET_INTERNAL_MODEM_RESET_DROP_LINES;
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
				ModemResetState = NET_INTERNAL_MODEM_RESET_WAIT_FOR_DSR;
			}

			return NG_EWOULDBLOCK;
		}
		case NET_INTERNAL_MODEM_RESET_WAIT_FOR_DSR:
		{
			Size = sizeof( DeviceFlags );
			ngDevioIoctl( NET_DeviceControlBlock, NG_IOCTL_DEVCTL,
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
				LOG_Debug( "Blocking" );
				return NG_EWOULDBLOCK;
			}

			ngDevioClose( NET_DeviceControlBlock );
			--NET_DevOpen;
			ModemResetState = NET_INTERNAL_MODEM_RESET_INIT;

			return Error;
		}
		default:
		{
			LOG_Debug( "Modem Reset: Unknown reset state: %d",
				ModemResetState );
		}
	}

	/* Shouldn't get here... */
	return NG_EWOULDBLOCK;
}

void NET_Update( void )
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

		NET_DNSPoll( );
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

						ngDevioClose( NET_DeviceControlBlock );
						--NET_DevOpen;

						ngIfOpen( NET_pInterface );
						++NET_IfaceOpen;
						ngPppStart( NET_pInterface );
						NET_Status = NET_STATUS_PPP_POLL;

						break;
					}
					case NG_ETIMEDOUT:
					{
						LOG_Debug( "Connection timed out" );
						NET_Status = NET_STATUS_DISCONNECTED;

						ngDevioClose( NET_DeviceControlBlock );
						--NET_DevOpen;
						break;
					}
					case 0x000E:
					{
						LOG_Debug( "Modem busy" );
						NET_Status = NET_STATUS_DISCONNECTED;

						ngDevioClose( NET_DeviceControlBlock );
						--NET_DevOpen;
						break;
					}
					default:
					{
						LOG_Debug( "Unknown modem state: 0x%08X", ModemState );
						NET_Status = NET_STATUS_DISCONNECTED;

						ngDevioClose( NET_DeviceControlBlock );
						--NET_DevOpen;
						break;
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
						int Error;
						LOG_Debug( "NET_STATUS_PPP_POLL: PPP dead" );
						Error = ngIfClose( NET_pInterface );

						if( Error != NG_EOK )
						{
							LOG_Debug( "Could not close interface: %d",
								Error );
						}
						--NET_IfaceOpen;

						NET_Status = NET_STATUS_RESET;
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
					int Error;
					LOG_Debug( "NET_STATUS_CONNECTED: PPP dead" );
					Error = ngIfClose( NET_pInterface );

					if( Error != NG_EOK )
					{
						LOG_Debug( "Could not close interface: %d",
							Error );
					}
					--NET_IfaceOpen;
					NET_Status = NET_STATUS_RESET;
					break;
				}

				NET_DNSPoll( );

				break;
			}
			case NET_STATUS_RESET:
			{
				int ModemState = NET_ResetInternalModem( 2, 5 );
				//LOG_Debug( "Resetting" );

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
						LOG_Debug( "Modem reset" );
						ntInfExit( );
						ngExit( 0 );
						NET_Status = NET_STATUS_NODEVICE;
						NET_Initialised = false;
						break;
					}
					default:
					{
						LOG_Debug( "Failed to reset the internal modem: %d",
							ModemState );
						NET_Status = NET_STATUS_DISCONNECTED;
						break;
					}
				}
				break;
			}
		}
	}
}

void NET_AddModemCommand( NGmdmscript *p_pModemScript, size_t *p_pElement,
	char *p_pAction, char *p_pAnswer, char *p_pOutput, int p_GoTo,
	int p_Return, int p_TimeOut )
{
	p_pModemScript[ ( *p_pElement ) ].mdm_action = p_pAction;
	p_pModemScript[ ( *p_pElement ) ].mdm_answer = p_pAnswer;
	p_pModemScript[ ( *p_pElement ) ].mdm_output = p_pOutput;
	p_pModemScript[ ( *p_pElement ) ].mdm_goto = p_GoTo;
	p_pModemScript[ ( *p_pElement ) ].mdm_retval = p_Return;
	p_pModemScript[ ( *p_pElement ) ].mdm_timeout = p_TimeOut;

	++( *p_pElement );
}

int NET_ConnectToISP( void )
{
	Uint32 Flag;
	int Value;

	/* Can't do anything until the network stack is initialised */
	if( NET_Initialised == false )
	{
		LOG_Debug( "NET_ConnectToISP <ERROR> Network not initialised\n" );

		return 1;
	}

	if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
	{
		char DialCmd[ NET_MAX_DIAL_STR ];
		char User[ 49 ];
		char Password[ 17 ];
		char *pUser, *pPassword;
		struct in_addr IPAddress;
		int OpenReturn;
		size_t ModemIndex = 0;
		size_t Index = 0;

		NET_pModemScript = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			sizeof( NGmdmscript ) * 11, "NET: Modem script" );
		NET_pCountryInit = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			NET_MAX_COUNTRY_INIT_STR, "NET: Modem country initialisation" );
		NET_pModemFlags = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			NET_MAX_MODEM_FLAGS_STR, "NET: Modem flags" );
		NET_pUserInit = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			NET_MAX_USER_INIT_STR, "NET: Modem user initialisation" );
		NET_pDial = MEM_AllocateFromBlock( NET_pNetMemoryBlock,
			NET_MAX_DIAL_STR, "NET: Modem dial" );

		NET_AddModemCommand( NET_pModemScript, &ModemIndex, "ATE0", "OK",
			NULL, 1, NG_EOK, 1 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NET_pCountryInit,
			"OK", NULL, 2, NG_EOK, 2 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NET_pModemFlags,
			"OK", NULL, 3, NG_EOK, 1 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NET_pUserInit,
			"OK", NULL, 4, NG_EOK, 1 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NET_pDial,
			"CONNECT", NULL, -1, 0x0100, 45 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL, "NO CARRIER",
			NULL, -1, 0x000B, 0 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL, "NO ANSWER",
			NULL, -1, 0x000C, 0 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL,
			"NO DIALTOME", NULL, -1, 0x000D, 0 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL, "BUSY",
			NULL, -1, 0x000E, 0 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL, "ERROR",
			NULL, -1, NG_EINVAL, 0 );
		NET_AddModemCommand( NET_pModemScript, &ModemIndex, NULL, NULL, NULL,
			-1, NG_EOK, 0 );

		LOG_Debug( "Starting the modem ISP connection" );

		/* For now, only try the first number */
		ntInfGetDialString( 0, 0, DialCmd, sizeof( DialCmd ) );
		strncpy( NET_pCountryInit, "AT", NET_MAX_COUNTRY_INIT_STR );
		ntInfBuildFlagString( NET_pModemFlags, NET_MAX_MODEM_FLAGS_STR );
#if defined ( DEBUG ) || defined( DEVELOPMENT )
		strcat( NET_pModemFlags, "M1L3" );
#endif /* DEBUG || DEVELOPMENT */
		ntInfGetModemInit( 0, NET_pUserInit, NET_MAX_USER_INIT_STR );
		strncpy( NET_pDial, DialCmd, NET_MAX_DIAL_STR );
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

		OpenReturn = ngDevioOpen( &NET_Device, 0, &NET_DeviceControlBlock );
		if( OpenReturn == 0 )
		{
			int DeviceFlags;

			++NET_DevOpen;

			LOG_Debug( "Device opened" );

			DeviceFlags = ( NG_DEVCF_SER_DTR | NG_DEVCF_SER_RTS |
				NG_DEVCF_SER_NOHUPCL );
			NET_ChangeDeviceParams( DeviceFlags, 0 );
			
			LOG_Debug( "Country initialisation" );
			LOG_Debug( NET_pCountryInit );
			LOG_Debug( "Modem flags" );
			LOG_Debug( NET_pModemFlags );
			LOG_Debug( "User initialisation" );
			LOG_Debug( NET_pUserInit );
			LOG_Debug( "Dial" );
			LOG_Debug( NET_pDial );

			LOG_Debug( "Attempting to start the modem initialisation" );
			ngModemInit( &NET_ModemState, NET_DeviceControlBlock,
				NET_pModemScript );
			NET_Status = NET_STATUS_NEGOTIATING;
			LOG_Debug( "Finished modem initialisation" );

			return 0;
		}
		else
		{
			LOG_Debug( "Failed to open device: %d", OpenReturn );
		}
	}
	else if( NET_DeviceHardware == NET_DEVICE_HARDWARE_LAN )
	{
		struct in_addr IP, Subnet, Gateway, DNS1, DNS2;
		int Error;

		/* Should check for static/DHCP/PPPoE */
		/* Just use static configuration for now */
		Error = ntInfGetIPAddr( 3, &IP );

		if( Error || !IP.s_addr )
		{
			LOG_Debug( "Failed to acquire static IP address from flash RAM" );

			return 1;
		}

		Error = ngIfSetOption( NET_pInterface, NG_IFO_ADDR,
			( void * )&IP.s_addr );

		if( Error )
		{
			LOG_Debug( "Failed to set the IP address" );

			return 1;
		}

		Error = ntInfGetSubnetMask( &Subnet );

		if( Error || !Subnet.s_addr )
		{
			LOG_Debug( "Failed to acquire the subnet mask from flash RAM" );

			return 1;
		}

		Error = ngIfSetOption( NET_pInterface, NG_IFO_NETMASK,
			( void * )&Subnet.s_addr );

		if( Error )
		{
			LOG_Debug( "Failed to set the subnet mask" );

			return 1;
		}

		Error = ntInfGetPrimaryGateway( &Gateway );

		if( Error || !Gateway.s_addr )
		{
			LOG_Debug( "Failed to retrieve the static gateway from flash "
				"RAM" );

			return 1;
		}

		Error = ngRouteDefault( Gateway.s_addr );

		if( Error )
		{
			LOG_Debug( "Failed to add the default gateway to the routing "
				"table" );

			return 1;
		}

		Error = ntInfGetPrimaryDnsAddress( 3, &DNS1 );

		if( Error || !DNS1.s_addr )
		{
			LOG_Debug( "Could not get the primary DNS address from flash "
				"RAM" );
		}

		ntInfGetSecondaryDnsAddress( 3, &DNS2 );

		ngDnsInit( NULL, DNS1.s_addr, DNS2.s_addr );

		NET_Status = NET_STATUS_CONNECTED;

		return 0;
	}

	return 1;
}

int NET_DisconnectFromISP( void )
{
	if( NET_Status != NET_STATUS_DISCONNECTED )
	{
		LOG_Debug( "Disconnecting from ISP" );

		if( NET_DeviceHardware == NET_DEVICE_HARDWARE_MODEM )
		{
			switch( NET_Status )
			{
				case NET_STATUS_NEGOTIATING:
				{
					ngDevioClose( NET_DeviceControlBlock );
					--NET_DevOpen;
					NET_Status = NET_STATUS_RESET;
					break;
				}
				case NET_STATUS_PPP_POLL:
				{
					ngPppStop( NET_pInterface );
					break;
				}
				case NET_STATUS_CONNECTED:
				{
					ngPppStop( NET_pInterface );
					NET_Status = NET_STATUS_PPP_POLL;
					break;
				}
			}
		}
		else if( NET_DeviceHardware == NET_DEVICE_HARDWARE_LAN )
		{
			NET_Status = NET_STATUS_DISCONNECTED;
		}
	}

	return 0;
}

static void *NET_MemoryBufferAlloc( Uint32 p_Size, NGuint *p_pAddress )
{
	void *pMemoryBlock;

	pMemoryBlock = MEM_AllocateFromBlock( NET_pNetMemoryBlock, p_Size,
		"NET: Stack memory" );

	if( pMemoryBlock == NULL )
	{
		LOG_Debug( "[NET_MemoryBufferAlloc] <ERROR> Unable to fulfil memory "
			"allocation request for %lu bytes of memory\n", p_Size );

		return pMemoryBlock;
	}

#if defined ( DEBUG )
	LOG_Debug( "[NET_MemoryBufferAlloc] <INFO> Allocated %lu bytes of "
		"memory\n", p_Size, pMemoryBlock );
#endif /* DEBUG */

	return pMemoryBlock;
}

static void NET_MemoryBufferFree( NGuint *p_pAddress )
{
	MEM_FreeFromBlock( NET_pNetMemoryBlock, p_pAddress );
}

void NET_ChangeDeviceParams( int p_Set, int p_Clear )
{
	int Size, DeviceFlags;

	Size = sizeof( DeviceFlags );

	ngDevioIoctl( NET_DeviceControlBlock, NG_IOCTL_DEVCTL,
		NG_DEVCTL_GCFLAGS, &DeviceFlags, &Size );

	DeviceFlags &= ~p_Clear;
	DeviceFlags |= p_Set;

	ngDevioIoctl( NET_DeviceControlBlock, NG_IOCTL_DEVCTL,
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

int NET_GetDevOpen( void )
{
	return NET_DevOpen;
}

int NET_GetIfaceOpen( void )
{
	return NET_IfaceOpen;
}

int NET_DNSRequest( PDNS_REQUEST p_pQuery, const char *p_pDomain )
{
	if( strlen( p_pDomain ) > NG_DNS_NAME_MAX )
	{
		LOG_Debug( "NET_DNSRequest <ERROR> Could not add domain name: \"%s\" "
			"Name is too long\n", p_pDomain );

		return 1;
	}

	ngADnsGetTicket( &p_pQuery->Ticket, p_pDomain );

	p_pQuery->Status = DNS_REQUEST_POLLING;
	p_pQuery->IP = 0;

#if defined ( DEBUG )
	p_pQuery->IPAddress[ 0 ] = '\0';
	strncpy( p_pQuery->Domain, p_pDomain, NG_DNS_NAME_MAX );
#endif /* DEBUG */

	/* Add the query to the array */
	ARY_Append( &NET_DNSRequests, &p_pQuery );

	return 0;
}

void NET_DNSRemoveRequest( PDNS_REQUEST p_pQuery )
{
	/* Find the ticket in the array and remove it */
	size_t Index, ArraySize = ARY_GetCount( &NET_DNSRequests );

	for( Index = 0; Index < ArraySize; ++Index )
	{
		PDNS_REQUEST pRequest = ARY_GetItem( &NET_DNSRequests, Index );

		if( pRequest->Ticket == p_pQuery->Ticket )
		{
			ARY_RemoveAtUnordered( &NET_DNSRequests, Index );
			ngADnsReleaseTicket( &pRequest->Ticket );

			break;
		}
	}
}

void NET_DNSPoll( void )
{
	int PollStatus = ngADnsPoll( &NET_DNSAnswer );

	if( PollStatus != NG_EWOULDBLOCK )
	{
		/* Check which request succeeded */
		size_t Index, ArraySize = ARY_GetCount( &NET_DNSRequests );

		for( Index = 0; Index < ArraySize; ++Index )
		{
			if( PollStatus == NG_EOK )
			{
				size_t *pDNSRequestPointer = ARY_GetItem( &NET_DNSRequests,
					Index );
				PDNS_REQUEST pRequest =
					( PDNS_REQUEST )( *pDNSRequestPointer );

				if( NET_DNSAnswer.ticket == pRequest->Ticket )
				{
					struct in_addr **ppAddrList;
					pRequest->Status = DNS_REQUEST_RESOLVED;
					ARY_RemoveAtUnordered( &NET_DNSRequests, Index );
					ngADnsReleaseTicket( &pRequest->Ticket );

					ppAddrList =
						( struct in_addr ** )NET_DNSAnswer.addr->h_addr_list;

#if defined ( DEBUG )
					sprintf( pRequest->IPAddress, "%s",
						inet_ntoa( *ppAddrList[ 0 ] ) );
#endif /* DEBUG */

					pRequest->IP = ( *ppAddrList[ 0 ] ).s_addr;

					break;
				}
			}
			else if( PollStatus == NG_ETIMEDOUT )
			{
				size_t *pDNSRequestPointer = ARY_GetItem( &NET_DNSRequests,
					Index );
				PDNS_REQUEST pRequest =
					( PDNS_REQUEST )( *pDNSRequestPointer );

				if( NET_DNSAnswer.ticket == pRequest->Ticket )
				{
					ARY_RemoveAtUnordered( &NET_DNSRequests, Index );
					ngADnsReleaseTicket( &pRequest->Ticket );

					pRequest->Status = DNS_REQUEST_FAILED;

					break;
				}
			}
		}
	}
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

