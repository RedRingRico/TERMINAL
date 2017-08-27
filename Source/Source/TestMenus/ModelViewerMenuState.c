#include <TestMenuState.h>
#include <Memory.h>
#include <Peripheral.h>
#include <Log.h>
#include <Text.h>
#include <DebugAdapter.h>
#include <Model.h>

#define DA_MDL_MODELINFO	100
#define DA_MDL_MODELDATA	101

#if defined ( DEBUG ) || defined ( DEVELOPMENT )

typedef struct
{
	GAMESTATE	Base;
	Uint32		ModelSize;
	Uint32		ModelSizePopulated;
	Uint8		*pDAData;
	Uint8		*pModelData;
	Sint32		DADataSize;
	bool		DAConnected;
	MODEL		Model;
}MODELVIEWER_GAMESTATE,*PMODELVIEWER_GAMESTATE;

typedef struct
{
	Uint16	Sequence;
	Uint16	Size;
}MODEL_CHUNK,*PMODEL_CHUNK;

static MODELVIEWER_GAMESTATE ModelViewerState;

static int MDLV_Load( void *p_pArgs )
{
	ModelViewerState.pDAData = NULL;

	memset( ModelViewerState.Model.Name, '\0',
		sizeof( ModelViewerState.Model.Name ) );
	ModelViewerState.Model.PolygonCount = 0;
	ModelViewerState.Model.MeshCount = 0;
	ModelViewerState.Model.pMeshes = NULL;
	ModelViewerState.Model.pMemoryBlock = NULL;

	ModelViewerState.pModelData = NULL;

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
	PQUEUE pDAQueue =
		&ModelViewerState.Base.pGameStateManager->DebugAdapter.Queue;

	if( g_Peripherals[ 0 ].press & PDD_DGT_TB )
	{
		GSM_PopState( ModelViewerState.Base.pGameStateManager );
	}

	if( QUE_GetCount(
		&ModelViewerState.Base.pGameStateManager->DebugAdapter.Queue ) != 0 )
	{
		PDEBUG_ADAPTER_MESSAGE pMessage;

		pMessage = QUE_GetFront(
			&ModelViewerState.Base.pGameStateManager->DebugAdapter.Queue );

		switch( pMessage->ID )
		{
			case DA_MDL_MODELINFO:
			{
				MDL_DeleteModel( &ModelViewerState.Model );

				if( ModelViewerState.pModelData != NULL )
				{
					MEM_FreeFromBlock( ModelViewerState.Base.
						pGameStateManager->MemoryBlocks.pGraphicsMemory,
						ModelViewerState.pModelData );
					ModelViewerState.pModelData = NULL;
				}

				ModelViewerState.ModelSize = ( Uint32 )( *pMessage->Data );

				ModelViewerState.pModelData = MEM_AllocateFromBlock(
					ModelViewerState.Base.pGameStateManager->MemoryBlocks.
					pGraphicsMemory, ModelViewerState.ModelSize,
					"Model Viewer: Temporary mesh data" );

				QUE_Dequeue( pDAQueue, NULL );

				break;
			}
			case DA_MDL_MODELDATA:
			{
				MODEL_CHUNK ModelChunk;

				memcpy( &ModelChunk, pMessage->Data, sizeof( ModelChunk ) );

				memcpy( &ModelViewerState.pModelData[
					ModelChunk.Sequence * MAX_DEBUG_ADAPTER_MESSAGE_SIZE ],
					&pMessage->Data[ sizeof( ModelChunk ) ], ModelChunk.Size );

				ModelViewerState.ModelSizePopulated += ModelChunk.Size;

				if( ModelViewerState.ModelSizePopulated ==
					ModelViewerState.ModelSize )
				{
					MDL_LoadModelFromMemory( &ModelViewerState.Model,
						ModelViewerState.pModelData,
						ModelViewerState.ModelSize,
						ModelViewerState.Base.pGameStateManager->MemoryBlocks.
						pGraphicsMemory );
				}

				QUE_Dequeue( pDAQueue, NULL );

				break;
			}
		}
	}

	return 0;
}

static int MDLV_Render( void *p_pArgs )
{
	KMPACKEDARGB TextColour;
	float TextLength;
	Uint8 ChannelStatus;
	Sint32 Channel = 0;
	char PrintString[ 64 ];

	PGLYPHSET pGlyphSet = GSM_GetGlyphSet(
		ModelViewerState.Base.pGameStateManager, GSM_GLYPH_SET_GUI_1 );

	TextColour.dwPacked = 0xFFFFFFFF;

	TXT_MeasureString( pGlyphSet, "[B] back", &TextLength );
	TXT_RenderString( pGlyphSet, &TextColour, 640.0f - 64.0f - TextLength,
		480.0f - ( 32.0f + ( float )pGlyphSet->LineHeight ),
		"[B] back" );

	sprintf( PrintString, "Mesh count: %d", ModelViewerState.Model.MeshCount );

	TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
		480.0f - ( float )pGlyphSet->LineHeight * 1.5f, PrintString );

	if( strlen( ModelViewerState.Model.Name ) > 0 )
	{
		TextColour.dwPacked = 0xFFCCAA00;

		TXT_MeasureString( pGlyphSet, ModelViewerState.Model.Name,
			&TextLength );
		TXT_RenderString( pGlyphSet, &TextColour,
			320.0f - TextLength * 0.5f,
			480.0f - ( float )pGlyphSet->LineHeight * 1.5f,
			ModelViewerState.Model.Name );
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

#endif /* DEBUG || DEVELOPMENT */

