#include <Model.h>
#include <FileSystem.h>
#include <Log.h>

typedef struct _tagCHUNK
{
	Uint32 ID;
	Uint32 Size;
}CHUNK,*PCHUNK;

static int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh );
static KMSTRIPHEAD	MDL_ModelStripHead;

int MDL_Initialise( void )
{
	KMPACKEDARGB BaseColour;
	KMSTRIPCONTEXT ModelContext;
	TEXTURE ModelTexture;

	TEX_LoadTexture( &ModelTexture, "/MODELS/CUBE.PVR" );

	memset( &ModelContext, 0, sizeof( KMSTRIPCONTEXT ) );

	ModelContext.nSize = sizeof( ModelContext );
	ModelContext.StripControl.nListType = KM_TRANS_POLYGON;
	ModelContext.StripControl.nUserClipMode = KM_USERCLIP_DISABLE;
	ModelContext.StripControl.nShadowMode = KM_NORMAL_POLYGON;
	ModelContext.StripControl.bOffset = KM_FALSE;
	ModelContext.StripControl.bGouraud = KM_TRUE;
	ModelContext.ObjectControl.nDepthCompare = KM_ALWAYS;
	ModelContext.ObjectControl.nCullingMode = KM_NOCULLING;
	ModelContext.ObjectControl.bZWriteDisable = KM_FALSE;
	ModelContext.ObjectControl.bDCalcControl = KM_FALSE;
	BaseColour.dwPacked = 0xFFFFFFFF;
	ModelContext.type.splite.Base = BaseColour;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nSRCBlendingMode =
		KM_SRCALPHA;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nDSTBlendingMode =
		KM_INVSRCALPHA;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bSRCSelect = KM_FALSE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bDSTSelect = KM_FALSE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nFogMode = KM_NOFOG;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bColorClamp = KM_FALSE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bUseAlpha = KM_FALSE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bIgnoreTextureAlpha =
		KM_TRUE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nFlipUV = KM_NOFLIP;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nClampUV = KM_CLAMP_UV;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nFilterMode = KM_BILINEAR;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].bSuperSampleMode = KM_FALSE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].dwMipmapAdjust = 
		KM_MIPMAP_D_ADJUST_1_00;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].nTextureShadingMode =
		KM_MODULATE;
	ModelContext.ImageControl[ KM_IMAGE_PARAM1 ].pTextureSurfaceDesc =
		&ModelTexture.SurfaceDescription;

	kmGenerateStripHead05( &MDL_ModelStripHead, &ModelContext );

	return 0;
}

void MDL_Terminate( void )
{
}

int MDL_LoadModel( PMODEL p_pModel, const char *p_pFileName )
{
	GDFS FileHandle;
	long FileBlocks;
	Sint32 FileSize;
	char *pFileContents;
	size_t FilePosition = 0;
	MODEL_HEADER Header;
	size_t MeshIndex = 0;
	/* Temporarily testing out the texture */
	TEXTURE ModelTexture;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open model: %s", p_pFileName );
		return 1;
	}

	gdFsGetFileSize( FileHandle, &FileSize );
	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	pFileContents = syMalloc( FileBlocks * 2048 );

	if( gdFsReqRd32( FileHandle, FileBlocks, pFileContents ) < 0 )
	{
		syFree( pFileContents );
		LOG_Debug( "Could not load the model into memory: %s", p_pFileName );

		return 1;
	}

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	memcpy( &Header, pFileContents, sizeof( Header ) );

	if( Header.ID[ 0 ] != 'T' ||
		Header.ID[ 1 ] != 'M' ||
		Header.ID[ 2 ] != 'L' ||
		Header.ID[ 3 ] != 'M' )
	{
		syFree( pFileContents );

		LOG_Debug( "Model file is not recognised as being valid: %s",
			p_pFileName );

		return 1;
	}

	LOG_Debug( "Attempting to load model: %s", Header.Name );
	LOG_Debug( "\t%d meshes found", Header.MeshCount );
	p_pModel->MeshCount = Header.MeshCount;

	p_pModel->pMeshes = syMalloc( Header.MeshCount * sizeof( MESH ) );

	FilePosition += sizeof( Header );

	while( FilePosition != FileSize )
	{
		CHUNK Chunk;
		memcpy( &Chunk, &pFileContents[ FilePosition ], sizeof( CHUNK ) );
		FilePosition += sizeof( CHUNK );

		switch( Chunk.ID )
		{
			case 1:
			{
				int MeshSize = 0;

				MeshSize = MDL_ReadMeshData( pFileContents + FilePosition,
					&p_pModel->pMeshes[ MeshIndex ] );

				if( MeshSize == 0 )
				{
					syFree( pFileContents );
					syFree( p_pModel->pMeshes );

					return 1;
				}

				FilePosition += MeshSize;
				++MeshIndex;

				break;
			}
			default:
			{
				FilePosition += Chunk.Size;
				break;
			}
		}
	}

	syFree( pFileContents );

	return 0;
}

