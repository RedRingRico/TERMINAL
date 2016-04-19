#ifndef __TERMINAL_TEXTURE_H__
#define __TERMINAL_TEXTURE_H__

#include <Memory.h>
#include <shinobi.h>
#include <kamui2.h>

typedef struct _tagTEXTURE
{
	KMINT32			Width;
	KMINT32			Height;
	KMTEXTURETYPE	Flags;
	KMSURFACEDESC	SurfaceDescription;
	Uint32			ReferenceCount;
}TEXTURE,*PTEXTURE;

int TEX_LoadTexture( PTEXTURE p_pTexture, const char *p_pFileName,
	PMEMORY_BLOCK p_pMemoryBlock );
void TEX_DeleteTexture( PTEXTURE p_pTexture );

#endif /* __TERMINAL_TEXTURE_H__ */

