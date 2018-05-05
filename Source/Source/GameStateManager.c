#include <GameStateManager.h>
#include <GameState.h>
#include <MainMenuState.h>
#include <Peripheral.h>
#include <Renderer.h>
#include <Log.h>
#include <DebugAdapter.h>
#include <string.h>
#include <StorageUnit.h>
#include <GitVersion.h>
#include <Keyboard.h>

static Sint8 g_ConsoleID[ SYD_CFG_IID_SIZE + 1 ];
static char g_ConsoleIDPrint[ ( SYD_CFG_IID_SIZE * 2 ) + 1 ];

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PRENDERER p_pRenderer, PGAMESTATE_MEMORY_BLOCKS p_pMemoryBlocks )
{
	Sint32 DrivesMounted = 0;
	size_t Index = 0;

	if( p_pRenderer == NULL )
	{
		LOG_Debug( "GSM_Initialise: Renderer interface is null" );

		return 1;
	}

	p_pGameStateManager->pRegistry = NULL;
	p_pGameStateManager->pRegistry->pNext = NULL;

	p_pGameStateManager->pTopGameState = NULL;
	p_pGameStateManager->Running = false;

	p_pGameStateManager->MemoryBlocks.pSystemMemory =
		p_pMemoryBlocks->pSystemMemory;
	p_pGameStateManager->MemoryBlocks.pGraphicsMemory =
		p_pMemoryBlocks->pGraphicsMemory;
	p_pGameStateManager->MemoryBlocks.pAudioMemory =
		p_pMemoryBlocks->pAudioMemory;

	p_pGameStateManager->ppGlyphSet = MEM_AllocateFromBlock(
		p_pGameStateManager->MemoryBlocks.pSystemMemory,
		sizeof( PGLYPHSET ) * 2, "GSM: Glyph set" );

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	p_pGameStateManager->DebugAdapter.pData = MEM_AllocateFromBlock(
		p_pGameStateManager->MemoryBlocks.pSystemMemory,
		64 * 1024, "Debug Adapter Memory" );

	p_pGameStateManager->DebugAdapter.DataSize = 64 * 1024;
	p_pGameStateManager->DebugAdapter.DataRead = 0;
	p_pGameStateManager->DebugAdapter.Connected = false;

	QUE_Initialise( &p_pGameStateManager->DebugAdapter.Queue,
		p_pGameStateManager->MemoryBlocks.pSystemMemory, 64,
		sizeof( DEBUG_ADAPTER_MESSAGE ), 20, "Debug Adapter Queue" );
	
	/* Get the console's ID for debug/development purposes */
	memset( g_ConsoleIDPrint, '\0', sizeof( g_ConsoleIDPrint ) );
	if( syCfgGetIndividualID( g_ConsoleID ) != SYD_CFG_IID_OK )
	{
		sprintf( g_ConsoleIDPrint, "ERROR" );
	}
	g_ConsoleID[ SYD_CFG_IID_SIZE ] = '\0';

	sprintf( g_ConsoleIDPrint, "%02X%02X%02X%02X%02X%02X",
		( unsigned char )g_ConsoleID[ 0 ],
		( unsigned char )g_ConsoleID[ 1 ],
		( unsigned char )g_ConsoleID[ 2 ],
		( unsigned char )g_ConsoleID[ 3 ],
		( unsigned char )g_ConsoleID[ 4 ],
		( unsigned char )g_ConsoleID[ 5 ] );
#endif /* DEBUG || DEVELOPMENT */

	if( STK_Initialise( &p_pGameStateManager->GameStateStack,
		p_pGameStateManager->MemoryBlocks.pSystemMemory, 10,
		sizeof( GAMESTATE ), 0,	"GSM: Game State Stack" )  != 0 )
	{
		LOG_Debug( "GSM_Initialise <ERROR> Failed to allocate memory for the "
			"game state stack\n" );

		return 1;
	}

	p_pGameStateManager->pRenderer = p_pRenderer;

	/* Mount all available memory units */
	if( SU_MountDrives( &DrivesMounted ) != 0 )
	{
		LOG_Debug( "GSM_Initialise <ERROR> Failed to mount memory units" );
		return 1;
	}

	/* Set up the keyboard(s) */
	for( Index = 0; Index < 4; ++Index )
	{
		p_pGameStateManager->Keyboard[ Index ] =
			KBD_Create( PDD_PORT_A0 + Index * 6 );
	}

	return 0;
}

