#include <Matrix4x4.h>
#include <machine.h>
#include <SHC/private.h>
#include <Log.h>
#include <mathf.h>
#include <kamui2.h>

static float g_SH4Vector[ 4 ];
static float g_Result[ 4 ];

void MAT44_SetIdentity( PMATRIX4X4 p_pMatrix )
{
	p_pMatrix->M00 = 1.0f;
	p_pMatrix->M01 = p_pMatrix->M02 = p_pMatrix->M03 = p_pMatrix->M10 = 0.0f;
	p_pMatrix->M11 = 1.0f;
	p_pMatrix->M12 = p_pMatrix->M13 = p_pMatrix->M20 = p_pMatrix->M21 = 0.0f;
	p_pMatrix->M22 = 1.0f;
	p_pMatrix->M23 = p_pMatrix->M30 = p_pMatrix->M31 = p_pMatrix->M32 = 0.0f;
	p_pMatrix->M33 = 1.0f;
}

void MAT44_Multiply( PMATRIX4X4 p_pOut, const PMATRIX4X4 p_pLeft,
	const PMATRIX4X4 p_pRight )
{
	/* Load the matrix on the right to be multilied with */
	ld_ext( ( void * )p_pRight );

	mtrx4mul( ( void * )p_pLeft, ( void * )p_pOut );
}

void MAT44_Translate( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pTranslate )
{
	p_pMatrix->M30 = p_pTranslate->X;
	p_pMatrix->M31 = p_pTranslate->Y;
	p_pMatrix->M32 = p_pTranslate->Z;
}

void MAT44_SetAsScaleV3( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pScale )
{
	p_pMatrix->M00 = p_pScale->X;
	p_pMatrix->M11 = p_pScale->Y;
	p_pMatrix->M22 = p_pScale->Z;
	p_pMatrix->M33 = 1.0f;
}

void MAT44_RotateAxisAngle( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pAxis,
	const float p_Angle )
{
	MATRIX4X4 Rotation;
	float Sine, Cosine;
	unsigned short Radian;

	/* X */
	Radian = ( unsigned short )( ( ( p_pAxis->X * p_Angle ) * 65536.0f ) /
		( 2.0f * 3.1415926f ) );
	fsca( Radian, &Sine, &Cosine );

	Rotation.M00 = 1.0f;
	Rotation.M01 = 0.0f;
	Rotation.M02 = 0.0f;
	Rotation.M03 = 0.0f;

	Rotation.M10 = 0.0f;
	Rotation.M11 = Cosine;
	Rotation.M12 = Sine;
	Rotation.M13 = 0.0f;

	Rotation.M20 = 0.0f;
	Rotation.M21 = -Sine;
	Rotation.M22 = Cosine;
	Rotation.M23 = 0.0f;

	Rotation.M30 = 0.0f;
	Rotation.M31 = 0.0f;
	Rotation.M32 = 0.0f;
	Rotation.M33 = 1.0f;

	MAT44_Multiply( p_pMatrix, p_pMatrix, &Rotation );

	/* Y */
	Radian = ( unsigned short )( ( (p_pAxis->Y * p_Angle ) * 65536.0f ) /
		( 2.0f * 3.1415926f ) );
	fsca( Radian, &Sine, &Cosine );

	Rotation.M00 = Cosine;
	Rotation.M01 = 0.0f;
	Rotation.M02 = -Sine;
	Rotation.M03 = 0.0f;

	Rotation.M10 = 0.0f;
	Rotation.M11 = 1.0f;
	Rotation.M12 = 0.0f;
	Rotation.M13 = 0.0f;

	Rotation.M20 = Sine;
	Rotation.M21 = 0.0f;
	Rotation.M22 = Cosine;
	Rotation.M23 = 0.0f;

	Rotation.M30 = 0.0f;
	Rotation.M31 = 0.0f;
	Rotation.M32 = 0.0f;
	Rotation.M33 = 1.0f;

	MAT44_Multiply( p_pMatrix, p_pMatrix, &Rotation );

	/* Z */
	Radian = ( unsigned short )( ( ( p_pAxis->Z * p_Angle ) * 65536.0f ) /
		( 2.0f * 3.1415926f ) );
	fsca( Radian, &Sine, &Cosine );

	Rotation.M00 = Cosine;
	Rotation.M01 = Sine;
	Rotation.M02 = 0.0f;
	Rotation.M03 = 0.0f;

	Rotation.M10 = -Sine;
	Rotation.M11 = Cosine;
	Rotation.M12 = 0.0f;
	Rotation.M13 = 0.0f;

	Rotation.M20 = 0.0f;
	Rotation.M21 = 0.0f;
	Rotation.M22 = 1.0f;
	Rotation.M23 = 0.0f;

	Rotation.M30 = 0.0f;
	Rotation.M31 = 0.0f;
	Rotation.M32 = 0.0f;
	Rotation.M33 = 1.0f;

	MAT44_Multiply( p_pMatrix, p_pMatrix, &Rotation );
}

