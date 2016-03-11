#include <Networking.h>
#include <Log.h>
#include <ngdc.h>
#include <ngip/ethernet.h>
#include <ngos.h>
#include <ngtcp.h>
#include <ngudp.h>
#include <ngppp.h>
#include <ngappp.h>

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
/* PPP interface */
static NGapppifnet NET_PPPIface;
/* Device input buffer */
static NGubyte NET_DeviceInputBuffer[ NET_MAX_DEVIO_SIZE ];
/* Device output buffer */
static NGubyte NET_DeviceOutputBuffer[ NET_MAX_DEVIO_SIZE ];
/* Van Jacobson compression table (must be 32 elements) */
static NGpppvjent NET_VanJacobsonTable[ 32 ];


/* Consider making this use the memory manager? */
static void *NET_StaticBufferAlloc( Uint32 p_Size, NGuint *p_pAddr );

static NGcfgent NET_InternalModelStack[ ] =
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
	NG_CFG_IFADDWAIT,			NG_CFG_PTR( &NET_PPPIface ),
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

int NET_Initialise( void )
{
	/* For now, just look for the standard modem, later this will also include
	 * the LAN/BBA */
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
	 if( ngInit( NET_InternalModelStack ) != NG_EOK )
	 {
		 LOG_Debug( "Failed to initialise the NexGen network stack" );

		 ngExit( 1 );

		 return 1;
	 }

	 ngYield( );

	 LOG_Debug( "Initialsed NexGen: %s", ngGetVersionString( ) );

	return 0;
}

void NET_Terminate( void )
{
	ngExit( 0 );
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

/* Required for the debug version of NexGen */
void ngStdOutChar( int p_Char, void *p_pData )
{
#if defined ( DEBUG )
	/*static char PrintBuffer[ 128 ];
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
	}*/
#endif /* DEBUG */
}

