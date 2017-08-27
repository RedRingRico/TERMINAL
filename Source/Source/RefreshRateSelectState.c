#include <RefreshRateSelectState.h>
#include <Renderer.h>
#include <Peripheral.h>
#include <AspectRatioSelectState.h>
#include <Log.h>

typedef struct _tagREFRESHRATESELECT_GAMESTATE
{
	GAMESTATE	Base;
	PGLYPHSET	pGlyphSet;
	bool		CableSelect;
	bool		SixtyHz;
	bool		ModeTest;
	bool		TestPass;
	bool		ModeSelected;
	int			TestStage;
	Uint32		StartTime;
	Uint32		ElapsedTime;
	KMBYTE		Alpha;
}REFRESHRATESELECT_GAMESTATE,*PREFRESHRATESELECT_GAMESTATE;

static REFRESHRATESELECT_GAMESTATE RefreshRateSelectState;

static int RRSS_Load( void *p_pArgs )
{
	PREFRESHRATESELECT pArguments = p_pArgs;

	if( pArguments == NULL )
	{
		LOG_Debug( "[RRSS_Load] <ERROR> Arguments pointer is null" );

		return 1;
	}

	RefreshRateSelectState.pGlyphSet = pArguments->pGlyphSet;

	return 0;
}

static int RRSS_Initialise( void *p_pArgs )
{
	RefreshRateSelectState.CableSelect = true;
	RefreshRateSelectState.SixtyHz = false;
	RefreshRateSelectState.ModeTest = false;
	RefreshRateSelectState.TestPass = false;
	RefreshRateSelectState.ModeSelected = false;
	RefreshRateSelectState.TestStage = false;
	RefreshRateSelectState.StartTime = 0L;
	RefreshRateSelectState.ElapsedTime = 0UL;
	RefreshRateSelectState.Alpha = 255;

	return 0;
}

static int RRSS_Update( void *p_pArgs )
{
	static float Alpha = 1.0f, AlphaInc = 0.2f;

	RefreshRateSelectState.StartTime = syTmrGetCount( );

	if( RefreshRateSelectState.CableSelect == true )
	{
		if( ( g_Peripherals[ 0 ].press & PDD_DGT_TA ) &&
			( RefreshRateSelectState.ModeTest == false ) )
		{
			RefreshRateSelectState.ModeTest = true;
			RefreshRateSelectState.TestStage = 0;
		}

		if( RefreshRateSelectState.ModeTest == true )
		{
			if( RefreshRateSelectState.SixtyHz == true )
			{
				if( RefreshRateSelectState.TestStage == 0 )
				{
					if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
					{
						RefreshRateSelectState.ElapsedTime = 0UL;
						RefreshRateSelectState.TestStage = 1;

						kmSetDisplayMode( KM_DSPMODE_NTSCNI640x480,
							KM_DSPBPP_RGB888, TRUE, FALSE );
					}
				}
				else if( RefreshRateSelectState.TestStage == 1 )
				{
					if( RefreshRateSelectState.ElapsedTime >= 5000000UL )
					{
						RefreshRateSelectState.TestStage = 2;
						kmSetDisplayMode( KM_DSPMODE_PALNI640x480EXT,
							KM_DSPBPP_RGB888, TRUE, FALSE );
					}
				}
				else if( RefreshRateSelectState.TestStage == 2 )
				{
					if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
					{
						RefreshRateSelectState.ModeTest = false;
						RefreshRateSelectState.ModeSelected = true;
					}

					if( ( g_Peripherals[ 0 ].press & PDD_DGT_KL ) ||
						( g_Peripherals[ 0 ].press & PDD_DGT_KR ) )
					{
						Alpha = 1.0f;
						RefreshRateSelectState.TestPass =
							!RefreshRateSelectState.TestPass;
					}
				}
			}
			else
			{
				RefreshRateSelectState.ModeSelected = true;
			}
		}
		/* Not currently testing the display mode, present the player with
		 * an option to select the display mode */
		else
		{
			if( ( g_Peripherals[ 0 ].press & PDD_DGT_KL ) ||
				( g_Peripherals[ 0 ].press & PDD_DGT_KR ) )
			{
				RefreshRateSelectState.SixtyHz =
					!RefreshRateSelectState.SixtyHz;
				Alpha = 1.0f;
			}
		}

		if( RefreshRateSelectState.ModeSelected )
		{
			RefreshRateSelectState.CableSelect = false;
		}
	}
	else
	{
		ASPECTRATIOSELECT AspectRatioArgs;

		AspectRatioArgs.pGlyphSet = RefreshRateSelectState.pGlyphSet;

		/* Done, change state */
		if( ( RefreshRateSelectState.TestPass == true ) &&
			( RefreshRateSelectState.SixtyHz == true ) )
		{
			kmSetDisplayMode( KM_DSPMODE_NTSCNI640x480,
				KM_DSPBPP_RGB888, TRUE, FALSE );
		}

		GSM_ChangeState( RefreshRateSelectState.Base.pGameStateManager,
			GAME_STATE_ASPECTRATIOSELECT, &AspectRatioArgs, NULL );
	}

	Alpha += AlphaInc;

	if( Alpha <= 0.0f )
	{
		AlphaInc = 0.02f;
		Alpha = 0.0f;
		RefreshRateSelectState.Alpha = 0;
	}
	else if( Alpha >= 1.0f )
	{
		AlphaInc = -0.02f;
		Alpha = 1.0f;
		RefreshRateSelectState.Alpha = 255;
	}
	else
	{
		RefreshRateSelectState.Alpha = ( KMBYTE )( Alpha * 255.0f );
	}

	return 0;
}

