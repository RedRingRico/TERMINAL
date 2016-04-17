#include <MultiPlayerState.h>
#include <NetworkClient.h>
#include <NetworkCore.h>
#include <NetworkMessage.h>
#include <GameState.h>
#include <Peripheral.h>
#include <Log.h>
#include <Queue.h>

static const Uint32 GLS_MESSAGE_CONNECT = 0x000100;
static const Uint32 MAX_PACKETS_PER_UPDATE = 10;

typedef enum 
{
	SERVERLIST_STATE_CONNECT,
	SERVERLIST_STATE_GETSERVERLIST,
	SERVERLIST_STATE_UPDATESERVERLIST,
	SERVERLIST_STATE_DISCONNECT
}SERVERLIST_STATE;

typedef struct _tagGAMESERVER
{
	char					*pName;
	/* Ping is measured in milliseconds */
	Uint16					Ping;
	Uint8					TotalSlots;
	Uint8					FreeSlots;
	struct _tagGAMESERVER	*pNext;
}GAMESERVER,*PGAMESERVER;

typedef struct _tagPACKET
{
	NETWORK_MESSAGE	Message;
	SOCKET_ADDRESS	Address;
}PACKET,*PPACKET;


typedef struct _tagGLS_GAMESTATE
{
	GAMESTATE			Base;
	NETWORK_CLIENT		NetworkClient;
	PGAMESERVER			pGameServerList;
	SERVERLIST_STATE	ServerListState;
	QUEUE				PacketQueue;
}GLS_GAMESTATE,*PGLS_GAMESTATE;

static GLS_GAMESTATE GameListServerState;

static void GLS_FreeServerList( void );
static void GLS_ProcessIncomingPackets( void );
static void GLS_ReadIncomingPackets( void );
static void GLS_ProcessQueuedPackets( void );
static void GLS_UpdateBytesSentLastFrame( void );
static void GLS_ProcessPacket( PNETWORK_MESSAGE p_pMessage,
	PSOCKET_ADDRESS p_pAddress );

static int GLS_Load( void *p_pArgs )
{
	GameListServerState.pGameServerList = NULL;

	return 0;
}

static int GLS_Initialise( void *p_pArgs )
{
	/* Just in case the state wasn't terminated */
	GLS_FreeServerList( );

	LOG_Debug( "GLS_Initialise <INFO> Initialising client\n" );

	NCL_Initialise( &GameListServerState.NetworkClient, "192.168.2.116",
		50001 );

	LOG_Debug( "GLS_Initialise <INFO> Setting server state\n" );

	GameListServerState.ServerListState = SERVERLIST_STATE_CONNECT;

	LOG_Debug( "GLS_Initialise <INFO> Initialising packet queue\n" );

	QUE_Initialise( &GameListServerState.PacketQueue,
		GameListServerState.Base.pGameStateManager->MemoryBlocks.pSystemMemory,
		20, sizeof( PACKET ), 0, "Network Message Queue" );

	LOG_Debug( "GLS_Initialise <INFO> Done initialising\n" );

	return 0;
}

static int GLS_Update( void *p_pArgs )
{
	static Uint8 MessageBuffer[ 1300 ];
	static size_t MessageBufferLength = sizeof( MessageBuffer );

	if( GameListServerState.Base.Paused == false )
	{
		switch( GameListServerState.ServerListState )
		{
			case SERVERLIST_STATE_CONNECT:
			{
				NETWORK_MESSAGE Message;

				MSG_CreateNetworkMessage( &Message, MessageBuffer,
					MessageBufferLength );
				MSG_WriteUInt32( &Message, 256 );
				MSG_WriteByte( &Message, strlen( "Rico" ) + 1 );
				MSG_WriteString( &Message, "Rico", strlen( "Rico" ) + 1 );
				NCL_SendMessage( &GameListServerState.NetworkClient,
					&Message );

				MSG_DestroyNetworkMessage( &Message );

				/* Obviously, this will be set from the GLS_ProcessPacket
				 * function */
				GameListServerState.ServerListState =
					SERVERLIST_STATE_GETSERVERLIST;
				LOG_Debug( "CONNECTING TO GAME LIST SERVER..." );

				break;
			}
			case SERVERLIST_STATE_GETSERVERLIST:
			{
				break;
			}
			default:
			{
				LOG_Debug( "Unkown server list state\n" );
				break;
			}
		}

		GLS_ProcessIncomingPackets( );

		NET_Update( );

		if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
		{
			LOG_Debug( "Popping GLS state\n" );
			GSM_PopState( GameListServerState.Base.pGameStateManager );
		}
	}

	return 0;
}

