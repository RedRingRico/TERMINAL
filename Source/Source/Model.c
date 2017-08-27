#include <Model.h>
#include <FileSystem.h>
#include <Log.h>
#include <SHC/umachine.h>
#include <SHC/private.h>
#include <mathf.h>
#include <Plane.h>
#include <Frustum.h>

typedef struct _tagCHUNK
{
	Uint32 ID;
	Uint32 Size;
}CHUNK,*PCHUNK;

static int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh,
	PMEMORY_BLOCK p_pMemoryBlock );
static KMSTRIPHEAD	MDL_ModelStripHead;

static TEXTURE g_ModelTexture;

int MDL_Initialise( PMEMORY_BLOCK p_pMemoryBlock )
{
	KMPACKEDARGB BaseColour;
	KMSTRIPCONTEXT ModelContext;

	TEX_LoadTexture( &g_ModelTexture, "/MODELS/CUBE.PVR", p_pMemoryBlock );

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
		&g_ModelTexture.SurfaceDescription;

	kmGenerateStripHead05( &MDL_ModelStripHead, &ModelContext );

	return 0;
}

void MDL_Terminate( void )
{
	TEX_DeleteTexture( &g_ModelTexture );
}

int MDL_LoadModel( PMODEL p_pModel, const char *p_pFileName,
	PMEMORY_BLOCK p_pMemoryBlock )
{
	GDFS FileHandle;
	long FileBlocks;
	Sint32 FileSize;
	char *pFileContents;
	size_t FilePosition = 0;
	MODEL_HEADER Header;
	size_t MeshIndex = 0;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open model: %s", p_pFileName );
		return 1;
	}

	gdFsGetFileSize( FileHandle, &FileSize );
	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	pFileContents = MEM_AllocateFromBlock( p_pMemoryBlock,
		FileBlocks * 2048, "Model buffer" );

	if( gdFsReqRd32( FileHandle, FileBlocks, pFileContents ) < 0 )
	{
		MEM_FreeFromBlock( p_pMemoryBlock, pFileContents );
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
		MEM_FreeFromBlock( p_pMemoryBlock, pFileContents );

		LOG_Debug( "Model file is not recognised as being valid: %s",
			p_pFileName );

		return 1;
	}

	LOG_Debug( "Attempting to load model: %s", Header.Name );
	LOG_Debug( "\t%d meshes found", Header.MeshCount );
	p_pModel->MeshCount = Header.MeshCount;

	p_pModel->pMeshes = MEM_AllocateFromBlock( p_pMemoryBlock,
		Header.MeshCount * sizeof( MESH ), "Model mesh buffer" );

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
					&p_pModel->pMeshes[ MeshIndex ], p_pMemoryBlock );

				if( MeshSize == 0 )
				{
					MEM_FreeFromBlock( p_pMemoryBlock, pFileContents );
					MEM_FreeFromBlock( p_pMemoryBlock, p_pModel->pMeshes );

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

	MEM_FreeFromBlock( p_pMemoryBlock, pFileContents );

	p_pModel->pMemoryBlock = p_pMemoryBlock;

	return 0;
}

