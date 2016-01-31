#ifndef __TERMINAL_RENDERER_H__
#define __TERMINAL_RENDERER_H__

#include <kamui2.h>

typedef struct _tagDREAMCAST_RENDERERCONFIGURATION
{
	PPKMSURFACEDESC		ppSurfaceDescription;
	KMUINT32			FramebufferCount;
	KMUINT32			TextureMemorySize;
	KMUINT32			MaximumTextureCount;
	KMUINT32			MaximumSmallVQTextureCount;
	PKMDWORD			pTextureWorkArea;
	PKMDWORD			pVertexBuffer;
	PKMVERTEXBUFFDESC	pVertexBufferDesc;
	KMUINT32			VertexBufferSize;
	KMUINT32			PassCount;
	KMPASSINFO			PassInfo[ KM_MAX_DISPLAY_LIST_PASS ];
}DREAMCAST_RENDERERCONFIGURATION;

int REN_Initialise( DREAMCAST_RENDERERCONFIGURATION *p_pConfiguration );
void REN_Terminate( void );

void REN_SetClearColour( float p_Red, float p_Green, float p_Blue );

int REN_Clear( void );
int REN_SwapBuffers( void );

void REN_DrawPrimitives16( PKMSTRIPHEAD p_pStripHead, PKMVERTEX_16 p_pVertices,
	KMUINT32 p_Count );

#endif /* __TERMINAL_RENDERER_H__ */

