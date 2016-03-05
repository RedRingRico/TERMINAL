#include <Plane.h>
#include <Arithmetic.h>

bool PLANE_IntersectsAABB( const PPLANE p_pPlane, const PAABB p_pAABB )
{
	VECTOR3 Minimum, Maximum;

	/* X */
	if( p_pPlane->Normal.X >= 0.0f )
	{
		Minimum.X = p_pAABB->Minimum.X;
		Maximum.X = p_pAABB->Maximum.X;
	}
	else
	{
		Minimum.X = p_pAABB->Maximum.X;
		Maximum.X = p_pAABB->Minimum.X;
	}

	/* Y */
	if( p_pPlane->Normal.Y >= 0.0f )
	{
		Minimum.Y = p_pAABB->Minimum.Y;
		Maximum.Y = p_pAABB->Maximum.Y;
	}
	else
	{
		Minimum.Y = p_pAABB->Maximum.Y;
		Maximum.Y = p_pAABB->Minimum.Y;
	}

	/* Z */
	if( p_pPlane->Normal.Z >= 0.0f )
	{
		Minimum.Z = p_pAABB->Minimum.Z;
		Maximum.Z = p_pAABB->Maximum.Z;
	}
	else
	{
		Minimum.Z = p_pAABB->Maximum.Z;
		Maximum.Z = p_pAABB->Minimum.Z;
	}

	if( VEC3_Dot( &p_pPlane->Normal, &Minimum ) + p_pPlane->Distance > 0.0f )
	{
		return false;
	}

	if( VEC3_Dot( &p_pPlane->Normal, &Maximum ) + p_pPlane->Distance >= 0.0f )
	{
		return true;
	}

	return false;
}

PLANE_CLASS PLANE_ClassifyAABB( const PPLANE p_pPlane, const PAABB p_pAABB )
{
	float Class;
	VECTOR3 Minimum, Maximum;

	/* X */
	if( p_pPlane->Normal.X >= 0.0f )
	{
		Minimum.X = p_pAABB->Minimum.X;
		Maximum.X = p_pAABB->Maximum.X;
	}
	else
	{
		Minimum.X = p_pAABB->Maximum.X;
		Maximum.X = p_pAABB->Minimum.X;
	}

	/* Y */
	if( p_pPlane->Normal.Y >= 0.0f )
	{
		Minimum.Y = p_pAABB->Minimum.Y;
		Maximum.Y = p_pAABB->Maximum.Y;
	}
	else
	{
		Minimum.Y = p_pAABB->Maximum.Y;
		Maximum.Y = p_pAABB->Minimum.Y;
	}
	
	/* Z */
	if( p_pPlane->Normal.Z >= 0.0f )
	{
		Minimum.Z = p_pAABB->Minimum.Z;
		Maximum.Z = p_pAABB->Maximum.Z;
	}
	else
	{
		Minimum.Z = p_pAABB->Maximum.Z;
		Maximum.Z = p_pAABB->Minimum.Z;
	}

	if( VEC3_Dot( &p_pPlane->Normal, &Minimum ) + p_pPlane->Distance > 0.0f )
	{
		return PLANE_CLASS_FRONT;
	}

	if( VEC3_Dot( &p_pPlane->Normal, &Maximum ) + p_pPlane->Distance >= 0.0f )
	{
		return PLANE_CLASS_PLANAR;
	}

	return PLANE_CLASS_BACK;
}

PLANE_CLASS PLANE_ClassifyVECTOR3( const PPLANE p_pPlane,
	const VECTOR3 *p_pVector )
{
	float Side = VEC3_Dot( p_pVector, &p_pPlane->Normal ) + p_pPlane->Distance;

	if( Side > ARI_EPSILON )
	{
		return PLANE_CLASS_FRONT;
	}

	if( Side < -ARI_EPSILON )
	{
		return PLANE_CLASS_BACK;
	}

	return PLANE_CLASS_PLANAR;
}

