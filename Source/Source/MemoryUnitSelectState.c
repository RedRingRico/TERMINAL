#include <MemoryUnitSelectState.h>
#include <AspectRatioSelectState.h>
#include <RefreshRateSelectState.h>
#include <MainMenuState.h>
#include <Peripheral.h>
#include <GameState.h>
#include <StorageUnit.h>
#include <Log.h>

/*
From Quake III Arena's VM loop:

Stage 1: Looking for VM



Stage 2: VMU matching criteria
No VMU matching the criteria: There is no VM connercted with at least n free blocks
                              n free blocks are requied for saving

                              <timeout>

                              Press any key (surely this should be button?)


Found a VMU: VM found

             n free blocks are required for saving (If no game file found)
			 Game file found

             n blocks free

             Press any key (surely this should be button?)

*/
#define MUS_DISCOVERING_VMU					1
#define MUS_NO_VMUS_FOUND					2
#define MUS_VMU_GAME_SAVE_FOUND				3
#define MUS_VMU_SPACE_AVAILABLE				4

typedef struct _tagMEMORYUNITSELECT_GAMESTATE
{
	GAMESTATE	Base;
	bool		ConfigurationFound;
	Uint8		ConfigurationDrive;
	Uint8		ConfigurationDriveLast;
	Uint32		TimeOut;
	Uint32		VMUState;
}MEMORYUNITSELECT_GAMESTATE, *PMEMORYUNITSELECT_GAMESTATE;

static size_t g_StorageUnitCount, g_StorageUnitCountLast;
static PSTORAGEUNIT_INFO g_pStorageUnitsAvailable;

static MEMORYUNITSELECT_GAMESTATE MemoryUnitSelectState;

static Sint32 MUSS_Load( void *p_pArgs )
{
	MemoryUnitSelectState.ConfigurationFound = false;

	return 0;
}

static Sint32 MUSS_Initialise( void *p_pArgs )
{
	/* If the game configuration file is present, skip the whole thing 
	 * (for now, just look for a file that doesn't exist to force this)
	 */
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
	MemoryUnitSelectState.VMUState = MUS_DISCOVERING_VMU;

	return 0;
}

