#include <MultiPlayerState.h>
#include <NetworkClient.h>
#include <NetworkCore.h>
#include <NetworkMessage.h>
#include <GameState.h>
#include <Peripheral.h>
#include <Log.h>
#include <Queue.h>
#include <Array.h>

static const Uint32 GLS_MESSAGE_CONNECT = 0x000100;
static const Uint32 MAX_PACKETS_PER_UPDATE = 10;
static const size_t GAMESERVER_PACKET_SIZE = 6;

typedef enum 
{
	SERVERLIST_STATE_CONNECTING,
	SERVERLIST_STATE_GETSERVERLIST,
	SERVERLIST_STATE_DISPLAYSERVERLIST,
	SERVERLIST_STATE_UPDATESERVERLIST,
	SERVERLIST_STATE_DISCONNECTING
}SERVERLIST_STATE;

typedef struct _tagGAMESERVER
{
	Uint32					IP;
	char					*pName;
	Uint16					Port;
	/* Ping is measured in milliseconds */
	Uint16					Ping;
	Uint8					TotalSlots;
	Uint8					FreeSlots;
	struct _tagGAMESERVER	*pNext;
}GAMESERVER,*PGAMESERVER;

typedef struct _tagGAMESERVER_PACKET
{
	Uint32	IP;
	Uint16	Port;
}GAMESERVER_PACKET,*PGAMESERVER_PACKET;

typedef struct _tagPACKET
{
	NETWORK_MESSAGE	Message;
	SOCKET_ADDRESS	Address;
}PACKET,*PPACKET;

typedef struct _tagGLS_GAMESTATE
{
	GAMESTATE			Base;
	NETWORK_CLIENT		NetworkClient;
	ARRAY				Servers;
	SERVERLIST_STATE	ServerListState;
	QUEUE				PacketQueue;
	/* Elapsed time in one of the sub-states */
	Uint32				StateTimer;
	Uint32				StateTimerStart;
	Uint32				StateMessageTries;
}GLS_GAMESTATE,*PGLS_GAMESTATE;

static GLS_GAMESTATE GameListServerState;

static void GLS_ProcessIncomingPackets( void );
static void GLS_ReadIncomingPackets( void );
static void GLS_ProcessQueuedPackets( void );
static void GLS_UpdateBytesSentLastFrame( void );
static void GLS_ProcessPacket( PNETWORK_MESSAGE p_pMessage,
	PSOCKET_ADDRESS p_pAddress );

static int GLS_Load( void *p_pArgs )
{
	ARY_Initialise( &GameListServerState.Servers,
		GameListServerState.Base.pGameStateManager->MemoryBlocks.pSystemMemory,
		100, sizeof( GAMESERVER ), 50, "Game Server Array" );

	return 0;
}

static int GLS_Initialise( void *p_pArgs )
{
	static Uint8 MessageBuffer[ 1400 ];
	static size_t MessageBufferLength = sizeof( MessageBuffer );
	NETWORK_MESSAGE Message;

	NCL_Initialise( &GameListServerState.NetworkClient, "192.168.2.116",
		50001 );

	GameListServerState.ServerListState = SERVERLIST_STATE_CONNECTING;

	QUE_Initialise( &GameListServerState.PacketQueue,
		GameListServerState.Base.pGameStateManager->MemoryBlocks.pSystemMemory,
		20, sizeof( PACKET ), 0, "Network Message Queue" );

	MSG_CreateNetworkMessage( &Message, MessageBuffer,
		MessageBufferLength,
		GameListServerState.Base.pGameStateManager->MemoryBlocks.
			pSystemMemory );

	MSG_WriteUInt32( &Message, PACKET_TYPE_LISTREQUEST );
	MSG_WriteInt32( &Message, 0 );
	MSG_WriteUInt16( &Message, 0 );

	NCL_SendMessage( &GameListServerState.NetworkClient,
		&Message );

	MSG_DestroyNetworkMessage( &Message );

	GameListServerState.StateMessageTries = 1UL;
	GameListServerState.StateTimer = 0UL;
	GameListServerState.StateTimerStart = syTmrGetCount( );

	return 0;
}