static int RRSS_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;

	TextColour.dwPacked = 0xFFFFFFFF;

	if( RefreshRateSelectState.CableSelect == true )
	{
		if( RefreshRateSelectState.ModeTest == true )
		{
			if( RefreshRateSelectState.SixtyHz == true )
			{
				/* Start of display test */
				if( RefreshRateSelectState.TestStage == 0 )
				{
					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						"The display will change mode now for five seconds",
						&TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						( float )RefreshRateSelectState.pGlyphSet->LineHeight *
							4.0f,
						"The display will change mode now for five seconds" );

					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						"Press 'A' to continue", &TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float )
							RefreshRateSelectState.pGlyphSet->LineHeight /
								2.0f ),
						"Press 'A' to continue" );
				}
				/* Display test */
				else if( RefreshRateSelectState.TestStage == 1 )
				{
					char TimeLeft[ 80 ];

					sprintf( TimeLeft, "Time remaining: %lu",
						5000000UL - RefreshRateSelectState.ElapsedTime );
					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						TimeLeft, &TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float )
							RefreshRateSelectState.pGlyphSet->LineHeight *
								6.0f ),
						TimeLeft );

					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						"Some cool artwork", &TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float )
							RefreshRateSelectState.pGlyphSet->LineHeight /
								2.0f ),
						"Some cool artwork" );
				}
				else if( RefreshRateSelectState.TestStage == 2 )
				{
					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						"Did the picture appear stable?", &TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						( float )RefreshRateSelectState.pGlyphSet->LineHeight *
							4.0f,
						"Did the picture appear stable?" );

					TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
						"Press 'A' to continue", &TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength / 2.0f ),
						( float )RefreshRateSelectState.pGlyphSet->LineHeight *
							5.5f,
						"Press 'A' to continue" );


					if( RefreshRateSelectState.TestPass == true )
					{
						TextColour.byte.bAlpha = RefreshRateSelectState.Alpha;
					}
					else
					{
						TextColour.byte.bAlpha = 255;
					}

					TXT_MeasureString( RefreshRateSelectState.pGlyphSet, "Yes",
						&TextLength );
					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 320.0f - ( TextLength ) - 50.0f,
						( 240.0f - ( float )
							RefreshRateSelectState.pGlyphSet->LineHeight /
								2.0f ),
						"Yes" );

					if( RefreshRateSelectState.TestPass == false )
					{
						TextColour.byte.bAlpha = RefreshRateSelectState.Alpha;
					}
					else
					{
						TextColour.byte.bAlpha = 255;
					}

					TXT_RenderString( RefreshRateSelectState.pGlyphSet,
						&TextColour, 370.0f, ( 240.0f - ( float )
							RefreshRateSelectState.pGlyphSet->LineHeight /
								2.0f ), "No" );
				}
			}
		}
		else
		{
			TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
				"Select output mode", &TextLength );
			TXT_RenderString( RefreshRateSelectState.pGlyphSet, &TextColour,
				320.0f - ( TextLength / 2.0f ),
				( float )RefreshRateSelectState.pGlyphSet->LineHeight * 4.0f,
				"Select output mode" );

			TXT_MeasureString( RefreshRateSelectState.pGlyphSet,
				"Press 'A' to select", &TextLength );
			TXT_RenderString( RefreshRateSelectState.pGlyphSet, &TextColour,
				320.0f - ( TextLength / 2.0f ),
				( 480.0f -
					( float )RefreshRateSelectState.pGlyphSet->LineHeight *
						4.0f ), "Press 'A' to select" );

			if( RefreshRateSelectState.SixtyHz == false )
			{
				TextColour.byte.bAlpha = RefreshRateSelectState.Alpha;
			}
			else
			{
				TextColour.byte.bAlpha = 255;
			}

			TXT_MeasureString( RefreshRateSelectState.pGlyphSet, "50Hz",
				&TextLength );
			TXT_RenderString( RefreshRateSelectState.pGlyphSet, &TextColour,
				270.0f - ( TextLength ), ( 240.0f -
					( float )RefreshRateSelectState.pGlyphSet->LineHeight /
						2.0f ), "50Hz" );

			if( RefreshRateSelectState.SixtyHz == true )
			{
				TextColour.byte.bAlpha = RefreshRateSelectState.Alpha;
			}
			else
			{
				TextColour.byte.bAlpha = 255;
			}

			TXT_RenderString( RefreshRateSelectState.pGlyphSet, &TextColour,
				370.0f, ( 240.0f - ( float )
					RefreshRateSelectState.pGlyphSet->LineHeight / 2.0f ),
				"60Hz" );
		}
	}

	/* Must sample time outside the frame update, otherwise it's incorrect */
	RefreshRateSelectState.ElapsedTime +=
		syTmrCountToMicro( syTmrDiffCount(
			RefreshRateSelectState.StartTime, syTmrGetCount( ) ) );

	return 0;
}

static int RRSS_Terminate( void *p_pArgs )
{
	return 0;
}

static int RRSS_Unload( void *p_pArgs )
{
	return 0;
}

static int RRSS_VSyncCallback( void *p_pArgs )
{
	return 0;
}

int RRSS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	RefreshRateSelectState.Base.Load = &RRSS_Load;
	RefreshRateSelectState.Base.Initialise = &RRSS_Initialise;
	RefreshRateSelectState.Base.Update = &RRSS_Update;
	RefreshRateSelectState.Base.Render = &RRSS_Render;
	RefreshRateSelectState.Base.Terminate = &RRSS_Terminate;
	RefreshRateSelectState.Base.Unload = &RRSS_Unload;
	RefreshRateSelectState.Base.VSyncCallback = &RRSS_VSyncCallback;
	RefreshRateSelectState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	RefreshRateSelectState.Base.VisibleToDebugAdapter = false;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_REFRESHRATESELECT, ( GAMESTATE * )&RefreshRateSelectState );
}