static int GLS_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour = 0xFFFFFFFF;
	float TextLength;
	PGLYPHSET pGlyphSet;

	if( GameListServerState.Base.Paused == false )
	{
		pGlyphSet = GSM_GetGlyphSet(
			GameListServerState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

		REN_Clear( );

		TXT_RenderString( pGlyphSet, &TextColour, 320.0f, 240.0f,
			"GAME SERVER LIST STATE" );

		TXT_MeasureString( pGlyphSet, "[B] back",
			&TextLength );
		TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f - TextLength,
			480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
			"[B] back" );

		REN_SwapBuffers( );
	}

	return 0;
}

static int GLS_Terminate( void *p_pArgs )
{
	QUE_Terminate( &GameListServerState.PacketQueue );
	GLS_FreeServerList( );
	NCL_Terminate( &GameListServerState.NetworkClient );

	return 0;
}

static int GLS_Unload( void *p_pArgs )
{
	return 0;
}

int MP_RegisterGameListServerWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	GameListServerState.Base.Load = &GLS_Load;
	GameListServerState.Base.Initialise = &GLS_Initialise;
	GameListServerState.Base.Update = &GLS_Update;
	GameListServerState.Base.Render = &GLS_Render;
	GameListServerState.Base.Terminate = &GLS_Terminate;
	GameListServerState.Base.Unload = &GLS_Unload;
	GameListServerState.Base.pGameStateManager = p_pGameStateManager;

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MULTIPLAYER_GAMELISTSERVER,
		( GAMESTATE * )&GameListServerState );
}

static void GLS_FreeServerList( void )
{
	while( GameListServerState.pGameServerList != NULL )
	{
		PGAMESERVER pNext = GameListServerState.pGameServerList->pNext;

		syFree( GameListServerState.pGameServerList );

		GameListServerState.pGameServerList = pNext;
	}
}


static void GLS_ProcessIncomingPackets( void )
{
	GLS_ReadIncomingPackets( );
	GLS_ProcessQueuedPackets( );
	GLS_UpdateBytesSentLastFrame( );
}

static void GLS_ReadIncomingPackets( void )
{
	static Uint8 PacketMemory[ 1300 ];
	static size_t PacketSize = sizeof( PacketMemory );
	PACKET NetworkPacket;
	int ReceivedPackets = 0;
	int TotalReadBytes = 0;
	NETWORK_MESSAGE NetworkMessage;

	MSG_CreateNetworkMessage( &NetworkMessage, PacketMemory, PacketSize );

	while( ReceivedPackets < MAX_PACKETS_PER_UPDATE )
	{
		int ReadBytes = NET_SocketReceiveFrom(
			&GameListServerState.NetworkClient.Socket, PacketMemory,
			PacketSize, &NetworkPacket.Address );


		if( ReadBytes == 0 )
		{
			/* Nothing to read */
			break;
		}
		else if( ReadBytes > 0 )
		{
			LOG_Debug( "Got packet!\n" );
			MSG_CopyNetworkMessage( &NetworkPacket.Message, &NetworkMessage,
				ReadBytes );
			++ReceivedPackets;
			TotalReadBytes += ReadBytes;

			QUE_Enqueue( &GameListServerState.PacketQueue, &NetworkPacket );
		}
		else
		{
			LOG_Debug( "GLS_ReadIncomingPackets <ERROR> Unknown read "
				"error\n" );
		}
	}
}

static void GLS_ProcessQueuedPackets( void )
{
	/* Ugly code ahoy! */
	PPACKET pOriginalPacket;
	PPACKET pNextPacket = syMalloc( sizeof( PACKET ) );
	pOriginalPacket = pNextPacket;

	while( QUE_IsEmpty( &GameListServerState.PacketQueue ) == false )
	{
		LOG_Debug( "Processing packet\n" );
		/* This will be used for testing with different latencies, though it's
		 * not yet enabled, this is a reminder */

		pNextPacket = QUE_GetFront( &GameListServerState.PacketQueue );
		GLS_ProcessPacket( &pNextPacket->Message, &pNextPacket->Address );
		/* Free up the memory used by the packet */
		MSG_DestroyNetworkMessage( pNextPacket );
		QUE_Dequeue( &GameListServerState.PacketQueue, NULL );
	}

	syFree( pOriginalPacket );
}

static void GLS_UpdateBytesSentLastFrame( void )
{
}

static void GLS_ProcessPacket( PNETWORK_MESSAGE p_pMessage,
	PSOCKET_ADDRESS p_pAddress )
{
	Uint32 PacketType;

	PacketType = MSG_ReadUInt32( p_pMessage );

	switch( PacketType )
	{
		default:
		{
			LOG_Debug( "Unknown packet type received: 0x%08X\n", PacketType );
			break;
		}
	}
}

