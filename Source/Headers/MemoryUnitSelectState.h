#ifndef __TERMINAL_MEMORYUNITSELECTSTATE_H__
#define __TERMINAL_MEMORYUNITSELECTSTATE_H__

#include <GameStateManager.h>
#include <Text.h>

static const char *GAME_STATE_MEMORYUNITSELECT = "Memory Unit select";

typedef struct _tagMEMORYUNITSELECT
{
	PGLYPHSET	pGlyphSet;
}MEMORYUNITSELECT,*PMEMORYUNITSELECT;

int MUSS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager );

#endif /* __TERMINAL_MEMORYUNITSELECTSTATE_H__ */