void GSM_Terminate( PGAMESTATE_MANAGER p_pGameStateManager )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	size_t Index = 0;

	/* Pop all states */
	while( STK_GetCount( &p_pGameStateManager->GameStateStack ) )
	{
		GSM_PopState( p_pGameStateManager );
	}

#if defined ( DEBUG ) || defined( DEVELOPMENT )
	MEM_FreeFromBlock( p_pGameStateManager->MemoryBlocks.pSystemMemory,
		p_pGameStateManager->DebugAdapter.pData );
	QUE_Terminate( &p_pGameStateManager->DebugAdapter.Queue );
#endif /* DEBUG || DEVELOPMENT */

	while( pRegistryItr != NULL )
	{
		PGAMESTATE_REGISTRY pNext = pRegistryItr->pNext;

		MEM_FreeFromBlock( p_pGameStateManager->MemoryBlocks.pSystemMemory,
			pRegistryItr->pName );
		MEM_FreeFromBlock( p_pGameStateManager->MemoryBlocks.pSystemMemory,
			pRegistryItr->pGameState );
		MEM_FreeFromBlock( p_pGameStateManager->MemoryBlocks.pSystemMemory,
			pRegistryItr );

		pRegistryItr = pNext;
	}

	MEM_FreeFromBlock( p_pGameStateManager->MemoryBlocks.pSystemMemory,
		p_pGameStateManager->ppGlyphSet );

	STK_Terminate( &p_pGameStateManager->GameStateStack );

	for( Index = 0; Index < 4; ++Index )
	{
		KBD_Destroy( p_pGameStateManager->Keyboard[ Index ] );
	}

}

