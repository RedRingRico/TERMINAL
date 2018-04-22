#ifndef __TERMINAL_ASPECTRATIOSELECTSTATE_H__
#define __TERMINAL_ASPECTRATIOSELECTSTATE_H__

#include <GameStateManager.h>
#include <Text.h>

static const char *GAME_STATE_ASPECTRATIOSELECT = "Aspect ratio select";

typedef struct _tagASPECTRATIOSELECT
{
	PGLYPHSET	pGlyphSet;
}ASPECTRATIOSELECT,*PASPECTRATIOSELECT;

Sint32 ARSS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_ASPECRATIOSELECTSTATE_H__ */

