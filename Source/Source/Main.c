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
#include <Vector3.h>
#include <mathf.h>

#define MAX_TEXTURES ( 4096 )
#define MAX_SMALLVQ ( 0 )

#pragma aligndata32( g_pTextureWorkArea )
KMDWORD g_pTextureWorkArea[ MAX_TEXTURES * 24 / 4 + MAX_SMALLVQ * 76 / 4 ];

#define MILESTONE_STRING "Milestone 0 - Technical Debt"

float CameraFocus = 1500.0f;

unsigned char RecvBuf[ 1024 * 8 ];
unsigned char SendBuf[ 1024 * 8 ];

typedef struct _tagRENDER_VERTEX
{
	float		X;
	float		Y;
	float		Z;
	float		InvW;
	KMUINT32	Colour;
}RENDER_VERTEX;

typedef struct _tagVERTEX
{
	VECTOR3 Position;
	VECTOR3 Normal;
}VERTEX;

void Perspective( const VERTEX *p_pVertex, RENDER_VERTEX *p_pRenderVertex );
bool SelectPALRefresh( GLYPHSET *p_pGlyphSet );

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
	SYE_CBL AVCable;

	/*RENDER_VERTEX Cube[ 36 ];
	VERTEX CubeVerts[ 36 ];
	KMVERTEX_01 CubeKMVerts[ 36 ];
	VECTOR3 LightPosition = { 10.0f, 10.0f, 9.0f };*/

	if( HW_Initialise( KM_DSPBPP_RGB888, &AVCable ) != 0 )
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

	scif_init( RecvBuf, 1024 * 8, SendBuf, 1024 * 8 );
	scif_open( BPS_115200 );
	scif_putq( '[' );
	scif_putq( 'T' );
	scif_putq( 'E' );
	scif_putq( 'R' );
	scif_putq( 'M' );
	scif_putq( 'I' );
	scif_putq( 'N' );
	scif_putq( 'A' );
	scif_putq( 'L' );
	scif_putq( ']' );
	scif_putq( 0x1B );
	scif_putq( '[' );
	scif_putq( '2' );
	scif_putq( ';' );
	scif_putq( '1' );
	scif_putq( 'f' );

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

	scif_close( );

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

	/* Oh, fun -_- */
	/*CubeVerts[ 0 ].Position.X = -10.0f;
	CubeVerts[ 0 ].Position.Y = -10.0f;
	CubeVerts[ 0 ].Position.Z = 10.0f;
	CubeVerts[ 0 ].Normal.X = -0.5773491849436035f;
	CubeVerts[ 0 ].Normal.Y = -0.5773491849436035f;
	CubeVerts[ 0 ].Normal.Z = 0.5773491849436035f;

	CubeVerts[ 1 ].Position.X = -10.0f;
	CubeVerts[ 1 ].Position.Y = 10.0f;
	CubeVerts[ 1 ].Position.Z = 10.0f;
	CubeVerts[ 1 ].Normal.X = -0.5773491849436035f;
	CubeVerts[ 1 ].Normal.Y = 0.5773491849436035f;
	CubeVerts[ 1 ].Normal.Z = 0.5773491849436035f;

	CubeVerts[ 2 ].Position.X = 10.0f;
	CubeVerts[ 2 ].Position.Y = -10.0f;
	CubeVerts[ 2 ].Position.Z = 10.0f;
	CubeVerts[ 2 ].Normal.X = 0.5773491849436035f;
	CubeVerts[ 2 ].Normal.Y = -0.5773491849436035f;
	CubeVerts[ 2 ].Normal.Z = 0.5773491849436035f;

	CubeVerts[ 3 ].Position.X = 10.0f;
	CubeVerts[ 3 ].Position.Y = 10.0f;
	CubeVerts[ 3 ].Position.Z = 10.0f;
	CubeVerts[ 3 ].Normal.X = 0.5773491849436035f;
	CubeVerts[ 3 ].Normal.Y = 0.5773491849436035f;
	CubeVerts[ 3 ].Normal.Z = 0.5773491849436035f;

	{
		int i;
		for( i = 0; i < 4; ++i )
		{
			VECTOR3 Colour = { 1.0f, 1.0f, 1.0f };
			VECTOR3 LightColour = { 1.0f, 1.0f, 1.0f };
			VECTOR3 DiffuseLight;
			VECTOR3 LightNormal;
			VERTEX LightPos;
			RENDER_VERTEX LightPosRender;
			float LightIntensity;

			Perspective( &CubeVerts[ i ], &Cube[ i ] );
			LightPos.Position.X = LightPosition.X;
			LightPos.Position.Y = LightPosition.Y;
			LightPos.Position.Z = LightPosition.Z;
			Perspective( &LightPos, &LightPosRender );
			LightPosition.X = LightPosRender.X;
			LightPosition.Y = LightPosRender.Y;
			LightPosition.Z = LightPosRender.Z;
			VEC3_Subtract( &LightPosition, &CubeVerts[ i ].Position,
				&LightNormal );
			VEC3_Normalise( &LightNormal );
			Cube[ i ].Colour = 0xFFFFFFFF;
			LightIntensity = VEC3_Dot(
				&CubeVerts[ i ].Normal, &LightNormal );
			if( LightIntensity < 0.0f )
			{
				LightIntensity = 0.0f;
			}
			VEC3_MultiplyV( &Colour, &LightColour, &DiffuseLight );
			VEC3_MultiplyF( &DiffuseLight, LightIntensity, &DiffuseLight );

			CubeKMVerts[ i ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
			CubeKMVerts[ i ].fX = Cube[ i ].X;
			CubeKMVerts[ i ].fY = Cube[ i ].Y;
			CubeKMVerts[ i ].u.fInvW = Cube[ i ].InvW;
			CubeKMVerts[ i ].fBaseAlpha = 1.0f;
			CubeKMVerts[ i ].fBaseRed = DiffuseLight.X;
			CubeKMVerts[ i ].fBaseGreen = DiffuseLight.Y;
			CubeKMVerts[ i ].fBaseBlue = DiffuseLight.Z;
		}

		CubeKMVerts[ 3 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
	}*/

	if( AVCable == SYE_CBL_PAL )
	{
		bool SixtyHz;
		SixtyHz = SelectPALRefresh( &GlyphSet );

		if( SixtyHz )
		{
			kmSetDisplayMode( KM_DSPMODE_NTSCNI640x480, KM_DSPBPP_RGB888,
				TRUE, FALSE);
		}
	}

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

		REN_Clear( );

		/*REN_DrawPrimitives01( NULL, CubeKMVerts, 4 );*/

		TextColour.dwPacked = 0xFF00FF00;
		TEX_RenderString( &GlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 4.0f ), "[TERMINAL]" );
		TEX_MeasureString( &GlyphSet, VersionString, &TextLength );
		TEX_RenderString( &GlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			480.0f - ( ( float )GlyphSet.LineHeight * 3.0f ), VersionString );

		TextColour.dwPacked = 0xFFFFFFFF;

		TEX_MeasureString( &GlyphSet, MILESTONE_STRING, &TextLength );
		TEX_RenderString( &GlyphSet, &TextColour, 0.0f,
			( float )GlyphSet.LineHeight * 2.0f,
			MILESTONE_STRING );

		if( Alpha < 0.0f )
		{
			AlphaByte = 0;
		}
		else if( Alpha > 1.0f )
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
			Alpha = 0.0f;
		}
		
		if( Alpha >= 1.0f )
		{
			AlphaInc = -0.02f;
			Alpha = 1.0f;
		}
	}

	LOG_Debug( "Rebooting" );

	REN_Terminate( );
	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );
}

