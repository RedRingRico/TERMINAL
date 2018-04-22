#ifndef __TERMINAL_MULTIPLAYERSTATE_H__
#define __TERMINAL_MULTIPLAYERSTATE_H__

#include <GameStateManager.h>
#include <NetworkSocketAddress.h>
#include <shinobi.h>

static const char *GAME_STATE_MULTIPLAYER_MAIN =
	"Multi Player [Main]";
static const char *GAME_STATE_MULTIPLAYER_ISPCONNECT =
	"Multi Player [ISP Connect]";
static const char *GAME_STATE_MULTIPLAYER_GAMELISTSERVER =
	"Multi Player [Game List Server]";
static const char *GAME_STATE_MULTIPLAYER_GAME =
	"Multi Player [Game]";

typedef struct _tagMULTIPLAYER_GAME_ARGS
{
	Uint32	IP;
	Uint16	Port;
}MULTIPLAYER_GAME_ARGS,*PMULTIPLAYER_GAME_ARGS;

Sint32 MP_RegisterMainWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

Sint32 MP_RegisterISPConnectWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

Sint32 MP_RegisterGameListServerWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

Sint32 MP_RegisterMultiPlayerGameWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_MULTIPLAYERSTATE_H__ */

