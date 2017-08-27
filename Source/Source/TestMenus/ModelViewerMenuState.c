#include <TestMenuState.h>
#include <Memory.h>
#include <Peripheral.h>
#include <Log.h>
#include <Text.h>
#include <DebugAdapter.h>

#define DA_MDL_CONNECT		1
#define DA_MDL_DISCONNECT	2
#define DA_MDL_MODELINFO	3
#define DA_MDL_MODELDATA	4

#if defined ( DEBUG ) || defined ( DEVELOPMENT )

typedef struct
{
	GAMESTATE	Base;
	Uint8		*pDAData;
	Sint32		DADataSize;
	bool		DAConnected;
	char		Name[ 64 ];
}MODELVIEWER_GAMESTATE,*PMODELVIEWER_GAMESTATE;

static MODELVIEWER_GAMESTATE ModelViewerState;

static void HandleDebugAdapterData( int p_BytesToRead );

static int MDLV_Load( void *p_pArgs )
{
	ModelViewerState.pDAData = NULL;

	return 0;
}

static int MDLV_Initialise( void *p_pArgs )
{
	ModelViewerState.DAConnected = false;
	/* Allocate 64KiB for the debug adapter interface */
	ModelViewerState.DADataSize = 64 * 1024;
	ModelViewerState.pDAData = MEM_AllocateFromBlock(
		ModelViewerState.Base.pGameStateManager->MemoryBlocks.pSystemMemory,
		ModelViewerState.DADataSize, "Model viewer [DA]" );

	if( ModelViewerState.pDAData == NULL )
	{
		LOG_Debug( "MDLV_Initialise <ERROR> Failed to allocate the memory for "
			"the Debug Adapter" );

		return 1;
	}

	return 0;
}

static int MDLV_Update( void *p_pArgs )
{
	int DAData;
	Uint8 ChannelStatus;
	char DataTemp[ 64 ];
	int DataSize;

	if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
	{
		GSM_PopState( ModelViewerState.Base.pGameStateManager );
	}

	if( g_Peripherals[ 0 ].press & PDD_DGT_TY )
	{
		LOG_Debug( "TEST" );
	}

	/*if( DA_GetChannelStatus( 3, &ChannelStatus ) == 0 )
	{
		* Handle the debug adapter interface *
		DAData =  DA_GetData( ModelViewerState.pDAData,
			ModelViewerState.DADataSize, 3 );

		if( DAData > 0 )
		{
			LOG_Debug( "Got some data!" );
			HandleDebugAdapterData( DAData );
		}
	}*/

	/*if( ( DAData = DA_GetData( ModelViewerState.pDAData,
		ModelViewerState.DADataSize, 3, &DataSize ) ) == 0 )
	{
		HandleDebugAdapterData( DataSize );
	}*/

	//if( QUE_IsEmpty( &p_pGameStateManager->DebugAdapter.Queue ) == false )
	HandleDebugAdapterData( 0 );

	return 0;
}

static int MDLV_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;
	Uint8 ChannelStatus;
	Sint32 Channel = 0;
	char DataTemp[ 64 ];

	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		ModelViewerState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( pGlyphSet, "[B] back", &TextLength );
	TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
		"[B] back" );

	if( ModelViewerState.Base.pGameStateManager->DebugAdapter.Connected )
	{
		TextColour.dwPacked = 0xFF00FF00;

		TXT_RenderString( pGlyphSet, &TextColour, 32.0f, 32.0f,
			"Connected" );
	}
	else
	{
		TextColour.dwPacked = 0xFFFF0000;

		TXT_RenderString( pGlyphSet, &TextColour, 32.0f, 32.0f,
			"Disconnected" );
	}

	if( strlen( ModelViewerState.Name ) > 0 )
	{
		TextColour.dwPacked = 0xFFCCAA00;

		TXT_MeasureString( pGlyphSet, ModelViewerState.Name, &TextLength );
		TXT_RenderString( pGlyphSet, &TextColour,
			320.0f - TextLength * 0.5f,
			480.0f - ( float )pGlyphSet->LineHeight * 1.5f,
			ModelViewerState.Name );
	}

	return 0;
}

static int MDLV_Terminate( void *p_pArgs )
{
	if( ModelViewerState.pDAData != NULL )
	{
		MEM_FreeFromBlock( ModelViewerState.Base.pGameStateManager->
			MemoryBlocks.pSystemMemory, ModelViewerState.pDAData );

		ModelViewerState.pDAData = NULL;
	}

	return 0;
}

static int MDLV_Unload( void *p_pArgs )
{
	return 0;
}

static int MDLV_VSyncCallback( void *p_pArgs )
{
	return 0;
}

int MDLV_RegisterWithGameStateManager(
	PGAMESTATE_MANAGER p_pGameStateManager )
{
	ModelViewerState.Base.Load = &MDLV_Load;
	ModelViewerState.Base.Initialise = &MDLV_Initialise;
	ModelViewerState.Base.Update = &MDLV_Update;
	ModelViewerState.Base.Render = &MDLV_Render;
	ModelViewerState.Base.Terminate = &MDLV_Terminate;
	ModelViewerState.Base.Unload = &MDLV_Unload;
	ModelViewerState.Base.VSyncCallback = &MDLV_VSyncCallback;
	ModelViewerState.Base.pGameStateManager = p_pGameStateManager;
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	ModelViewerState.Base.VisibleToDebugAdapter  = true;
#endif /* DEBUG || DEVELOPMENT */

	return GSM_RegisterGameState( p_pGameStateManager, GAME_STATE_MODELVIEWER,
		( GAMESTATE * )&ModelViewerState );
}

static void HandleDebugAdapterData( int p_BytesToRead )
{
	/* Format:
	 * 8-bit ID (Connect/Disconnect/Model Info/Model Data)
	 * Payload
	 */
	/* First, get the ID */
	/*Uint8 ID;
	int BytesRead = 0;

	while( BytesRead < p_BytesToRead )
	{
		ID = ModelViewerState.pDAData[ BytesRead ];

		switch( ID )
		{
			case DA_MDL_CONNECT:
			{
				ModelViewerState.DAConnected = true;
				++BytesRead;

				break;
			}
			case DA_MDL_DISCONNECT:
			{
				ModelViewerState.DAConnected = false;
				++BytesRead;

				break;
			}
			case DA_MDL_MODELINFO:
			{
				++BytesRead;

				memcpy( ModelViewerState.Name,
					&ModelViewerState.pDAData[ BytesRead ], 32 );

				BytesRead += 32;

				break;
			}
			case DA_MDL_MODELDATA:
			{
				break;
			}
			default:
			{
			}
		}
	}*/
}

#endif /* DEBUG || DEVELOPMENT */

