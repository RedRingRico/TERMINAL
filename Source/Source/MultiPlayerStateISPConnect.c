#include <MultiPlayerState.h>
#include <GameState.h>
#include <NetworkCore.h>
#include <Peripheral.h>
#include <Log.h>
#include <MainMenuState.h>

typedef struct _tagMPISP_GAMESTATE
{
	GAMESTATE	Base;
	NET_STATUS	NetStatus;
	NET_STATUS	PreviousNetStatus;
	bool		UserDisconnect;
	bool		WaitForDisconnect;
	size_t		DotsToDraw;
	Uint32		ElapsedDotDrawTime;
}MPISP_GAMESTATE,*PMPISP_GAMESTATE;

static MPISP_GAMESTATE ISPConnectState;

static int MPISP_Load( void *p_pArgs )
{
	NETWORK_CONFIGURATION NetConfig;

	NetConfig.pMemoryBlock = 
		ISPConnectState.Base.pGameStateManager->MemoryBlocks.pSystemMemory;

	if( NET_Initialise( &NetConfig ) != 0 )
	{
		return 1;
	}

	LOG_Debug( "Load" );

	return 0;
}

static int MPISP_Initialise( void *p_pArgs )
{
	if( NET_ConnectToISP( ) != 0 )
	{
		GSM_ChangeState( ISPConnectState.Base.pGameStateManager,
			GAME_STATE_MAINMENU, NULL, NULL );
	}

	ISPConnectState.UserDisconnect = false;
	ISPConnectState.WaitForDisconnect = false;
	ISPConnectState.DotsToDraw = 0;
	ISPConnectState.ElapsedDotDrawTime = ISPConnectState.Base.ElapsedGameTime;
	ISPConnectState.PreviousNetStatus = NET_GetStatus( );

	return 0;
}

static int MPISP_Update( void *p_pArgs )
{
	if( ISPConnectState.Base.Paused == false )
	{
		Uint32 StartTime = syTmrGetCount( );
		ISPConnectState.NetStatus = NET_GetStatus( );

		if( ISPConnectState.UserDisconnect == true )
		{
			NET_DisconnectFromISP( );
			ISPConnectState.WaitForDisconnect = true;
			ISPConnectState.UserDisconnect = false;
		}

		switch( ISPConnectState.NetStatus )
		{
			case NET_STATUS_DISCONNECTED:
			case NET_STATUS_NODEVICE:
			{
				ISPConnectState.UserDisconnect = false;
				GSM_ChangeState( ISPConnectState.Base.pGameStateManager,
					GAME_STATE_MAINMENU, NULL, NULL );

				break;
			}
			case NET_STATUS_CONNECTED:
			{
				if( ISPConnectState.WaitForDisconnect == false )
				{
					GSM_PushState( ISPConnectState.Base.pGameStateManager,
						GAME_STATE_MULTIPLAYER_GAMELISTSERVER, NULL, NULL );

					/* When the next state is popped off, we should
					 * disconnect */
					ISPConnectState.UserDisconnect = true;
				}

				break;
			}
			default:
			{
				break;
			}
		}

		switch( ISPConnectState.NetStatus )
		{
			case NET_STATUS_NEGOTIATING:
			case NET_STATUS_PPP_POLL:
			case NET_STATUS_RESET:
			{
				Uint32 CurrentTime = syTmrGetCount( );

				if( syTmrCountToMicro( syTmrDiffCount(
					ISPConnectState.ElapsedDotDrawTime,
						CurrentTime ) ) >= 200000UL )
				{
					ISPConnectState.ElapsedDotDrawTime = CurrentTime;

					/* Reset the dots if the state has changed */
					if( ISPConnectState.PreviousNetStatus !=
						ISPConnectState.NetStatus )
					{
						ISPConnectState.PreviousNetStatus =
							ISPConnectState.NetStatus;
						ISPConnectState.DotsToDraw = 0;
					}
					else
					{
						++ISPConnectState.DotsToDraw;

						if( ISPConnectState.DotsToDraw > 5 )
						{
							ISPConnectState.DotsToDraw = 0;
						}
					}
				}
				break;
			}
			case NET_STATUS_CONNECTED:
			case NET_STATUS_DISCONNECTED:
			case NET_STATUS_NODEVICE:
			{
				ISPConnectState.DotsToDraw = 0;
				break;
			}
			default:
			{
				break;
			}
		}

		if( ISPConnectState.WaitForDisconnect == false )
		{
			if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
			{
				ISPConnectState.UserDisconnect = true;
			}
		}

		NET_Update( );
	}

	return 0;
}

