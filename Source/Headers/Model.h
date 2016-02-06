#ifndef __TERMINAL_MODEL_H__
#define __TERMINAL_MODEL_H__

#include <shinobi.h>
#include <Mesh.h>
#include <Matrix4x4.h>

typedef struct _tagMODEL_HEADER
{
	char	ID[ 4 ];
	char	Name[ 32 ];
	Uint32	MeshCount;
}MODEL_HEADER,*PMODEL_HEADER;

typedef struct _tagMODEL
{
	size_t	PolygonCount;
	size_t	MeshCount;
	MESH	*pMeshes;
}MODEL,*PMODEL;

int MDL_LoadModel( PMODEL p_pModel, const char *p_pFileName );
void MDL_DeleteModel( PMODEL p_pModel );
void MDL_RenderModel( PMODEL p_pModel, const MATRIX4X4 *p_pTransform );

#endif /* __TERMINAL_MODEL_H__ */

