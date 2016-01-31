#include <Text.h>
#include <FileSystem.h>
#include <Log.h>
#include <string.h>

KMSTRIPHEAD	TEX_StripHead;
KMVERTEX_16 TEX_TextBuffer[ 64 ];

int TEX_Initialise( void )
{
	KMPACKEDARGB BaseColour;
	KMSTRIPCONTEXT TextContext;

	memset( &TextContext, 0, sizeof( KMSTRIPCONTEXT ) );

	TextContext.nSize = sizeof( TextContext );
	TextContext.StripControl.nListType = KM_TRANS_POLYGON;
	TextContext.StripControl.nUserClipMode = KM_USERCLIP_DISABLE;
	TextContext.StripControl.nShadowMode = KM_NORMAL_POLYGON;
	TextContext.StripControl.bOffset = KM_FALSE;
	TextContext.StripControl.bGouraud = KM_TRUE;
	TextContext.ObjectControl.nDepthCompare = KM_ALWAYS;
	TextContext.ObjectControl.nCullingMode = KM_NOCULLING;
	TextContext.ObjectControl.bZWriteDisable = KM_FALSE;
	TextContext.ObjectControl.bDCalcControl = KM_FALSE;
	BaseColour.dwPacked = 0xFF00FF00;
	TextContext.type.splite.Base = BaseColour;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nSRCBlendingMode = KM_SRCALPHA;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nDSTBlendingMode =
		KM_INVSRCALPHA;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bSRCSelect = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bDSTSelect = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFogMode = KM_NOFOG;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bColorClamp = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bUseAlpha = KM_TRUE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bIgnoreTextureAlpha = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFlipUV = KM_NOFLIP;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nClampUV = KM_CLAMP_UV;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFilterMode = KM_BILINEAR;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bSuperSampleMode = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].dwMipmapAdjust = 
		KM_MIPMAP_D_ADJUST_1_00;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nTextureShadingMode =
		KM_MODULATE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].pTextureSurfaceDesc = NULL;

	kmGenerateStripHead16( &TEX_StripHead, &TextContext );

	return 0;
}

int TEX_CreateGlyphSetFromFile( char *p_pFileName, GLYPHSET *p_pGlyphSet )
{
	GDFS FileHandle;
	long FileBlocks;
	char *pFileContents;
	char *pLine;
	char Token[ 256 ];
	char *pLineChar;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open glyph file" );

		return 1;
	}

	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	pFileContents = syMalloc( FileBlocks * 2048 );

	if( gdFsReqRd32( FileHandle, FileBlocks, pFileContents ) < 0 )
	{
		LOG_Debug( "Could not load the glyph file into memory" );

		return 1;
	}

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	pLine = strtok( pFileContents, "\n" );

	while( pLine )
	{
		char *pEnd = NULL;
		char *pStart = pLine;
		size_t TokenLength = 0;
		char Key[ 32 ], Value[ 32 ];

		if( ( pEnd = strchr( pStart, ' ' ) ) )
		{
			TokenLength = ( size_t )pEnd - ( size_t )pStart;
			Token[ TokenLength ] = '\0';
			strncpy( Token, pStart, TokenLength );

			if( strcmp( Token, "common" ) == 0 )
			{
				pStart = pEnd + 1;

				while( pEnd = strchr( pStart, ' ' ) )
				{
					char *pNewEnd;

					TokenLength = ( size_t )pEnd - ( size_t )pStart;
					Token[ TokenLength ] = '\0';
					strncpy( Token, pStart, TokenLength );

					if( ( pNewEnd = strchr( Token, '=' ) ) == NULL )
					{
						break;
					}

					Key[ pNewEnd - Token ] = '\0';
					strncpy( Key, Token, pNewEnd - Token );

					Value[ ( size_t )pNewEnd - 1 ] = '\0';
					strncpy( Value, pNewEnd + 1, strlen( pNewEnd - 1 ) );

					if( strcmp( Key, "lineHeight" ) == 0 )
					{
						p_pGlyphSet->LineHeight = atoi( Value );
					}
					else if( strcmp( Key, "base" ) == 0 )
					{
						p_pGlyphSet->BaseLine = atoi( Value );
					}
					else if( strcmp( Key, "scaleW" ) == 0 )
					{
						p_pGlyphSet->Width = atoi( Value );
					}
					else if( strcmp( Key, "scaleH" ) == 0 )
					{
						p_pGlyphSet->Height = atoi( Value );
					}

					pStart = pEnd + 1;
				}
			}
			else if( strcmp( Token, "char" ) == 0 )
			{
				Uint16 Char = 0;
				pStart = pEnd + 1;

				while( pEnd = strchr( pStart, ' ' ) )
				{
					char *pNewEnd;

					TokenLength = ( size_t )pEnd - ( size_t )pStart;

					/* Space was encountered as first char, length will be
					 * zero, add one to advance */
					if( TokenLength == 0 )
					{
						TokenLength = 1;
					}

					Token[ TokenLength ] = '\0';
					strncpy( Token, pStart, TokenLength );

					if( ( pNewEnd = strchr( Token, '=' ) ) == NULL )
					{
						pStart = pEnd + 1;
						continue;
					}

					Key[ pNewEnd - Token ] = '\0';
					strncpy( Key, Token, pNewEnd - Token );

					Value[ ( size_t )pNewEnd - 1 ] = '\0';
					strncpy( Value, pNewEnd + 1, strlen( pNewEnd - 1 ) );

					/* id is usually first, the rest will not work without
					 * it */
					if( strcmp( Key, "id" ) == 0 )
					{
						Char = atoi( Value );
					}
					else if( strcmp( Key, "x" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].X = atoi( Value );
					}
					else if( strcmp( Key, "y" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].Y = atoi( Value );
					}
					else if( strcmp( Key, "width" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].Width = atoi( Value );
					}
					else if( strcmp( Key, "height" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].Height = atoi( Value );
					}
					else if( strcmp( Key, "xoffset" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].XOffset = atoi( Value );
					}
					else if( strcmp( Key, "yoffset" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].YOffset = atoi( Value );
					}
					else if( strcmp( Key, "xadvance" ) == 0 )
					{
						p_pGlyphSet->Glyphs[ Char ].XAdvance = atoi( Value );
					}

					pStart = pEnd + 1;
				}
			}
		}

		pLine = strtok( NULL, "\n" );
	}	

	syFree( pFileContents );

	return 0;
}

