#ifndef __TERMINAL_VECTOR3_H__
#define __TERMINAL_VECTOR3_H__

typedef struct _tagVECTOR3
{
	float X;
	float Y;
	float Z;
}VECTOR3,*PVECTOR3;

void VEC3_Add( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_Subtract( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_MultiplyV( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_MultiplyF( PVECTOR3 p_pOut, PVECTOR3 p_pVector, float p_Scalar );
void VEC3_Divide( PVECTOR3 p_pOut, PVECTOR3 p_pVector, float p_Scalar );
float VEC3_Magnitude( PVECTOR3 p_pVector );
float VEC3_MagnitudeSq( PVECTOR3 p_pVector );
float VEC3_Distance( PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
float VEC3_DistanceSq( PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_Normalise( PVECTOR3 p_pVector );
float VEC3_Dot( PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_Cross( PVECTOR3 p_pOut, PVECTOR3 p_pLeft, PVECTOR3 p_pRight );
void VEC3_Clean( PVECTOR3 p_pVector );
void VEC3_Zero( PVECTOR3 p_pVector );

#endif /* __TERMINAL_VECTOR3_H__ */

