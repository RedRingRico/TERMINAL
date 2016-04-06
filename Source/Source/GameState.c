#include <GameState.h>

void GS_Pause( PGAME_STATE p_pGameState )
{
	p_pGameState->Paused = true;
}

void GS_Resume( PGAME_STATE p_pGameState )
{
	p_pGameState->Paused = false;
}

