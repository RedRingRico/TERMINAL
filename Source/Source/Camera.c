#include <Camera.h>
#include <Vector3.h>
#include <mathf.h>

static float g_SH4Vector[ 4 ];
static float g_Result[ 4 ];

void CAM_CalculateViewMatrix( PMATRIX4X4 p_pMatrix, const PCAMERA p_pCamera )
{
	VECTOR3 Up, Right, Direction;

	MAT44_SetIdentity( p_pMatrix );

	VEC3_Subtract( &Direction, &p_pCamera->LookAt, &p_pCamera->Position );
	VEC3_Normalise( &Direction );

	VEC3_Cross( &Right, &p_pCamera->WorldUp, &Direction );
	VEC3_Normalise( &Right );

	VEC3_Cross( &Up, &Direction, &Right );
	VEC3_Normalise( &Up );

	/* R U D 0
	 * R U D 0
	 * R U D 0
	 * P P P 1 */
	p_pMatrix->M00 = Right.X;
	p_pMatrix->M01 = Up.X;
	p_pMatrix->M02 = Direction.X;

	p_pMatrix->M10 = Right.Y;
	p_pMatrix->M11 = Up.Y;
	p_pMatrix->M12 = Direction.Y;

	p_pMatrix->M20 = Right.Z;
	p_pMatrix->M21 = Up.Z;
	p_pMatrix->M22 = Direction.Z;

	p_pMatrix->M30 = -VEC3_Dot( &Right, &p_pCamera->Position );
	p_pMatrix->M31 = -VEC3_Dot( &Up, &p_pCamera->Position );
	p_pMatrix->M32 = -VEC3_Dot( &Direction, &p_pCamera->Position );
}

void CAM_CalculateProjectionMatrix( PMATRIX4X4 p_pMatrix,
	const PCAMERA p_pCamera )
{
	register float FOV2, Q;

	FOV2 = cosf( p_pCamera->FieldOfView / 2.0f );
	FOV2 = FOV2 / sinf( p_pCamera->FieldOfView / 2.0f );
	Q = p_pCamera->FarPlane / ( p_pCamera->FarPlane - p_pCamera->NearPlane );

	MAT44_SetIdentity( p_pMatrix );

	p_pMatrix->M00 = ( 1.0f / p_pCamera->AspectRatio ) * FOV2;
	p_pMatrix->M11 = FOV2;
	p_pMatrix->M22 = Q;
	p_pMatrix->M32 = -Q * p_pCamera->NearPlane;
	p_pMatrix->M23 = 1.0f;
	p_pMatrix->M33 = 0.0f;
}

void CAM_CalculateScreenMatrix( PMATRIX4X4 p_pMatrix,
	const PCAMERA p_pCamera )
{
	register float HalfWidth, HalfHeight;

	HalfWidth = p_pCamera->GateWidth * 0.5f;
	HalfHeight = p_pCamera->GateHeight * 0.5f;

	MAT44_SetIdentity( p_pMatrix );

	p_pMatrix->M00 = HalfWidth;
	p_pMatrix->M11 = -HalfHeight;
	p_pMatrix->M20 = HalfWidth;
	p_pMatrix->M21 = HalfHeight;
}

void CAM_TransformNonClipPerspective( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride,const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix, const CAMERA *p_pCamera )
{
	register float H, *pDestVector, HalfWidth, HalfHeight, RHW;
	register int Index;
	size_t UStride, TStride;
	MATRIX4X4 Transform, Projection;

#if defined ( DEBUG )
	if( ( p_UntransformedStride < ( sizeof( float ) * 3 ) ) ||
		( p_TransformedStride < ( sizeof( float ) * 3 ) ) )
	{
		return;
	}
#endif /* DEBUG */

	HalfWidth = p_pCamera->GateWidth / 2.0f;
	HalfHeight = p_pCamera->GateHeight / 2.0f;

	UStride = ( p_UntransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );

	TStride = ( p_TransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );

	H = tanf( p_pCamera->FieldOfView / 2.0f );

	MAT44_SetIdentity( &Projection );

	Projection.M00 = 1.0f/ ( p_pCamera->AspectRatio * H );
	Projection.M11 = 1.0f / H;
	Projection.M22 = p_pCamera->FarPlane /
		( p_pCamera->FarPlane - p_pCamera->NearPlane );
	Projection.M32 = p_pCamera->FarPlane * -p_pCamera->NearPlane /
		( p_pCamera->FarPlane - p_pCamera->NearPlane );
	Projection.M23 = 1.0f;
	Projection.M33 = 0.0f;

	MAT44_Multiply( &Transform, p_pMatrix, &Projection );

	ld_ext( ( void * )&Transform );

	pDestVector = p_pTransformedVertices;

	g_SH4Vector[ 3 ] = 1.0f;

	for( Index = p_VertexCount; Index > 0; --Index )
	{
		g_SH4Vector[ 0 ] = *( p_pVertices++ );
		g_SH4Vector[ 1 ] = *( p_pVertices++ );
		g_SH4Vector[ 2 ] = *( p_pVertices++ );

		p_pVertices += UStride;

		ftrv( ( float * )g_SH4Vector, ( float * )g_Result );

		RHW = 1.0f / g_Result[ 2 ];

		*( pDestVector++ ) = RHW * HalfWidth * g_Result[ 0 ] + HalfWidth;
		*( pDestVector++ ) = RHW * -HalfHeight * g_Result[ 1 ] + HalfHeight;
		*( pDestVector++ ) = RHW;

		pDestVector += TStride;
	}
}