int MDL_LoadModelFromMemory( PMODEL p_pModel, const Uint8 *p_pMemory,
	const size_t p_MemorySize, PMEMORY_BLOCK p_pMemoryBlock )
{
	MODEL_HEADER Header;
	size_t MemoryPosition = 0;
	size_t MeshIndex = 0;

	memcpy( &Header, p_pMemory, sizeof( Header ) );

	if( Header.ID[ 0 ] != 'T' ||
		Header.ID[ 1 ] != 'M' ||
		Header.ID[ 2 ] != 'L' ||
		Header.ID[ 3 ] != 'M' )
	{
		LOG_Debug( "[MDL_LoadModelFromMemory] <ERROR> "
			"Invalid header detected" );

		return 1;
	}

	p_pModel->MeshCount = Header.MeshCount;
	p_pModel->pMeshes = MEM_AllocateFromBlock( p_pMemoryBlock,
		Header.MeshCount * sizeof( MESH ), "Model mesh buffer" );
	memcpy( p_pModel->Name, Header.Name, 32 );

	MemoryPosition += sizeof( Header );
	
	/*while( MemoryPosition != p_MemorySize )
	{
		CHUNK Chunk;
		memcpy( &Chunk, &p_pMemory[ MemoryPosition ], sizeof( Chunk ) );
		MemoryPosition += sizeof( Chunk );

		switch( Chunk.ID )
		{
			case 1:
			{
				int MeshSize = 0;

				MeshSize = MDL_ReadMeshData( p_pMemory + MemoryPosition,
					&p_pModel->pMeshes[ MeshIndex ], p_pMemoryBlock );

				if( MeshSize == 0 )
				{
					MEM_FreeFromBlock( p_pMemoryBlock, p_pModel->pMeshes );

					return 1;
				}

				MemoryPosition += MeshSize;
				++MeshIndex;

				break;
			}
			default:
			{
				MemoryPosition += Chunk.Size;
			}
		}
	}*/

	p_pModel->pMemoryBlock = p_pMemoryBlock;

	return 0;
}

void MDL_DeleteModel( PMODEL p_pModel )
{
	size_t Mesh;

	if( p_pModel->pMeshes )
	{
		for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
		{
			if( p_pModel->pMeshes[ Mesh ].Vertices.pPosition != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					p_pModel->pMeshes[ Mesh ].Vertices.pPosition );
			}

			if( p_pModel->pMeshes[ Mesh ].Vertices.pNormal != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					p_pModel->pMeshes[ Mesh ].Vertices.pNormal );
			}

			if( p_pModel->pMeshes[ Mesh ].Vertices.pUV != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					p_pModel->pMeshes[ Mesh ].Vertices.pUV );
			}

			if( p_pModel->pMeshes[ Mesh ].pIndices != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					p_pModel->pMeshes[ Mesh ].pIndices );
			}

			if( p_pModel->pMeshes[ Mesh ].pKamuiVertices != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					p_pModel->pMeshes[ Mesh ].pKamuiVertices );
			}

			if( &p_pModel->pMeshes[ Mesh ] != NULL )
			{
				MEM_FreeFromBlock( p_pModel->pMemoryBlock,
					&p_pModel->pMeshes[ Mesh ] );
			}
		}

		MEM_FreeFromBlock( p_pModel->pMemoryBlock, p_pModel->pMeshes );
	}
}

void MDL_CalculateLighting( PMODEL p_pModel, const MATRIX4X4 *p_pTransform,
	const VECTOR3 *p_pLightPosition )
{
	size_t Mesh;
	MATRIX4X4 Inverse;
	VECTOR3 Light, LightT;
	float Normal[ 4 ], LightDirection[ 4 ];
	float *pNormals;

	MAT44_Copy( &Inverse, p_pTransform );
	MAT44_Inverse( &Inverse );

	Light.X = p_pLightPosition->X;
	Light.Y = p_pLightPosition->Y;
	Light.Z = p_pLightPosition->Z;

	VEC3_Normalise( &Light );

	LightT.X =	Light.X * Inverse.M00 +
				Light.Y * Inverse.M10 +
				Light.Z * Inverse.M20;
	LightT.Y =	Light.X * Inverse.M01 +
				Light.Y * Inverse.M11 +
				Light.Z * Inverse.M21;
	LightT.Z =	Light.X * Inverse.M02 +
				Light.Y * Inverse.M12 +
				Light.Z * Inverse.M22;
	
	LightDirection[ 0 ] = LightT.X;
	LightDirection[ 1 ] = LightT.Y;
	LightDirection[ 2 ] = LightT.Z;
	LightDirection[ 3 ] = 0.0f;

	for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
	{
		size_t Vertex;
		KMVERTEX_05 *pTLVertex = p_pModel->pMeshes[ Mesh ].pKamuiVertices;

		Normal[ 0 ] = 0.0f;
		pNormals = ( float * )p_pModel->pMeshes[ Mesh ].Vertices.pNormal;

		for( Vertex = 0; Vertex < p_pModel->pMeshes[ Mesh ].IndexCount;
			++Vertex )
		{
			float Intensity;
			Normal[ 0 ] = *pNormals++;
			Normal[ 1 ] = *pNormals++;
			Normal[ 2 ] = *pNormals++;

			Intensity = fipr( Normal, LightDirection );

			if( Intensity < 0.0f )
			{
				Intensity = 0.0f;
			}

			pTLVertex->fBaseRed = Intensity;
			pTLVertex->fBaseGreen = Intensity;
			pTLVertex->fBaseBlue = Intensity;

			++pTLVertex;
		}
	}
}

