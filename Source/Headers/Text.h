#ifndef __TERMINAL_TEXT_H__
#define __TERMINAL_TEXT_H__

#include <shinobi.h>
#include <kamui2.h>
#include <Texture.h>

typedef struct _tagGLYPH
{
	Uint16	X;
	Uint16	Y;
	Uint16	Width;
	Uint16	Height;
	Sint16	XOffset;
	Sint16	YOffset;
	Sint16	XAdvance;
}GLYPH,*PGLYPH;

typedef struct _tagGLYPHSET
{
	/*KMSURFACEDESC	Texture;*/
	TEXTURE			Texture;
	GLYPH			Glyphs[ 256 ];
	Uint16			LineHeight;
	Uint16			BaseLine;
	Uint16			Width;
	Uint16			Height;
}GLYPHSET,*PGLYPHSET;

int TXT_Initialise( void );

int TXT_CreateGlyphSetFromFile( char *p_pFileName,
	GLYPHSET *p_pGlyphSet );
int TXT_SetTextureForGlyphSet( char *p_pFileName,
	GLYPHSET *p_pGlyphSet );

void TXT_RenderString( GLYPHSET *p_pGlyphSet, KMPACKEDARGB *p_pColour,
	float p_X, float p_Y, char *p_pString );

void TXT_MeasureString( GLYPHSET *p_pGlyphSet, char *p_pString,
	float *p_pWidth );

#endif /* __TERMINAL_TEXT_H__ */

