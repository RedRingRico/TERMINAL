#ifndef __TERMINAL_GAMESTATE_H__
#define __TERMINAL_GAMESTATE_H__

#include <Stack.h>
#include <GameStateManager.h>

/* Game state functions can accept a parameter of any type, for multiple
 * parameters, use a struct */
typedef int ( *GS_Function )( void * );

typedef struct _tagGAMESTATE
{
	GS_Function						Load;
	GS_Function						Initialise;
	GS_Function						Update;
	GS_Function						Render;
	GS_Function						Terminate;
	GS_Function						Unload;
	GS_Function						VSyncCallback;
	struct _tagGAMESTATE_MANAGER	*pGameStateManager;
	bool							Paused;
	Uint32							ElapsedGameTime;
}GAMESTATE,*PGAMESTATE;

void GS_Copy( PGAMESTATE p_pCopy, PGAMESTATE p_pOriginal );

void GS_Pause( PGAMESTATE p_pGameState );
void GS_Resume( PGAMESTATE p_pGameState );

#endif /* __TERMINAL_GAMESTATE_H__ */

