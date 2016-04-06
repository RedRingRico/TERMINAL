#include <GameStateManager.h>
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
	const char *p_pStateName )
{
	GAMESTATE GameState;
	PGAMESTATE_REGISTRY pRegistryItr;

	/* Pop all states */
	while( STK_GetCount( &p_pGameStateManager->GameStateStack ) )
	{
		GSM_PopState( p_pGameStateManager );
	}

	if( GSM_IsStateInRegistry( p_pGameStateManager, p_pStateName,
		&GameState ) == false )
	{
		LOG_Debug( "GSM_ChangeState <ERROR> Could not locate game state \""
			"%s\"\n", p_pStateName );

		return 1;
	}

	if( STK_Push( &p_pGameStateManager->GameStateStack, &GameState ) != 0 )
	{
		LOG_Debug( "GSM_ChangeState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		return 1;
	}

	return 0;
}

int GSM_PushState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName )
{
	GAMESTATE GameState;

	if( GSM_IsStateInRegistry( p_pGameStateManager, p_pStateName,
		&GameState ) == false )
	{
		LOG_Debug( "GSM_PushState <ERROR> State \"%s\" not in registry\n",
			p_pStateName );

		return 1;
	}

	if( STK_Push( &p_pGameStateManager->GameStateStack, &GameState ) != 0 )
	{
		LOG_Debug( "GSM_PushState <ERROR> Something went wrong pushing the "
			"state onto the stack\n" );

		return 1;
	}

	p_pGameStateManager->pTopGameState = STK_GetTopItem(
		&p_pGameStateManager->GameStateStack );

	p_pGameStateManager->pTopGameState->Load( NULL );
	p_pGameStateManager->pTopGameState->Initialise( NULL );

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

	return 0;
}

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, PGAMESTATE p_pGameState )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	PGAMESTATE_REGISTRY pNewEntry;
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
	strncpy( pNewEntry->pName, p_pGameStateName, strlen( p_pGameStateName ) );
	pNewEntry->pName[ strlen( p_pGameStateName ) ] = '\0';

	GS_Copy( &pNewEntry->GameState, p_pGameState );

	pNewEntry->pNext = NULL;
	pAppendTo->pNext = pNewEntry;

	return 0;
}

bool GSM_IsStateInRegistry( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pName, PGAMESTATE p_pGameState )
{
	PGAMESTATE_REGISTRY pRegistryItr = p_pGameStateManager->pRegistry;
	bool StatePresent = false;

	while( pRegistryItr != NULL )
	{
		if( strcmp( p_pName, pRegistryItr->pName ) == 0 )
		{
			StatePresent = true;
			break;
		}
		pRegistryItr = pRegistryItr->pNext;
	}

	if( StatePresent == false )
	{
		return false;
	}

	if( p_pGameState != NULL )
	{
		p_pGameState = &pRegistryItr->GameState;
	}

	return StatePresent;
}

