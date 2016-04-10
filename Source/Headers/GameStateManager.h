#ifndef __TERMINAL_GAMESTATEMANAGER_H__
#define __TERMINAL_GAMESTATEMANAGER_H__

#include <GameState.h>
#include <GameOptions.h>
#include <Memory.h>

typedef struct _tagGAMESTATE_REGISTRY
{
	/* The name should be replaced with an unsigned int to improve
	 * performance */
	char							*pName;
	struct _tagGAMESTATE			*pGameState;
	struct _tagGAMESTATE_REGISTRY	*pNext;
}GAMESTATE_REGISTRY,*PGAMESTATE_REGISTRY;

typedef struct _tagGAMESTATE_MANAGER
{
	PMEMORY_BLOCK			pMemoryBlock;
	GAME_OPTIONS			GameOptions;
	GAMESTATE_REGISTRY		*pRegistry;
	struct _tagGAMESTATE	*pTopGameState;
	bool					Running;
	STACK					GameStateStack;
}GAMESTATE_MANAGER,*PGAMESTATE_MANAGER;

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PMEMORY_BLOCK p_pMemoryBlock );
void GSM_Terminate( PGAMESTATE_MANAGER p_pGameStateManager );

int GSM_ChangeState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName, void *p_pGameStateLoadArguments,
	void *p_pGameStateInitialiseArguments );
int GSM_PushState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pStateName, void *p_pGameStateLoadArguments,
	void *p_pGameStateInitialiseArguments );
int GSM_PopState( PGAMESTATE_MANAGER p_pGameStateManager );

int GSM_Run( PGAMESTATE_MANAGER p_pGameStateManager );
int GSM_Quit( PGAMESTATE_MANAGER p_pGameStateManager );
bool GSM_IsRunning( PGAMESTATE_MANAGER p_pGameStateManager );

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, struct _tagGAMESTATE *p_pGameState );
bool GSM_IsStateInRegistry( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pName, struct _tagGAMESTATE **p_ppGameState );

#endif /* __TERMINAL_GAMESTATEMANAGER_H__ */

