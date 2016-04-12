#ifndef __TERMINAL_MULTIPLAYERSTATE_H__
#define __TERMINAL_MULTIPLAYERSTATE_H__

#include <GameStateManager.h>
#include <shinobi.h>

static const char *GAME_STATE_MULTIPLAYER_MAIN =
	"Multi Player [Main]";
static const char *GAME_STATE_MULTIPLAYER_ISPCONNECT =
	"Multi Player [ISP Connect]";
static const char *GAME_STATE_MULTIPLAYER_GAMELISTSERVERCONNECT =
	"Multi Player [Game List Server Connect]";

int MP_RegisterMainWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

int MP_RegisterISPConnectWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_MULTIPLAYERSTATE_H__ */

