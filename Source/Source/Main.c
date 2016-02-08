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
#include <Camera.h>
#include <Model.h>

#define MAX_TEXTURES ( 4096 )
#define MAX_SMALLVQ ( 0 )

#pragma aligndata32( g_pTextureWorkArea )
KMDWORD g_pTextureWorkArea[ MAX_TEXTURES * 24 / 4 + MAX_SMALLVQ * 76 / 4 ];

#define MILESTONE_STRING "Milestone 0 - Technical Debt"

float CameraFocus = 1500.0f;

unsigned char RecvBuf[ 1024 * 8 ];
unsigned char SendBuf[ 1024 * 8 ];

char g_VersionString[ 256 ];
Sint8 g_ConsoleID[ SYD_CFG_IID_SIZE + 1 ];
char g_ConsoleIDPrint[ ( SYD_CFG_IID_SIZE * 2 ) + 1 ];

typedef struct _tagRENDER_VERTEX
{
	float		X;
	float		Y;
	float		InvW;
}RENDER_VERTEX;

typedef struct _tagVERTEX
{
	VECTOR3 Position;
	VECTOR3 Normal;
}VERTEX;

bool SelectPALRefresh( GLYPHSET *p_pGlyphSet );
float TestAspectRatio( GLYPHSET *p_pGlyphSet );
void DrawOverlayText( GLYPHSET *p_pGlyphSet );

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
	Uint32 StartTime, EndTime;
	SYE_CBL AVCable;
	Uint32 ElapsedTime = 0UL;
	Uint32 FPS = 0UL;
	Uint32 FPSTimer = 0UL;
	Uint16 FPSCounter = 0U;
	Uint32 TimeDifference = 0UL;
	Uint32 RenderStartTime = 0UL, RenderEndTime = 0UL;
	char PrintBuffer[ 80 ];

	CAMERA TestCamera;
	float YRotation = 0.0f;
	MATRIX4X4 Projection;
	MODEL TestModel;

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

	memset( g_VersionString, '\0', sizeof( g_VersionString ) );
	sprintf( g_VersionString, "[TERMINAL] | %s | %s", GIT_VERSION,
		GIT_TAGNAME );

	scif_init( RecvBuf, 1024 * 8, SendBuf, 1024 * 8 );
	/* The perfect speed for a cool peripheral */
	scif_open( BPS_19200 );

	/* Clear the terminal */
	scif_putq( 0x1B );
	scif_putq( '[' );
	scif_putq( '2' );
	scif_putq( 'J' );

	/* Test the output */
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

	if( TXT_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the text system" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TXT_CreateGlyphSetFromFile( "/FONTS/WHITERABBIT.FNT",
		&GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph descriptions" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TXT_SetTextureForGlyphSet( "/FONTS/WHITERABBIT.PVR", &GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph texture" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	memset( g_ConsoleIDPrint, '\0', sizeof( g_ConsoleIDPrint ) );
	if( syCfgGetIndividualID( g_ConsoleID ) != SYD_CFG_IID_OK )
	{
		sprintf( g_ConsoleIDPrint, "ERROR" );
	}
	g_ConsoleID[ SYD_CFG_IID_SIZE ] = '\0';

	sprintf( g_ConsoleIDPrint, "%02X%02X%02X%02X%02X%02X",
		( unsigned char )g_ConsoleID[ 0 ],
		( unsigned char )g_ConsoleID[ 1 ],
		( unsigned char )g_ConsoleID[ 2 ],
		( unsigned char )g_ConsoleID[ 3 ],
		( unsigned char )g_ConsoleID[ 4 ],
		( unsigned char )g_ConsoleID[ 5 ] );

	LOG_Debug( "Console ID: %s\n", g_ConsoleIDPrint );

	REN_SetClearColour( 0.0f, 17.0f / 255.0f, 43.0f / 255.0f );

	TestCamera.Position.X = 0.0f;
	TestCamera.Position.Y = 0.0f;
	TestCamera.Position.Z = 0.0f;

	TestCamera.LookAt.X = 0.0f;
	TestCamera.LookAt.Y = 0.0f;
	TestCamera.LookAt.Z = 1.0f;

	TestCamera.WorldUp.X = 0.0f;
	TestCamera.WorldUp.Y = 1.0f;
	TestCamera.WorldUp.Z = 0.0f;

	TestCamera.GateWidth = 640.0f;
	TestCamera.GateHeight = 480.0f;

	TestCamera.AspectRatio = 16.0f / 9.0f;
	TestCamera.FieldOfView = ( 3.141592654f / 4.0f );
	TestCamera.NearPlane = 1.0f;
	TestCamera.FarPlane = 100000.0f;

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

	TestCamera.AspectRatio = TestAspectRatio( &GlyphSet );

	if( MDL_LoadModel( &TestModel, "/MODELS/CUBE.TML" ) != 0 )
	{
		LOG_Debug( "Faile dto load the test model" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	CAM_CalculateProjectionMatrix( &Projection, &TestCamera );

	g_Peripherals[ 0 ].press = 0;

	StartTime = syTmrGetCount( );

	while( Run )
	{
		static float Alpha = 1.0f;
		static float AlphaInc = 0.02f;
		KMBYTE AlphaByte;
		float TextLength;
		KMPACKEDARGB TextColour;
		Uint32 UpdateTime = 0UL;
		static Uint32 RenderTime = 0UL;

		StartTime = syTmrGetCount( );

		if( g_Peripherals[ 0 ].press & PDD_DGT_ST )
		{
			Run = 0;
		}

		UpdateTime = syTmrGetCount( );
		UpdateTime =
			syTmrCountToMicro( syTmrDiffCount( StartTime, UpdateTime ) );

		REN_Clear( );

		RenderStartTime = syTmrGetCount( );

		{
			int i;
			MATRIX4X4 View, ViewProjection;
			MATRIX4X4 World;
			VECTOR3 LightWorldPos;
			VECTOR3 LightPosition = { 15.0f, 15.0f, -15.0f };
			VECTOR3 CubeRotate = { 0.0f, 1.0f, 1.0f };
			VECTOR3 CubePosition = { 0.0f, 0.0f, 100.0f };
			MATRIX4X4 Rotation;

			MAT44_SetIdentity( &World );
			MAT44_SetIdentity( &Rotation );

			MAT44_RotateAxisAngle( &World, &CubeRotate, YRotation );
			MAT44_RotateAxisAngle( &Rotation, &CubeRotate, YRotation );
			MAT44_Translate( &World, &CubePosition );

			CAM_CalculateViewMatrix( &View, &TestCamera );
			MAT44_Multiply( &ViewProjection, &World, &View );
			MAT44_Multiply( &ViewProjection, &ViewProjection, &Projection );

			MDL_RenderModel( &TestModel, &ViewProjection );

			/*MAT44_TransformVerticesRHW( ( float * )Cube, ( float * )CubeVerts,
				10, sizeof( Cube[ 0 ] ), sizeof( CubeVerts[ 0 ] ),
				&ViewProjection );

			MAT44_TransformVertices( ( float * )( TNormals ),
				( float * )( CubeVerts )+3, 10, sizeof( TNormals[ 0 ] ),
				sizeof( CubeVerts[ 0 ] ), &Rotation );

			for( i = 0; i < 10; ++i )
			{
				VECTOR3 Colour = { 1.0f, 1.0f, 1.0f };
				VECTOR3 LightColour = { 1.0f, 1.0f, 1.0f };
				VECTOR3 DiffuseLight;
				VECTOR3 LightNormal;
				VERTEX LightPos;
				RENDER_VERTEX LightPosRender;
				float LightIntensity;

				VEC3_Subtract( &LightNormal, &LightPosition,
					&CubeVerts[ i ].Position );
				VEC3_Normalise( &LightNormal );
				LightIntensity = VEC3_Dot( &TNormals[ i ], &LightNormal );
				if( LightIntensity < 0.0f )
				{
					LightIntensity = 0.0f;
				}
				VEC3_MultiplyV( &DiffuseLight, &Colour, &LightColour );
				VEC3_MultiplyF( &DiffuseLight, &DiffuseLight, LightIntensity );

				CubeKMVerts[ i ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
				CubeKMVerts[ i ].fX = Cube[ i ].X;
				CubeKMVerts[ i ].fY = Cube[ i ].Y;
				CubeKMVerts[ i ].u.fInvW = Cube[ i ].InvW;
				CubeKMVerts[ i ].fBaseAlpha = 1.0f;
				
				CubeKMVerts[ i ].fBaseRed = DiffuseLight.X;
				CubeKMVerts[ i ].fBaseGreen = DiffuseLight.Y;
				CubeKMVerts[ i ].fBaseBlue = DiffuseLight.Z;
			}

			CubeKMVerts[ 9 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;*/
		}

		//REN_DrawPrimitives01( NULL, CubeKMVerts, 10 );

		DrawOverlayText( &GlyphSet );

		TextColour.dwPacked = 0xFFFFFFFF;

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
		TXT_MeasureString( &GlyphSet, "PRESS START", &TextLength );
		TXT_RenderString( &GlyphSet, &TextColour,
			320.0f -( TextLength / 2.0f ),
			360.0f, "PRESS START" );

		sprintf( PrintBuffer, "%lu [%lu]", FPS, TimeDifference );

		if( FPS >= 40 )
		{
			TextColour.dwPacked = 0xFF00FF00;
		}
		else if( FPS >= 15 )
		{
			TextColour.dwPacked = 0xFFFFFF00;
		}
		else
		{
			TextColour.dwPacked = 0xFFFF0000;
		}

		TXT_MeasureString( &GlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( &GlyphSet, &TextColour, 640.0f - TextLength,
			480.0f - ( float )GlyphSet.LineHeight * 3.0f, PrintBuffer );

		TextColour.dwPacked = 0xFFFFFFFF;
		sprintf( PrintBuffer, "%lu | %lu", UpdateTime, RenderTime );
		TXT_MeasureString( &GlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( &GlyphSet, &TextColour, 640.0f - TextLength,
			480.0f - ( float )GlyphSet.LineHeight * 4.0f, PrintBuffer );

		RenderEndTime = syTmrGetCount( );

		REN_SwapBuffers( );

		RenderTime = syTmrCountToMicro(
			syTmrDiffCount( RenderStartTime, RenderEndTime ) );

		++FPSCounter;

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

		YRotation += 0.01f;

		EndTime = syTmrGetCount( );

		TimeDifference =
			syTmrCountToMicro( syTmrDiffCount( StartTime, EndTime ) );

		ElapsedTime += TimeDifference;
		FPSTimer += TimeDifference;

		if( FPSTimer >= 1000000UL )
		{
			FPSTimer = 0UL;
			FPS = FPSCounter;
			FPSCounter = 0UL;
		}
	}

	MDL_DeleteModel( &TestModel );

	LOG_Debug( "Rebooting" );

	REN_Terminate( );
	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );
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
					TXT_MeasureString( p_pGlyphSet, "The display will change "
						"mode now for five seconds", &TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						( float )p_pGlyphSet->LineHeight * 4.0f,
						"The display will change mode now for five seconds" );

					TXT_MeasureString( p_pGlyphSet, "Press 'A' to continue",
						&TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
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
					TXT_MeasureString( p_pGlyphSet,
						"Time remaining: 0000000", &TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						240.0f - ( ( float ) p_pGlyphSet->LineHeight * 6.0f ),
						TimeLeft );

					TXT_MeasureString( p_pGlyphSet,
						"Some fucking cool artwork", &TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
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
					TXT_MeasureString( p_pGlyphSet, "Did the picture appear "
						"stable?", &TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
						320.0f - ( TextLength / 2.0f ),
						( float )p_pGlyphSet->LineHeight * 4.0f,
						"Did the picture appear stable?" );

					TXT_MeasureString( p_pGlyphSet, "Press 'A' to confirm",
						&TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
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

					TXT_MeasureString( p_pGlyphSet, "Yes", &TextLength );
					TXT_RenderString( p_pGlyphSet, &TextColour,
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

					TXT_RenderString( p_pGlyphSet, &TextColour,
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

			TXT_MeasureString( p_pGlyphSet, "Select output mode",
				&TextLength );
			TXT_RenderString( p_pGlyphSet, &TextColour,
				320.0f - ( TextLength / 2.0f ),
				( float )p_pGlyphSet->LineHeight * 4.0f,
				"Select output mode" );

			TXT_MeasureString( p_pGlyphSet, "Press 'A' to select",
				&TextLength );
			TXT_RenderString( p_pGlyphSet, &TextColour,
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

			TXT_MeasureString( p_pGlyphSet, "50Hz", &TextLength );
			TXT_RenderString( p_pGlyphSet, &TextColour,
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

			TXT_RenderString( p_pGlyphSet, &TextColour,
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

		DrawOverlayText( p_pGlyphSet );

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

float TestAspectRatio( GLYPHSET *p_pGlyphSet )
{
	float AspectRatio = 4.0f / 3.0f;
	float TextLength;
	bool SelectAspect = true;
	bool FourThree = true;
	CAMERA AspectCamera;
	KMVERTEX_01 Square[ 4 ];
	VECTOR3 SquareVertsT[ 4 ];
	VECTOR3 SquareVerts[ 4 ];
	MATRIX4X4 Projection, View, ViewProjection, World;
	VECTOR3 Translate = { 0.0f, 0.0f, 100.0f };
	int i;
	float Alpha = 1.0f;
	float AlphaInc = 0.02f;
	KMBYTE AlphaByte;
	KMPACKEDARGB TextColour;
	float Spacing = 50.0f;
	KMPACKEDARGB BaseColour;
	KMSTRIPCONTEXT SquareContext;
	KMSTRIPHEAD	SquareStripHead;
	bool AButtonHeld = false;

	memset( &SquareContext, 0, sizeof( KMSTRIPCONTEXT ) );

	SquareContext.nSize = sizeof( SquareContext );
	SquareContext.StripControl.nListType = KM_TRANS_POLYGON;
	SquareContext.StripControl.nUserClipMode = KM_USERCLIP_DISABLE;
	SquareContext.StripControl.nShadowMode = KM_NORMAL_POLYGON;
	SquareContext.StripControl.bOffset = KM_FALSE;
	SquareContext.StripControl.bGouraud = KM_TRUE;
	SquareContext.ObjectControl.nDepthCompare = KM_ALWAYS;
	SquareContext.ObjectControl.nCullingMode = KM_NOCULLING;
	SquareContext.ObjectControl.bZWriteDisable = KM_FALSE;
	SquareContext.ObjectControl.bDCalcControl = KM_FALSE;
	BaseColour.dwPacked = 0xFFFFFFFF;
	SquareContext.type.splite.Base = BaseColour;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nSRCBlendingMode =
		KM_SRCALPHA;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nDSTBlendingMode =
		KM_INVSRCALPHA;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bSRCSelect = KM_FALSE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bDSTSelect = KM_FALSE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nFogMode = KM_NOFOG;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bColorClamp = KM_FALSE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bUseAlpha = KM_TRUE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bIgnoreTextureAlpha =
		KM_FALSE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nFlipUV = KM_NOFLIP;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nClampUV = KM_CLAMP_UV;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nFilterMode = KM_BILINEAR;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].bSuperSampleMode = KM_FALSE;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].dwMipmapAdjust = 
		KM_MIPMAP_D_ADJUST_1_00;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].nTextureShadingMode =
		KM_MODULATE_ALPHA;
	SquareContext.ImageControl[ KM_IMAGE_PARAM1 ].pTextureSurfaceDesc = NULL;

	kmGenerateStripHead01( &SquareStripHead, &SquareContext );

	SquareVerts[ 0 ].X = -10.0f;
	SquareVerts[ 0 ].Y = -10.0f;
	SquareVerts[ 0 ].Z = -10.0f;

	SquareVerts[ 1 ].X = -10.0f;
	SquareVerts[ 1 ].Y = 10.0f;
	SquareVerts[ 1 ].Z = -10.0f;

	SquareVerts[ 2 ].X = 10.0f;
	SquareVerts[ 2 ].Y = -10.0f;
	SquareVerts[ 2 ].Z = -10.0f;

	SquareVerts[ 3 ].X = 10.0f;
	SquareVerts[ 3 ].Y = 10.0f;
	SquareVerts[ 3 ].Z = -10.0f;

	MAT44_SetIdentity( &World );
	MAT44_Translate( &World, &Translate );

	AspectCamera.Position.X = 0.0f;
	AspectCamera.Position.Y = 0.0f;
	AspectCamera.Position.Z = 0.0f;

	AspectCamera.LookAt.X = 0.0f;
	AspectCamera.LookAt.Y = 0.0f;
	AspectCamera.LookAt.Z = 1.0f;

	AspectCamera.WorldUp.X = 0.0f;
	AspectCamera.WorldUp.Y = 1.0f;
	AspectCamera.WorldUp.Z = 0.0f;

	AspectCamera.GateWidth = 640.0f;
	AspectCamera.GateHeight = 480.0f;

	AspectCamera.AspectRatio = AspectRatio;
	AspectCamera.FieldOfView = ( 3.141592654f / 4.0f );
	AspectCamera.NearPlane = 1.0f;
	AspectCamera.FarPlane = 100000.0f;
	
	CAM_CalculateProjectionMatrix( &Projection, &AspectCamera );

	TextColour.byte.bBlue = 255;
	TextColour.byte.bGreen = 254;
	TextColour.byte.bRed = 83;

	/* Seems pretty unclean to me, but on PAL-60 select, there's no way around
	 * it */
	if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
	{
		AButtonHeld = true;
	}

	while( SelectAspect )
	{
		if( !AButtonHeld )
		{
			if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
			{
				SelectAspect = false;
			}
		}
		else
		{
			if( g_Peripherals[ 0 ].release & PDD_DGT_TA )
			{
				AButtonHeld = false;
			}
		}

		if( ( g_Peripherals[ 0 ].press & PDD_DGT_KL ) ||
			( g_Peripherals[ 0 ].press & PDD_DGT_KR ) )
		{
			FourThree = !FourThree;
			Alpha = 1.0f;
		}

		TextColour.byte.bAlpha = 140;

		REN_Clear( );

		TXT_MeasureString( p_pGlyphSet, "Select aspect ratio",
			&TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			( float )p_pGlyphSet->LineHeight * 4.0f,
			"Select aspect ratio" );

		TXT_MeasureString( p_pGlyphSet, "The square should not be distorted",
			&TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			( float )p_pGlyphSet->LineHeight * 5.5f,
			"The square should not be distorted" );

		TXT_MeasureString( p_pGlyphSet, "Press 'A' to select",
			&TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour,
			320.0f - ( TextLength / 2.0f ),
			( 480.0f - ( float )p_pGlyphSet->LineHeight * 4.0f ),
			"Press 'A' to select" );

		if( FourThree )
		{
			TextColour.byte.bAlpha = AlphaByte;
			AspectCamera.AspectRatio = 4.0f / 3.0f;
		}
		else
		{
			TextColour.byte.bAlpha = 140;
			AspectCamera.AspectRatio = 16.0f / 9.0f;
		}

		TXT_MeasureString( p_pGlyphSet, "4:3", &TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour,
			320.0f - ( TextLength ) - Spacing,
			360.0f,
			"4:3" );

		if( !FourThree )
		{
			TextColour.byte.bAlpha = AlphaByte;
		}
		else
		{
			TextColour.byte.bAlpha = 140;
		}

		TXT_RenderString( p_pGlyphSet, &TextColour,
			320.0f + Spacing,
			360.0f,
			"16:9" );

		CAM_CalculateProjectionMatrix( &Projection, &AspectCamera );
		CAM_CalculateViewMatrix( &View, &AspectCamera );

		MAT44_Multiply( &ViewProjection, &World, &View );
		MAT44_Multiply( &ViewProjection, &ViewProjection, &Projection );

		MAT44_TransformVerticesRHW( ( float * )SquareVertsT,
			( float * )SquareVerts, 4, sizeof( SquareVertsT[ 0 ] ),
			sizeof( SquareVerts[ 0 ] ), &ViewProjection );

		for( i = 0; i < 4; ++i )
		{
			Square[ i ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
			Square[ i ].fX = SquareVertsT[ i ].X;
			Square[ i ].fY = SquareVertsT[ i ].Y;
			Square[ i ].u.fInvW = SquareVertsT[ i ].Z;
			Square[ i ].fBaseAlpha = 0.7f;
			Square[ i ].fBaseRed = 83.0f / 255.0f;
			Square[ i ].fBaseGreen = 254.0f / 255.0f;
			Square[ i ].fBaseBlue = 1.0f;
		}

		Square[ 3 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;

		REN_DrawPrimitives01( &SquareStripHead, Square, 4 );

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

		DrawOverlayText( p_pGlyphSet );

		REN_SwapBuffers( );
	}

	return AspectCamera.AspectRatio;
}

void DrawOverlayText( GLYPHSET *p_pGlyphSet )
{
	float TextLength;
	KMPACKEDARGB TextColour;

	TextColour.dwPacked = 0x7F00FF00;

	TXT_MeasureString( p_pGlyphSet, g_VersionString, &TextLength );
	TXT_RenderString( p_pGlyphSet, &TextColour,
		320.0f - ( TextLength / 2.0f ),
		480.0f - ( ( float )p_pGlyphSet->LineHeight * 3.0f ),
		g_VersionString );

	TextColour.dwPacked = 0x7FFFFFFF;

	TXT_MeasureString( p_pGlyphSet, MILESTONE_STRING, &TextLength );
	TXT_RenderString( p_pGlyphSet, &TextColour, 0.0f,
		480.0f - ( float )p_pGlyphSet->LineHeight * 2.0f,
		MILESTONE_STRING );

	TXT_MeasureString( p_pGlyphSet, g_ConsoleIDPrint, &TextLength );
	TXT_RenderString( p_pGlyphSet, &TextColour, 640.0f - TextLength,
		480.0f - ( float )p_pGlyphSet->LineHeight * 2.0f, g_ConsoleIDPrint );
}

