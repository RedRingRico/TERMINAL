#include <TestMenuState.h>
#include <Peripheral.h>
#include <Keyboard.h>

#if defined ( DEBUG ) || defined ( DEVELOPMENT )

typedef struct
{
	GAMESTATE	Base;
	PKEYBOARD	Keyboard[ 4 ];
}KEYBOARDTEST_GAMESTATE,*PKEYBOARDTEST_GAMESTATE;

static KEYBOARDTEST_GAMESTATE KeyboardTestState;

static Sint32 KBDT_Load( void *p_pArgs )
{
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
	return 0;
}

static Sint32 KBDT_Terminate( void *p_pArgs )
{
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