int GSM_ChangeState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName, void *p_pGameStateLoadArguments,
	void *p_pGameStateInitialiseArguments )
{
	PGAMESTATE pGameState;
	PGAMESTATE_REGISTRY pRegistryItr;

	/* Pop all states */
	while( STK_GetCount( &p_pGameStateManager->GameStateStack ) )
	{
		GSM_PopState( p_pGameStateManager );
	}

	if( GSM_IsStateInRegistry( p_pGameStateManager, p_pStateName,
		&pGameState ) == false )
	{
		LOG_Debug( "GSM_ChangeState <ERROR> Could not locate game state \""
			"%s\"\n", p_pStateName );

		return 1;
	}

	if( GSM_PushState( p_pGameStateManager, p_pStateName,
		p_pGameStateLoadArguments, p_pGameStateInitialiseArguments, false ) !=
		0 )
	{
		LOG_Debug( "GSM_ChangeState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		p_pGameStateManager->Running = false;

		return 1;
	}

	p_pGameStateManager->Running = true;
	p_pGameStateManager->pTopGameState->Update( p_pGameStateManager );

	return 0;
}

int GSM_PushState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName, void *p_pGameStateLoadArguments,
	void *p_pGameStateInitialiseArguments, bool p_PauseCurrentState )
{
	PGAMESTATE pGameState;

	if( GSM_IsStateInRegistry( p_pGameStateManager, p_pStateName,
		&pGameState ) == false )
	{
		LOG_Debug( "GSM_PushState <ERROR> State \"%s\" not in registry\n",
			p_pStateName );

		return 1;
	}

	if( p_PauseCurrentState == true )
	{
		if( p_pGameStateManager->pTopGameState != NULL )
		{
			GS_Pause( p_pGameStateManager->pTopGameState );
		}
	}

	if( STK_Push( &p_pGameStateManager->GameStateStack, pGameState ) != 0 )
	{
		LOG_Debug( "GSM_PushState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		return 1;
	}

	p_pGameStateManager->pTopGameState = STK_GetTopItem(
		&p_pGameStateManager->GameStateStack );

	GS_Resume( p_pGameStateManager->pTopGameState );

	/*MEM_ListMemoryBlocks(
		p_pGameStateManager->MemoryBlocks.pSystemMemory );*/
	/* Seems like the best time to do garbage collection */
	MEM_GarbageCollectMemoryBlock(
		p_pGameStateManager->MemoryBlocks.pSystemMemory );
	MEM_GarbageCollectMemoryBlock(
		p_pGameStateManager->MemoryBlocks.pGraphicsMemory );
	MEM_GarbageCollectMemoryBlock(
		p_pGameStateManager->MemoryBlocks.pAudioMemory );

	if( p_pGameStateManager->pTopGameState->Load(
			p_pGameStateLoadArguments ) != 0 )
	{
		LOG_Debug( "GSM_PushState <ERROR> Something went wrong loading the "
			"state \"%s\"", p_pStateName );

		GSM_PopState( p_pGameStateManager );

		return 1;
	}

	if( p_pGameStateManager->pTopGameState->Initialise(
		p_pGameStateInitialiseArguments ) != 0 )
	{
		LOG_Debug( "GSM_PushState <ERROR> Something went wrong initialising "
			"the state \"%s\"", p_pStateName );

		GSM_PopState( p_pGameStateManager );

		return 1;
	}

	return 0;
}

int GSM_PopState( PGAMESTATE_MANAGER p_pGameStateManager )
{
	if( STK_Pop( &p_pGameStateManager->GameStateStack, NULL ) != 0 )
	{
		LOG_Debug( "GSM_PopState <ERROR> Failed to pop game state from "
			"stack\n" );

		return 1;
	}

	/* Terminate game state */
	p_pGameStateManager->pTopGameState->Terminate( NULL );
	p_pGameStateManager->pTopGameState->Unload( NULL );

	/* Re-acquire top state */
	p_pGameStateManager->pTopGameState = STK_GetTopItem(
		&p_pGameStateManager->GameStateStack );

	if( p_pGameStateManager->pTopGameState != NULL )
	{
		GS_Resume( p_pGameStateManager->pTopGameState );
	}

	return 0;
}

int GSM_Run( PGAMESTATE_MANAGER p_pGameStateManager )
{
	Uint32 StartTime = syTmrGetCount( );
	Uint32 EndTime, TimeDifference;
	PGAMESTATE pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack,
		0 );
	size_t StackItem = 0;
	size_t StackCount = STK_GetCount( &p_pGameStateManager->GameStateStack );
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	int BytesRead = 0;
	Uint8 ChannelStatus = 0;
	Uint32 RenderStartTime, RenderEndTime;
	static Uint32 FPS = 0L, FPSTimer = 0L, FPSCounter = 0L, RenderTime = 0L;

	if( DA_GetChannelStatus( 3, &ChannelStatus ) == 0 )
	{
		if( ChannelStatus & DA_IPRDY )
		{
			/* Get the Debug Adapter information */
			DA_GetData( p_pGameStateManager->DebugAdapter.pData,
				p_pGameStateManager->DebugAdapter.DataSize, 3,
				&p_pGameStateManager->DebugAdapter.DataRead );

			/* Put all the pending messages into the queue */
			while( BytesRead < p_pGameStateManager->DebugAdapter.DataRead )
			{
				DEBUG_ADAPTER_MESSAGE NewMessage;
				memcpy( &NewMessage.ID,
					&p_pGameStateManager->DebugAdapter.pData[ BytesRead ],
					sizeof( Uint16 ) );
				BytesRead += 2;
				memcpy( &NewMessage.Length,
					&p_pGameStateManager->DebugAdapter.pData[ BytesRead ],
					sizeof( Uint16 ) );
				BytesRead += 2;
				if( NewMessage.Length > 0 )
				{
					memcpy( NewMessage.Data,
						&p_pGameStateManager->DebugAdapter.pData[ BytesRead ],
						NewMessage.Length );
				}
				BytesRead += NewMessage.Length;

				QUE_Enqueue( &p_pGameStateManager->DebugAdapter.Queue,
					&NewMessage );
			}

			/* Handle non-specific messages */
			while( QUE_IsEmpty( &p_pGameStateManager->DebugAdapter.Queue ) ==
				false )
			{
				bool DebugMessageHandled = false;
				PDEBUG_ADAPTER_MESSAGE pGenericMessage;

				pGenericMessage = QUE_GetFront(
					&p_pGameStateManager->DebugAdapter.Queue );

				switch( pGenericMessage->ID )
				{
					case DA_CONNECT:
					{
						PGAMESTATE_REGISTRY pRegistryItr =
							p_pGameStateManager->pRegistry;

						struct REGISTRY_ENTRIES
						{
							Uint16		ID;
							Uint16		Length;
							Uint16		NameCount;
							char		Names[ 1022 ];
						};

						struct REGISTRY_ENTRIES RegistryEntries;
						Sint32 TotalStateNameSize = 0;
						Sint32 StateNameCount = 0;
						Sint32 BytesWritten;

						p_pGameStateManager->DebugAdapter.Connected = true;
						QUE_Dequeue( &p_pGameStateManager->DebugAdapter.Queue,
							NULL );

						LOG_Debug( "Debug Adapter Connected" );

						while( pRegistryItr != NULL )
						{
							PGAMESTATE_REGISTRY pNext = pRegistryItr->pNext;

							if( pRegistryItr->pGameState->VisibleToDebugAdapter )
							{
								memcpy( &RegistryEntries.Names[ TotalStateNameSize ],
									pRegistryItr->pName,
									strlen( pRegistryItr->pName ) + 1 );

								TotalStateNameSize +=
									strlen( pRegistryItr->pName ) + 1;

								++StateNameCount;
							}

							pRegistryItr = pNext;
						}

						RegistryEntries.ID = 0x0003;
						RegistryEntries.Length = sizeof( Uint16 ) * 2 +
							TotalStateNameSize;
						RegistryEntries.NameCount = StateNameCount;

						if( DA_SendData( 3, sizeof( RegistryEntries ),
							&RegistryEntries, &BytesWritten ) != 0 )
						{
							LOG_Debug( "Error sending registered game states" );
						}

						DebugMessageHandled = true;
						break;
					}
					case DA_DISCONNECT:
					{
						p_pGameStateManager->DebugAdapter.Connected = false;
						QUE_Dequeue( &p_pGameStateManager->DebugAdapter.Queue,
							NULL );

						LOG_Debug( "Debug Adapter Disconnected" );
						DebugMessageHandled = true;
						break;
					}
					case DA_SWITCHGAMESTATE:
					{
						DEBUG_ADAPTER_MESSAGE SwitchMessageConfirm;

						if( GSM_ChangeState( p_pGameStateManager,
							( char * )pGenericMessage->Data, NULL,
							NULL ) != 0 )
						{
						}

						QUE_Dequeue( &p_pGameStateManager->DebugAdapter.Queue,
							NULL );

						DebugMessageHandled = true;
						break;
					}
					default:
					{
					}
				}

				// Nothing more can be done with the current item in the queue
				if( DebugMessageHandled == false )
				{
					break;
				}
			}
		}
	}
#endif /* DEBUG || DEVELOPMENT */

	/* Walk the stack */
	for( StackItem = 0; StackItem < StackCount; ++StackItem )
	{
		pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack,
			StackItem );
		if( pGameState->Paused == false )
		{
			pGameState->Update( p_pGameStateManager );
		}
	}

	pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack, 0 );

	p_pGameStateManager->pRenderer->VisiblePolygons =
		p_pGameStateManager->pRenderer->CulledPolygons = 0;

	REN_Clear( );
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	RenderStartTime = syTmrGetCount( );
#endif /* DEBUG || DEVELOPMENT */
	/* Walk the stack */
	for( StackItem = 0; StackItem < StackCount; ++StackItem )
	{
		pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack,
			StackItem );
		if( pGameState->Paused == false )
		{
			pGameState->Render( NULL );
		}
	}
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	{
		KMPACKEDARGB TextColour;
		Uint8 ChannelStatus;
		Sint32 Channel = 0;
		static char FPSString[ 6 ];
		static char MemoryUnits[ 6 ];
		static char PrintBuffer[ 128 ];
		static float TextLength;
		float FPSXOffset;
		PGLYPHSET pGlyphSet = GSM_GetGlyphSet( p_pGameStateManager,
			GSM_GLYPH_SET_GUI_1 );
		size_t ConnectedMemoryUnits = 0;
		size_t MountedMemoryUnits = 0;

		sprintf( FPSString, "%d", FPS );
		TXT_MeasureString( pGlyphSet, FPSString, &FPSXOffset );

		TextColour.dwPacked = 0xFFFFFFFF;

		if( p_pGameStateManager->DebugAdapter.Connected )
		{
			TextColour.dwPacked = 0xFF00FF00;

			TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
				( float )pGlyphSet->LineHeight * 1.5f, "Connected" );
		}
		else
		{
			TextColour.dwPacked = 0xFFFF0000;

			TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
				( float )pGlyphSet->LineHeight * 1.5f, "Disconnected" );
		}

		if( FPS >= 60 )
		{
			TextColour.dwPacked = 0xFF00DD00;
		}
		else if( FPS >= 30 )
		{
			TextColour.dwPacked = 0xFFCCCC00;
		}
		else if( FPS >= 15 )
		{
			TextColour.dwPacked = 0xFFFFA500;
		}
		else
		{
			TextColour.dwPacked = 0xFFFF0000;
		}

		TXT_RenderString( pGlyphSet, &TextColour, 620.0f - FPSXOffset,
			( float )pGlyphSet->LineHeight * 1.5f, FPSString );

		TextColour.dwPacked = 0xFFFFFFFF;

		SU_GetConnectedStorageUnits( NULL, &ConnectedMemoryUnits );
		SU_GetMountedStorageUnits( NULL, &MountedMemoryUnits );
		sprintf( MemoryUnits, "[%d/%d]", MountedMemoryUnits,
			ConnectedMemoryUnits );

		TXT_RenderString( pGlyphSet, &TextColour, 10.0f, 480.0f -
			( float )pGlyphSet->LineHeight * 5.0f, MemoryUnits );
		TextColour.byte.bRed = 83;
		TextColour.byte.bGreen = 254;
		TextColour.byte.bBlue = 255;
		TextColour.byte.bAlpha = 200;
		sprintf( PrintBuffer, "Tag: %s | Branch: %s | Version: %s",
			GIT_TAGNAME, GIT_BRANCH, GIT_VERSION );
		TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			480.0f - ( float )pGlyphSet->LineHeight * 2.0f, PrintBuffer );

		TextColour.byte.bRed = 230;
		TextColour.byte.bGreen = 30;
		TextColour.byte.bBlue = 230;

		TXT_MeasureString( pGlyphSet, g_ConsoleIDPrint, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
			( float )pGlyphSet->LineHeight * 2.5f, g_ConsoleIDPrint );

		TextColour.byte.bRed = 255;
		TextColour.byte.bGreen = 255;
		TextColour.byte.bBlue = 255;
		sprintf( PrintBuffer, "%lu",
			p_pGameStateManager->pTopGameState->ElapsedGameTime );
		TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour, 620.0f - TextLength,
			( float )pGlyphSet->LineHeight * 2.5f, PrintBuffer );
	}
