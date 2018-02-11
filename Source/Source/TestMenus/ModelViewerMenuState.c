#include <TestMenuState.h>
#include <Memory.h>
#include <Peripheral.h>
#include <Log.h>
#include <Text.h>
#include <DebugAdapter.h>
#include <Model.h>
#include <Matrix4x4.h>

#define DA_MDL_MODELINFO	100
#define DA_MDL_MODELDATA	101

#if defined ( DEBUG ) || defined ( DEVELOPMENT )

typedef struct
{
	GAMESTATE	Base;
	Uint32		ModelSize;
	Uint32		ModelSizePopulated;
	Uint8		*pModelData;
	bool		RenderModel;
	MATRIX4X4	View;
	MATRIX4X4	Projection;
	MATRIX4X4	Screen;
	MATRIX4X4	World;
	VECTOR3		LightWorldPos;
	VECTOR3		LightPosition;
	VECTOR3		ModelPosition;
	float		ModelRotation;
	CAMERA		Camera;
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
	memset( ModelViewerState.Model.Name, '\0',
		sizeof( ModelViewerState.Model.Name ) );
	ModelViewerState.Model.PolygonCount = 0;
	ModelViewerState.Model.MeshCount = 0;
	ModelViewerState.Model.pMeshes = NULL;
	ModelViewerState.Model.pMemoryBlock = NULL;

	ModelViewerState.ModelPosition.X = 0.0f;
	ModelViewerState.ModelPosition.Y = 0.0f;
	ModelViewerState.ModelPosition.Z = 0.0f;

	ModelViewerState.pModelData = NULL;
	ModelViewerState.RenderModel = false;

	ModelViewerState.LightPosition.X = 0.0f;
	ModelViewerState.LightPosition.Y = 1.0f;
	ModelViewerState.LightPosition.Z = 0.0f;

	ModelViewerState.Camera.Position.X = 0.0f;
	ModelViewerState.Camera.Position.Y = 2.0f;
	ModelViewerState.Camera.Position.Z = -8.0f;

	ModelViewerState.Camera.LookAt.X = 0.0f;
	ModelViewerState.Camera.LookAt.Y = 0.0f;
	ModelViewerState.Camera.LookAt.Z = 0.0f;

	ModelViewerState.Camera.WorldUp.X = 0.0f;
	ModelViewerState.Camera.WorldUp.Y = 1.0f;
	ModelViewerState.Camera.WorldUp.Z = 0.0f;

	ModelViewerState.Camera.GateWidth = 640.0f;
	ModelViewerState.Camera.GateHeight = 480.0f;

	ModelViewerState.Camera.FieldOfView = ( 3.141592654f / 4.0f );
	ModelViewerState.Camera.NearPlane = 0.001f; /* 1cm */
	ModelViewerState.Camera.FarPlane = 10000.0f; /* 10km (too much?) */

	ModelViewerState.Camera.AspectRatio =
		ModelViewerState.Base.pGameStateManager->GameOptions.AspectRatio;

	ModelViewerState.ModelRotation = 0.0f;

	CAM_CalculateProjectionMatrix( &ModelViewerState.Projection,
		&ModelViewerState.Camera );
	CAM_CalculateScreenMatrix( &ModelViewerState.Screen,
		&ModelViewerState.Camera );

	return 0;
}

