#ifndef __TERMINAL_RENDERER_H__
#define __TERMINAL_RENDERER_H__

#include <kamui2.h>
#include <shinobi.h>

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

typedef struct _tagRENDERER
{
	Uint32			VisiblePolygons;
	Uint32			CulledPolygons;
	PKMVERTEX_05	pVertices05;
}RENDERER,*PRENDERER;

int REN_Initialise( PRENDERER p_pRenderer,
	const DREAMCAST_RENDERERCONFIGURATION *p_pConfiguration );
void REN_Terminate( void );

void REN_SetClearColour( float p_Red, float p_Green, float p_Blue );

int REN_Clear( void );
int REN_SwapBuffers( void );

void REN_DrawPrimitives00( PKMSTRIPHEAD p_pStripHead, PKMVERTEX_00 p_pVertices,
	KMUINT32 p_Count );
void REN_DrawPrimitives01( PKMSTRIPHEAD p_pStripHead, PKMVERTEX_01 p_pVertices,
	KMUINT32 p_Count );
void REN_DrawPrimitives05( PKMSTRIPHEAD p_pStripHead, PKMVERTEX_05 p_pVertices,
	KMUINT32 p_Count);
void REN_DrawPrimitives16( PKMSTRIPHEAD p_pStripHead, PKMVERTEX_16 p_pVertices,
	KMUINT32 p_Count );

void REN_DrawPrimitives05Cached( PRENDERER p_pRenderer,
	PKMSTRIPHEAD p_pStripHead, KMUINT32 p_Offset, KMUINT32 p_Count );

#endif /* __TERMINAL_RENDERER_H__ */

