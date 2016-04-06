#ifndef __TERMINAL_GAMESTATE_H__
#define __TERMINAL_GAMESTATE_H__

#include <Stack.h>

/* Game state functions can accept a parameter of any type, for multiple
 * parameters, use a struct */
typedef int ( *GS_Function )( void * );

typedef struct _tagGAME_STATE
{
	GS_Function	Load;
	GS_Function	Initialise;
	GS_Function	Update;
	GS_Function	Render;
	GS_Function	Terminate;
	GS_Function	Unload;
	bool		Paused;
}GAME_STATE,*PGAME_STATE;

void GS_Pause( PGAME_STATE p_pGameState );
void GS_Resume( PGAME_STATE p_pGameState );

#endif /* __TERMINAL_GAMESTATE_H__ */