void Perspective( const VERTEX *p_pVertex, RENDER_VERTEX *p_pRenderVertex )
{
	const VECTOR3 CameraPosition = { 0.0f, 0.0f, 150.0f };
	float X, Y, Z;

	X = p_pVertex->Position.X + CameraPosition.X;
	Y = -p_pVertex->Position.Y + CameraPosition.Y;
	Z = p_pVertex->Position.Z + CameraPosition.Z;

	p_pRenderVertex->X = ( CameraFocus * X ) / Z + 320.0f;
	p_pRenderVertex->Y = ( CameraFocus * Y ) / Z + 240.0f;
	p_pRenderVertex->InvW = 1.0f / Z;
}

bool SelectPALRefresh( GLYPHSET *p_pGlyphSet )
{
	bool CableSelect = true;
	bool SixtyHz = false;
	float Spacing = 50.0f;
	bool ModeTest = false;
	int TestStage = 0;
	Uint32 StartTime, EndTime;
	Uint32 ElapsedTime;
	bool TestPass = false;
	bool ModeSelected = false;

	while( CableSelect )
	{
		static float Alpha = 1.0f;
		static float AlphaInc = 0.02f;
		KMBYTE AlphaByte;
		float TextLength;
		KMPACKEDARGB TextColour;

		if( ModeSelected )
		{
			CableSelect = false;
			continue;
		}

		StartTime = syTmrGetCount( );

		if( ( g_Peripherals[ 0 ].press & PDD_DGT_TA ) &&
			( ModeTest == false ) )
		{
			ModeTest = true;
			TestStage = 0;
		}

		REN_Clear( );

		TextColour.dwPacked = 0xFFFFFFFF;

		if( ModeTest )
		{
			if( SixtyHz )
			{
				if( TestStage == 0 )
				{
					TEX_MeasureString( p_pGlyphSet, "The display will change "
						"mode now for five seconds", &TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						( float )p_pGlyphSet->LineHeight * 4.0f,
						"The display will change mode now for five seconds" );

					TEX_MeasureString( p_pGlyphSet, "Press 'A' to continue",
						&TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float )p_pGlyphSet->LineHeight / 2.0f ),
						"Press 'A' to continue" );

					if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
					{
						ElapsedTime = 0UL;
						TestStage = 1;

						if( SixtyHz )
						{
							kmSetDisplayMode( KM_DSPMODE_NTSCNI640x480,
								KM_DSPBPP_RGB888, TRUE, FALSE);
						}
						else
						{
							kmSetDisplayMode( KM_DSPMODE_PALNI640x480EXT,
								KM_DSPBPP_RGB888, TRUE, FALSE);
						}
					}
				}
				/* Display test */
				else if( TestStage == 1 )
				{
					char TimeLeft[ 80 ];
					memset( TimeLeft, '\0', sizeof( TimeLeft ) );
					sprintf( TimeLeft, "Time remaining: %ld",
						5000000UL - ElapsedTime );
					TEX_MeasureString( p_pGlyphSet,
						"Time remaining: 0000000", &TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float ) p_pGlyphSet->LineHeight * 6.0f ),
						TimeLeft );

					TEX_MeasureString( p_pGlyphSet,
						"Some fucking cool artwork", &TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float )p_pGlyphSet->LineHeight / 2.0f ),
						"Some fucking cool artwork" );

					if( ElapsedTime >= 5000000UL )
					{
						TestStage = 2;

						kmSetDisplayMode( KM_DSPMODE_PALNI640x480EXT,
							KM_DSPBPP_RGB888, TRUE, FALSE);
					}
				}
				/* Confirm test */
				else if( TestStage == 2 )
				{
					TEX_MeasureString( p_pGlyphSet, "Did the picture appear "
						"stable?", &TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						( float )p_pGlyphSet->LineHeight * 4.0f,
						"Did the picture appear stable?" );

					TEX_MeasureString( p_pGlyphSet, "Press 'A' to confirm",
						&TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						( float )p_pGlyphSet->LineHeight * 5.5f,
						"Press 'A' to confirm" );

					if( ( g_Peripherals[ 0 ].press & PDD_DGT_KL ) ||
						( g_Peripherals[ 0 ].press & PDD_DGT_KR ) )
					{
						TestPass = !TestPass;
						Alpha = 1.0f;
					}

					if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
					{
						ModeTest = false;
						ModeSelected = true;
					}

					if( TestPass )
					{
						TextColour.byte.bAlpha = AlphaByte;
					}
					else
					{
						TextColour.byte.bAlpha = 255;
					}

					TEX_MeasureString( p_pGlyphSet, "Yes", &TextLength );
					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength ) - Spacing,
						( 240.0f - ( float )p_pGlyphSet->LineHeight / 2.0f ),
						"Yes" );

					if( !TestPass )
					{
						TextColour.byte.bAlpha = AlphaByte;
					}
					else
					{
						TextColour.byte.bAlpha = 255;
					}

					TEX_RenderString( p_pGlyphSet, &TextColour,
						320.0f + Spacing,
						( 240.0f - ( float )p_pGlyphSet->LineHeight / 2.0f ),
						"No" );
				}
			}
			else
			{
				ModeSelected = true;
			}
		}
		else
		{
			if( ( g_Peripherals[ 0 ].press & PDD_DGT_KL ) ||
				( g_Peripherals[ 0 ].press & PDD_DGT_KR ) )
			{
				SixtyHz = !SixtyHz;
				Alpha = 1.0f;
			}

			TEX_MeasureString( p_pGlyphSet, "Select output mode",
				&TextLength );
			TEX_RenderString( p_pGlyphSet, &TextColour,
				320.0f - ( TextLength / 2.0f ),
				( float )p_pGlyphSet->LineHeight * 4.0f,
				"Select output mode" );

			TEX_MeasureString( p_pGlyphSet, "Press 'A' to select",
				&TextLength );
			TEX_RenderString( p_pGlyphSet, &TextColour,
				320.0f - ( TextLength / 2.0f ),
				( 480.0f - ( float )p_pGlyphSet->LineHeight * 4.0f ),
				"Press 'A' to select" );

			if( !SixtyHz )
			{
				TextColour.byte.bAlpha = AlphaByte;
			}
			else
			{
				TextColour.byte.bAlpha = 255;
			}

			TEX_MeasureString( p_pGlyphSet, "50Hz", &TextLength );
			TEX_RenderString( p_pGlyphSet, &TextColour,
				320.0f - ( TextLength ) - Spacing,
				( 240.0f - ( float )p_pGlyphSet->LineHeight / 2.0f ),
				"50Hz" );

			if( SixtyHz )
			{
				TextColour.byte.bAlpha = AlphaByte;
			}
			else
			{
				TextColour.byte.bAlpha = 255;
			}

			TEX_RenderString( p_pGlyphSet, &TextColour,
				320.0f + Spacing,
				( 240.0f - ( float )p_pGlyphSet->LineHeight / 2.0f ),
				"60Hz" );
		}

		Alpha += AlphaInc;

		if( Alpha <= 0.0f )
		{
			AlphaInc = 0.02f;
			Alpha = 0.0f;
			AlphaByte = 0;
		}
		else if( Alpha >= 1.0f )
		{
			AlphaInc = -0.02f;
			Alpha = 1.0f;
			AlphaByte = 255;
		}
		else
		{
			AlphaByte = ( KMBYTE )( Alpha * 255.0f );
		}

		REN_SwapBuffers( );

		EndTime = syTmrGetCount( );

		ElapsedTime +=
			syTmrCountToMicro( syTmrDiffCount( StartTime, EndTime ) );
	}

	if( TestPass && SixtyHz )
	{
		return true;
	}

	return false;
}

