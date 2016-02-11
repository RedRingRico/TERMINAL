#include <Vector3.h>
#include <Arithmetic.h>
#include <mathf.h>
#include <SHC/umachine.h>
#include <SHC/private.h>

static float g_SH4Vector1[ 4 ];
static float g_SH4Vector2[ 4 ];

void VEC3_Add( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	p_pOut->X = p_pLeft->X + p_pRight->X;
	p_pOut->Y = p_pLeft->Y + p_pRight->Y;
	p_pOut->Z = p_pLeft->Z + p_pRight->Z;
}

void VEC3_Subtract( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	p_pOut->X = p_pLeft->X - p_pRight->X;
	p_pOut->Y = p_pLeft->Y - p_pRight->Y;
	p_pOut->Z = p_pLeft->Z - p_pRight->Z;
}

void VEC3_MultiplyV( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	p_pOut->X = p_pLeft->X * p_pRight->X;
	p_pOut->Y = p_pLeft->Y * p_pRight->Y;
	p_pOut->Z = p_pLeft->Z * p_pRight->Z;
}

void VEC3_MultiplyF( PVECTOR3 p_pOut, PVECTOR3 p_pVector, float p_Scalar )
{
	p_pOut->X = p_pVector->X * p_Scalar;
	p_pOut->Y = p_pVector->Y * p_Scalar;
	p_pOut->Z = p_pVector->Z * p_Scalar;
}

void VEC3_Divide( PVECTOR3 p_pOut, PVECTOR3 p_pVector, float p_Scalar )
{
	p_pOut->X = p_pVector->X / p_Scalar;
	p_pOut->Y = p_pVector->Y / p_Scalar;
	p_pOut->Z = p_pVector->Z / p_Scalar;
}

float VEC3_Magnitude( PVECTOR3 p_pVector )
{
	return sqrtf( ( p_pVector->X * p_pVector->X ) +
		( p_pVector->Y * p_pVector->Y ) +
		( p_pVector->Z * p_pVector->Z ) );
}

float VEC3_MagnitudeSq( PVECTOR3 p_pVector )
{
	return ( ( p_pVector->X * p_pVector->X ) +
		( p_pVector->Y * p_pVector->Y ) +
		( p_pVector->Z * p_pVector->Z ) );
}

float VEC3_Distance( PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	float X, Y, Z;

	X = p_pLeft->X - p_pRight->X;
	Y = p_pLeft->Y - p_pRight->Y;
	Z = p_pLeft->Z - p_pRight->Z;

	return sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
}

float VEC3_DistanceSq( PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	float X, Y, Z;

	X = p_pLeft->X - p_pRight->X;
	Y = p_pLeft->Y - p_pRight->Y;
	Z = p_pLeft->Z - p_pRight->Z;

	return ( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
}

void VEC3_Normalise( PVECTOR3 p_pVector )
{
	float Factor;
	float Squared;

	Squared = ( p_pVector->X * p_pVector->X ) +
		( p_pVector->Y * p_pVector->Y ) +
		( p_pVector->Z * p_pVector->Z );
	
	if( Squared == 0.0f )
	{
		return;
	}

	Factor = ( float )fsrra( Squared );

	p_pVector->X *= Factor;
	p_pVector->Y *= Factor;
	p_pVector->Z *= Factor;
}

float VEC3_Dot( PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	/* This may be faster if there was no copying involved */
	/*g_SH4Vector1[ 0 ] = p_pLeft->X;
	g_SH4Vector1[ 1 ] = p_pLeft->Y;
	g_SH4Vector1[ 2 ] = p_pLeft->Z;
	g_SH4Vector1[ 3 ] = 0.0f;

	g_SH4Vector2[ 0 ] = p_pRight->X;
	g_SH4Vector2[ 1 ] = p_pRight->Y;
	g_SH4Vector2[ 2 ] = p_pRight->Z;
	g_SH4Vector2[ 3 ] = 0.0f;

	return ( fipr( g_SH4Vector1, g_SH4Vector2 ) );*/

	return ( ( p_pLeft->X * p_pRight->X ) +
		( p_pLeft->Y * p_pRight->Y ) +
		( p_pLeft->Z * p_pRight->Z ) );
}

void VEC3_Cross( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight )
{
	p_pOut->X = ( p_pLeft->Y * p_pRight->Z ) - ( p_pLeft->Z * p_pRight->Y );
	p_pOut->Y = ( p_pLeft->Z * p_pRight->X ) - ( p_pLeft->X * p_pRight->Z );
	p_pOut->Z = ( p_pLeft->X * p_pRight->Y ) - ( p_pLeft->Y * p_pRight->X );
}

void VEC3_Clean( PVECTOR3 p_pVector )
{
	if( ARI_IsZero( p_pVector->X ) )
	{
		p_pVector->X = 0.0f;
	}
	if( ARI_IsZero( p_pVector->Y ) )
	{
		p_pVector->Y = 0.0f;
	}
	if( ARI_IsZero( p_pVector->Z ) )
	{
		p_pVector->Z = 0.0f;
	}
}

void VEC3_Zero( PVECTOR3 p_pVector )
{
	p_pVector->X = p_pVector->Y = p_pVector->Z = 0.0f;
}

