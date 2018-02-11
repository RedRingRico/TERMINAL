#include <TestMenuState.h>
#include <Menu.h>
#include <Peripheral.h>
#include <Log.h>
#include <Text.h>
#if defined ( DEBUG ) || defined ( DEVELOPMENT )

#define TEST_MENU_ITEMS 7

typedef struct
{
	GAMESTATE	Base;
	MENU		Menu;
}TESTMENU_GAMESTATE,*PTESTMENU_GAMESTATE;

static TESTMENU_GAMESTATE TestMenuState;

static int LaunchModelViewer( void *p_pArgs );

static int TMU_Load( void *p_pArgs )
{
	KMPACKEDARGB TextColour, HighlightColour;
	MENU_ITEM MenuItems[ TEST_MENU_ITEMS ];
	SELECTION_HIGHLIGHT_STRING SelectionHighlight;
	size_t MenuItemCount = 0;

	TextColour.dwPacked = 0xFFFFFF00;
	HighlightColour.dwPacked = 0xFF00FF00;

	MenuItems[ MenuItemCount ].pName = "Model Viewer";
	MenuItems[ MenuItemCount ].Function = LaunchModelViewer;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "Bump Mapping";
	MenuItems[ MenuItemCount ].Function = NULL;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "Model Animation [Coming 20XX]";
	MenuItems[ MenuItemCount ].Function = NULL;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "Real-Time Reflections [Coming 20XX]";
	MenuItems[ MenuItemCount ].Function = NULL;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "Shadow Mapping [Coming 20XX]";
	MenuItems[ MenuItemCount ].Function = NULL;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "X-Ray [Coming 20XX]";
	MenuItems[ MenuItemCount ].Function = NULL;

	++MenuItemCount;

	MenuItems[ MenuItemCount ].pName = "Volumetric Lighting [Coming 20XX]";
	MenuItems[ MenuItemCount ].Function = NULL;

	SelectionHighlight.Base.Type = SELECTION_HIGHLIGHT_TYPE_STRING;
	SelectionHighlight.Base.HighlightColour = HighlightColour;
	SelectionHighlight.pString = "> ";

	if( MNU_Initialise( &TestMenuState.Menu, MenuItems,
		sizeof( MenuItems ) / sizeof( MenuItems[ 0 ] ), &SelectionHighlight,
		GSM_GetGlyphSet( TestMenuState.Base.pGameStateManager,
			GSM_GLYPH_SET_GUI_1 ), TextColour, MENU_ITEM_ALIGNMENT_CENTRE,
		TestMenuState.Base.pGameStateManager->MemoryBlocks.pSystemMemory ) !=
		0 )
	{
		LOG_Debug( "[TMU_Load] <ERROR> Failed to initialise the menu\n");

		return 1;
	}

	return 0;
}

static int TMU_Initialise( void *p_pArgs )
{
	return 0;
}

static int TMU_Update( void *p_pArgs )
{
	if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
	{
		MNU_SelectPreviousMenuItem( &TestMenuState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
	{
		MNU_SelectNextMenuItem( &TestMenuState.Menu );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		PMENU_ITEM pMenuItem;

		pMenuItem = MNU_GetSelectedMenuItem( &TestMenuState.Menu );

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
		GSM_PopState( TestMenuState.Base.pGameStateManager );
	}

	return 0;
}

static int TMU_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;

	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		TestMenuState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( pGlyphSet, "[TERMINAL] //TEST MENU", &TextLength );
	TXT_RenderString( pGlyphSet, &TextColour,
		320.0f - ( TextLength * 0.5f ), 32.0f,
		"[TERMINAL] //TEST MENU" );

	TXT_MeasureString( pGlyphSet, "[A] select | [B] back", &TextLength );
	TXT_RenderString( pGlyphSet, &TextColour,
		640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
		"[A] select | [B] back" );

	MNU_Render( &TestMenuState.Menu, 1.5f, 320.0f, 240.0f );

	return 0;
}

static int TMU_Terminate( void *p_pArgs )
{
	return 0;
}

static int TMU_Unload( void *p_pArgs )
{
	MNU_Terminate( &TestMenuState.Menu );

	return 0;
}

static int TMU_VSyncCallback( void *p_pArgs )
{
	return 0;
}

int TMU_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	TestMenuState.Base.Load = &TMU_Load;
	TestMenuState.Base.Initialise = &TMU_Initialise;
	TestMenuState.Base.Update = &TMU_Update;
	TestMenuState.Base.Render = &TMU_Render;
	TestMenuState.Base.Terminate = &TMU_Terminate;
	TestMenuState.Base.Unload = &TMU_Unload;
	TestMenuState.Base.VSyncCallback = &TMU_VSyncCallback;
	TestMenuState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	TestMenuState.Base.VisibleToDebugAdapter  = true;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager, GAME_STATE_TESTMENU,
		( GAMESTATE * )&TestMenuState );
}

static int LaunchModelViewer( void *p_pArgs )
{
	return GSM_PushState( TestMenuState.Base.pGameStateManager,
		GAME_STATE_MODELVIEWER, NULL, NULL, true );
}

#endif /* DEBUG || DEVELOPMENT */

