#include <Frustum.h>

int FRUS_CreateFromViewProjection( PFRUSTUM p_pFrustum,
	PMATRIX4X4 p_pViewProjection )
{
	size_t Index;

	p_pFrustum->Plane[ FRUSTUM_INDEX_NEAR ].Normal.X = -p_pViewProjection->M02;
	p_pFrustum->Plane[ FRUSTUM_INDEX_NEAR ].Normal.Y = -p_pViewProjection->M12;
	p_pFrustum->Plane[ FRUSTUM_INDEX_NEAR ].Normal.Z = -p_pViewProjection->M22;
	p_pFrustum->Plane[ FRUSTUM_INDEX_NEAR ].Distance = -p_pViewProjection->M32;	

	p_pFrustum->Plane[ FRUSTUM_INDEX_FAR ].Normal.X =
		-( p_pViewProjection->M03 - p_pViewProjection->M02 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_FAR ].Normal.Y =
		-( p_pViewProjection->M13 - p_pViewProjection->M12 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_FAR ].Normal.Z =
		-( p_pViewProjection->M23 - p_pViewProjection->M22 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_FAR ].Distance =
		-( p_pViewProjection->M33 - p_pViewProjection->M32 );

	p_pFrustum->Plane[ FRUSTUM_INDEX_LEFT ].Normal.X =
		-( p_pViewProjection->M03 + p_pViewProjection->M00 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_LEFT ].Normal.Y =
		-( p_pViewProjection->M13 + p_pViewProjection->M10 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_LEFT ].Normal.Z =
		-( p_pViewProjection->M23 + p_pViewProjection->M20 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_LEFT ].Distance =
		-( p_pViewProjection->M33 + p_pViewProjection->M30 );
	
	p_pFrustum->Plane[ FRUSTUM_INDEX_RIGHT ].Normal.X =
		-( p_pViewProjection->M03 - p_pViewProjection->M00 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_RIGHT ].Normal.Y =
		-( p_pViewProjection->M13 - p_pViewProjection->M10 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_RIGHT ].Normal.Z =
		-( p_pViewProjection->M23 - p_pViewProjection->M20 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_RIGHT ].Distance =
		-( p_pViewProjection->M33 - p_pViewProjection->M30 );

	p_pFrustum->Plane[ FRUSTUM_INDEX_TOP ].Normal.X =
		-( p_pViewProjection->M03 - p_pViewProjection->M01 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_TOP ].Normal.Y =
		-( p_pViewProjection->M13 - p_pViewProjection->M11 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_TOP ].Normal.Z =
		-( p_pViewProjection->M23 - p_pViewProjection->M21 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_TOP ].Distance =
		-( p_pViewProjection->M33 - p_pViewProjection->M31 );

	p_pFrustum->Plane[ FRUSTUM_INDEX_BOTTOM ].Normal.X =
		-( p_pViewProjection->M03 + p_pViewProjection->M01 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_BOTTOM ].Normal.Y =
		-( p_pViewProjection->M13 + p_pViewProjection->M11 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_BOTTOM ].Normal.Z =
		-( p_pViewProjection->M23 + p_pViewProjection->M21 );
	p_pFrustum->Plane[ FRUSTUM_INDEX_BOTTOM ].Distance =
		-( p_pViewProjection->M33 + p_pViewProjection->M31 );

	for( Index = 0; Index < 6; ++Index )
	{
		float Magnitude = VEC3_Magnitude( &p_pFrustum->Plane[ Index ].Normal );
		VEC3_Divide( &p_pFrustum->Plane[ Index ].Normal,
			&p_pFrustum->Plane[ Index ].Normal, Magnitude );
		p_pFrustum->Plane[ Index ].Distance /= Magnitude;
	}

	return 0;
}

FRUSTUM_CLASS FRUS_ClassifyAABB( PFRUSTUM p_pFrustum, PAABB p_pAABB )
{
	VECTOR3 Minimum, Maximum;
	bool Intersect = false;
	size_t i;

	/* Planes should have their normals pointing outward */
	for( i = 0; i < 6; ++i )
	{
		/* X */
		if( p_pFrustum->Plane[ i ].Normal.X >= 0.0f )
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
		if( p_pFrustum->Plane[ i ].Normal.Y >= 0.0f )
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
		if( p_pFrustum->Plane[ i ].Normal.Z >= 0.0f )
		{
			Minimum.Z = p_pAABB->Minimum.Z;
			Maximum.Z = p_pAABB->Maximum.Z;
		}
		else
		{
			Minimum.Z = p_pAABB->Maximum.Z;
			Maximum.Z = p_pAABB->Minimum.Z;
		}

		/* On the normal's side */
		if( VEC3_Dot( &p_pFrustum->Plane[ i ].Normal, &Minimum ) +
			p_pFrustum->Plane[ i ].Distance > 0.0f )
		{
			return FRUSTUM_CLASS_OUTSIDE;
		}

		/* Boundary crossing */
		if( VEC3_Dot( &p_pFrustum->Plane[ i ].Normal, &Maximum ) +
			p_pFrustum->Plane[ i ].Distance >= 0.0f )
		{
			Intersect = true;
		}
	}

	if( Intersect == true )
	{
		return FRUSTUM_CLASS_INTERSECT;
	}

	return FRUSTUM_CLASS_INSIDE;
}
