#ifndef __TERMINAL_GAMESTATEMANAGER_H__
#define __TERMINAL_GAMESTATEMANAGER_H__

#include <GameState.h>
#include <GameOptions.h>
#include <Memory.h>
#include <Text.h>

#define GSM_GLYPH_SET_DEBUG	0
#define GSM_GLYPH_SET_GUI_1	1

/* This should probably also include alignment */
typedef struct _tagGAMESTATE_MEMORY_BLOCKS
{
	PMEMORY_BLOCK	pSystemMemory;
	PMEMORY_BLOCK	pGraphicsMemory;
	PMEMORY_BLOCK	pAudioMemory;
}GAMESTATE_MEMORY_BLOCKS,*PGAMESTATE_MEMORY_BLOCKS;

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
	GAME_OPTIONS			GameOptions;
	GAMESTATE_REGISTRY		*pRegistry;
	struct _tagGAMESTATE	*pTopGameState;
	PGLYPHSET				*ppGlyphSet;
	GAMESTATE_MEMORY_BLOCKS	MemoryBlocks;
	bool					Running;
	STACK					GameStateStack;
}GAMESTATE_MANAGER,*PGAMESTATE_MANAGER;

int GSM_Initialise( PGAMESTATE_MANAGER p_pGameStateManager,
	PGAMESTATE_MEMORY_BLOCKS p_pMemoryBlocks );
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

int GSM_RegisterGlyphSet( PGAMESTATE_MANAGER p_pGameStateManager,
	const Uint32 p_Index, PGLYPHSET p_pGlyphSet );
PGLYPHSET GSM_GetGlyphSet( PGAMESTATE_MANAGER p_pGameStateManager,
	const Uint32 p_Index );

int GSM_RegisterGameState( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pGameStateName, struct _tagGAMESTATE *p_pGameState );
bool GSM_IsStateInRegistry( PGAMESTATE_MANAGER p_pGameStateManager,
	const char *p_pName, struct _tagGAMESTATE **p_ppGameState );

#endif /* __TERMINAL_GAMESTATEMANAGER_H__ */

