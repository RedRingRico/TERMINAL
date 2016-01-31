#ifndef __TERMINAL_TEXT_H__
#define __TERMINAL_TEXT_H__

#include <shinobi.h>
#include <kamui2.h>

typedef struct _tagGLYPH
{
	Uint16	X;
	Uint16	Y;
	Uint16	Width;
	Uint16	Height;
	Uint16	XOffset;
	Uint16	YOffset;
	Uint16	XAdvance;
}GLYPH;

typedef struct _tagGLYPHSET
{
	KMSURFACEDESC	Texture;
	GLYPH			Glyphs[ 256 ];
	Uint16			LineHeight;
	Uint16			BaseLine;
	Uint16			Width;
	Uint16			Height;
}GLYPHSET;

int TEX_Initialise( void );

int TEX_CreateGlyphSetFromFile( char *p_pFileName,
	GLYPHSET *p_pGlyphSet );
int TEX_SetTextureForGlyphSet( char *p_pFileName,
	GLYPHSET *p_pGlyphSet );

void TEX_RenderString( GLYPHSET *p_pGlyphSet, float p_X, float p_Y,
	char *p_pString );

void TEX_MeasureString( GLYPHSET *p_pGlyphSet, char *p_pString,
	float *p_pWidth );

#endif /* __TERMINAL_TEXT_H__ */

