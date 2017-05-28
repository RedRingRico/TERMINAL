#ifndef __TERMINAL_TESTMENUSTATE_H__
#define __TERMINAL_TESTMENUSTATE_H__

#include <GameStateManager.h>

static const char *GAME_STATE_TESTMENU = "Test menu";
static const char *GAME_STATE_MODELVIEWER = "Model viewer";

int TMU_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

int MDLV_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_TESTMENUSTATE_H__ */

