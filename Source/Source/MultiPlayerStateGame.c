#include <MultiPlayerState.h>
#include <NetworkClient.h>
#include <NetworkMessage.h>
#include <Peripheral.h>
#include <Queue.h>
#include <Log.h>

static const Uint32 MAX_PACKETS_PER_UPDATE = 10;

typedef enum
{
	MULTIPLAYER_STATE_JOIN,
	MULTIPLAYER_STATE_CONNECTED,
	MULTIPLAYER_ZTATE_DISCONNECTED
}MULTIPLAYER_STATE;

typedef struct _tagPACKET
{
	NETWORK_MESSAGE	Message;
	SOCKET_ADDRESS	Address;
}PACKET,*PPACKET;

typedef struct _tagGAME_CLIENT
{
	Uint32		ID;
}GAME_CLIENT,*PGAME_CLIENT;

typedef struct _tagMPG_GAMESTATE
{
	GAMESTATE			Base;
	NETWORK_CLIENT		Client;
	MULTIPLAYER_STATE	MultiPlayerState;
	Uint32				IP;
	Uint16				Port;
	QUEUE				PacketQueue;
	GAME_CLIENT			GameClient;
	Uint32				ConnectionTimeOutStart;
}MPG_GAMESTATE,*PMPG_GAMESTATE;

static MPG_GAMESTATE MultiPlayerGameState;

static void MPG_ProcessIncomingPackets( void );
static void MPG_ReadIncomingPackets( void );
static void MPG_ProcessQueuedPackets( void );
static void MPG_ProcessPacket( PNETWORK_MESSAGE p_pMessage,
	PSOCKET_ADDRESS p_pAddress );

static Sint32 MPG_Load( void *p_pArgs )
{
	PMULTIPLAYER_GAME_ARGS pArgs = p_pArgs;
	struct in_addr Address;
	Address.s_addr = pArgs->IP;

	NCL_Initialise( &MultiPlayerGameState.Client, inet_ntoa( Address ),
		pArgs->Port );

	MultiPlayerGameState.IP = pArgs->IP;
	MultiPlayerGameState.Port = pArgs->Port;

	QUE_Initialise( &MultiPlayerGameState.PacketQueue,
		MultiPlayerGameState.Base.pGameStateManager->MemoryBlocks.
			pSystemMemory,
		20, sizeof( PACKET ), 0, "Network Message Queue" );
	
	return 0;
}

static Sint32 MPG_Initialise( void *p_pArgs )
{
	Uint8 MessageBuffer[ 1400 ];
	size_t MessageBufferLength = sizeof( MessageBuffer );
	NETWORK_MESSAGE JoinMessage;

	MSG_CreateNetworkMessage( &JoinMessage, MessageBuffer,
		MessageBufferLength,
		MultiPlayerGameState.Base.pGameStateManager->
			MemoryBlocks.pSystemMemory );

	MSG_WriteUInt32( &JoinMessage, PACKET_TYPE_CLIENTJOIN );
	MSG_WriteString( &JoinMessage, "TEST USER" );

	NCL_SendMessage( &MultiPlayerGameState.Client, &JoinMessage );

	MSG_DestroyNetworkMessage( &JoinMessage );

	MultiPlayerGameState.MultiPlayerState = MULTIPLAYER_STATE_JOIN;
	MultiPlayerGameState.ConnectionTimeOutStart = syTmrGetCount( );

	return 0;
}

static Sint32 MPG_Update( void *p_pArgs )
{
	switch( MultiPlayerGameState.MultiPlayerState )
	{
		case MULTIPLAYER_STATE_JOIN:
		{
			/* Time out after trying to connect for ten seconds (this should be
			 * configurable at some later point) */
			if( syTmrCountToMicro( syTmrDiffCount(
				MultiPlayerGameState.ConnectionTimeOutStart,
					syTmrGetCount( ) ) ) >= 10000000UL )
			{
				NET_Update( );
				MPG_ProcessIncomingPackets( );
				GSM_PopState( MultiPlayerGameState.Base.pGameStateManager );
			}

			break;
		}
		case MULTIPLAYER_STATE_CONNECTED:
		{
			if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
			{
				Uint8 MessageBuffer[ 1400 ];
				size_t MessageBufferLength = sizeof( MessageBuffer );
				NETWORK_MESSAGE LeaveMessage;

				MSG_CreateNetworkMessage( &LeaveMessage, MessageBuffer,
					MessageBufferLength,
					MultiPlayerGameState.Base.pGameStateManager->
						MemoryBlocks.pSystemMemory );

				MSG_WriteUInt32( &LeaveMessage, PACKET_TYPE_CLIENTLEAVE );
				
				NCL_SendMessage( &MultiPlayerGameState.Client,
					&LeaveMessage );

				MSG_DestroyNetworkMessage( &LeaveMessage );
			}

			break;
		}
		default:
		{
			break;
		}
	}

	NET_Update( );
	MPG_ProcessIncomingPackets( );

	if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
	{
		GSM_PopState( MultiPlayerGameState.Base.pGameStateManager );
	}

	return 0;
}