void MAT44_Inverse( PMATRIX4X4 p_pMatrix )
{
	MATRIX4X4 TempMatrix;
	float Determinate;
	float Positive, Negative, Temp;

	Positive = Negative = 0.0f;

	Temp = p_pMatrix->M00 * p_pMatrix->M11 * p_pMatrix->M22;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}
	Temp = p_pMatrix->M01 * p_pMatrix->M12 * p_pMatrix->M20;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}
	Temp = p_pMatrix->M02 * p_pMatrix->M10 * p_pMatrix->M21;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}
	Temp = -p_pMatrix->M02 * p_pMatrix->M11 * p_pMatrix->M20;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}
	Temp = -p_pMatrix->M01 * p_pMatrix->M10 * p_pMatrix->M22;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}
	Temp = -p_pMatrix->M00 * p_pMatrix->M12 * p_pMatrix->M21;
	if( Temp >= 0.0f )
	{
		Positive += Temp;
	}
	else
	{
		Negative += Temp;
	}

	Determinate = Positive + Negative;

	if( ARI_IsZero( Determinate ) ||
		ARI_IsZero( Determinate / ( Positive - Negative ) ) )
	{
		LOG_Debug( "No inverse" );
		return;
	}

	Determinate = 1.0f / Determinate;

	TempMatrix.M00 = ( ( p_pMatrix->M11 * p_pMatrix->M22 ) -
		( p_pMatrix->M12 * p_pMatrix->M21 ) ) * Determinate;
	TempMatrix.M10 = -( ( p_pMatrix->M10 * p_pMatrix->M22 ) -
		( p_pMatrix->M12 * p_pMatrix->M20 ) ) * Determinate;
	TempMatrix.M20 = ( ( p_pMatrix->M10 * p_pMatrix->M21 ) -
		( p_pMatrix->M11 * p_pMatrix->M20 ) ) * Determinate;

	TempMatrix.M01 = -( ( p_pMatrix->M01 * p_pMatrix->M22 ) -
		( p_pMatrix->M02 * p_pMatrix->M21 ) ) * Determinate;
	TempMatrix.M11 = ( ( p_pMatrix->M00 * p_pMatrix->M22 ) -
		( p_pMatrix->M02 * p_pMatrix->M20 ) ) * Determinate;
	TempMatrix.M21 = -( ( p_pMatrix->M00 * p_pMatrix->M21 ) -
		( p_pMatrix->M01 * p_pMatrix->M20 ) ) * Determinate;
	
	TempMatrix.M02 = ( ( p_pMatrix->M01 * p_pMatrix->M12 ) -
		( p_pMatrix->M02 * p_pMatrix->M11 ) ) * Determinate;
	TempMatrix.M12 = -( ( p_pMatrix->M00 * p_pMatrix->M12 ) -
		( p_pMatrix->M02 *  p_pMatrix->M10 ) ) * Determinate;
	TempMatrix.M22 = ( ( p_pMatrix->M00 * p_pMatrix->M11 ) -
		( p_pMatrix->M01 * p_pMatrix->M10 ) ) * Determinate;

	TempMatrix.M30 = -( p_pMatrix->M30 * TempMatrix.M00 +
		p_pMatrix->M31 * TempMatrix.M10 +
		p_pMatrix->M32 * TempMatrix.M20 );
	TempMatrix.M31 = -( p_pMatrix->M30 * TempMatrix.M01 +
		p_pMatrix->M31 * TempMatrix.M11 +
		p_pMatrix->M32 * TempMatrix.M21 );
	TempMatrix.M32 = -( p_pMatrix->M30 * TempMatrix.M02 +
		p_pMatrix->M31 * TempMatrix.M12 +
		p_pMatrix->M32 * TempMatrix.M22 );

	TempMatrix.M03 = TempMatrix.M13 = TempMatrix.M23 = 0.0f;
	TempMatrix.M33 = 1.0f;

	MAT44_Copy( p_pMatrix, &TempMatrix );
}

