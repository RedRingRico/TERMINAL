#include <MemoryUnitSelectState.h>
#include <AspectRatioSelectState.h>
#include <RefreshRateSelectState.h>
#include <MainMenuState.h>
#include <Peripheral.h>
#include <GameState.h>
#include <StorageUnit.h>
#include <Log.h>

typedef struct _tagMEMORYUNITSELECT_GAMESTATE
{
	GAMESTATE	Base;
	bool		ConfigurationFound;
	Uint8		ConfigurationDrive;
	Uint8		ConfigurationDriveLast;
}MEMORYUNITSELECT_GAMESTATE, *PMEMORYUNITSELECT_GAMESTATE;

static size_t g_StorageUnitCount, g_StorageUnitCountLast;
static PSTORAGEUNIT_INFO g_pStorageUnitsAvailable;

static MEMORYUNITSELECT_GAMESTATE MemoryUnitSelectState;

static int MUSS_Load( void *p_pArgs )
{
	MemoryUnitSelectState.ConfigurationFound = false;

	return 0;
}

static int MUSS_Initialise( void *p_pArgs )
{
	/* If the game configuration file is present, skip the whole thing 
	 * (for now, just look for a file that doesn't exist to force this)
	 */
	MemoryUnitSelectState.ConfigurationFound = SU_FindFileAcrossDrives(
		"ERMINAL.SYS", true, &MemoryUnitSelectState.ConfigurationDrive );

	g_pStorageUnitsAvailable = MEM_AllocateFromBlock(
		MemoryUnitSelectState.Base.pGameStateManager->
			MemoryBlocks.pSystemMemory,
		sizeof( STORAGEUNIT_INFO ) * SU_MAX_DRIVES, "Storage units" );
	memset( g_pStorageUnitsAvailable, 0,
		sizeof( STORAGEUNIT_INFO ) * SU_MAX_DRIVES );

	memset( g_pStorageUnitsAvailable, 0,
		sizeof( STORAGEUNIT_INFO ) * SU_MAX_DRIVES );
	SU_GetMountedStorageUnits( g_pStorageUnitsAvailable,
		&g_StorageUnitCount );
	g_StorageUnitCountLast = g_StorageUnitCount;

	return 0;
}

static int MUSS_Update( void *p_pArgs )
{
	static Uint8 SelectedDrive = 0;
	/* For now, just quit */
	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		SU_SaveFile( 0, "TERMINAL.SYS", NULL, 0, "Game Options",
			"[TERMINAL]" );
		GSM_Quit( MemoryUnitSelectState.Base.pGameStateManager );
	}

	if( g_StorageUnitCount )
	{
		if( g_Peripherals[ 0 ].press & PDD_DGT_KU )
		{
			if( SelectedDrive == 0 )
			{
				SelectedDrive = g_StorageUnitCount - 1;
			}
			else
			{
				--SelectedDrive;
			}

			MemoryUnitSelectState.ConfigurationDrive = SU_FlagToDrive(
				g_pStorageUnitsAvailable[ SelectedDrive ].Flags );
		}

		if( g_Peripherals[ 0 ].press & PDD_DGT_KD )
		{
			if( SelectedDrive == g_StorageUnitCount - 1 )
			{
				SelectedDrive = 0;
			}
			else
			{
				++SelectedDrive;
			}

			MemoryUnitSelectState.ConfigurationDrive = SU_FlagToDrive(
				g_pStorageUnitsAvailable[ SelectedDrive ].Flags );
		}
	}

	if( MemoryUnitSelectState.ConfigurationFound == true )
	{
		MAINMENU MainMenuArgs;

		/* Load the configuration */

		/* Done, onto the main menu */
		GSM_ChangeState( MemoryUnitSelectState.Base.pGameStateManager,
			GAME_STATE_MAINMENU, NULL, NULL );
	}
	else
	{
		/* Present all inserted memory units to the player */
		memset( g_pStorageUnitsAvailable, 0,
			sizeof( STORAGEUNIT_INFO ) * SU_MAX_DRIVES );
		g_StorageUnitCountLast = g_StorageUnitCount;
		SU_GetMountedStorageUnits( g_pStorageUnitsAvailable,
			&g_StorageUnitCount );

		if( g_StorageUnitCountLast != g_StorageUnitCount )
		{
			if( SelectedDrive > g_StorageUnitCount - 1 )
			{
				SelectedDrive = g_StorageUnitCount - 1;
			}

			MemoryUnitSelectState.ConfigurationDrive = SU_FlagToDrive(
				g_pStorageUnitsAvailable[ SelectedDrive ].Flags );
		}

		/*switch( syCblCheck( ) )
		{
			case SYE_CBL_PAL:
			{
				GSM_ChangeState( MemoryUnitSelectState.Base.pGameStateManager,
					GAME_STATE_REFRESHRATESELECT, NULL, NULL );
				break;
			}
			default:
			{
				GSM_ChangeState( MemoryUnitSelectState.Base.pGameStateManager,
					GAME_STATE_ASPECTRATIOSELECT, NULL, NULL );
			}
		}*/
	}

	return 0;
}

