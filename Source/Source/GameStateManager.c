#include <GameStateManager.h>
#include <Log.h>
#include <string.h>

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager )
{
	p_pGameStateManager->pRegistry = NULL;
	p_pGameStateManager->pRegistry->pNext = NULL;
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
				"re-register game state \"%s\"", p_pGameStateName );

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

	pAppendTo->pNext = pNewEntry;

	return 0;
}