#endif /* DEBUG || DEVELOPMENT */
	REN_SwapBuffers( );

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	RenderEndTime = syTmrGetCount( );

	RenderTime = syTmrCountToMicro( syTmrDiffCount( RenderStartTime,
		RenderEndTime ) );

	++FPSCounter;
#endif /* DEBUG || DEVELOPMENT */

	EndTime = syTmrGetCount( );

	TimeDifference = syTmrCountToMicro( syTmrDiffCount( StartTime, EndTime ) );

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	FPSTimer += TimeDifference;

	if( FPSTimer > 1000000UL )
	{
		FPSTimer = 0UL;
		FPS = FPSCounter;
		FPSCounter = 0L;
	}
#endif /* DEBUG || DEVELOPMENT */

	p_pGameStateManager->pTopGameState->ElapsedGameTime += TimeDifference;

	return 0;
}

int GSM_Quit( PGAMESTATE_MANAGER p_pGameStateManager )
{
	p_pGameStateManager->Running = false;

	return 0;
}

bool GSM_IsRunning( PGAMESTATE_MANAGER p_pGameStateManager )
{
	return p_pGameStateManager->Running;
}

int GSM_RegisterGlyphSet( PGAMESTATE_MANAGER p_pGameStateManager,
	const Uint32 p_Index, PGLYPHSET p_pGlyphSet )
{
	p_pGameStateManager->ppGlyphSet[ p_Index ] = p_pGlyphSet;

	return 0;
}

