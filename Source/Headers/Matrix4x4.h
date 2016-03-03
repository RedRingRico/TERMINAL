#ifndef __TERMINAL_MATRIX4X4_H__
#define __TERMINAL_MATRIX4X4_H__

#include <shinobi.h>
#include <Vector3.h>
#include <kamui2.h>

typedef struct _tagMATRIX4X4
{
	float M00, M01, M02, M03;
	float M10, M11, M12, M13;
	float M20, M21, M22, M23;
	float M30, M31, M32, M33;
}MATRIX4X4,*PMATRIX4X4;

#include <Camera.h>

void MAT44_SetIdentity( PMATRIX4X4 p_pMatrix );

void MAT44_Multiply( PMATRIX4X4 p_pOut, const PMATRIX4X4 p_pLeft,
	const PMATRIX4X4 p_pRight );

void MAT44_Translate( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pTranslate );
void MAT44_SetAsScaleV3( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pScale );
void MAT44_SetAsScaleF( PMATRIX4X4 p_pOut, const float p_Scale );

void MAT44_RotateAxisAngle( PMATRIX4X4 p_pMatrix, const PVECTOR3 p_pAxis,
	const float p_Angle );

void MAT44_Inverse( PMATRIX4X4 p_pMatrix );

void MAT44_Copy( PMATRIX4X4 p_pMatrix, const PMATRIX4X4 p_pOriginal );

void MAT44_TransformVertices( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride,const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix );

void MAT44_TransformVerticesRHW( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride,const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix );

void MAT44_ClipVertices( KMVERTEX_05 *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_Stride );

#endif /* __TERMINAL_MATRIX4X4_H__ */

