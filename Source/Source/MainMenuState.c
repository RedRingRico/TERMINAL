#include <MainMenuState.h>
#include <GameState.h>
#include <Peripheral.h>
#include <Renderer.h>
#include <Menu.h>
#include <Log.h>
#include <MultiPlayerState.h>
#include <TestMenuState.h>

#if defined ( DEBUG ) || defined( DEVELOPMENT )
#define MENU_ITEMS 5
#else
#define MENU_ITEMS 4
#endif /* DEBUG || DEVELOPMENT */

typedef struct _tagMAINMENU_GAMESTATE
{
	GAMESTATE	Base;
	PGLYPHSET	pGlyphSet;
	MENU		Menu;
}MAINMENU_GAMESTATE,*PMAINMENU_GAMESTATE;

static MAINMENU_GAMESTATE MainMenuState;

static int BootROM( void *p_pArgs );
static int LaunchMultiPlayer( void *p_pArgs );
static int LaunchTestMenu( void *p_pArgs );

static int MMS_Load( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	KMPACKEDARGB HighlightColour;
	MENU_ITEM MenuItems[ MENU_ITEMS ];
	SELECTION_HIGHLIGHT_STRING SelectionHighlight;

	TextColour.dwPacked = 0xFFFFFFFF;
	HighlightColour.dwPacked = 0xFF00FF00;

	MainMenuState.pGlyphSet = GSM_GetGlyphSet(
		MainMenuState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	MenuItems[ 0 ].pName = "SINGLE PLAYER [COMING 20XX]";
	MenuItems[ 0 ].Function = NULL;

	MenuItems[ 1 ].pName = "MULTI PLAYER";
	MenuItems[ 1 ].Function = LaunchMultiPlayer;

	MenuItems[ 2 ].pName = "OPTIONS";
	MenuItems[ 2 ].Function = NULL;

	MenuItems[ 3 ].pName = "RESET TO BOOTROM";
	MenuItems[ 3 ].Function = &BootROM;

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	MenuItems[ 4 ].pName = "TEST";
	MenuItems[ 4 ].Function = &LaunchTestMenu;
#endif /* DEBUG || DEVELOPMENT */

	SelectionHighlight.Base.Type = SELECTION_HIGHLIGHT_TYPE_STRING;
	SelectionHighlight.Base.HighlightColour = HighlightColour;
	SelectionHighlight.pString = "$ ";

	if( MNU_Initialise( &MainMenuState.Menu, MenuItems,
		sizeof( MenuItems ) / sizeof( MenuItems[ 0 ] ), &SelectionHighlight,
		GSM_GetGlyphSet( MainMenuState.Base.pGameStateManager,
			GSM_GLYPH_SET_GUI_1 ), TextColour, MENU_ITEM_ALIGNMENT_LEFT,
		MainMenuState.Base.pGameStateManager->MemoryBlocks.pSystemMemory ) !=
		0 )
	{
		LOG_Debug( "MMS_Load <ERROR> Failed to initialise the menu\n" );

		return 1;
	}

	return 0;
}

static int MMS_Initialise( void *p_pArgs )
{
	return 0;
}

static int MMS_Update( void *p_pArgs )
{
	if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
	{
		MNU_SelectPreviousMenuItem( &MainMenuState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
	{
		MNU_SelectNextMenuItem( &MainMenuState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		PMENU_ITEM pMenuItem;

		pMenuItem = MNU_GetSelectedMenuItem( &MainMenuState.Menu );

		if( pMenuItem )
		{
			if( pMenuItem->Function )
			{
				pMenuItem->Function( NULL );
			}
		}
	}

	return 0;
}

static int MMS_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( MainMenuState.pGlyphSet, "[TERMINAL] //MAIN MENU",
		&TextLength );
	TXT_RenderString( MainMenuState.pGlyphSet, &TextColour,
		320.0f - ( TextLength * 0.5f ), 32.0f,
		"[TERMINAL] //MAIN MENU" );

	TXT_MeasureString( MainMenuState.pGlyphSet, "[A] select",
		&TextLength );
	TXT_RenderString( MainMenuState.pGlyphSet, &TextColour,
		640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )MainMenuState.pGlyphSet->LineHeight ),
		"[A] select" );

	MNU_Render( &MainMenuState.Menu, 1.5f, 320.0f, 240.0f );

	return 0;
}

static int MMS_Terminate( void *p_pArgs )
{
	return 0;
}

static int MMS_Unload( void *p_pArgs )
{
	MNU_Terminate( &MainMenuState.Menu );

	return 0;
}

static int MMS_VSyncCallback( void *p_pArgs )
{
	return 0;
}

int MMS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	MainMenuState.Base.Load = &MMS_Load;
	MainMenuState.Base.Initialise = &MMS_Initialise;
	MainMenuState.Base.Update = &MMS_Update;
	MainMenuState.Base.Render = &MMS_Render;
	MainMenuState.Base.Terminate = &MMS_Terminate;
	MainMenuState.Base.Unload = &MMS_Unload;
	MainMenuState.Base.VSyncCallback = &MMS_VSyncCallback;
	MainMenuState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	MainMenuState.Base.VisibleToDebugAdapter = true;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager, GAME_STATE_MAINMENU,
		( GAMESTATE * )&MainMenuState );
}

static int LaunchMultiPlayer( void *p_pArgs )
{
	GSM_PushState( MainMenuState.Base.pGameStateManager,
		GAME_STATE_MULTIPLAYER_MAIN, NULL, NULL, true );

	return 0;
}

static int LaunchTestMenu( void *p_pArgs )
{
	GSM_PushState( MainMenuState.Base.pGameStateManager,
		GAME_STATE_TESTMENU, NULL, NULL, true );

	return 0;
}

static int BootROM( void *p_pArgs )
{
	GSM_Quit( MainMenuState.Base.pGameStateManager );

	return 0;
}