PGLYPHSET GSM_GetGlyphSet( PGAMESTATE_MANAGER p_pGameStateManager,
	const Uint32 p_Index )
{
	return p_pGameStateManager->ppGlyphSet[ p_Index ];
}

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, PGAMESTATE p_pGameState )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	PGAMESTATE_REGISTRY pNewEntry;

	/* First item in registry */
	if( p_pGameStateManager->pRegistry == NULL )
	{
#if defined ( DEBUG )
		char Name[ 64 ] = "GS Name: ";
		char Registry[ 64 ] = "GS Registry: ";
		char GameState[ 64 ] = "GS: ";
		strcat( Name, p_pGameStateName );
		strcat( Registry, p_pGameStateName );
		strcat( GameState, p_pGameStateName );
#else
		char *Name = p_pGameStateName;
		char *Registry = p_pGameStateName;
		char *GameState = p_pGameStateName;
#endif /* DEBUG */

		p_pGameStateManager->pRegistry =
			MEM_AllocateFromBlock(
				p_pGameStateManager->MemoryBlocks.pSystemMemory,
				sizeof( GAMESTATE_REGISTRY ), Registry );

		pNewEntry = p_pGameStateManager->pRegistry;

		pNewEntry->pName = MEM_AllocateFromBlock(
			p_pGameStateManager->MemoryBlocks.pSystemMemory,
			strlen( p_pGameStateName ) + 1, Name );
		strncpy( pNewEntry->pName, p_pGameStateName,
			strlen( p_pGameStateName ) );
		pNewEntry->pName[ strlen( p_pGameStateName ) ] = '\0';

		pNewEntry->pGameState = MEM_AllocateFromBlock(
			p_pGameStateManager->MemoryBlocks.pSystemMemory,
			sizeof( GAMESTATE ), GameState );

		GS_Copy( pNewEntry->pGameState, p_pGameState );

		pNewEntry->pNext = NULL;
	}
	else
	{
		PGAMESTATE_REGISTRY pAppendTo;
#if defined ( DEBUG )
		char Name[ 64 ] = "GS Name: ";
		char Registry[ 64 ] = "GS Registry: ";
		char GameState[ 64 ] = "GS: ";
		strcat( Name, p_pGameStateName );
		strcat( Registry, p_pGameStateName );
		strcat( GameState, p_pGameStateName );
#else
		char *Name = p_pGameStateName;
		char *Registry = p_pGameStateName;
		char *GameState = p_pGameStateName;
#endif /* DEBUG */

		while( pRegistryItr != NULL )
		{
			if( strcmp( p_pGameStateName, pRegistryItr->pName ) == 0 )
			{
				LOG_Debug( "GSM_RegisterGameState <ERROR> Attempting to "
					"re-register game state \"%s\"\n", p_pGameStateName );

				return 1;
			}

			pAppendTo = pRegistryItr;
			pRegistryItr = pRegistryItr->pNext;
		}

		pNewEntry = MEM_AllocateFromBlock(
				p_pGameStateManager->MemoryBlocks.pSystemMemory,
				sizeof( GAMESTATE_REGISTRY ), Registry );
 

		pNewEntry->pName = MEM_AllocateFromBlock(
			p_pGameStateManager->MemoryBlocks.pSystemMemory,
			strlen( p_pGameStateName ) + 1, Name );

		strncpy( pNewEntry->pName, p_pGameStateName,
			strlen( p_pGameStateName ) );
		pNewEntry->pName[ strlen( p_pGameStateName ) ] = '\0';

		pNewEntry->pGameState = MEM_AllocateFromBlock(
			p_pGameStateManager->MemoryBlocks.pSystemMemory,
			sizeof( GAMESTATE ), GameState );

		GS_Copy( pNewEntry->pGameState, p_pGameState );

		pNewEntry->pNext = NULL;
		pAppendTo->pNext = pNewEntry;
	}

	return 0;
}