static int MDLV_Initialise( void *p_pArgs )
{
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

				memcpy( &ModelViewerState.ModelSize, pMessage->Data,
					sizeof( Uint32 ) );

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
					ModelChunk.Sequence *( MAX_DEBUG_ADAPTER_MESSAGE_SIZE -
						sizeof( MODEL_CHUNK ) ) ],
					&pMessage->Data[ sizeof( ModelChunk ) ], ModelChunk.Size );

				ModelViewerState.ModelSizePopulated += ModelChunk.Size;

				if( ModelViewerState.ModelSizePopulated ==
					ModelViewerState.ModelSize )
				{
					if( MDL_LoadModelFromMemory( &ModelViewerState.Model,
						ModelViewerState.pModelData,
						ModelViewerState.ModelSize,
						ModelViewerState.Base.pGameStateManager->MemoryBlocks.
						pGraphicsMemory ) == 0 )
					{
						ModelViewerState.RenderModel = true;

						MEM_FreeFromBlock( ModelViewerState.Base.
							pGameStateManager->MemoryBlocks.pGraphicsMemory,
							ModelViewerState.pModelData );
						ModelViewerState.pModelData = NULL;
					}
				}

				QUE_Dequeue( pDAQueue, NULL );

				break;
			}
		}
	}

	if( ModelViewerState.RenderModel )
	{
		MATRIX4X4 CameraMatrix;
		VECTOR3 CameraDefaultLookAt = { 0.0f, 1.7f, 1.0f };
		VECTOR3 CameraDefaultPosition = { 0.0f, 2.0f, -8.0f };
		VECTOR3 StickMove = { 0.0f, 0.0f, 0.0f };
		VECTOR3 Acceleration = { 0.0f, 0.0f, 0.0f };
		static float StickRotate = 0.0f;
		static VECTOR3 CameraMove = { 0.0f, 0.0f, 0.0f };

		if( g_Peripherals[ 0 ].x1 > 0 )
		{
			StickMove.X = ( float )g_Peripherals[ 0 ].x1 / 127.0f;
		}
		if( g_Peripherals[ 0 ].x1 < 0 )
		{
			StickMove.X = ( float )g_Peripherals[ 0 ].x1 / 128.0f;
		}

		if( g_Peripherals[ 0 ].y1 > 0 )
		{
			StickMove.Z = -( float )g_Peripherals[ 0 ].y1 / 127.0f;
		}
		if( g_Peripherals[ 0 ].y1 < 0 )
		{
			StickMove.Z = -( float )g_Peripherals[ 0 ].y1 / 128.0f;
		}

		if( ARI_IsZero( StickMove.X ) == false ||
			ARI_IsZero( StickMove.Z ) == false )
		{
			VEC3_Normalise( &StickMove );

			StickRotate = atan2f( StickMove.X, StickMove.Z );
		}

		if( g_Peripherals[ 0 ].l > 0 )
		{
			ModelViewerState.ModelRotation +=
				( ( float )g_Peripherals[ 0 ].l / 255.0f ) * 0.2f;
		}

		if( g_Peripherals[ 0 ].r > 0 )
		{
			ModelViewerState.ModelRotation -=
				( ( float )g_Peripherals[ 0 ].r / 255.0f ) * 0.2f;
		}

		MAT44_SetIdentity( &CameraMatrix );

		MAT44_TransformVertices( &Acceleration, &StickMove, 1,
			sizeof( VECTOR3 ), sizeof( VECTOR3 ), &CameraMatrix );

		VEC3_MultiplyF( &Acceleration, &Acceleration, 0.3f );

		CameraMove.X += Acceleration.X;
		CameraMove.Z += Acceleration.Z;

		MAT44_Translate( &CameraMatrix, &CameraMove );

		MAT44_TransformVertices( &ModelViewerState.Camera.Position,
			&CameraDefaultPosition, 1, sizeof( VECTOR3 ), sizeof( VECTOR3 ),
			&CameraMatrix );

		MAT44_TransformVertices( &ModelViewerState.Camera.LookAt,
			&CameraDefaultLookAt, 1, sizeof( VECTOR3 ), sizeof( VECTOR3 ),
			&CameraMatrix );
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

	if( ModelViewerState.RenderModel )
	{
		VECTOR3 RotateAxis = { 0.0f, 1.0f, 0.0f };
		MAT44_SetIdentity( &ModelViewerState.World );
		CAM_CalculateViewMatrix( &ModelViewerState.View,
			&ModelViewerState.Camera );

		MDL_CalculateLighting( &ModelViewerState.Model,
			&ModelViewerState.World, &ModelViewerState.LightPosition );

		MAT44_RotateAxisAngle( &ModelViewerState.World, &RotateAxis,
			ModelViewerState.ModelRotation );
		MAT44_Translate( &ModelViewerState.World,
			&ModelViewerState.ModelPosition );

		MDL_RenderModel( &ModelViewerState.Model,
			ModelViewerState.Base.pGameStateManager->pRenderer,
			&ModelViewerState.World, &ModelViewerState.View,
			&ModelViewerState.Projection, &ModelViewerState.Screen );

		sprintf( PrintString, "Camera Position: <%f %f %f>",
			ModelViewerState.Camera.Position.X,
			ModelViewerState.Camera.Position.Y,
			ModelViewerState.Camera.Position.Z );

		TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
			( float )pGlyphSet->LineHeight * 2.5f, PrintString );

		sprintf( PrintString, "Camera Look At: <%f %f %f>",
			ModelViewerState.Camera.LookAt.X,
			ModelViewerState.Camera.LookAt.Y,
			ModelViewerState.Camera.LookAt.Z );

		TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
			( float )pGlyphSet->LineHeight * 3.5f, PrintString );

		sprintf( PrintString, "Visible polygons: %d",
			ModelViewerState.Base.pGameStateManager->pRenderer->
				VisiblePolygons );

		TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
			448.0f - ( float )pGlyphSet->LineHeight * 3.5f, PrintString );

		sprintf( PrintString, "Culled polygons: %d",
			ModelViewerState.Base.pGameStateManager->pRenderer->
				CulledPolygons );

		TXT_RenderString( pGlyphSet, &TextColour, 20.0f,
			448.0f - ( float )pGlyphSet->LineHeight * 2.5f, PrintString );
	}

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

	return 0;
}

static int MDLV_Unload( void *p_pArgs )
{
	MDL_DeleteModel( &ModelViewerState.Model );

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

