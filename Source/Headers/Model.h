#ifndef __TERMINAL_MODEL_H__
#define __TERMINAL_MODEL_H__

#include <shinobi.h>
#include <Mesh.h>
#include <Vector3.h>
#include <Matrix4x4.h>
#include <Renderer.h>

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

int MDL_Initialise( void );
void MDL_Terminate( void );

int MDL_LoadModel( PMODEL p_pModel, const char *p_pFileName );
void MDL_DeleteModel( PMODEL p_pModel );
void MDL_CalculateLighting( PMODEL p_pModel, const MATRIX4X4 *p_pTransform,
	const VECTOR3 *p_pLightPosition );
/*void MDL_RenderModel( PMODEL p_pModel, const MATRIX4X4 *p_pTransform );*/

void MDL_RenderModel( PMODEL p_pModel, RENDERER *p_pRenderer,
	const MATRIX4X4 *p_pWorld,
	const MATRIX4X4 *p_pView, const MATRIX4X4 *p_pProjection,
	const MATRIX4X4 *p_pScreen );

#endif /* __TERMINAL_MODEL_H__ */

