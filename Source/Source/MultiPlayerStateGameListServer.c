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

/*#if defined ( DEBUG ) || defined ( DEVELOPMENT )
#define LIST_SERVER_DOMAIN "dev.list.dreamcast.live"
#else*/
#define LIST_SERVER_DOMAIN "list.dreamcast.live"
/*#endif *//* DEBUG || DEVELOPMENT */

typedef enum 
{
	SERVERLIST_STATE_RESOLVING_DOMAIN,
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
	NETWORK_CLIENT		Client;
	ARRAY				Servers;
	SERVERLIST_STATE	ServerListState;
	QUEUE				PacketQueue;
	/* Elapsed time in one of the sub-states */
	Uint32				StateTimer;
	Uint32				StateTimerStart;
	Uint32				StateMessageTries;
	size_t				SelectedServer;
	size_t				ServerCount;
	DNS_REQUEST			DNSRequest;
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
	NET_DNSRequest( &GameListServerState.DNSRequest, LIST_SERVER_DOMAIN );

	GameListServerState.ServerListState = SERVERLIST_STATE_RESOLVING_DOMAIN;
	GameListServerState.StateMessageTries = 1UL;
	GameListServerState.StateTimer = 0UL;
	GameListServerState.SelectedServer = 0;
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
			case SERVERLIST_STATE_RESOLVING_DOMAIN:
			{
				switch( GameListServerState.DNSRequest.Status )
				{
					case DNS_REQUEST_POLLING:
					{
						break;
					}
					case DNS_REQUEST_RESOLVED:
					{
						NETWORK_MESSAGE Message;
						struct in_addr Address;
						Address.s_addr = GameListServerState.DNSRequest.IP;

						NCL_Initialise( &GameListServerState.Client,
							inet_ntoa( Address ), 50001 );

						QUE_Initialise( &GameListServerState.PacketQueue,
							GameListServerState.Base.pGameStateManager->
								MemoryBlocks.pSystemMemory,
							20, sizeof( PACKET ), 0, "Network Message Queue" );

						MSG_CreateNetworkMessage( &Message, MessageBuffer,
							MessageBufferLength,
							GameListServerState.Base.pGameStateManager->
								MemoryBlocks.pSystemMemory );

						MSG_WriteUInt32( &Message, PACKET_TYPE_LISTREQUEST );
						MSG_WriteInt32( &Message, 0 );
						MSG_WriteUInt16( &Message, 0 );

						NCL_SendMessage( &GameListServerState.Client,
							&Message );

						GameListServerState.ServerListState =
							SERVERLIST_STATE_CONNECTING;

						break;
					}
					case DNS_REQUEST_FAILED:
					{
						/* Could not resolve the domain name, tell the user,
						 * allow for a retry */
						break;
					}
					default:
					{
					}
				}

				break;
			}
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
						GameListServerState.Base.pGameStateManager->
							MemoryBlocks.pSystemMemory );

					MSG_WriteUInt32( &Message, PACKET_TYPE_LISTREQUEST );
					MSG_WriteInt32( &Message, 0 );
					MSG_WriteUInt16( &Message, 0 );

					NCL_SendMessage( &GameListServerState.Client,
						&Message );

					MSG_DestroyNetworkMessage( &Message );

					GameListServerState.StateTimerStart = syTmrGetCount( );

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
				if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
				{
					if( GameListServerState.ServerCount > 0 )
					{
						if( GameListServerState.SelectedServer == 0 )
						{
							GameListServerState.SelectedServer =
								GameListServerState.ServerCount - 1;
						}
						else
						{
							--GameListServerState.SelectedServer;
						}
					}
				}

				if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
				{
					if( GameListServerState.ServerCount > 0 )
					{
						if( GameListServerState.SelectedServer == 
							( GameListServerState.ServerCount - 1 ) )
						{
							GameListServerState.SelectedServer = 0;
						}
						else
						{
							++GameListServerState.SelectedServer;
						}
					}
				}

				if( g_Peripherals[ 0 ].press & PDD_DGT_TX )
				{
					NETWORK_MESSAGE Message;

					MSG_CreateNetworkMessage( &Message, MessageBuffer,
						MessageBufferLength,
						GameListServerState.Base.pGameStateManager->
							MemoryBlocks.pSystemMemory );

					MSG_WriteUInt32( &Message, PACKET_TYPE_LISTREQUEST );
					MSG_WriteInt32( &Message, 0 );
					MSG_WriteUInt16( &Message, 0 );

					NCL_SendMessage( &GameListServerState.Client,
						&Message );

					MSG_DestroyNetworkMessage( &Message );
				}

				if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
				{
					/* Push the multi player game state onto the stack */
					MULTIPLAYER_GAME_ARGS StateArgs;

					PGAMESERVER pGameServer = ARY_GetItem(
						&GameListServerState.Servers,
						GameListServerState.SelectedServer );

					StateArgs.IP = pGameServer->IP;
					StateArgs.Port = ntohs( pGameServer->Port );

					GSM_PushState( GameListServerState.Base.pGameStateManager,
						GAME_STATE_MULTIPLAYER_GAME, &StateArgs, NULL );

					GameListServerState.ServerListState =
						SERVERLIST_STATE_CONNECTING;
				}

				break;
			}
			default:
			{
				LOG_Debug( "Unknown server list state\n" );
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
			case SERVERLIST_STATE_RESOLVING_DOMAIN:
			{
#if defined ( DEBUG )
				TXT_RenderString( pGlyphSet, &TextColour, 320.0f,
					240.0f + ( ( float )pGlyphSet->LineHeight ),
					GameListServerState.DNSRequest.Domain );
#endif /* ( DEBUG ) */

				if( GameListServerState.DNSRequest.Status ==
					DNS_REQUEST_POLLING )
				{
					sprintf( InfoString, "RESOLVING %s", LIST_SERVER_DOMAIN );
				}
				else if( GameListServerState.DNSRequest.Status ==
					DNS_REQUEST_RESOLVED )
				{
					sprintf( InfoString, "RESOLVED %s", LIST_SERVER_DOMAIN );
#if defined ( DEBUG )
					TXT_RenderString( pGlyphSet, &TextColour, 320.0f,
						240.0f + ( ( float )pGlyphSet->LineHeight * 2.0f ),
						GameListServerState.DNSRequest.IPAddress );
#endif /* DEBUG */
				}
				else if( GameListServerState.DNSRequest.Status ==
					DNS_REQUEST_FAILED )
				{
					sprintf( InfoString, "FAILED TO RESOLVE %s",
						LIST_SERVER_DOMAIN );
				}
				else
				{
					sprintf( InfoString, "UNKNOWN ERROR RESOLVING %s",
						LIST_SERVER_DOMAIN );
				}

				break;
			}
			case SERVERLIST_STATE_CONNECTING:
			{
				sprintf( InfoString, "CONNECTING [ATTEMPT #%lu]",
					GameListServerState.StateMessageTries );

				TXT_MeasureString( pGlyphSet, "[B] back", &TextLength );
				TXT_RenderString( pGlyphSet, &TextColour,
					640.0f - 64.0f - TextLength,
					480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
					"[B] back" );

				break;
			}
			case SERVERLIST_STATE_GETSERVERLIST:
			{
				sprintf( InfoString, "GETTING SERVER LIST" );

				TXT_MeasureString( pGlyphSet, "[B] back",
					&TextLength );
				TXT_RenderString( pGlyphSet, &TextColour,
					640.0f - 64.0f - TextLength,
					480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
					"[B] back" );

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

					if( GameListServerState.SelectedServer == Index )
					{
						TextColour.dwPacked = 0xFF00FF00;
					}
					else
					{
						TextColour.dwPacked = 0xFFFFFFFF;
					}

					GameServer = ARY_GetItem( &GameListServerState.Servers,
						Index );

					Address.s_addr = GameServer->IP;

					sprintf( IPPort, "%s:%u", inet_ntoa( Address ),
						ntohs( GameServer->Port ) );

					TXT_RenderString( pGlyphSet, &TextColour, 32.0f,
						96.0f + ( ( float )pGlyphSet->LineHeight * Index ),
						IPPort );
				}

				TextColour.dwPacked = 0xFFFFFFFF;

				TXT_MeasureString( pGlyphSet,
					"[X] update    [Y] filter    [A] join    [B] back",
					&TextLength );
				TXT_RenderString( pGlyphSet, &TextColour,
					320.0f - ( TextLength * 0.5f ),
					480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
					"[X] update    [Y] filter    [A] join    [B] back" );

				break;
			}
			default:
			{
				LOG_Debug( "Unknown server list state\n" );
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

		TXT_MeasureString( pGlyphSet, InfoString, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour,
			320.0f - ( TextLength * 0.5f ), 240.0f, InfoString );

		REN_SwapBuffers( );
	}

	return 0;
}

static int GLS_Terminate( void *p_pArgs )
{
	QUE_Terminate( &GameListServerState.PacketQueue );
	NCL_Terminate( &GameListServerState.Client );

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
			&GameListServerState.Client.Socket, PacketMemory,
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
						GameListServerState.ServerCount = 0;

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

			GameListServerState.ServerCount =
				ARY_GetCount( &GameListServerState.Servers );

			/* Reset the connection attempt counter */
			GameListServerState.StateMessageTries = 0;

			break;
		}
		default:
		{
			LOG_Debug( "Unknown packet type received: 0x%08X\n", PacketType );
			break;
		}
	}
}