static int MUSS_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	char PrintBuffer[ 40 ];
	float TextLength;
	Uint8 Drive = 0, DriveNumber = 0;
	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		MemoryUnitSelectState.Base.pGameStateManager,
		GSM_GLYPH_SET_GUI_1 );

	TextColour.byte.bRed = 83;
	TextColour.byte.bGreen = 254;
	TextColour.byte.bBlue = 255;
	TextColour.byte.bAlpha = 230;

	if( g_StorageUnitCount )
	{
		for( ; Drive < SU_MAX_DRIVES; ++Drive )
		{
			if( MemoryUnitSelectState.ConfigurationDrive == Drive )
			{
				TextColour.byte.bRed = 83;
				TextColour.byte.bGreen = 254;
				TextColour.byte.bBlue = 255;
				TextColour.byte.bAlpha = 255;

				TXT_MeasureString( pGlyphSet, "$ ", &TextLength );

				TXT_RenderString( pGlyphSet, &TextColour, 320.0f - TextLength,
					40.0f + ( float )pGlyphSet->LineHeight * ( float )Drive,
					"$ " );
			}

			if( SU_FlagToDrive(
				g_pStorageUnitsAvailable[ DriveNumber ].Flags ) == Drive )
			{
				TextColour.byte.bRed = 83;
				TextColour.byte.bGreen = 254;
				TextColour.byte.bBlue = 255;
				TextColour.byte.bAlpha = 220;

				sprintf( PrintBuffer, "VMU inserted at slot %d", Drive );

				TXT_RenderString( pGlyphSet, &TextColour, 320.0f,
					40.0f + ( float )pGlyphSet->LineHeight * ( float )Drive,
					PrintBuffer );

				++DriveNumber;
			}
			else
			{
				TextColour.byte.bRed = 83;
				TextColour.byte.bGreen = 254;
				TextColour.byte.bBlue = 255;
				TextColour.byte.bAlpha = 140;

				sprintf( PrintBuffer, "VMU not present at slot %d", Drive );

				TXT_RenderString( pGlyphSet, &TextColour, 320.0f,
					40.0f + ( float )pGlyphSet->LineHeight * ( float )Drive,
					PrintBuffer );
			}
		}

		TextColour.byte.bRed = 83;
		TextColour.byte.bGreen = 254;
		TextColour.byte.bBlue = 255;
		TextColour.byte.bAlpha = 140;
		sprintf( PrintBuffer, "%d Visual Memory Unit%s detected",
			g_StorageUnitCount, ( g_StorageUnitCount == 1 ? "" : "s" ) );

		TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour, 320.0f - TextLength * 0.5f,
			440.0f - ( float )pGlyphSet->LineHeight * 0.5f, PrintBuffer );
	}
	else
	{
		sprintf( PrintBuffer, "No Visual Memory Units detected!" );
		TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour, 320.0f - TextLength * 0.5f,
			240.0f - ( float )pGlyphSet->LineHeight * 0.5f, PrintBuffer );
	}

	return 0;
}

static int MUSS_Terminate( void *p_pArgs )
{
	return 0;
}

static int MUSS_Unload( void *p_pArgs )
{
	 MEM_FreeFromBlock( MemoryUnitSelectState.Base.pGameStateManager->
	 	MemoryBlocks.pSystemMemory, g_pStorageUnitsAvailable );
	 g_pStorageUnitsAvailable = NULL;

	return 0;
}

static int MUSS_VSyncCallback( void *p_pArgs )
{
	return 0;
}

int MUSS_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	MemoryUnitSelectState.Base.Load = &MUSS_Load;
	MemoryUnitSelectState.Base.Initialise = &MUSS_Initialise;
	MemoryUnitSelectState.Base.Update = &MUSS_Update;
	MemoryUnitSelectState.Base.Render = &MUSS_Render;
	MemoryUnitSelectState.Base.Terminate = &MUSS_Terminate;
	MemoryUnitSelectState.Base.Unload = &MUSS_Unload;
	MemoryUnitSelectState.Base.VSyncCallback = &MUSS_VSyncCallback;
	MemoryUnitSelectState.Base.pGameStateManager = p_pGameStateManager;

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	MemoryUnitSelectState.Base.VisibleToDebugAdapter = false;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager,
		GAME_STATE_MEMORYUNITSELECT, ( GAMESTATE * )&MemoryUnitSelectState );
}
