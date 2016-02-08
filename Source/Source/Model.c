#include <Model.h>
#include <FileSystem.h>
#include <Log.h>

typedef struct _tagCHUNK
{
	Uint32 ID;
	Uint32 Size;
}CHUNK,*PCHUNK;

static int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh );

int MDL_LoadModel( PMODEL p_pModel, const char *p_pFileName )
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

void MDL_RenderModel( PMODEL p_pModel, const MATRIX4X4 *p_pTransform )
{
	size_t Mesh;
	for( Mesh = 0; Mesh < p_pModel->MeshCount; ++Mesh )
	{
		MAT44_TransformVerticesRHW(
			( float * )( p_pModel->pMeshes[ Mesh ].pKamuiVertices ) + 1,
			( float * )p_pModel->pMeshes[ Mesh ].pVertices,
			p_pModel->pMeshes[ Mesh ].IndexCount,
			sizeof( KMVERTEX_01 ), sizeof( MODEL_VERTEX ), p_pTransform );

		p_pModel->pMeshes[ Mesh ].pKamuiVertices[
			p_pModel->pMeshes[ Mesh ].IndexCount - 1 ].ParamControlWord =
				KM_VERTEXPARAM_ENDOFSTRIP;

		REN_DrawPrimitives01( NULL, p_pModel->pMeshes[ Mesh ].pKamuiVertices,
			p_pModel->pMeshes[ Mesh ].IndexCount );
	}

}

int MDL_ReadMeshData( char *p_pData, PMESH p_pMesh )
{
	MESH_CHUNK MeshChunk;
	size_t DataPosition = 0;
	size_t Index = 0;
	MODEL_VERTEX *pOriginalVertices;

	memcpy( &MeshChunk, p_pData, sizeof( MESH_CHUNK ) );
	DataPosition += sizeof( MESH_CHUNK );

	p_pMesh->FaceCount = MeshChunk.ListCount / 3;
	p_pMesh->pVertices = syMalloc( sizeof( MODEL_VERTEX ) *
		MeshChunk.ListCount );
	p_pMesh->pIndices = syMalloc( sizeof( Uint32 ) * MeshChunk.ListCount );
	p_pMesh->pKamuiVertices =
		syMalloc( sizeof( KMVERTEX_01 ) * MeshChunk.ListCount );
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

		p_pMesh->pKamuiVertices[ Index ].ParamControlWord =
			KM_VERTEXPARAM_NORMAL;

		p_pMesh->pKamuiVertices[ Index ].fBaseAlpha = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseRed = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseGreen = 1.0f;
		p_pMesh->pKamuiVertices[ Index ].fBaseBlue = 1.0f;
	}

	syFree( pOriginalVertices );

	return DataPosition;
}

