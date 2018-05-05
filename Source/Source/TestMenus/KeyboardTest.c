#include <TestMenuState.h>
#include <Peripheral.h>
#include <Keyboard.h>
#include <Hardware.h>
#include <Renderer.h>

#if defined ( DEBUG ) || defined ( DEVELOPMENT )

typedef struct
{
	GAMESTATE	Base;
	PGLYPHSET	pGlyphSet;
	PKEYBOARD	Keyboard[ 4 ];
}KEYBOARDTEST_GAMESTATE,*PKEYBOARDTEST_GAMESTATE;

static KEYBOARDTEST_GAMESTATE KeyboardTestState;

static Sint32 KBDT_Load( void *p_pArgs )
{
	KeyboardTestState.pGlyphSet = GSM_GetGlyphSet(
		KeyboardTestState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	return 0;
}

static Sint32 KBDT_Initialise( void *p_pArgs )
{
	size_t Index = 0;

	for( Index = 0; Index < 4; ++Index )
	{
		KeyboardTestState.Keyboard[ Index ] =
			KeyboardTestState.Base.pGameStateManager->Keyboard[ Index ];
	}

	return 0;
}

static Sint32 KBDT_Update( void *p_pArgs )
{
	if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
	{
		GSM_PopState( KeyboardTestState.Base.pGameStateManager );
	}

	if( KBD_KeyPressed( KeyboardTestState.Keyboard[ 3 ] ) )
	{
		Uint16 Data;
		Uint16 OrgData = KBD_GetChar( KeyboardTestState.Keyboard[ 3 ] );
		Data = OrgData & 0xFF;

		if( Data == 0x1B )
		{
			GSM_PopState( KeyboardTestState.Base.pGameStateManager );
		}
	}

	return 0;
}

static Sint32 KBDT_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;
	static char PrintBuffer[ 128 ];
	size_t Index;

	TextColour.dwPacked = 0xFFFFFFFF;

	/* Show which ports keyboards are connected to */
	for( Index = 0; Index < 4; ++Index )
	{
		if( KeyboardTestState.Keyboard[ Index ]->Flags & KBD_READY )
		{
			PKEYBOARD pKeyboard = KeyboardTestState.Keyboard[ Index ];
			Uint8 *pRawData = pKeyboard->pRawData->key;

			TXT_MeasureString( KeyboardTestState.pGlyphSet,
				HW_PortToName( pKeyboard->Port ), &TextLength );

			TXT_RenderString( KeyboardTestState.pGlyphSet, &TextColour,
				( 140.0f * Index ) - ( TextLength * 0.5f ), 64.0f,
				HW_PortToName( pKeyboard->Port ) );

			sprintf( PrintBuffer,
				"%02X %02X %02X %02X %02X %02X %02X",
				pKeyboard->pRawData->ctrl,
				pRawData[ 0 ], pRawData[ 1 ], pRawData[ 2 ],
				pRawData[ 3 ], pRawData[ 4 ], pRawData[ 5 ] );

			TXT_RenderString( KeyboardTestState.pGlyphSet, &TextColour,
				( 140.0f * Index ) - ( TextLength * 0.5f ),
				64.0f + ( float )KeyboardTestState.pGlyphSet->LineHeight,
				"Raw input:" );

			TXT_RenderString( KeyboardTestState.pGlyphSet, &TextColour,
				( 140.0f * Index ) - ( TextLength * 0.5f ),
				64.0f + ( float )KeyboardTestState.pGlyphSet->LineHeight *
					2.0f,
				PrintBuffer );

			sprintf( PrintBuffer, "Delay count: %d", pKeyboard->DelayCount );

			TXT_RenderString( KeyboardTestState.pGlyphSet, &TextColour,
				( 140.0f * Index ) - ( TextLength * 0.5f ),
				64.0f + ( float )KeyboardTestState.pGlyphSet->LineHeight *
					3.0f,
				PrintBuffer );

			sprintf( PrintBuffer, "Rate count: %d", pKeyboard->RateCount );

			TXT_RenderString( KeyboardTestState.pGlyphSet, &TextColour,
				( 140.0f * Index ) - ( TextLength * 0.5f ),
				64.0f + ( float )KeyboardTestState.pGlyphSet->LineHeight *
					4.0f,
				PrintBuffer );
		}
	}

	return 0;
}

static Sint32 KBDT_Terminate( void *p_pArgs )
{
	size_t Index = 0;

	for( Index = 0; Index < 4; ++Index )
	{
		KBD_Flush( KeyboardTestState.Keyboard[ Index ] );
	}

	return 0;
}

static Sint32 KBDT_Unload( void *p_pArgs )
{
	return 0;
}

static Sint32 KBDT_VSyncCallback( void *p_pArgs )
{
	return 0;
}

Sint32 KBDT_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	KeyboardTestState.Base.Load = &KBDT_Load;
	KeyboardTestState.Base.Initialise = &KBDT_Initialise;
	KeyboardTestState.Base.Update = &KBDT_Update;
	KeyboardTestState.Base.Render = &KBDT_Render;
	KeyboardTestState.Base.Terminate = &KBDT_Terminate;
	KeyboardTestState.Base.Unload = &KBDT_Unload;
	KeyboardTestState.Base.VSyncCallback = &KBDT_VSyncCallback;
	KeyboardTestState.Base.pGameStateManager = p_pGameStateManager;

	KeyboardTestState.Base.VisibleToDebugAdapter = true;

	return GSM_RegisterGameState( p_pGameStateManager, GAME_STATE_KEYBOARDTEST,
		( GAMESTATE * )&KeyboardTestState );
}


#endif /* DEBUG || DEVELOPMENT */