static int GLS_Update( void *p_pArgs )
{
	static Uint8 MessageBuffer[ 1400 ];
	static size_t MessageBufferLength = sizeof( MessageBuffer );

	if( GameListServerState.Base.Paused == false )
	{
		switch( GameListServerState.ServerListState )
		{
			case SERVERLIST_STATE_CONNECTING:
			{
				GameListServerState.StateTimer =
					syTmrCountToMicro( syTmrDiffCount(
						GameListServerState.StateTimerStart,
						syTmrGetCount( ) ) );

				if( GameListServerState.StateTimer >= 2000000UL )
				{
					NETWORK_MESSAGE Message;

					MSG_CreateNetworkMessage( &Message, MessageBuffer,
						MessageBufferLength,
						GameListServerState.Base.pGameStateManager->MemoryBlocks.
							pSystemMemory );

					MSG_WriteUInt32( &Message, PACKET_TYPE_LISTREQUEST );
					MSG_WriteInt32( &Message, 0 );
					MSG_WriteUInt16( &Message, 0 );

					NCL_SendMessage( &GameListServerState.NetworkClient,
						&Message );

					MSG_DestroyNetworkMessage( &Message );

					++GameListServerState.StateMessageTries;
				}

				if( GameListServerState.StateMessageTries > 10 )
				{
					LOG_Debug( "Unable to connect to game server\n" );
					LOG_Debug( "Popping GLS state\n" );
					GSM_PopState( GameListServerState.Base.pGameStateManager );
				}

				break;
			}
			case SERVERLIST_STATE_GETSERVERLIST:
			{
				break;
			}
			case SERVERLIST_STATE_DISPLAYSERVERLIST:
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
		char InfoString[ 128 ] = { '\0' };

		pGlyphSet = GSM_GetGlyphSet(
			GameListServerState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

		REN_Clear( );

		switch( GameListServerState.ServerListState )
		{
			case SERVERLIST_STATE_CONNECTING:
			{
				sprintf( InfoString, "CONNECTING" );
				break;
			}
			case SERVERLIST_STATE_GETSERVERLIST:
			{
				sprintf( InfoString, "GETTING SERVER LIST" );
				break;
			}
			case SERVERLIST_STATE_DISPLAYSERVERLIST:
			{
				size_t Servers, Index;
				char IPPort[ 32 ];

				TXT_MeasureString( pGlyphSet, "SERVER LIST", &TextLength );
				TXT_RenderString( pGlyphSet, &TextColour,
					320.0f - ( TextLength * 0.5f ), 32.0f, "SERVER LIST" );

				Servers = ARY_GetCount( &GameListServerState.Servers );

				for( Index = 0; Index < Servers; ++Index )
				{
					PGAMESERVER GameServer;
					struct in_addr Address;

					GameServer = ARY_GetItem( &GameListServerState.Servers,
						Index );

					Address.s_addr = GameServer->IP;

					sprintf( IPPort, "%s:%u", inet_ntoa( Address ),
						ntohs( GameServer->Port ) );

					TXT_RenderString( pGlyphSet, &TextColour, 64.0f,
						96.0f + ( ( float )pGlyphSet->LineHeight * Index ),
						IPPort );
				}

				break;
			}
			default:
			{
				LOG_Debug( "Unkown server list state\n" );
				break;
			}
		}

#if defined ( DEBUG )
		{
			char NetString[ 80 ];
			sprintf( NetString, "Open interfaces: %d", NET_GetIfaceOpen( ) );
			TXT_RenderString( pGlyphSet, &TextColour, 0.0f, 0.0f, NetString );

			sprintf( NetString, "Open devices: %d", NET_GetDevOpen( ) );
			TXT_RenderString( pGlyphSet, &TextColour, 0.0f, 20.0f, NetString );
		}
#endif /* DEBUG */

		TXT_RenderString( pGlyphSet, &TextColour, 320.0f, 240.0f,
			InfoString );

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
	NCL_Terminate( &GameListServerState.NetworkClient );

	return 0;
}

static int GLS_Unload( void *p_pArgs )
{
	ARY_Terminate( &GameListServerState.Servers );

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

static void GLS_ProcessIncomingPackets( void )
{
	GLS_ReadIncomingPackets( );
	GLS_ProcessQueuedPackets( );
	GLS_UpdateBytesSentLastFrame( );
}

static void GLS_ReadIncomingPackets( void )
{
	static Uint8 PacketMemory[ 1400 ];
	static size_t PacketSize = sizeof( PacketMemory );
	PACKET NetworkPacket;
	int ReceivedPackets = 0;
	int TotalReadBytes = 0;
	NETWORK_MESSAGE NetworkMessage;

	MSG_CreateNetworkMessage( &NetworkMessage, PacketMemory, PacketSize,
		GameListServerState.Base.pGameStateManager->MemoryBlocks.
			pSystemMemory );

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
		case PACKET_TYPE_LISTRESPONSE:
		{
			/* Extract the list of servers */
			size_t Servers = ( p_pMessage->MaxSize - sizeof( Uint32 ) ) /
				GAMESERVER_PACKET_SIZE;
			size_t Index;
			GAMESERVER_PACKET GameServerPacket;
			GAMESERVER GameServer;

			GameListServerState.ServerListState =
				SERVERLIST_STATE_GETSERVERLIST;

			for( Index = 0; Index < Servers; ++Index )
			{
				MSG_Read( p_pMessage, &GameServerPacket,
					GAMESERVER_PACKET_SIZE );
				
				if( GameServerPacket.IP != 0 && GameServerPacket.Port != 0 )
				{
					/*LOG_Debug( "Server: 0x%08X:0x%04X\n", GameServerPacket.IP,
						GameServerPacket.Port );*/
					if( GameServerPacket.IP == 0xFFFFFFFF &&
						GameServerPacket.Port == 0xFFFF )
					{
						ARY_Clear( &GameListServerState.Servers );
						continue;
					}

					GameServer.IP = GameServerPacket.IP;
					GameServer.Port = GameServerPacket.Port;

					ARY_Append( &GameListServerState.Servers, &GameServer );
				}
				else
				{
					GameListServerState.ServerListState =
						SERVERLIST_STATE_DISPLAYSERVERLIST;
				}
			}

			break;
		}
		default:
		{
			LOG_Debug( "Unknown packet type received: 0x%08X\n", PacketType );
			break;
		}
	}
}

