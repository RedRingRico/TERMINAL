#ifndef __TERMINAL_MESH_H__
#define __TERMINAL_MESH_H__

#include <shinobi.h>
#include <Vector3.h>
#include <Renderer.h>
#include <Texture.h>

typedef enum _tagPRIMITIVE_TYPE
{
	PRIMITIVE_TYPE_TRIANGLE_LIST,
	PRIMITIVE_TYPE_TRIANGLE_STRIP
}PRIMITIVE_TYPE;

typedef struct _tagMODEL_VERTEX
{
	VECTOR3	Position;
	VECTOR3	Normal;
	float	UV[ 2 ];
}MODEL_VERTEX,*PMODEL_VERTEX;

typedef struct _tagMESH
{
	Uint32			FaceCount;
	Uint32			IndexCount;
	PRIMITIVE_TYPE	Type;
	MODEL_VERTEX	*pVertices;
	MODEL_VERTEX	*pTransformedVertices;
	Uint32			*pIndices;
	KMVERTEX_05		*pKamuiVertices;
	/* Change to a material type later */
	TEXTURE			Texture;
}MESH,*PMESH;

typedef struct _tagMESH_CHUNK
{
	Uint32	Flags;
	Uint32	VertexCount;
	Uint32	MaterialID;
	Uint32	ListCount;
	Uint32	StripCount;
	Uint32	FanCount;
}MESH_CHUNK,*PMESH_CHUNK;

/*int MES_ReadMeshData( void *p_pData, PMESH_CHUNK p_pChunk );*/

#endif /* __TERMINAL_MESH_H__ */

