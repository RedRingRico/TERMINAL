#ifndef __TERMINAL_MAINMENUSTATE_H__
#define __TERMINAL_MAINMENUSTATE_H__

#include <GameStateManager.h>
#include <Text.h>

static const char *GAME_STATE_MAINMENU = "Main menu";

typedef struct _tagMAINMENU
{
	PGLYPHSET	pGlyphSet;
}MAINMENU,*PMAINMENU;

Sint32 MMS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_MAINMENUSTATE_H__ */

