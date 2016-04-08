#include <GameStateManager.h>
#include <GameState.h>
#include <Log.h>
#include <string.h>

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PMEMORY_BLOCK p_pMemoryBlock )
{
	p_pGameStateManager->pRegistry = NULL;
	p_pGameStateManager->pRegistry->pNext = NULL;

	if( STK_Initialise( &p_pGameStateManager->GameStateStack, p_pMemoryBlock,
		10, sizeof( GAMESTATE ), 0,	"Game State Stack" )  != 0 )
	{
		LOG_Debug( "GSM_Initialise <ERROR> Failed to allocate memory for the "
			"game state stack\n" );

		return 1;
	}

	p_pGameStateManager->pMemoryBlock = p_pMemoryBlock;
	p_pGameStateManager->pTopGameState = NULL;
	p_pGameStateManager->Running = false;

	return 0;
}

void GSM_Terminate( PGAMESTATE_MANAGER p_pGameStateManager )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;

	while( pRegistryItr != NULL )
	{
		PGAMESTATE_REGISTRY pNext = pRegistryItr->pNext;

		syFree( pRegistryItr->pName );
		syFree( pRegistryItr );

		pRegistryItr = pNext;
	}

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
		p_pGameStateLoadArguments, p_pGameStateInitialiseArguments ) != 0 )
	{
		LOG_Debug( "GSM_ChangeState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		return 1;
	}

	p_pGameStateManager->Running = true;

	return 0;
}

int GSM_PushState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName, void *p_pGameStateLoadArguments,
	void *p_pGameStateInitialiseArguments )
{
	PGAMESTATE pGameState;

	if( GSM_IsStateInRegistry( p_pGameStateManager, p_pStateName,
		&pGameState ) == false )
	{
		LOG_Debug( "GSM_PushState <ERROR> State \"%s\" not in registry\n",
			p_pStateName );

		return 1;
	}

	if( p_pGameStateManager->pTopGameState != NULL )
	{
		GS_Pause( p_pGameStateManager->pTopGameState );
	}

	if( STK_Push( &p_pGameStateManager->GameStateStack, pGameState ) != 0 )
	{
		LOG_Debug( "GSM_PushState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		return 1;
	}

	p_pGameStateManager->pTopGameState = STK_GetTopItem(
		&p_pGameStateManager->GameStateStack );

	p_pGameStateManager->pTopGameState->Load( p_pGameStateLoadArguments );
	p_pGameStateManager->pTopGameState->Initialise(
		p_pGameStateInitialiseArguments );

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
	p_pGameStateManager->pTopGameState->Update( p_pGameStateManager );
	p_pGameStateManager->pTopGameState->Render( NULL );

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

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, PGAMESTATE p_pGameState, size_t p_Size )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	PGAMESTATE_REGISTRY pNewEntry;

	/* First item in registry */
	if( p_pGameStateManager->pRegistry == NULL )
	{
		p_pGameStateManager->pRegistry =
			syMalloc( sizeof( GAMESTATE_REGISTRY ) );

		pNewEntry = p_pGameStateManager->pRegistry;

		pNewEntry->pName = syMalloc( strlen( p_pGameStateName ) + 1 );
		strncpy( pNewEntry->pName, p_pGameStateName,
			strlen( p_pGameStateName ) );
		pNewEntry->pName[ strlen( p_pGameStateName ) ] = '\0';

		pNewEntry->pGameState = syMalloc( p_Size );

		GS_Copy( pNewEntry->pGameState, p_pGameState );

		pNewEntry->pNext = NULL;
	}
	else
	{
		PGAMESTATE_REGISTRY pAppendTo;

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

		pNewEntry = syMalloc( sizeof( GAMESTATE_REGISTRY ) );

		pNewEntry->pName = syMalloc( strlen( p_pGameStateName ) + 1 );
		strncpy( pNewEntry->pName, p_pGameStateName,
			strlen( p_pGameStateName ) );
		pNewEntry->pName[ strlen( p_pGameStateName ) ] = '\0';

		pNewEntry->pGameState = syMalloc( p_Size );

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

