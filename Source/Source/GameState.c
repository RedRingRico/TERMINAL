#include <GameState.h>

void GS_Copy( PGAMESTATE p_pCopy, PGAMESTATE p_pOriginal )
{
	memcpy( p_pCopy, p_pOriginal, sizeof( GAMESTATE ) );
}

void GS_Pause( PGAMESTATE p_pGameState )
{
	p_pGameState->Paused = true;
}

void GS_Resume( PGAMESTATE p_pGameState )
{
	p_pGameState->Paused = false;
}

