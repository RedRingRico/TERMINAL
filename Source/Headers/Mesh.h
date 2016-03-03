#ifndef __TERMINAL_MESH_H__
#define __TERMINAL_MESH_H__

#include <shinobi.h>
#include <Vector3.h>
#include <Renderer.h>
#include <Texture.h>
#include <AABB.h>

typedef enum _tagPRIMITIVE_TYPE
{
	PRIMITIVE_TYPE_TRIANGLE_LIST,
	PRIMITIVE_TYPE_TRIANGLE_STRIP
}PRIMITIVE_TYPE;

typedef struct _tagUV
{
	float U;
	float V;
}UV,*PUV;

typedef struct _tagMODEL_VERTEX
{
	VECTOR3	*pPosition;
	VECTOR3	*pNormal;
	UV		*pUV;
}MODEL_VERTEX,*PMODEL_VERTEX;

typedef struct _tagMODEL_VERTEX_PACKED
{
	VECTOR3	Position;
	VECTOR3	Normal;
	UV		UV;
}MODEL_VERTEX_PACKED,*PMODEL_VERTEX_PACKED;

typedef struct _tagMESH
{
	Uint32			FaceCount;
	Uint32			IndexCount;
	PRIMITIVE_TYPE	Type;
	MODEL_VERTEX	Vertices; // 24
	MODEL_VERTEX	*pTransformedVertices; // 36
	Uint32			*pIndices; // 40
	KMVERTEX_05		*pKamuiVertices; //44
	/* Change to a material type later */
	TEXTURE			Texture; // 48
	AABB			BoundingBox; //72
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

