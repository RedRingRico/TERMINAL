#include <MultiPlayerState.h>
#include <NetworkClient.h>
#include <Peripheral.h>

typedef enum
{
	MULTIPLAYER_STATE_JOIN,
	MULTIPLAYER_STATE_CONNECTED,
	MULTIPLAYER_ZTATE_DISCONNECTED
}MULTIPLAYER_STATE;

typedef struct _tagMPG_GAMESTATE
{
	GAMESTATE			Base;
	NETWORK_CLIENT		Client;
	MULTIPLAYER_STATE	MultiPlayerState;
	Uint32				IP;
	Uint16				Port;
}MPG_GAMESTATE,*PMPG_GAMESTATE;

static MPG_GAMESTATE MultiPlayerGameState;

static int MPG_Load( void *p_pArgs )
{
	PMULTIPLAYER_GAME_ARGS pArgs = p_pArgs;
	struct in_addr Address;
	Address.s_addr = pArgs->IP;

	NCL_Initialise( &MultiPlayerGameState.Client, inet_ntoa( Address ),
		pArgs->Port );

	MultiPlayerGameState.IP = pArgs->IP;
	MultiPlayerGameState.Port = pArgs->Port;
}

static int MPG_Initialise( void *p_pArgs )
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
}

static int MPG_Update( void *p_pArgs )
{
	if( MultiPlayerGameState.Base.Paused == false )
	{
		switch( MultiPlayerGameState.MultiPlayerState )
		{
			case MULTIPLAYER_STATE_JOIN:
			{
				break;
			}
		}

		NET_Update( );

		if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
		{
			GSM_PopState( MultiPlayerGameState.Base.pGameStateManager );
		}
	}
}

static int MPG_Render( void *p_pArgs )
{
	PGLYPHSET pGlyphSet;
	KMPACKEDARGB TextColour = 0xFFFFFFFF;
	float TextLength;

	if( MultiPlayerGameState.Base.Paused == false )
	{
		pGlyphSet = GSM_GetGlyphSet(
			MultiPlayerGameState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

		REN_Clear( );

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
		}

		REN_SwapBuffers( );
	}
}

static int MPG_Terminate( void *p_pArgs )
{
}

static int MPG_Unload( void *p_pArgs )
{
	NCL_Terminate( &MultiPlayerGameState.Client );
}

int MP_RegisterMultiPlayerGameWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	MultiPlayerGameState.Base.Load = &MPG_Load;
	MultiPlayerGameState.Base.Initialise = &MPG_Initialise;
	MultiPlayerGameState.Base.Update = &MPG_Update;
	MultiPlayerGameState.Base.Render = &MPG_Render;
	MultiPlayerGameState.Base.Terminate = &MPG_Terminate;
	MultiPlayerGameState.Base.Unload = &MPG_Unload;
	MultiPlayerGameState.Base.pGameStateManager = p_pGameStateManager;

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MULTIPLAYER_GAME,
		( GAMESTATE * )&MultiPlayerGameState );
}

