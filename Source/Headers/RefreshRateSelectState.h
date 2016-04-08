#ifndef __TERMINAL_REFRESHRATESELECTSTATE_H__
#define __TERMINAL_REFRESHRATESELECTSTATE_H__

#include <GameState.h>
#include <GameStateManager.h>
#include <Text.h>

static const char *GAME_STATE_REFRESHRATESELECT = "Refresh rate select";

typedef struct _tagREFRESHRATESELECT
{
	PGLYPHSET	pGlyphSet;
}REFRESHRATESELECT,*PREFRESHRATESELECT;

int RRSS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_REFRESHRATESELECTSTATE_H__ */