int TEX_SetTextureForGlyphSet( char *p_pFileName, GLYPHSET *p_pGlyphSet )
{
	GDFS FileHandle;
	long FileBlocks;
	PKMDWORD pTexture;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open texture for the glyph set" );

		return 1;
	}

	gdFsGetFileSctSize( FileHandle, &FileBlocks );
	pTexture = syMalloc( FileBlocks * 2048 );

	if( gdFsReqRd32( FileHandle, FileBlocks, pTexture ) < 0 )
	{
		LOG_Debug( "Failed to load the glyph set texture into memory" );

		return 1;
	}

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	kmCreateTextureSurface( &p_pGlyphSet->Texture, p_pGlyphSet->Width,
		p_pGlyphSet->Height,
		( KM_TEXTURE_TWIDDLED | KM_TEXTURE_ARGB4444 ) );

	/* Add 16 bytes to skip the PVRT header */
	if( kmLoadTexture( &( p_pGlyphSet )->Texture, pTexture + 4 ) !=
		KMSTATUS_SUCCESS )
	{
		syFree( pTexture );

		LOG_Debug( "Failed to load the glyph texture into video memory" );

		return 1;
	}

	syFree( pTexture );

	return 0;
}

void TEX_RenderString( GLYPHSET *p_pGlyphSet, float p_X, float p_Y,
	char *p_pString )
{
	size_t StringLength;
	size_t Char;
	float XPos = p_X, YPos = p_Y;
	union
	{
		KMFLOAT F[ 2 ];
		KMWORD	F16[ 4 ];
	}F;
	size_t IndexChar;

	StringLength = strlen( p_pString );

	kmChangeStripTextureSurface( &TEX_StripHead, KM_IMAGE_PARAM1,
		&( p_pGlyphSet )->Texture );

	for( Char = 0; Char < StringLength - 1; ++Char )
	{
		IndexChar = p_pString[ Char ] < 0 ?
			( p_pString[ Char ] & 0x7F ) + 128 : p_pString[ Char ];

		TEX_TextBuffer[ Char ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
		TEX_TextBuffer[ Char ].fAX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fAY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uA.fAZ = 256.0f;
		TEX_TextBuffer[ Char ].fBX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fBY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uB.fBZ = 256.0f;
		TEX_TextBuffer[ Char ].fCX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fCY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uC.fCZ = 256.0f;
		TEX_TextBuffer[ Char ].fDX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fDY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;

		F.F[ 1 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].X /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVA = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVB = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height ) /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVC = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		XPos += ( float )p_pGlyphSet->Glyphs[ IndexChar ].XAdvance;
	}

	IndexChar = p_pString[ Char ] < 0 ? p_pString[ Char ] + 128 :
		p_pString[ Char ];

	TEX_TextBuffer[ Char ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
	TEX_TextBuffer[ Char ].fAX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fAY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uA.fAZ = 256.0f;
	TEX_TextBuffer[ Char ].fBX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fBY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uB.fBZ = 256.0f;
	TEX_TextBuffer[ Char ].fCX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fCY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uC.fCZ = 256.0f;
	TEX_TextBuffer[ Char ].fDX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fDY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;

	F.F[ 1 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].X /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVA = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVB = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height ) /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVC = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	REN_DrawPrimitives16( &TEX_StripHead, TEX_TextBuffer, StringLength );
}

void TEX_MeasureString( GLYPHSET *p_pGlyphSet, char *p_pString,
	float *p_pWidth )
{
	size_t StringLength;
	size_t Char;

	( *p_pWidth ) = 0.0f;

	StringLength = strlen( p_pString );

	for( Char = 0; Char < StringLength; ++Char )
	{
		size_t IndexChar = p_pString[ Char ] < 0 ? p_pString[ Char ] + 128 :
			p_pString[ Char ];

		( *p_pWidth ) +=
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XAdvance;
	}
}

