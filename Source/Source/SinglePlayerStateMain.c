#include <SinglePlayerSTate.h>
#include <GameState.h>
#include <Menu.h>
#include <Log.h>
#include <Peripheral.h>

typedef struct _tagSPM_GAMESTATE
{
	GAMESTATE	Base;
	MENU		Menu;
}SPM_GAMESTATE, *PSPM_GAMESTATE;

static SPM_GAMESTATE SinglePlayerMainState;

static Sint32 SPM_Load( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	KMPACKEDARGB HighlightColour;
	MENU_ITEM MenuItems[ 2 ];
	SELECTION_HIGHLIGHT_STRING SelectionHighlight;

	TextColour.dwPacked = 0xFFFFFFFF;
	HighlightColour.dwPacked = 0xFF00FF00;

	MenuItems[ 0 ].pName = "NEW GAME";
	MenuItems[ 0 ].Function = NULL;

	MenuItems[ 1 ].pName = "LOAD GAME";
	MenuItems[ 1 ].Function = NULL;

	SelectionHighlight.Base.Type = SELECTION_HIGHLIGHT_TYPE_STRING;
	SelectionHighlight.Base.HighlightColour = HighlightColour;
	SelectionHighlight.pString = "~/ ";

	if( MNU_Initialise( &SinglePlayerMainState.Menu, MenuItems,
		sizeof( MenuItems ) / sizeof( MenuItems[ 0 ] ),
		&SelectionHighlight,
		GSM_GetGlyphSet( SinglePlayerMainState.Base.pGameStateManager,
			GSM_GLYPH_SET_GUI_1 ), TextColour, MENU_ITEM_ALIGNMENT_LEFT,
		SinglePlayerMainState.Base.pGameStateManager->
			MemoryBlocks.pSystemMemory ) != 0 )
	{
		LOG_Debug( "[SPM_Load] <ERROR> Failed to initialise the menu" );

		return 1;
	}

	return 0;
}

static Sint32 SPM_Initialise( void *p_pArgs )
{
	return 0;
}

static Sint32 SPM_Update( void *p_pArgs )
{
	if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
	{
		MNU_SelectPreviousMenuItem( &SinglePlayerMainState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
	{
		MNU_SelectNextMenuItem( &SinglePlayerMainState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		PMENU_ITEM pMenuItem;

		pMenuItem = MNU_GetSelectedMenuItem( &SinglePlayerMainState.Menu );

		if( pMenuItem )
		{
			if( pMenuItem->Function )
			{
				pMenuItem->Function( NULL );
			}
		}
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
	{
		GSM_PopState( SinglePlayerMainState.Base.pGameStateManager );
	}

	return 0;
}

static Sint32 SPM_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;

	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		SinglePlayerMainState.Base.pGameStateManager,
		GSM_GLYPH_SET_GUI_1 );

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( pGlyphSet, "[TERMINAL] //SINGLE PLAYER",
		&TextLength );
	TXT_RenderString( pGlyphSet, &TextColour,
		320.0f - ( TextLength * 0.5f ), 32.0f,
		"[TERMINAL] //SINGLE PLAYER" );

	TXT_MeasureString( pGlyphSet, "[A] select | [B] back",
		&TextLength );
	TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
		"[A] select | [B] back" );

	MNU_Render( &SinglePlayerMainState.Menu, 2.0f, 320.0f, 240.0f );

	return 0;
}

static Sint32 SPM_Terminate( void *p_pArgs )
{
	return 0;
}

static Sint32 SPM_Unload( void *p_pArgs )
{
	MNU_Terminate( &SinglePlayerMainState.Menu );

	return 0;
}

static Sint32 SPM_VSyncCallback( void *p_pArgs )
{
	return 0;
}

Sint32 SP_RegisterMainWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	SinglePlayerMainState.Base.Load = &SPM_Load;
	SinglePlayerMainState.Base.Initialise = &SPM_Initialise;
	SinglePlayerMainState.Base.Update = &SPM_Update;
	SinglePlayerMainState.Base.Render = &SPM_Render;
	SinglePlayerMainState.Base.Terminate = &SPM_Terminate;
	SinglePlayerMainState.Base.Unload = &SPM_Unload;
	SinglePlayerMainState.Base.VSyncCallback = &SPM_VSyncCallback;
	SinglePlayerMainState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	SinglePlayerMainState.Base.VisibleToDebugAdapter = true;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_SINGLEPLAYER_MAIN, ( PGAMESTATE )&SinglePlayerMainState );
}

