#include <Arithmetic.h>
#include <mathf.h>
#include <machine.h>
#include <SHC/private.h>

bool ARI_IsZero( const float p_Value )
{
	if( fabsf( p_Value ) < ARI_EPSILON )
	{
		return true;
	}

	return false;
}

float ARI_Tangent( const float p_Angle )
{
	float Cosine, Sine;
	unsigned short Radian;

	Radian = ( unsigned short )( ( p_Angle * 65536.0f ) /
		( 2.0f * 3.1415926f ) );
	fsca( p_Angle, &Sine, &Cosine );

	if( ARI_IsZero( Cosine ) )
	{
		/* Avoid divide by zero */
		Cosine = 1.0e-8;
	}

	return ( Sine / Cosine );
}

