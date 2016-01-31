#include <GitVersion.h>
#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <string.h>
#include <Log.h>
#include <Hardware.h>
#include <Peripheral.h>
#include <Renderer.h>
#include <Memory.h>
#include <Text.h>
#include <kamui2.h>
#include <sh4scif.h>

#define MAX_TEXTURES ( 4096 )
#define MAX_SMALLVQ ( 0 )

#pragma aligndata32( g_pTextureWorkArea )
KMDWORD g_pTextureWorkArea[ MAX_TEXTURES * 24 / 4 + MAX_SMALLVQ * 76 / 4 ];

void main( void )
{
	int Run = 1;
	DREAMCAST_RENDERERCONFIGURATION RendererConfiguration;
	PKMSURFACEDESC Framebuffer[ 2 ];
	KMSURFACEDESC FrontBuffer, BackBuffer;
	PKMDWORD pVertexBuffer;
	KMVERTEXBUFFDESC VertexBufferDesc;
	MEMORY_BLOCK MemoryBlock;
	void *pSomeMemory, *pBlock1, *pBlock2;
	GLYPHSET GlyphSet;
	char VersionString[ 256 ];
	Uint32 StartTime, EndTime;

	if( HW_Initialise( KM_DSPBPP_RGB888 ) != 0 )
	{
		HW_Terminate( );
		HW_Reboot( );
	}

	LOG_Initialise( NULL );
	LOG_Debug( "TERMINAL" );
	LOG_Debug( "Version: %s", GIT_VERSION );

	pSomeMemory = syMalloc( 1024*1024*8 );
	if( MEM_InitialiseMemoryBlock( &MemoryBlock, pSomeMemory, 1024*1024*8, 4,
		"System Root" ) != 0 )
	{
		LOG_Debug( "Could not allocate %ld bytes of memory", 1024*1024*8 );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	pVertexBuffer = ( PKMDWORD )syMalloc( 0x100000 );

	memset( &RendererConfiguration, 0, sizeof( RendererConfiguration ) );

	Framebuffer[ 0 ] = &FrontBuffer;
	Framebuffer[ 1 ] = &BackBuffer;

	RendererConfiguration.ppSurfaceDescription = Framebuffer;
	RendererConfiguration.FramebufferCount = 2;
	RendererConfiguration.TextureMemorySize = 1024 * 1024 * 5;
	RendererConfiguration.MaximumTextureCount = 4096;
	RendererConfiguration.MaximumSmallVQTextureCount = 0;
	RendererConfiguration.pTextureWorkArea = g_pTextureWorkArea;
	RendererConfiguration.pVertexBuffer = pVertexBuffer;
	RendererConfiguration.pVertexBufferDesc = &VertexBufferDesc;
	RendererConfiguration.VertexBufferSize = 0x100000;
	RendererConfiguration.PassCount = 1;

	RendererConfiguration.PassInfo[ 0 ].dwRegionArrayFlag =
		KM_PASSINFO_AUTOSORT;
	RendererConfiguration.PassInfo[ 0 ].nDirectTransferList =
		KM_OPAQUE_POLYGON;
	RendererConfiguration.PassInfo[ 0 ].fBufferSize[ 0 ] = 0.0f;
	RendererConfiguration.PassInfo[ 0 ].fBufferSize[ 1 ] = 0.0f;
	RendererConfiguration.PassInfo[ 0 ].fBufferSize[ 2 ] = 50.0f;
	RendererConfiguration.PassInfo[ 0 ].fBufferSize[ 3 ] = 0.0f;
	RendererConfiguration.PassInfo[ 0 ].fBufferSize[ 4 ] = 50.0f;

	REN_Initialise( &RendererConfiguration );

	if( TEX_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the text system" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TEX_CreateGlyphSetFromFile( "/FONTS/WHITERABBIT.FNT",
		&GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph descriptions" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TEX_SetTextureForGlyphSet( "/FONTS/WHITERABBIT.PVR", &GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph texture" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	memset( VersionString, '\0', sizeof( VersionString ) );

	sprintf( VersionString, "Build: %s", GIT_VERSION );

	REN_SetClearColour( 0.0f, 17.0f / 255.0f, 43.0f / 255.0f );

	while( Run )
	{
		static float Alpha = 1.0f;
		static float AlphaInc = 0.02f;
		KMBYTE AlphaByte;
		float TextLength;
		KMPACKEDARGB TextColour;
		if( g_Peripherals[ 0 ].press & PDD_DGT_ST )
		{
			Run = 0;
		}

		TextColour.dwPacked = 0xFF00FF00;

		REN_Clear( );
		TEX_MeasureString( &GlyphSet, "[TERMINAL]", &TextLength );
		TEX_RenderString( &GlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 4.0f ), "[TERMINAL]" );
		TEX_MeasureString( &GlyphSet, VersionString, &TextLength );
		TEX_RenderString( &GlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 3.0f ), VersionString );

		TextColour.dwPacked = 0x00FFFFFF;
		if( Alpha < 0.0f )
		{
			AlphaByte = 0;
		}
		else if( Alpha > 255.0f )
		{
			AlphaByte = 255;
		}
		else
		{
			AlphaByte = ( KMBYTE )( Alpha * 255.0f );
		}

		TextColour.byte.bAlpha = AlphaByte;
		TEX_MeasureString( &GlyphSet, "PRESS START", &TextLength );
		TEX_RenderString( &GlyphSet, &TextColour,
			320.0f -( TextLength / 2.0f ),
			240.0f - ( ( float )GlyphSet.LineHeight / 2.0f ), "PRESS START" );
		REN_SwapBuffers( );

		Alpha += AlphaInc;

		if( Alpha <= 0.0f )
		{
			AlphaInc = 0.02f;
		}
		
		if( Alpha >= 1.0f )
		{
			AlphaInc = -0.02f;
		}
	}

	LOG_Debug( "Rebooting" );

	REN_Terminate( );
	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );
}

