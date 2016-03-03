#ifndef __TERMINAL_PLANE_H__
#define __TERMINAL_PLANE_H__

#include <Vector3.h>
#include <AABB.h>
#include <shinobi.h>

typedef struct _tagPLANE
{
	VECTOR3	Normal;
	VECTOR3	Point;
	float	Distance;
}PLANE,*PPLANE;

typedef enum
{
	PLANE_CLASS_FRONT,
	PLANE_CLASS_BACK,
	PLANE_CLASS_PLANAR
}PLANE_CLASS;

bool PLANE_IntersectsAABB( const PPLANE p_pPlane, const PAABB p_pAABB );
PLANE_CLASS PLANE_ClassifyAABB( const PPLANE p_pPlane, const PAABB p_pAABB );

#endif /* __TERMINAL_PLANE_H__ */