void MAT44_Copy( PMATRIX4X4 p_pMatrix, const PMATRIX4X4 p_pOriginal )
{
	memcpy( p_pMatrix, p_pOriginal, sizeof( MATRIX4X4 ) );
}

void MAT44_TransformVertices( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride,const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix )
{
	register float *pDestVector;
	size_t Index, UStrideGap, TStrideGap;

#if defined ( DEBUG )
	if( ( p_UntransformedStride < ( sizeof( float ) * 3 ) ) ||
		( p_TransformedStride < ( sizeof( float ) * 3 ) ) )
	{
		return;
	}
#endif /* DEBUG */

	UStrideGap = ( p_UntransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );
	TStrideGap = ( p_TransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );

	ld_ext( ( void * )p_pMatrix );

	pDestVector = p_pTransformedVertices;

	g_SH4Vector[ 3 ] = 1.0f;

	for( Index = 0; Index < p_VertexCount; ++Index )
	{
		g_SH4Vector[ 0 ] = *( p_pVertices++ );
		g_SH4Vector[ 1 ] = *( p_pVertices++ );
		g_SH4Vector[ 2 ] = *( p_pVertices++ );

		p_pVertices += UStrideGap;

		ftrv( ( float * )g_SH4Vector, ( float * )g_Result );

		*( pDestVector++ ) = g_Result[ 0 ];
		*( pDestVector++ ) = g_Result[ 1 ];
		*( pDestVector++ ) = g_Result[ 2 ];

		pDestVector += TStrideGap;
	}
}

void MAT44_TransformVerticesRHW( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride, const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix )
{
	float *pDestVector, RHW;
	size_t Index, UStrideGap, TStrideGap;

#if defined ( DEBUG )
	if( ( p_UntransformedStride < ( sizeof( float ) * 3 ) ) ||
		( p_TransformedStride < ( sizeof( float ) * 3 ) ) )
	{
		return;
	}
#endif /* DEBUG */

	UStrideGap = ( p_UntransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );
	TStrideGap = ( p_TransformedStride - ( sizeof( float ) * 3 ) ) /
		sizeof( float );

	ld_ext( ( void * )p_pMatrix );

	pDestVector = p_pTransformedVertices;

	g_SH4Vector[ 3 ] = 1.0f;

	for( Index = 0; Index < p_VertexCount; ++Index )
	{
		g_SH4Vector[ 0 ] = *( p_pVertices++ );
		g_SH4Vector[ 1 ] = *( p_pVertices++ );
		g_SH4Vector[ 2 ] = *( p_pVertices++ );

		p_pVertices += UStrideGap;

		ftrv( ( float * )g_SH4Vector, ( float * )g_Result );

		RHW = 1.0f / g_Result[ 2 ];

		*( pDestVector++ ) = RHW * g_Result[ 0 ];
		*( pDestVector++ ) = RHW * g_Result[ 1 ];
		*( pDestVector++ ) = RHW;

		pDestVector += TStrideGap;
	}
}

void MAT44_ClipVertices( KMVERTEX_05 *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_Stride )
{
	Uint32 P1, P2, P3;
	float H;
	size_t VisibleTriangles;
	size_t i = 0;

	//H = tanf( p_pCamera->FieldOfView / 2.0f ) * p_pCamera->NearPlane;
	H = tanf( ( 3.141592654f / 4.0f ) / 2.0f ) * 1.0f;

	//VisibleTriangles = p_TriangleCount;

	/*while( i < VisibleTriangles )
	{
		P1 = p_pVertices[ i * 3 ];
		P2 = p_pVertices[ 
	}*/

	/*p_pTransformedVertices[ 0 ].u.fZ = H;
	p_pTransformedVertices[ 1 ].u.fZ = H;
	p_pTransformedVertices[ 3 ].u.fZ = H;*/

	/*for( i = 0; i < p_VertexCount; ++i )
	{
		if( p_pTransformedVertices[ i ].u.fZ <=  H )
		{
			p_pTransformedVertices[ i ].u.fZ = H;
		}
	}*/
}

