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
			if( p_pModel->pMeshes[ Mesh ].Vertices.pPosition )
			{
				syFree( p_pModel->pMeshes[ Mesh ].Vertices.pPosition );
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

int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh )
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
	p_pMesh->Vertices.pPosition = syMalloc( sizeof( VECTOR3 ) *
		MeshChunk.ListCount );
	p_pMesh->Vertices.pNormal = syMalloc( sizeof( VECTOR3 ) *
		MeshChunk.ListCount );
	p_pMesh->Vertices.pUV = syMalloc( sizeof( UV ) *
		MeshChunk.ListCount );
	p_pMesh->TransformedVertices.pPosition = syMalloc( sizeof( VECTOR3 ) *
		MeshChunk.ListCount );
	p_pMesh->pIndices = syMalloc( sizeof( Uint32 ) * MeshChunk.ListCount );
	p_pMesh->pKamuiVertices =
		syMalloc( sizeof( KMVERTEX_05 ) * MeshChunk.ListCount );
	p_pMesh->IndexCount = MeshChunk.ListCount;

	pOriginalVertices = syMalloc( sizeof( MODEL_VERTEX_PACKED ) *
		MeshChunk.VertexCount );

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
		/*memcpy( &p_pMesh->pVertices[ Index ], 
			&pOriginalVertices[ p_pMesh->pIndices[ Index ] ],
			sizeof( MODEL_VERTEX ) );*/

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
		p_pMesh->pKamuiVertices[ Index ].fBaseGreen = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseBlue = 1.0f;

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

	syFree( pOriginalVertices );

	return DataPosition;
}

Uint32 MDL_ClipMeshToPlane( PRENDERER p_pRenderer, const PMESH p_pMesh,
	const PPLANE p_pPlane )
{
	Uint32 Vertex;
	Uint32 VertexCounter = 0;
	Uint32 VertexPassCount = 0;
	Uint32 TriangleIndex = 0;
	KMVERTEX5 Triangle[ 3 ];
	bool V0 = false;
	bool V1 = false;
	bool V2 = false;

	/* For now, only render polygons on the back side of the plane, ignore
	 * intersections */
	for( Vertex = 0; Vertex < p_pMesh->IndexCount; ++Vertex )
	{
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
			V0 = V1 = V2 = false;
		}

		if( PLANE_ClassifyVECTOR3( p_pPlane,
			&p_pMesh->TransformedVertices.pPosition[ Vertex ] ) ==
				PLANE_CLASS_BACK )
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
			if( VertexCounter == 1 )
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
			if( VertexCounter == 2 )
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
		}

		++VertexCounter;
	}

	return TriangleIndex;
}