static Sint32 MUSS_Update( void *p_pArgs )
{
	static Uint8 SelectedDrive = 0;

	switch( MemoryUnitSelectState.VMUState )
	{
		case MUS_DISCOVERING_VMU:
		{
			if( g_StorageUnitCount )
			{
				/* First, attempt to find the TERMINAL.SYS file */
				MemoryUnitSelectState.ConfigurationFound =
					SU_FindFileAcrossDrives( "TERMINAL.SYS", false,
						&MemoryUnitSelectState.ConfigurationDrive );

				MemoryUnitSelectState.VMUState = MUS_NO_VMUS_FOUND;

				/* Failing that, look for any VMUs with enough free space */
				if( MemoryUnitSelectState.ConfigurationFound == false )
				{
					Uint8 Drives;

					if( SU_GetDrivesWithFreeBlocks( 1, true, &Drives ) ==
						SU_OK )
					{
						MemoryUnitSelectState.VMUState =
							MUS_VMU_SPACE_AVAILABLE;
					}
				}
				else
				{
					MemoryUnitSelectState.VMUState = MUS_VMU_GAME_SAVE_FOUND;
				}
			}
			else
			{
				MemoryUnitSelectState.VMUState = MUS_NO_VMUS_FOUND;
			}
			break;
		}
		case MUS_NO_VMUS_FOUND:
		{
			SU_GetMountedStorageUnits( g_pStorageUnitsAvailable,
				&g_StorageUnitCount );

			if( g_StorageUnitCount )
			{
				MemoryUnitSelectState.VMUState = MUS_DISCOVERING_VMU;
			}

			break;
		}
		case MUS_VMU_SPACE_AVAILABLE:
		{
			/* For now, just quit */
			if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
			{
				if( g_pStorageUnitsAvailable[ SelectedDrive ].Flags &
					SUI_FORMATTED )
				{
					char TestData[ 64 ];
					char TestDataRead[ 64 ];
					Uint32 DataOffset;
					Sint32 FileSize;

					strcpy( TestData, "[TERMINAL] Game save data" );
					
					SU_SaveFile( MemoryUnitSelectState.ConfigurationDrive,
						"TERMINAL.SYS", TestData, sizeof( TestData ),
						"Game Settings", "[TERMINAL]" );

					FileSize = SU_GetFileSize(
						MemoryUnitSelectState.ConfigurationDrive,
						"TERMINAL.SYS", &DataOffset, SU_FILETYPE_NORMAL );

					LOG_Debug( "Got file size %d", FileSize );
					LOG_Debug( "Data offset: %d", DataOffset );

					SU_LoadFile( MemoryUnitSelectState.ConfigurationDrive,
						"TERMINAL.SYS", TestDataRead, FileSize, DataOffset );

					LOG_Debug( "File contents: %s", TestDataRead );

					GSM_Quit( MemoryUnitSelectState.Base.pGameStateManager );
				}
			}
			break;
		}
		case MUS_VMU_GAME_SAVE_FOUND:
		{
			MAINMENU MainMenuArgs;

			/* Load the configuration */

			/* Done, onto the main menu */
			GSM_ChangeState( MemoryUnitSelectState.Base.pGameStateManager,
				GAME_STATE_MAINMENU, NULL, NULL );
			break;
		}
		default:
		{
			if( MemoryUnitSelectState.Base.pGameStateManager->pTopGameState->
					ElapsedGameTime >= MemoryUnitSelectState.TimeOut )
			{
				GSM_Quit( MemoryUnitSelectState.Base.pGameStateManager );
			}
		}
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

static Sint32 MUSS_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	char PrintBuffer[ 80 ];
	float TextLength;
	Uint8 Drive = 0, DriveNumber = 0;
	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		MemoryUnitSelectState.Base.pGameStateManager,
		GSM_GLYPH_SET_GUI_1 );

	TextColour.byte.bRed = 83;
	TextColour.byte.bGreen = 254;
	TextColour.byte.bBlue = 255;
	TextColour.byte.bAlpha = 230;

	switch( MemoryUnitSelectState.VMUState )
	{
		case MUS_DISCOVERING_VMU:
		{
			sprintf( PrintBuffer, "Discovering VMUs...",
				MemoryUnitSelectState.Base.pGameStateManager->pTopGameState->
					ElapsedGameTime );

			TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
			TXT_RenderString( pGlyphSet, &TextColour, 320.0f - TextLength * 0.5f,
				240.0f - ( float )pGlyphSet->LineHeight * 0.5f, PrintBuffer );

			break;
		}
		case MUS_NO_VMUS_FOUND:
		{
		/*
		From Quake III Arena's VM loop:

		Stage 1: Looking for VM



		Stage 2: VMU matching criteria
		No VMU matching the criteria: There is no VM connercted with at least n free blocks
									  n free blocks are requied for saving

									  <timeout>

									  Press any key (surely this should be button?)


		Found a VMU: VM found

					 n free blocks are required for saving (If no game file found)
					 Game file found

					 n blocks free

					 Press any key (surely this should be button?)

		*/

			sprintf( PrintBuffer, "No VMU found %d",
				MemoryUnitSelectState.Base.pGameStateManager->pTopGameState->
					ElapsedGameTime );

			TXT_MeasureString( pGlyphSet, PrintBuffer, &TextLength );
			TXT_RenderString( pGlyphSet, &TextColour, 320.0f - TextLength * 0.5f,
				440.0f, PrintBuffer );
			break;
		}
		default:
		{
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

						TXT_RenderString( pGlyphSet, &TextColour, 64.0f - TextLength,
							40.0f + ( float )pGlyphSet->LineHeight * ( float )Drive,
							"$ " );
					}

					if( SU_FlagToDrive(
						g_pStorageUnitsAvailable[ DriveNumber ].Flags ) == Drive )
					{
						if( g_pStorageUnitsAvailable[ DriveNumber ].Flags &
							SUI_FORMATTED )
						{
							TextColour.byte.bRed = 83;
							TextColour.byte.bGreen = 254;
							TextColour.byte.bBlue = 255;
							TextColour.byte.bAlpha = 220;

							sprintf( PrintBuffer, "VMU inserted at slot %d", Drive );
						}
						else
						{
							TextColour.byte.bRed = 255;
							TextColour.byte.bGreen = 129;
							TextColour.byte.bBlue = 38;
							TextColour.byte.bAlpha = 240;

							sprintf( PrintBuffer, "Unformatted VMU at slot %d",
								Drive );
						}

						TXT_RenderString( pGlyphSet, &TextColour, 64.0f,
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

						TXT_RenderString( pGlyphSet, &TextColour, 64.0f,
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
		}
	}

	return 0;
}

static Sint32 MUSS_Terminate( void *p_pArgs )
{
	return 0;
}

static Sint32 MUSS_Unload( void *p_pArgs )
{
	 MEM_FreeFromBlock( MemoryUnitSelectState.Base.pGameStateManager->
	 	MemoryBlocks.pSystemMemory, g_pStorageUnitsAvailable );
	 g_pStorageUnitsAvailable = NULL;

	return 0;
}

static Sint32 MUSS_VSyncCallback( void *p_pArgs )
{
	return 0;
}

Sint32 MUSS_RegisterWithGameStateManager(
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