void MDL_DeleteModel( PMODEL p_pModel )
{
	size_t Mesh;

	if( p_pModel->pMeshes )
	{
		for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
		{
			if( p_pModel->pMeshes[ Mesh ].pVertices )
			{
				syFree( p_pModel->pMeshes[ Mesh ].pVertices );
			}

			if( p_pModel->pMeshes[ Mesh ].pIndices )
			{
				syFree( p_pModel->pMeshes[ Mesh ].pIndices );
			}

			if( p_pModel->pMeshes[ Mesh ].pKamuiVertices )
			{
				syFree( p_pModel->pMeshes[ Mesh ].pKamuiVertices );
			}

			if( &p_pModel->pMeshes[ Mesh ] )
			{
				syFree( &p_pModel->pMeshes[ Mesh ] );
			}
		}

		syFree( p_pModel->pMeshes );
	}
}

void MDL_CalculateLighting( PMODEL p_pModel, const MATRIX4X4 *p_pTransform,
	const VECTOR3 *p_pLightPosition )
{
	size_t Mesh;
	for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
	{
		size_t Vertex;
		MAT44_TransformVertices(
			( float * )( p_pModel->pMeshes[ Mesh ].pTransformedVertices ) + 3,
			( float * )( p_pModel->pMeshes[ Mesh ].pVertices ) + 3,
			p_pModel->pMeshes[ Mesh ].IndexCount,
			sizeof( MODEL_VERTEX ), sizeof( MODEL_VERTEX ), p_pTransform );

		for( Vertex = 0; Vertex < p_pModel->pMeshes[ Mesh ].IndexCount;
			++Vertex )
		{
			VECTOR3 Colour = { 1.0f, 1.0f, 1.0f };
			VECTOR3 LightColour = { 1.0f, 1.0f, 1.0f };
			VECTOR3 DiffuseLight;
			VECTOR3 LightNormal;
			float LightIntensity;

			VEC3_Subtract( &LightNormal, p_pLightPosition,
				&p_pModel->pMeshes[ Mesh ].pTransformedVertices[ 
					Vertex ].Position );
			VEC3_Normalise( &LightNormal );

			LightIntensity = VEC3_Dot(
				&p_pModel->pMeshes[ Mesh ].pTransformedVertices[
					Vertex ].Normal,
				&LightNormal );
			if( LightIntensity < 0.0f )
			{
				LightIntensity = 0.0f;
			}
			VEC3_MultiplyV( &DiffuseLight, &Colour, &LightColour );
			VEC3_MultiplyF( &DiffuseLight, &DiffuseLight, LightIntensity );

			p_pModel->pMeshes[ Mesh ].pKamuiVertices[ Vertex ].fBaseRed =
				DiffuseLight.X;
			p_pModel->pMeshes[ Mesh ].pKamuiVertices[ Vertex ].fBaseGreen =
				DiffuseLight.Y;
			p_pModel->pMeshes[ Mesh ].pKamuiVertices[ Vertex ].fBaseBlue =
				DiffuseLight.Z;
		}
	}
}

