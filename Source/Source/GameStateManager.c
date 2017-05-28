#include <GameStateManager.h>
#include <GameState.h>
#include <MainMenuState.h>
#include <Peripheral.h>
#include <Renderer.h>
#include <Log.h>
#include <DebugAdapter.h>
#include <string.h>

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PGAMESTATE_MEMORY_BLOCKS p_pMemoryBlocks )
{
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

	if( STK_Initialise( &p_pGameStateManager->GameStateStack,
		p_pGameStateManager->MemoryBlocks.pSystemMemory, 10,
		sizeof( GAMESTATE ), 0,	"GSM: Game State Stack" )  != 0 )
	{
		LOG_Debug( "GSM_Initialise <ERROR> Failed to allocate memory for the "
			"game state stack\n" );

		return 1;
	}

	return 0;
}

void GSM_Terminate( PGAMESTATE_MANAGER p_pGameStateManager )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;

	/* Pop all states */
	while( STK_GetCount( &p_pGameStateManager->GameStateStack ) )
	{
		GSM_PopState( p_pGameStateManager );
	}

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
	PGAMESTATE pGameState = STK_GetItem( &p_pGameStateManager->GameStateStack,
		0 );
	size_t StackItem = 0;
	size_t StackCount = STK_GetCount( &p_pGameStateManager->GameStateStack );

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

	REN_Clear( );
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
	REN_SwapBuffers( );

	p_pGameStateManager->pTopGameState->ElapsedGameTime +=
		syTmrCountToMicro( syTmrDiffCount( StartTime, syTmrGetCount( ) ) );

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
	size_t StackItem =
		STK_GetCount( &p_pGameStateManager->GameStateStack ) - 1;

	/* Get the Debug Adapter information */
	DA_GetData( p_pGameStateManager->DebugAdapter.pData,
		p_pGameStateManager->DebugAdapter.DataSize, 3,
		&p_pGameStateManager->DebugAdapter.DataRead );

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