void MDL_RenderModel( PMODEL p_pModel, RENDERER *p_pRenderer,
	const MATRIX4X4 *p_pWorld,
	const MATRIX4X4 *p_pView, const MATRIX4X4 *p_pProjection,
	const MATRIX4X4 *p_pScreen )
{
	size_t Mesh = 0;
	MATRIX4X4 WVP;
	FRUSTUM ViewFrustum;

	MAT44_Multiply( &WVP, p_pView, p_pProjection );

	FRUS_CreateFromViewProjection( &ViewFrustum, &WVP );

	MAT44_Multiply( &WVP, p_pWorld, p_pView );
	MAT44_Multiply( &WVP, &WVP, p_pProjection );
	MAT44_Multiply( &WVP, &WVP, p_pScreen );

	for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
	{
		AABB BoundingBoxTransform;
		FRUSTUM_CLASS FrustumClass;

		MAT44_TransformVertices( ( float * )&BoundingBoxTransform.Minimum,
			( float * )&p_pModel->pMeshes[ Mesh ].BoundingBox.Minimum, 1,
			sizeof( VECTOR3 ), sizeof( VECTOR3 ), p_pWorld );

		MAT44_TransformVertices( ( float * )&BoundingBoxTransform.Maximum,
			( float * )&p_pModel->pMeshes[ Mesh ].BoundingBox.Maximum, 1,
			sizeof( VECTOR3 ), sizeof( VECTOR3 ), p_pWorld );

		FrustumClass = FRUS_ClassifyAABB( &ViewFrustum,
			&BoundingBoxTransform );

		if( FrustumClass == FRUSTUM_CLASS_OUTSIDE )
		{
			p_pRenderer->CulledPolygons +=
				p_pModel->pMeshes[ Mesh ].IndexCount / 3;

			continue;
		}

		/* Must be along one of the planes instead of inside or outside */
		if( FrustumClass != FRUSTUM_CLASS_INSIDE )
		{
			PLANE_CLASS NearClass = PLANE_ClassifyAABB(
				&ViewFrustum.Plane[ FRUSTUM_INDEX_NEAR ],
				&BoundingBoxTransform );

			if( NearClass == PLANE_CLASS_FRONT )
			{
				p_pRenderer->CulledPolygons +=
					p_pModel->pMeshes[ Mesh ].IndexCount / 3;

				continue;
			}
			else if( NearClass == PLANE_CLASS_PLANAR )
			{
				/* Clip it! */
				Uint32 VertexCount;

				MAT44_TransformVertices(
					( float * )p_pModel->pMeshes[ Mesh
						].TransformedVertices.pPosition,
					( float * )p_pModel->pMeshes[ Mesh ].Vertices.pPosition,
					p_pModel->pMeshes[ Mesh ].IndexCount,
					sizeof( VECTOR3 ), sizeof( VECTOR3 ), p_pWorld );

				VertexCount = MDL_ClipMeshToPlane( p_pRenderer,
					&p_pModel->pMeshes[ Mesh ],
					&ViewFrustum.Plane[ FRUSTUM_INDEX_NEAR ] );

				MAT44_TransformVerticesRHW( 
					( float * )( p_pRenderer->pVertices05 ) + 1,
					( float * )( p_pRenderer->pVertices05 ) + 1,
					VertexCount, sizeof( KMVERTEX_05 ), sizeof( KMVERTEX_05 ),
					&WVP );

				REN_DrawPrimitives05Cached( p_pRenderer, &MDL_ModelStripHead,
					0, VertexCount );
			}
			else
			{
				/* Render it! */
				goto RENDER_NORMAL;
			}
		}
		else
		{
RENDER_NORMAL:
			p_pRenderer->VisiblePolygons +=
				p_pModel->pMeshes[ Mesh ].IndexCount / 3;
		
			MAT44_TransformVerticesRHW(
				( float * )( p_pModel->pMeshes[ Mesh ].pKamuiVertices ) + 1,
				( float * )p_pModel->pMeshes[ Mesh ].Vertices.pPosition,
				p_pModel->pMeshes[ Mesh ].IndexCount,
				sizeof( KMVERTEX_05 ), sizeof( VECTOR3 ), &WVP );

			MAT44_ClipVertices(
				( p_pModel->pMeshes[ Mesh ].pKamuiVertices ),
				NULL, p_pModel->pMeshes[ Mesh ].IndexCount,
				sizeof( KMVERTEX_05 ) );

			REN_DrawPrimitives05( &MDL_ModelStripHead,
				p_pModel->pMeshes[ Mesh ].pKamuiVertices,
				p_pModel->pMeshes[ Mesh ].IndexCount );
		}
	}
}

