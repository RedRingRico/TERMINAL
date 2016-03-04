#ifndef __TERMINAL_FRUSTUM_H__
#define __TERMINAL_FRUSTUM_H__

#include <Plane.h>
#include <AABB.h>
#include <Matrix4x4.h>

typedef struct _tagFRUSTUM
{
	/* 0 - Near
	 * 1 - Far
	 * 2 - Left
	 * 3 - Right
	 * 4 - Top
	 * 5 - Bottom */
	PLANE	Plane[ 6 ];
}FRUSTUM,*PFRUSTUM;

typedef enum
{
	FRUSTUM_INDEX_NEAR		= 0,
	FRUSTUM_INDEX_FAR		= 1,
	FRUSTUM_INDEX_LEFT		= 2,
	FRUSTUM_INDEX_RIGHT		= 3,
	FRUSTUM_INDEX_TOP		= 4,
	FRUSTUM_INDEX_BOTTOM	= 5
}FRUSTUM_INDEX;

typedef enum
{
	FRUSTUM_CLASS_INSIDE,
	FRUSTUM_CLASS_OUTSIDE,
	FRUSTUM_CLASS_INTERSECT
}FRUSTUM_CLASS;

int FRUS_CreateFromViewProjection( PFRUSTUM p_pFrustum,
	PMATRIX4X4 p_pViewProjection );

FRUSTUM_CLASS FRUS_ClassifyAABB( PFRUSTUM p_pFrustum, PAABB p_pAABB );

#endif /* __TERMINAL_FRUSTUM_H__ */

