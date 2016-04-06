#ifndef __TERMIANL_GAMESTATEMANAGER_H__
#define __TERMINAL_GAMESTATEMANAGER_H__

#include <GameState.h>
#include <Memory.h>

typedef struct _tagGAMESTATE_REGISTRY
{
	/* The name should be replaced with an unsigned int to improve
	 * performance */
	char							*pName;
	GAMESTATE						GameState;
	struct _tagGAMESTATE_REGISTRY	*pNext;
}GAMESTATE_REGISTRY,*PGAMESTATE_REGISTRY;

typedef struct _tagGAMESTATE_MANAGER
{
	PMEMORY_BLOCK		pMemoryBlock;
	GAMESTATE_REGISTRY	*pRegistry;
	STACK				GameStateStack;
}GAMESTATE_MANAGER,*PGAMESTATE_MANAGER;

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PMEMORY_BLOCK p_pMemoryBlock );
void GSM_Terminate( PGAMESTATE_MANAGER p_pGameStateManager );

int GSM_ChangeState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName );
int GSM_PushState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName );
int GSM_PopState( PGAMESTATE_MANAGER p_pGameStateManager );

int GSM_Exit( void );

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, PGAMESTATE p_pGameState );
bool GSM_IsStateInRegistry( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pName, PGAMESTATE p_pGameState );

#endif /* __TERMINAL_GAMESTATEMANAGER_H__ */