static int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh,
	PMEMORY_BLOCK p_pMemoryBlock )
{
	MESH_CHUNK MeshChunk;
	size_t DataPosition = 0;
	size_t Index = 0;
	size_t EndStrip = 0;
	MODEL_VERTEX_PACKED *pOriginalVertices;
	AABB BoundingBox;

	memcpy( &MeshChunk, p_pData, sizeof( MESH_CHUNK ) );
	DataPosition += sizeof( MESH_CHUNK );

	p_pMesh->FaceCount = MeshChunk.ListCount / 3;
	p_pMesh->Vertices.pPosition = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( VECTOR3 ) * MeshChunk.ListCount, "Mesh: Position vertices" );
	p_pMesh->Vertices.pNormal = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( VECTOR3 ) *	MeshChunk.ListCount, "Mesh: Normal vertices" );
	p_pMesh->Vertices.pUV = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( UV ) *MeshChunk.ListCount, "Mesh: UV vertices" );
	p_pMesh->TransformedVertices.pPosition = MEM_AllocateFromBlock(
		p_pMemoryBlock, sizeof( VECTOR3 ) * MeshChunk.ListCount,
		"Mesh transformed position vertices" );
	p_pMesh->pIndices = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( Uint32 ) * MeshChunk.ListCount, "Mesh: Indices" );
	p_pMesh->pKamuiVertices = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( KMVERTEX_05 ) * MeshChunk.ListCount, "Mesh: Kamui vertices" );
	p_pMesh->IndexCount = MeshChunk.ListCount;

	pOriginalVertices = MEM_AllocateFromBlock( p_pMemoryBlock,
		sizeof( MODEL_VERTEX_PACKED ) * MeshChunk.VertexCount,
		"Mesh: Temporary vertices" );

	memcpy( pOriginalVertices, &p_pData[ DataPosition ],
		sizeof( MODEL_VERTEX_PACKED ) * MeshChunk.VertexCount );
	DataPosition += sizeof( MODEL_VERTEX_PACKED ) * MeshChunk.VertexCount;

	memcpy( p_pMesh->pIndices, &p_pData[ DataPosition ],
		sizeof( Uint32 ) * MeshChunk.ListCount );
	DataPosition += sizeof( Uint32 ) * MeshChunk.ListCount;

	memcpy( &BoundingBox.Minimum,
		&pOriginalVertices[ p_pMesh->pIndices[ Index ] ].Position,
		sizeof( VECTOR3 ) );
	memcpy( &BoundingBox.Maximum,
		&pOriginalVertices[ p_pMesh->pIndices[ Index ] ].Position,
		sizeof( VECTOR3 ) );

	for( Index = 0; Index < MeshChunk.ListCount; ++Index )
	{
		memcpy( &p_pMesh->Vertices.pPosition[ Index ],
			&pOriginalVertices[ p_pMesh->pIndices[ Index ] ].Position,
			sizeof( VECTOR3 ) );

		memcpy( &p_pMesh->Vertices.pNormal[ Index ],
			&pOriginalVertices[ p_pMesh->pIndices[ Index ] ].Normal,
			sizeof( VECTOR3 ) );

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
			pOriginalVertices[ p_pMesh->pIndices[ Index ] ].UV.U;
		p_pMesh->pKamuiVertices[ Index ].fV =
			pOriginalVertices[ p_pMesh->pIndices[ Index ] ].UV.V;

		p_pMesh->pKamuiVertices[ Index ].fBaseAlpha = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseRed = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseBlue = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseGreen = 1.0f;

		/* Bounding box */
		if( p_pMesh->Vertices.pPosition[ Index ].X < BoundingBox.Minimum.X )
		{
			BoundingBox.Minimum.X = p_pMesh->Vertices.pPosition[ Index ].X;
		}
		if( p_pMesh->Vertices.pPosition[ Index ].X > BoundingBox.Maximum.X )
		{
			BoundingBox.Maximum.X = p_pMesh->Vertices.pPosition[ Index ].X;
		}

		if( p_pMesh->Vertices.pPosition[ Index ].Y < BoundingBox.Minimum.Y )
		{
			BoundingBox.Minimum.Y = p_pMesh->Vertices.pPosition[ Index ].Y;
		}
		if( p_pMesh->Vertices.pPosition[ Index ].Y > BoundingBox.Maximum.Y )
		{
			BoundingBox.Maximum.Y = p_pMesh->Vertices.pPosition[ Index ].Y;
		}

		if( p_pMesh->Vertices.pPosition[ Index ].Z < BoundingBox.Minimum.Z )
		{
			BoundingBox.Minimum.Z = p_pMesh->Vertices.pPosition[ Index ].Z;
		}
		if( p_pMesh->Vertices.pPosition[ Index ].Z > BoundingBox.Maximum.Z )
		{
			BoundingBox.Maximum.Z = p_pMesh->Vertices.pPosition[ Index ].Z;
		}
	}

	memcpy( &p_pMesh->BoundingBox, &BoundingBox, sizeof( BoundingBox ) );

	MEM_FreeFromBlock( p_pMemoryBlock, pOriginalVertices );

	return DataPosition;
}

