#include <MultiPlayerState.h>
#include <GameState.h>
#include <Menu.h>
#include <Log.h>
#include <Peripheral.h>
#include <MainMenuState.h>
#include <kamui2.h>

typedef struct _tagMPM_GAMESTATE
{
	GAMESTATE	Base;
	MENU		Menu;
}MPM_GAMESTATE,*PMPM_GAMESTATE;

static MPM_GAMESTATE MultiPlayerMainState;

static Sint32 ConnectToISP( void *p_pArgs );

static Sint32 MPM_Load( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	KMPACKEDARGB HighlightColour;
	MENU_ITEM MenuItems[ 4 ];
	SELECTION_HIGHLIGHT_STRING SelectionHighlight;

	TextColour.dwPacked = 0xFFFFFFFF;
	HighlightColour.dwPacked = 0xFF00FF00;

	MenuItems[ 0 ].pName = "LOCAL [COMING 20XX]";
	MenuItems[ 0 ].Function = NULL;

	MenuItems[ 1 ].pName = "SERIAL [COMING 20XX]";
	MenuItems[ 1 ].Function = NULL;

	MenuItems[ 2 ].pName = "LAN [COMING 20XX]";
	MenuItems[ 2 ].Function = NULL;

	MenuItems[ 3 ].pName = "INTERNET";
	MenuItems[ 3 ].Function = ConnectToISP;

	SelectionHighlight.Base.Type = SELECTION_HIGHLIGHT_TYPE_STRING;
	SelectionHighlight.Base.HighlightColour = HighlightColour;
	SelectionHighlight.pString = "$ ";

	if( MNU_Initialise( &MultiPlayerMainState.Menu, MenuItems,
		sizeof( MenuItems ) / sizeof( MenuItems[ 0 ] ),
		&SelectionHighlight,
		GSM_GetGlyphSet( MultiPlayerMainState.Base.pGameStateManager,
			GSM_GLYPH_SET_GUI_1 ), TextColour, MENU_ITEM_ALIGNMENT_LEFT,
		MultiPlayerMainState.Base.pGameStateManager->MemoryBlocks.pSystemMemory
		) != 0 )
	{
		LOG_Debug( "MPM_Load <ERROR> Failed to initialise the menu\n" );

		return 1;
	}

	return 0;
}

static Sint32 MPM_Initialise( void *p_pArgs )
{
	return 0;
}

static Sint32 MPM_Update( void *p_pArgs )
{
	if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
	{
		MNU_SelectPreviousMenuItem( &MultiPlayerMainState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
	{
		MNU_SelectNextMenuItem( &MultiPlayerMainState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		PMENU_ITEM pMenuItem;

		pMenuItem = MNU_GetSelectedMenuItem( &MultiPlayerMainState.Menu );

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
		GSM_PopState( MultiPlayerMainState.Base.pGameStateManager );
	}

	return 0;
}

static Sint32 MPM_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;

	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		MultiPlayerMainState.Base.pGameStateManager,
		GSM_GLYPH_SET_GUI_1 );

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( pGlyphSet, "[TERMINAL] //MULTI PLAYER",
		&TextLength );
	TXT_RenderString( pGlyphSet, &TextColour,
		320.0f - ( TextLength * 0.5f ), 32.0f,
		"[TERMINAL] //MULTI PLAYER" );

	TXT_MeasureString( pGlyphSet, "[A] select | [B] back",
		&TextLength );
	TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
		"[A] select | [B] back" );

	MNU_Render( &MultiPlayerMainState.Menu, 2.0f, 320.0f, 240.0f );

	return 0;
}

static Sint32 MPM_Terminate( void *p_pArgs )
{
	return 0;
}

static Sint32 MPM_Unload( void *p_pArgs )
{
	MNU_Terminate( &MultiPlayerMainState.Menu );

	return 0;
}

static Sint32 MPM_VSyncCallback( void *p_pArgs )
{
	return 0;
}

Sint32 MP_RegisterMainWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	MultiPlayerMainState.Base.Load = &MPM_Load;
	MultiPlayerMainState.Base.Initialise = &MPM_Initialise;
	MultiPlayerMainState.Base.Update = &MPM_Update;
	MultiPlayerMainState.Base.Render = &MPM_Render;
	MultiPlayerMainState.Base.Terminate = &MPM_Terminate;
	MultiPlayerMainState.Base.Unload = &MPM_Unload;
	MultiPlayerMainState.Base.VSyncCallback = &MPM_VSyncCallback;
	MultiPlayerMainState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	MultiPlayerMainState.Base.VisibleToDebugAdapter = true;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MULTIPLAYER_MAIN, ( GAMESTATE * )&MultiPlayerMainState );
}

static Sint32 ConnectToISP( void *p_pArgs )
{
	GSM_ChangeState( MultiPlayerMainState.Base.pGameStateManager,
		GAME_STATE_MULTIPLAYER_ISPCONNECT, NULL, NULL );

	return 0;
}