static Sint32 MPG_Render( void *p_pArgs )
{
	PGLYPHSET pGlyphSet;
	KMPACKEDARGB TextColour = 0xFFFFFFFF;
	float TextLength;

	pGlyphSet = GSM_GetGlyphSet(
		MultiPlayerGameState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	switch( MultiPlayerGameState.MultiPlayerState )
	{
		case MULTIPLAYER_STATE_JOIN:
		{
			struct in_addr Address;
			char IPPort[ 128 ];

			Address.s_addr = MultiPlayerGameState.IP;

			sprintf( IPPort, "Establishing connection to %s:%u",
				inet_ntoa( Address ), MultiPlayerGameState.Port );

			TXT_MeasureString( pGlyphSet, IPPort, &TextLength );

			TXT_RenderString( pGlyphSet, &TextColour,
				320.0f - ( TextLength * 0.5f ),
				240.0f + ( ( float )pGlyphSet->LineHeight ),
				IPPort );

			break;
		}
		case MULTIPLAYER_STATE_CONNECTED:
		{
			char PlayerID[ 128 ];

			sprintf( PlayerID, "Connected with an ID of: %lu",
				MultiPlayerGameState.GameClient.ID );

			TXT_MeasureString( pGlyphSet, PlayerID, &TextLength );

			TXT_RenderString( pGlyphSet, &TextColour,
				320.0f - ( TextLength * 0.5f ),
				240.0f + ( ( float )pGlyphSet->LineHeight ),
				PlayerID );

			break;
		}
	}
}

static Sint32 MPG_Terminate( void *p_pArgs )
{
	return 0;
}

static Sint32 MPG_Unload( void *p_pArgs )
{
	QUE_Terminate( &MultiPlayerGameState.PacketQueue );
	NCL_Terminate( &MultiPlayerGameState.Client );

	return 0;
}

static Sint32 MPG_VSyncCallback( void *p_pArgs )
{
	return 0;
}

Sint32 MP_RegisterMultiPlayerGameWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	MultiPlayerGameState.Base.Load = &MPG_Load;
	MultiPlayerGameState.Base.Initialise = &MPG_Initialise;
	MultiPlayerGameState.Base.Update = &MPG_Update;
	MultiPlayerGameState.Base.Render = &MPG_Render;
	MultiPlayerGameState.Base.Terminate = &MPG_Terminate;
	MultiPlayerGameState.Base.Unload = &MPG_Unload;
	MultiPlayerGameState.Base.VSyncCallback = &MPG_VSyncCallback;
	MultiPlayerGameState.Base.pGameStateManager = p_pGameStateManager;

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MULTIPLAYER_GAME,
		( GAMESTATE * )&MultiPlayerGameState );
}

static void MPG_ProcessIncomingPackets( void )
{
	MPG_ReadIncomingPackets( );
	MPG_ProcessQueuedPackets( );
}

static void MPG_ReadIncomingPackets( void )
{
	static Uint8 PacketMemory[ 1400 ];
	static size_t PacketSize = sizeof( PacketMemory );
	PACKET NetworkPacket;
	Sint32 ReceivedPackets = 0;
	Sint32 TotalReadBytes = 0;
	NETWORK_MESSAGE NetworkMessage;

	MSG_CreateNetworkMessage( &NetworkMessage, PacketMemory, PacketSize,
		MultiPlayerGameState.Base.pGameStateManager->MemoryBlocks.
			pSystemMemory );

	while( ReceivedPackets < MAX_PACKETS_PER_UPDATE )
	{
		Sint32 ReadBytes = NET_SocketReceiveFrom(
			&MultiPlayerGameState.Client.Socket, PacketMemory,
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

			QUE_Enqueue( &MultiPlayerGameState.PacketQueue, &NetworkPacket );
		}
		else
		{
			LOG_Debug( "MPG_ReadIncomingPackets <ERROR> Unknown read "
				"error\n" );
		}
	}
}

static void MPG_ProcessQueuedPackets( void )
{
	while( QUE_IsEmpty( &MultiPlayerGameState.PacketQueue ) == false )
	{
		PPACKET pPacket;

		LOG_Debug( "Processing packet\n" );
		/* This will be used for testing with different latencies, though it's
		 * not yet enabled, this is a reminder */

		pPacket = QUE_GetFront( &MultiPlayerGameState.PacketQueue );
		MPG_ProcessPacket( &pPacket->Message, &pPacket->Address );

		/* Free up the memory used by the packet */
		MSG_DestroyNetworkMessage( &pPacket->Message );

		QUE_Dequeue( &MultiPlayerGameState.PacketQueue, NULL );
	}
}

static void MPG_ProcessPacket( PNETWORK_MESSAGE p_pMessage,
	PSOCKET_ADDRESS p_pAddress )
{
	Uint32 PacketType;

	PacketType = MSG_ReadUInt32( p_pMessage );

	switch( PacketType )
	{
		case PACKET_TYPE_CLIENTWELCOME:
		{
			MultiPlayerGameState.GameClient.ID = MSG_ReadUInt32( p_pMessage );

			MultiPlayerGameState.MultiPlayerState =
				MULTIPLAYER_STATE_CONNECTED;

			break;
		}
	}
}