static int MPISP_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;
	static bool ConnectionAlreadyAttempted = false;

	if( ISPConnectState.Base.Paused == false )
	{
		PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
			ISPConnectState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );
		char *pStopString = "[B] cancel";
		char InfoString[ 128 ] = { '\0' };
		char NetString[ 80 ];

		TextColour.dwPacked = 0xFFFFFFFF;

		REN_Clear( );

		switch( ISPConnectState.NetStatus )
		{
			case NET_STATUS_NEGOTIATING:
			{
				sprintf( InfoString, "NEGOTIATING" );
				break;
			}
			case NET_STATUS_PPP_POLL:
			{
				sprintf( InfoString, "POLLING PPP" );
				break;
			}
			case NET_STATUS_CONNECTED:
			{
				sprintf( InfoString, "CONNECTED" );
				pStopString = "[B] back";
				break;
			}
			case NET_STATUS_DISCONNECTED:
			{
				sprintf( InfoString, "DISCONNECTED" );
				pStopString = "[B] back";
				break;
			}
			case NET_STATUS_NODEVICE:
			{
				sprintf( InfoString, "DEVICE NOT FOUND" );
				pStopString = "[B] back";
				break;
			}
			case NET_STATUS_RESET:
			{
				sprintf( InfoString, "RESETTING" );
				break;
			}
			default:
			{
				break;
			}
		}

		if( ISPConnectState.WaitForDisconnect == false )
		{
			TXT_MeasureString( pGlyphSet, pStopString, &TextLength );
			TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f -
				TextLength, 480.0f - ( float )pGlyphSet->LineHeight * 4.0f,
				pStopString );
		}

#if defined ( DEBUG )
		sprintf( NetString, "Open interfaces: %d", NET_GetIfaceOpen( ) );
		TXT_RenderString( pGlyphSet, &TextColour, 0.0f, 0.0f, NetString );

		sprintf( NetString, "Open devices: %d", NET_GetDevOpen( ) );
		TXT_RenderString( pGlyphSet, &TextColour, 0.0f, 20.0f, NetString );
#endif /* DEBUG */

		if( ISPConnectState.DotsToDraw > 0 )
		{
			size_t Dot;

			strcat( InfoString, " " );

			for( Dot = 0; Dot < ISPConnectState.DotsToDraw; ++Dot )
			{
				strcat( InfoString, "." );
			}
		}

		TXT_RenderString( pGlyphSet, &TextColour, 320.0f, 240.0f,
			InfoString );

		REN_SwapBuffers( );
	}

	return 0;
}

static int MPISP_Terminate( void *p_pArgs )
{
	/* Wait for a successful disconnect */
	NET_Update( );

	NET_DisconnectFromISP( );

	ISPConnectState.NetStatus = NET_GetStatus( );

	while( ( ISPConnectState.NetStatus != NET_STATUS_DISCONNECTED ) &&
		( ISPConnectState.NetStatus != NET_STATUS_NODEVICE ) )
	{
		NET_Update( );
		ISPConnectState.NetStatus = NET_GetStatus( );
	}

	return 0;
}

static int MPISP_Unload( void *p_pArgs )
{
	LOG_Debug( "MPISP_Unload <INFO> Terminating the network\n" );

	NET_Terminate( );

	return 0;
}

int MP_RegisterISPConnectWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	ISPConnectState.Base.Load = &MPISP_Load;
	ISPConnectState.Base.Initialise = &MPISP_Initialise;
	ISPConnectState.Base.Update = &MPISP_Update;
	ISPConnectState.Base.Render = &MPISP_Render;
	ISPConnectState.Base.Terminate = &MPISP_Terminate;
	ISPConnectState.Base.Unload = &MPISP_Unload;
	ISPConnectState.Base.pGameStateManager = p_pGameStateManager;

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MULTIPLAYER_ISPCONNECT, ( GAMESTATE * )&ISPConnectState );
}