bool GSM_IsStateInRegistry( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pName, PGAMESTATE *p_ppGameState )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	bool StatePresent = false;
	size_t Itr = 0;

	while( pRegistryItr != NULL )
	{
		if( strcmp( p_pName, pRegistryItr->pName ) == 0 )
		{
			StatePresent = true;
			break;
		}
		pRegistryItr = pRegistryItr->pNext;
		++Itr;
	}

	if( StatePresent == false )
	{
		return false;
	}

	if( p_ppGameState != NULL )
	{
		( *p_ppGameState ) = pRegistryItr->pGameState;
	}

	return StatePresent;
}

Sint32 GSM_RunVSync( PGAMESTATE_MANAGER p_pGameStateManager )
{
	PGAMESTATE pGameState;
	Uint8 ID;
	int BytesRead = 0;
	Uint8 ChannelStatus;
	size_t StackItem =
		STK_GetCount( &p_pGameStateManager->GameStateStack ) - 1;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	/* Get the Debug Adapter information */
	DA_GetData( p_pGameStateManager->DebugAdapter.pData,
		p_pGameStateManager->DebugAdapter.DataSize, 3,
		&p_pGameStateManager->DebugAdapter.DataRead );
#endif /* DEBUG || DEVELOPMENT */

	/* Walk the stack */
	for( ; StackItem > 0; --StackItem )
	{
		pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack,
			StackItem );

		/* The message was handled, don't keep going */
		if( pGameState->VSyncCallback( p_pGameStateManager ) == 0 )
		{
			break;
		}
	}

	return 0;
}

