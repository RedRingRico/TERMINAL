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
		"System Root" ) == 0 )
	{
#if defined ( DEBUG )
		MEM_ListMemoryBlocks( &MemoryBlock );

		pBlock1 = MEM_AllocateFromBlock( &MemoryBlock, 1024*1024, "Block 1" );
		MEM_ListMemoryBlocks( &MemoryBlock );

		pBlock2 = MEM_AllocateFromBlock( &MemoryBlock, 1024*1024, "Block 2" );
		MEM_ListMemoryBlocks( &MemoryBlock );

		MEM_FreeFromBlock( &MemoryBlock, pBlock1 );
		MEM_ListMemoryBlocks( &MemoryBlock );

		MEM_FreeFromBlock( &MemoryBlock, pBlock2 );
		MEM_ListMemoryBlocks( &MemoryBlock );

		MEM_GarbageCollectMemoryBlock( &MemoryBlock );
		MEM_ListMemoryBlocks( &MemoryBlock );
#endif /* DEBUG */
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

	if( TEX_CreateGlyphSetFromFile( "/FONTS/TERMINAL.FNT", &GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph descriptions" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TEX_SetTextureForGlyphSet( "/FONTS/TERMINAL.PVR", &GlyphSet ) != 0 )
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
		float TextLength;
		if( g_Peripherals[ 0 ].press & PDD_DGT_ST )
		{
			Run = 0;
		}

		REN_Clear( );
		TEX_MeasureString( &GlyphSet, "[TERMINAL]", &TextLength );
		TEX_RenderString( &GlyphSet, 320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 4.0f ), "[TERMINAL]" );
		TEX_MeasureString( &GlyphSet, VersionString, &TextLength );
		TEX_RenderString( &GlyphSet, 320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 3.0f ), VersionString );
		REN_SwapBuffers( );
	}

	LOG_Debug( "Rebooting" );

	REN_Terminate( );
	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );
}