void MDL_RenderModel( PMODEL p_pModel, const MATRIX4X4 *p_pTransform )
{
	size_t Mesh = 0;
	for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
	{
		MAT44_TransformVerticesRHW(
			( float * )( p_pModel->pMeshes[ Mesh ].pKamuiVertices ) + 1,
			( float * )p_pModel->pMeshes[ Mesh ].pVertices,
			p_pModel->pMeshes[ Mesh ].IndexCount,
			sizeof( KMVERTEX_05 ), sizeof( MODEL_VERTEX ), p_pTransform );

		REN_DrawPrimitives05( &MDL_ModelStripHead,
			p_pModel->pMeshes[ Mesh ].pKamuiVertices,
			p_pModel->pMeshes[ Mesh ].IndexCount );
	}
}

int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh )
{
	MESH_CHUNK MeshChunk;
	size_t DataPosition = 0;
	size_t Index = 0;
	size_t EndStrip = 0;
	MODEL_VERTEX *pOriginalVertices;

	memcpy( &MeshChunk, p_pData, sizeof( MESH_CHUNK ) );
	DataPosition += sizeof( MESH_CHUNK );

	p_pMesh->FaceCount = MeshChunk.ListCount / 3;
	p_pMesh->pVertices = syMalloc( sizeof( MODEL_VERTEX ) *
		MeshChunk.ListCount );
	p_pMesh->pTransformedVertices = syMalloc( sizeof( MODEL_VERTEX ) *
		MeshChunk.ListCount );
	p_pMesh->pIndices = syMalloc( sizeof( Uint32 ) * MeshChunk.ListCount );
	p_pMesh->pKamuiVertices =
		syMalloc( sizeof( KMVERTEX_05 ) * MeshChunk.ListCount );
	p_pMesh->IndexCount = MeshChunk.ListCount;

	pOriginalVertices = syMalloc( sizeof( MODEL_VERTEX ) *
		MeshChunk.VertexCount );

	memcpy( pOriginalVertices, &p_pData[ DataPosition ],
		sizeof( MODEL_VERTEX ) * MeshChunk.VertexCount );
	DataPosition += sizeof( MODEL_VERTEX ) * MeshChunk.VertexCount;

	memcpy( p_pMesh->pIndices, &p_pData[ DataPosition ],
		sizeof( Uint32 ) * MeshChunk.ListCount );
	DataPosition += sizeof( Uint32 ) * MeshChunk.ListCount;

	for( Index = 0; Index < MeshChunk.ListCount; ++Index )
	{
		memcpy( &p_pMesh->pVertices[ Index ], 
			&pOriginalVertices[ p_pMesh->pIndices[ Index ] ],
			sizeof( MODEL_VERTEX ) );

		if( EndStrip == 2 )
		{
			p_pMesh->pKamuiVertices[ Index ].ParamControlWord =
				KM_VERTEXPARAM_ENDOFSTRIP;
			EndStrip = 0;
		}
		else
		{
			p_pMesh->pKamuiVertices[ Index ].ParamControlWord =
				KM_VERTEXPARAM_NORMAL;
			++EndStrip;
		}

		p_pMesh->pKamuiVertices[ Index ].fU =
			p_pMesh->pVertices[ Index ].UV[ 0 ];
		p_pMesh->pKamuiVertices[ Index ].fV =
			p_pMesh->pVertices[ Index ].UV[ 1 ];

		p_pMesh->pKamuiVertices[ Index ].fBaseAlpha = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseRed = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseGreen = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseBlue = 1.0f;
	}

	syFree( pOriginalVertices );

	return DataPosition;
}

