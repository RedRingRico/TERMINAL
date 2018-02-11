#ifndef __TERMINAL_SINGLEPLAYERSTATE_H__
#define __TERMINAL_SINGLEPLAYERSTATE_H__

#include <GameStateManager.h>
#include <sg_xpt.h>

static const char *GAME_STATE_SINGLEPLAYER_MAIN =
	"Single Player [Main]";

Sint32 SP_RegisterMainWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_SINGLEPLAYERSTATE_H__ */

