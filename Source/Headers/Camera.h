#ifndef __TERMINAL_CAMERA_H__
#define __TERMINAL_CAMERA_H__

#include <Matrix4x4.h>
#include <Vector3.h>

typedef struct _tagCAMERA
{
	MATRIX4X4	ViewMatrix;
	MATRIX4X4	ProjectionMatrix;
	VECTOR3		Position;
	VECTOR3		LookAt;
	VECTOR3		WorldUp;
	float		GateWidth;
	float		GateHeight;
	float		AspectRatio;
	float		FieldOfView;
	float		NearPlane;
	float		FarPlane;
}CAMERA,*PCAMERA;

void CAM_CalculateViewMatrix( PMATRIX4X4 p_pMatrix, const PCAMERA p_pCamera );
void CAM_CalculateProjectionMatrix( PMATRIX4X4 p_pMatrix,
	const PCAMERA p_pCamrea );

void CAM_TransformNonClipPerspective( float *p_pTransformedVertices,
	const float *p_pVertices, const size_t p_VertexCount,
	const size_t p_TransformedStride,const size_t p_UntransformedStride,
	const PMATRIX4X4 p_pMatrix, const CAMERA *p_pCamera );

#endif /* __TERMINAL_CAMERA_H__ */