Uint32 MDL_ClipMeshToPlane( PRENDERER p_pRenderer, const PMESH p_pMesh,
	const PPLANE p_pPlane )
{
	Uint32 Vertex;
	Uint32 VertexCounter = 0;
	Uint32 VertexPassCount = 0;
	Uint32 TriangleIndex = 0;
	Uint32 Inside = 0;
	Uint32 Outside = 0;
	KMVERTEX5 Triangle[ 3 ];
	KMVERTEX5 Quad[ 4 ];
	bool V0 = false;
	bool V1 = false;
	bool V2 = false;

	/* For now, only render polygons on the back side of the plane, ignore
	 * intersections */
	for( Vertex = 0; Vertex < p_pMesh->IndexCount; ++Vertex )
	{
		PLANE_CLASS VectorClass;

		VectorClass = PLANE_ClassifyVECTOR3( p_pPlane,
			&p_pMesh->TransformedVertices.pPosition[ Vertex ] );

		if( ( VectorClass == PLANE_CLASS_BACK ) ||
			( VectorClass == PLANE_CLASS_PLANAR ) )
		{
			if( VertexCounter == 0 )
			{
				Triangle[ 0 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 0 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 0 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 0 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 0 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 0 ].fBaseAlpha = 1.0f;
				Triangle[ 0 ].fBaseRed = 1.0f;
				Triangle[ 0 ].fBaseGreen = 1.0f;
				Triangle[ 0 ].fBaseBlue = 1.0f;

				V0 = true;
			}
			else if( VertexCounter == 1 )
			{
				Triangle[ 1 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 1 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 1 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 1 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 1 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 1 ].fBaseAlpha = 1.0f;
				Triangle[ 1 ].fBaseRed = 1.0f;
				Triangle[ 1 ].fBaseGreen = 1.0f;
				Triangle[ 1 ].fBaseBlue = 1.0f;

				V1 = true;
			}
			else if( VertexCounter == 2 )
			{
				Triangle[ 2 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 2 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 2 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 2 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 2 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 2 ].fBaseAlpha = 1.0f;
				Triangle[ 2 ].fBaseRed = 1.0f;
				Triangle[ 2 ].fBaseGreen = 1.0f;
				Triangle[ 2 ].fBaseBlue = 1.0f;

				V2 = true;
			}

			++Inside;
		}
		else /* Clipped side of the polygon */
		{
			if( VertexCounter == 0 )
			{
				Triangle[ 0 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 0 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 0 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 0 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 0 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 0 ].fBaseAlpha = 1.0f;
				Triangle[ 0 ].fBaseRed = 1.0f;
				Triangle[ 0 ].fBaseGreen = 1.0f;
				Triangle[ 0 ].fBaseBlue = 1.0f;

				V0 = false;
			}
			else if( VertexCounter == 1 )
			{
				Triangle[ 1 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 1 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 1 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 1 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 1 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 1 ].fBaseAlpha = 1.0f;
				Triangle[ 1 ].fBaseRed = 1.0f;
				Triangle[ 1 ].fBaseGreen = 1.0f;
				Triangle[ 1 ].fBaseBlue = 1.0f;

				V1 = false;
			}
			else if( VertexCounter == 2 )
			{
				Triangle[ 2 ].fX = p_pMesh->Vertices.pPosition[ Vertex ].X;
				Triangle[ 2 ].fY = p_pMesh->Vertices.pPosition[ Vertex ].Y;
				Triangle[ 2 ].u.fInvW =
					p_pMesh->Vertices.pPosition[ Vertex ].Z;

				Triangle[ 2 ].fU = p_pMesh->Vertices.pUV[ Vertex ].U;
				Triangle[ 2 ].fV = p_pMesh->Vertices.pUV[ Vertex ].V;

				Triangle[ 2 ].fBaseAlpha = 1.0f;
				Triangle[ 2 ].fBaseRed = 1.0f;
				Triangle[ 2 ].fBaseGreen = 1.0f;
				Triangle[ 2 ].fBaseBlue = 1.0f;

				V2 = false;
			}

			++Outside;
		}

		++VertexCounter;

		if( VertexCounter == 3 )
		{
			VertexCounter = 0;

			if( ( V0 == true ) && ( V1 == true ) && ( V2 == true ) )
			{
				Triangle[ 0 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
				Triangle[ 1 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
				Triangle[ 2 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
				memcpy( &p_pRenderer->pVertices05[ TriangleIndex ],
					&Triangle, sizeof( Triangle ) );
				TriangleIndex += 3;
			}
			else
			{
				if( Inside == 1 )
				{
					float Scale;
					VECTOR3 InsideV, OutsideV;
					float DotCur, DotNext;

					Triangle[ 0 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
					Triangle[ 1 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
					Triangle[ 2 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;

					if( V0 )
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 0 ].fX;
						InsideV.Y = Triangle[ 0 ].fY;
						InsideV.Z = Triangle[ 0 ].u.fInvW;

						OutsideV.X = Triangle[ 1 ].fX;
						OutsideV.Y = Triangle[ 1 ].fY;
						OutsideV.Z = Triangle[ 1 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );
						Scale = DotCur / DotNext;

						Triangle[ 1 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 1 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 1 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );

						OutsideV.X = Triangle[ 2 ].fX;
						OutsideV.Y = Triangle[ 2 ].fY;
						OutsideV.Z = Triangle[ 2 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						Scale = DotCur / DotNext;

						Triangle[ 2 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 2 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 2 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
					}
					else if( V1 )
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 1 ].fX;
						InsideV.Y = Triangle[ 1 ].fY;
						InsideV.Z = Triangle[ 1 ].u.fInvW;

						OutsideV.X = Triangle[ 0 ].fX;
						OutsideV.Y = Triangle[ 0 ].fY;
						OutsideV.Z = Triangle[ 0 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );
						Scale = DotCur / DotNext;

						Triangle[ 0 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 0 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 0 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );

						OutsideV.X = Triangle[ 2 ].fX;
						OutsideV.Y = Triangle[ 2 ].fY;
						OutsideV.Z = Triangle[ 2 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						Scale = DotCur / DotNext;

						Triangle[ 2 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 2 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 2 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
					}
					else
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 2 ].fX;
						InsideV.Y = Triangle[ 2 ].fY;
						InsideV.Z = Triangle[ 2 ].u.fInvW;

						OutsideV.X = Triangle[ 0 ].fX;
						OutsideV.Y = Triangle[ 0 ].fY;
						OutsideV.Z = Triangle[ 0 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );
						Scale = DotCur / DotNext;

						Triangle[ 0 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 0 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 0 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );

						OutsideV.X = Triangle[ 1 ].fX;
						OutsideV.Y = Triangle[ 1 ].fY;
						OutsideV.Z = Triangle[ 1 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						Scale = DotCur / DotNext;

						Triangle[ 1 ].fX = InsideV.X + ( Direction.X * Scale );
						Triangle[ 1 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Triangle[ 1 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
					}

					memcpy( &p_pRenderer->pVertices05[ TriangleIndex ],
						&Triangle, sizeof( Triangle ) );
					TriangleIndex += 3;
					++p_pRenderer->VisiblePolygons;
				}
				else if( Inside == 2 )
				{
					float Scale;
					VECTOR3 InsideV, OutsideV, Direction;
					float DotCur, DotNext;

					if( V0 == false )
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 1 ].fX;
						InsideV.Y = Triangle[ 1 ].fY;
						InsideV.Z = Triangle[ 1 ].u.fInvW;

						OutsideV.X = Triangle[ 0 ].fX;
						OutsideV.Y = Triangle[ 0 ].fY;
						OutsideV.Z = Triangle[ 0 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 0 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 0 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 0 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 0 ].fBaseAlpha = 1.0f;
						Quad[ 0 ].fBaseRed = 1.0f;
						Quad[ 0 ].fBaseGreen = 1.0f;
						Quad[ 0 ].fBaseBlue = 1.0f;
						Quad[ 0 ].fU = Triangle[ 0 ].fU;
						Quad[ 0 ].fV = Triangle[ 0 ].fV;


						InsideV.X = Triangle[ 2 ].fX;
						InsideV.Y = Triangle[ 2 ].fY;
						InsideV.Z = Triangle[ 2 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 2 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 2 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 2 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 2 ].fBaseAlpha = 1.0f;
						Quad[ 2 ].fBaseRed = 1.0f;
						Quad[ 2 ].fBaseGreen = 1.0f;
						Quad[ 2 ].fBaseBlue = 1.0f;
						Quad[ 2 ].fU = Triangle[ 0 ].fU;
						Quad[ 2 ].fV = Triangle[ 0 ].fV;

						memcpy( &Quad[ 1 ], &Triangle[ 1 ],
							sizeof( KMVERTEX_05 ) );
						memcpy( &Quad[ 3 ], &Triangle[ 2 ],
							sizeof( KMVERTEX_05 ) );
					}
					else if( V1 == false )
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 2 ].fX;
						InsideV.Y = Triangle[ 2 ].fY;
						InsideV.Z = Triangle[ 2 ].u.fInvW;

						OutsideV.X = Triangle[ 1 ].fX;
						OutsideV.Y = Triangle[ 1 ].fY;
						OutsideV.Z = Triangle[ 1 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 0 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 0 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 0 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 0 ].fBaseAlpha = 1.0f;
						Quad[ 0 ].fBaseRed = 1.0f;
						Quad[ 0 ].fBaseGreen = 1.0f;
						Quad[ 0 ].fBaseBlue = 1.0f;
						Quad[ 0 ].fU = Triangle[ 1 ].fU;
						Quad[ 0 ].fV = Triangle[ 1 ].fV;

						InsideV.X = Triangle[ 0 ].fX;
						InsideV.Y = Triangle[ 0 ].fY;
						InsideV.Z = Triangle[ 0 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 2 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 2 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 2 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 2 ].fBaseAlpha = 1.0f;
						Quad[ 2 ].fBaseRed = 1.0f;
						Quad[ 2 ].fBaseGreen = 1.0f;
						Quad[ 2 ].fBaseBlue = 1.0f;
						Quad[ 2 ].fU = Triangle[ 0 ].fU;
						Quad[ 2 ].fV = Triangle[ 0 ].fV;

						memcpy( &Quad[ 1 ], &Triangle[ 2 ],
							sizeof( KMVERTEX_05 ) );
						memcpy( &Quad[ 3 ], &Triangle[ 0 ],
							sizeof( KMVERTEX_05 ) );
					}
					else
					{
						VECTOR3 Direction;

						InsideV.X = Triangle[ 0 ].fX;
						InsideV.Y = Triangle[ 0 ].fY;
						InsideV.Z = Triangle[ 0 ].u.fInvW;

						OutsideV.X = Triangle[ 2 ].fX;
						OutsideV.Y = Triangle[ 2 ].fY;
						OutsideV.Z = Triangle[ 2 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 0 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 0 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 0 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 0 ].fBaseAlpha = 1.0f;
						Quad[ 0 ].fBaseRed = 1.0f;
						Quad[ 0 ].fBaseGreen = 1.0f;
						Quad[ 0 ].fBaseBlue = 1.0f;
						Quad[ 0 ].fU = Triangle[ 2 ].fU;
						Quad[ 0 ].fV = Triangle[ 2 ].fV;

						InsideV.X = Triangle[ 1 ].fX;
						InsideV.Y = Triangle[ 1 ].fY;
						InsideV.Z = Triangle[ 1 ].u.fInvW;

						VEC3_Subtract( &Direction, &OutsideV, &InsideV );

						DotNext = VEC3_Dot( &p_pPlane->Normal, &Direction );
						DotCur = -( VEC3_Dot( &p_pPlane->Normal, &InsideV ) +
							p_pPlane->Distance );

						Scale = DotCur / DotNext;

						Quad[ 2 ].fX = InsideV.X + ( Direction.X * Scale );
						Quad[ 2 ].fY = InsideV.Y + ( Direction.Y * Scale );
						Quad[ 2 ].u.fInvW = InsideV.Z +
							( Direction.Z * Scale );
						Quad[ 2 ].fBaseAlpha = 1.0f;
						Quad[ 2 ].fBaseRed = 1.0f;
						Quad[ 2 ].fBaseGreen = 1.0f;
						Quad[ 2 ].fBaseBlue = 1.0f;
						Quad[ 2 ].fU = Triangle[ 2 ].fU;
						Quad[ 2 ].fV = Triangle[ 2 ].fV;

						memcpy( &Quad[ 1 ], &Triangle[ 0 ],
							sizeof( KMVERTEX_05 ) );
						memcpy( &Quad[ 3 ], &Triangle[ 1 ],
							sizeof( KMVERTEX_05 ) );
					}

					Quad[ 0 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
					Quad[ 1 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
					Quad[ 2 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
					Quad[ 3 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;

					memcpy( &p_pRenderer->pVertices05[ TriangleIndex ],
						&Quad, sizeof( Quad ) );
					TriangleIndex += 4;

					++p_pRenderer->GeneratedPolygons;
					++p_pRenderer->VisiblePolygons;
				}
				else
				{
					++p_pRenderer->CulledPolygons;
				}
			}
			V0 = V1 = V2 = false;
			Inside = Outside = 0;
		}
	}

	return TriangleIndex;
}

