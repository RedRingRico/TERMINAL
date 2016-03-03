#include <Renderer.h>
#include <GitVersion.h>
#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <string.h>
#include <Log.h>
#include <Hardware.h>
#include <Peripheral.h>
#include <Memory.h>
#include <Text.h>
#include <kamui2.h>
#include <sh4scif.h>
#include <Vector3.h>
#include <mathf.h>
#include <Camera.h>
#include <Model.h>
#include <DA.h>
#include <Audio.h>

#include <FileSystem.h>

#include <ac.h>
#include <am.h>

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
bool g_ConnectedToDA = false;

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

typedef struct _tagPERF_INFO
{
	Uint32	UpdateTime;
	Uint32	RenderTime;
	Uint32	LoopTime;
	Uint32	FPS;
}PERF_INFO, *PPERF_INFO;

typedef struct _tagSOUND
{
	KTU32	*pMemoryLocation;
	Sint32	Size;
}SOUND;

bool SelectPALRefresh( GLYPHSET *p_pGlyphSet );
float TestAspectRatio( GLYPHSET *p_pGlyphSet );
void DrawOverlayText( GLYPHSET *p_pGlyphSet );
void DrawDebugOverlay_Int( GLYPHSET *p_pGlyphSet, PPERF_INFO p_pPerfInfo );

#if defined ( DEBUG )
#define DrawDebugOverlay DrawDebugOverlay_Int
#else
#define DrawDebugOverlay sizeof
#endif /* DEBUG */
bool LoadSoundBank( Uint32 *p_pAICA, char *p_pName, Sint32 *p_pSize );

SOUND g_Select, g_Accept;

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
	PERF_INFO PerfInfo;
	AUDIO_PARAMETERS AudioParameters;

	CAMERA TestCamera;
	MATRIX4X4 Projection;
	MODEL Level, Hiro;

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

	AudioParameters.IntCallback = NULL;

	/* Initialise sound */
	if( AUD_Initialise( &AudioParameters ) != 0 )
	{
		LOG_Debug( "Failed to set up the Audio64 interface" );

		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	acSetTransferMode( AC_TRANSFER_DMA );
	acSystemSetVolumeMode( USELINEAR );
	acSystemSetMasterVolume( 14 );

	g_Select.pMemoryLocation = acSystemGetFirstFreeSoundMemory( );

	if( !LoadSoundBank( g_Select.pMemoryLocation, "/AUDIO/SELECT.WAV",
		&g_Select.Size ) )
	{
		LOG_Debug( "Failed to load the audio sample: /AUDIO/SELECT.WAV" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	g_Accept.pMemoryLocation = acSystemGetFirstFreeSoundMemory( );
	g_Accept.pMemoryLocation += ( g_Select.Size >> 2 );

	if( !LoadSoundBank( g_Accept.pMemoryLocation, "/AUDIO/ACCEPT.WAV",
		&g_Accept.Size ) )
	{
		LOG_Debug( "Failed to load the audio sample: /AUDIO/ACCEPT.WAV" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TXT_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the text system" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TXT_CreateGlyphSetFromFile( "/FONTS/WHITERABBIT.FNT",
		&GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph descriptions" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( TXT_SetTextureForGlyphSet( "/FONTS/WHITERABBIT.PVR", &GlyphSet ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph texture" );

		AUD_Terminate( );
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
	TestCamera.Position.Y = 200.0f;
	TestCamera.Position.Z = -800.0f;

	TestCamera.LookAt.X = 0.0f;
	TestCamera.LookAt.Y = 170.0f;
	TestCamera.LookAt.Z = 1.0f;

	TestCamera.WorldUp.X = 0.0f;
	TestCamera.WorldUp.Y = 1.0f;
	TestCamera.WorldUp.Z = 0.0f;

	TestCamera.GateWidth = 640.0f;
	TestCamera.GateHeight = 480.0f;

	TestCamera.FieldOfView = ( 3.141592654f / 4.0f );
	TestCamera.NearPlane = 1.0f;
	TestCamera.FarPlane = 1000.0f;

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

	if( MDL_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the model library" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( MDL_LoadModel( &Hiro, "/MODELS/HIRO.TML" ) != 0 )
	{
		LOG_Debug( "Failed to load the Hiro model" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( MDL_LoadModel( &Level, "/MODELS/MOCK_LEVEL.TML" ) != 0 )
	{
		LOG_Debug( "Failed to load the level model" );

		AUD_Terminate( );
		REN_Terminate( );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	CAM_CalculateProjectionMatrix( &Projection, &TestCamera );

	g_Peripherals[ 0 ].press = 0;
	memset( &PerfInfo, 0, sizeof( PerfInfo ) );

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
		int i;
		MATRIX4X4 View, ViewProjection;
		MATRIX4X4 World;
		VECTOR3 LightWorldPos;
		VECTOR3 LightPosition = { 0.0f, 1.0f, 0.0f };
		static VECTOR3 PlayerMove = { 0.0f, 0.0f, 0.0f };
		VECTOR3 StickMove = { 0.0f, 0.0f, 0.0f };
		VECTOR3 RotateAxis = { 0.0f, 1.0f, 0.0f };
		VECTOR3 CameraDefaultLookRef = { 0.0f, 170.0f, 1.0f };
		VECTOR3 CameraShoulderLookRef = { 100.0f, 170.0f, 1.0f };
		VECTOR3 CameraDefaultPosRef = { 0.0f, 200.0f, -800.0f };
		VECTOR3 CameraShoulderPosRef = { 150.0f, 200.0f, -200.0f };
		static float Rotate = 0.0f;
		static float CameraRotation = 0.0f;
		static float PlayerRotate = 0.0f;
		static float StickRotate = 0.0f;

		StartTime = syTmrGetCount( );

		if( g_Peripherals[ 0 ].press & PDD_DGT_ST )
		{
			Run = 0;
		}

		if( g_Peripherals[ 0 ].x1 > 0 )
		{
			StickMove.X = ( float )g_Peripherals[ 0 ].x1 / 127.0f;
		}
		if( g_Peripherals[ 0 ].x1 < 0 )
		{
			StickMove.X = ( float )g_Peripherals[ 0 ].x1 / 128.0f;
		}

		if( g_Peripherals[ 0 ].y1 > 0 )
		{
			StickMove.Z = -( float )g_Peripherals[ 0 ].y1 / 127.0f;
		}
		if( g_Peripherals[ 0 ].y1 < 0 )
		{
			StickMove.Z = -( float )g_Peripherals[ 0 ].y1 / 128.0f;
		}

		VEC3_Add( &PlayerMove, &PlayerMove, &StickMove );
		/*VEC3_Add( &TestCamera.Position, &TestCamera.Position, &StickMove );
		VEC3_Add( &TestCamera.LookAt, &TestCamera.LookAt, &StickMove );*/

		if( ARI_IsZero( StickMove.X ) == false ||
			ARI_IsZero( StickMove.Z ) == false )
		{
			VEC3_Normalise( &StickMove );

			StickRotate = atan2f( StickMove.X, StickMove.Z );
		}

		/* L trigger activates over-the-shoulder camera */
		if( g_Peripherals[ 0 ].l > 128 )
		{
			MATRIX4X4 CameraMatrix;
			VECTOR3 Thrust = { 0.0f, 0.0f, 0.0f };
			VECTOR3 Acceleration = { 0.0f, 0.0f, 0.0f };

			MAT44_SetIdentity( &CameraMatrix );

			Rotate += StickMove.X * 0.04f;

			Thrust.Z = StickMove.Z;

			MAT44_RotateAxisAngle( &CameraMatrix, &RotateAxis, Rotate );

			MAT44_TransformVertices( &Acceleration, &Thrust, 1,
				sizeof( VECTOR3 ), sizeof( VECTOR3 ), &CameraMatrix );

			VEC3_MultiplyF( &Acceleration, &Acceleration, 10.0f );
			
			PlayerMove.X += Acceleration.X;
			PlayerMove.Z += Acceleration.Z;

			MAT44_Translate( &CameraMatrix, &PlayerMove );

			MAT44_TransformVertices( &TestCamera.Position,
				&CameraShoulderPosRef, 1, sizeof( VECTOR3 ), sizeof( VECTOR3 ),
				&CameraMatrix );

			MAT44_TransformVertices( &TestCamera.LookAt,
				&CameraShoulderLookRef, 1, sizeof( VECTOR3 ),
				sizeof( VECTOR3 ), &CameraMatrix );

			PlayerRotate = Rotate;
			CameraRotation = Rotate;
		}
		else
		{
			MATRIX4X4 CameraMatrix;
			MATRIX4X4 PlayerMatrix;
			VECTOR3 CameraDirection = { 0.0f, 0.0f, 1.0f };
			VECTOR3 Thrust = { 0.0f, 0.0f, 0.0f };
			VECTOR3 Acceleration = { 0.0f, 0.0f, 0.0f };

			PlayerRotate = CameraRotation + StickRotate;

			MAT44_SetIdentity( &CameraMatrix );
			MAT44_SetIdentity( &PlayerMatrix );

			Thrust.X = StickMove.X;
			Thrust.Z = StickMove.Z;

			MAT44_RotateAxisAngle( &PlayerMatrix, &RotateAxis,
				CameraRotation );
			MAT44_TransformVertices( &Acceleration, &Thrust, 1,
				sizeof( VECTOR3 ), sizeof( VECTOR3 ), &PlayerMatrix );

			VEC3_MultiplyF( &Acceleration, &Acceleration, 10.0f );

			PlayerMove.X += Acceleration.X;
			PlayerMove.Z += Acceleration.Z;

			MAT44_RotateAxisAngle( &CameraMatrix, &RotateAxis,
				CameraRotation );
			MAT44_Translate( &CameraMatrix, &PlayerMove );

			MAT44_TransformVertices( &TestCamera.Position,
				&CameraDefaultPosRef, 1, sizeof( VECTOR3 ), sizeof( VECTOR3 ),
				&CameraMatrix );

			MAT44_TransformVertices( &TestCamera.LookAt, &CameraDefaultLookRef,
				1, sizeof( VECTOR3 ), sizeof( VECTOR3 ), &CameraMatrix );
		}

		MAT44_SetIdentity( &World );

		CAM_CalculateViewMatrix( &View, &TestCamera );
		
		MDL_CalculateLighting( &Hiro, &World, &LightPosition );
		MDL_CalculateLighting( &Level, &World, &LightPosition );

		UpdateTime = syTmrGetCount( );
		UpdateTime =
			syTmrCountToMicro( syTmrDiffCount( StartTime, UpdateTime ) );
		PerfInfo.UpdateTime = UpdateTime;

		REN_Clear( );

		RenderStartTime = syTmrGetCount( );


		MAT44_RotateAxisAngle( &World, &RotateAxis, PlayerRotate );
		MAT44_Translate( &World, &PlayerMove );
		MAT44_Multiply( &ViewProjection, &World, &View );
		MAT44_Multiply( &ViewProjection, &ViewProjection, &Projection );

		MDL_RenderModel( &Hiro, &ViewProjection );

		MAT44_SetIdentity( &World );
		MAT44_Multiply( &ViewProjection, &World, &View );
		MAT44_Multiply( &ViewProjection, &ViewProjection, &Projection );

		MDL_RenderModel( &Level, &ViewProjection );

		TextColour.dwPacked = 0xFFFFFF00;

		sprintf( PrintBuffer, "Rotate:        %f", Rotate );
		TXT_RenderString( &GlyphSet, &TextColour,
			10.0f, ( float )GlyphSet.LineHeight * 3.0f, PrintBuffer );

		sprintf( PrintBuffer, "Player rotate: %f", PlayerRotate );
		TXT_RenderString( &GlyphSet, &TextColour,
			10.0f, ( float )GlyphSet.LineHeight * 4.0f, PrintBuffer );

		sprintf( PrintBuffer, "Camera rotate: %f", CameraRotation );
		TXT_RenderString( &GlyphSet, &TextColour,
			10.0f, ( float )GlyphSet.LineHeight * 5.0f, PrintBuffer );

		sprintf( PrintBuffer, "Camera position: %f %f %f",
			TestCamera.Position.X,TestCamera.Position.Y,TestCamera.Position.Z );
		TXT_RenderString( &GlyphSet, &TextColour,
			10.0f, ( float )GlyphSet.LineHeight * 6.0f, PrintBuffer );


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
		/*TXT_MeasureString( &GlyphSet, "PRESS START", &TextLength );
		TXT_RenderString( &GlyphSet, &TextColour,
			320.0f -( TextLength / 2.0f ),
			360.0f, "PRESS START" );*/

		if( DA_IPRDY & DA_GetChannelStatus( 3 ) )
		{
			if( DA_GetData( PrintBuffer, 80, 3 ) )
			{
				g_ConnectedToDA = true;
			}
		}

		RenderEndTime = syTmrGetCount( );

		DrawOverlayText( &GlyphSet );
		DrawDebugOverlay( &GlyphSet, &PerfInfo );

		REN_SwapBuffers( );

		RenderTime = syTmrCountToMicro(
			syTmrDiffCount( RenderStartTime, RenderEndTime ) );
		PerfInfo.RenderTime = RenderTime;

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

		EndTime = syTmrGetCount( );

		TimeDifference =
			syTmrCountToMicro( syTmrDiffCount( StartTime, EndTime ) );
		PerfInfo.LoopTime = TimeDifference;

		ElapsedTime += TimeDifference;
		FPSTimer += TimeDifference;

		if( FPSTimer >= 1000000UL )
		{
			FPSTimer = 0UL;
			FPS = FPSCounter;
			FPSCounter = 0UL;
			PerfInfo.FPS = FPS;
		}
	}

	MDL_DeleteModel( &Hiro );
	MDL_DeleteModel( &Level );
	MDL_Terminate( );

	LOG_Debug( "Rebooting" );

	AUD_Terminate( );
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

		if( DA_IPRDY & DA_GetChannelStatus( 3 ) )
		{
			char PrintBuffer[ 80 ];
			if( DA_GetData( PrintBuffer, 80, 3 ) )
			{
				g_ConnectedToDA = true;
			}
		}

		if( ModeSelected )
		{
			CableSelect = false;
			continue;
		}

		StartTime = syTmrGetCount( );

		if( ( g_Peripherals[ 0 ].press & PDD_DGT_TA ) &&
			( ModeTest == false ) )
		{
			acDigiOpen( 10, ( KTU32 )g_Accept.pMemoryLocation,
				g_Select.Size, AC_16BIT, 44100 );
			acDigiRequestEvent( 10, ( g_Accept.Size >> 1 ) - 1 );
			acDigiPlay( 10, 0, AC_LOOP_OFF );

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

						acDigiOpen( 10, ( KTU32 )g_Accept.pMemoryLocation,
							g_Select.Size, AC_16BIT, 44100 );
						acDigiRequestEvent( 10, ( g_Accept.Size >> 1 ) - 1 );
						acDigiPlay( 10, 0, AC_LOOP_OFF );

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
						acDigiOpen( 10, ( KTU32 )g_Select.pMemoryLocation,
							g_Select.Size, AC_16BIT, 44100 );
						acDigiRequestEvent( 10, ( g_Select.Size >> 1 ) - 1 );
						acDigiPlay( 10, 0, AC_LOOP_OFF );

						TestPass = !TestPass;
						Alpha = 1.0f;
					}

					if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
					{
						acDigiOpen( 10, ( KTU32 )g_Accept.pMemoryLocation,
							g_Select.Size, AC_16BIT, 44100 );
						acDigiRequestEvent( 10, ( g_Accept.Size >> 1 ) - 1 );
						acDigiPlay( 10, 0, AC_LOOP_OFF );

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
				acDigiOpen( 10, ( KTU32 )g_Select.pMemoryLocation,
					g_Select.Size, AC_16BIT, 44100 );
				acDigiRequestEvent( 10, ( g_Select.Size >> 1 ) - 1 );
				acDigiPlay( 10, 0, AC_LOOP_OFF );

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
		DrawDebugOverlay( p_pGlyphSet, NULL );

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
		if( DA_IPRDY & DA_GetChannelStatus( 3 ) )
		{
			char PrintBuffer[ 80 ];
			if( DA_GetData( PrintBuffer, 80, 3 ) )
			{
				g_ConnectedToDA = true;
			}
		}

		if( !AButtonHeld )
		{
			if( g_Peripherals[ 0 ].press & PDD_DGT_TA )
			{
				acDigiOpen( 11, ( KTU32 )g_Accept.pMemoryLocation,
					g_Accept.Size, AC_16BIT, 44100 );
				acDigiRequestEvent( 11, ( g_Accept.Size >> 1 ) - 1 );
				acDigiPlay( 11, 0, AC_LOOP_OFF );

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
			acDigiOpen( 10, ( KTU32 )g_Select.pMemoryLocation, g_Select.Size,
				AC_16BIT, 44100 );
			acDigiRequestEvent( 10, ( g_Select.Size >> 1 ) - 1 );
			acDigiPlay( 10, 0, AC_LOOP_OFF );

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
		DrawDebugOverlay( p_pGlyphSet, NULL );

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

void DrawDebugOverlay_Int( GLYPHSET *p_pGlyphSet, PPERF_INFO p_pPerfInfo )
{
	float TextLength;
	KMPACKEDARGB TextColour;

	if( g_ConnectedToDA )
	{
		TextColour.dwPacked = 0x9F00FF00;
		TXT_RenderString( p_pGlyphSet, &TextColour,
			0.0f, 0.0f, "[DA: ONLINE]" );
	}
	else
	{
		TextColour.dwPacked = 0x9FFF0000;
		TXT_RenderString( p_pGlyphSet, &TextColour,
			0.0f, 0.0f, "[DA: OFFLINE]" );
	}

	if( p_pPerfInfo )
	{
		char PrintBuffer[ 80 ];
		TextColour.dwPacked = 0xFFFFFFFF;
		sprintf( PrintBuffer, "%lu [%lu]", p_pPerfInfo->FPS,
			p_pPerfInfo->LoopTime );

		if( p_pPerfInfo->FPS >= 40 )
		{
			TextColour.dwPacked = 0xFF00FF00;
		}
		else if( p_pPerfInfo->FPS >= 15 )
		{
			TextColour.dwPacked = 0xFFFFFF00;
		}
		else
		{
			TextColour.dwPacked = 0xFFFF0000;
		}

		TXT_MeasureString( p_pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour, 640.0f - TextLength,
			480.0f - ( float )p_pGlyphSet->LineHeight * 3.0f, PrintBuffer );

		TextColour.dwPacked = 0xFFFFFFFF;
		sprintf( PrintBuffer, "%lu | %lu", p_pPerfInfo->UpdateTime,
			p_pPerfInfo->RenderTime );
		TXT_MeasureString( p_pGlyphSet, PrintBuffer, &TextLength );
		TXT_RenderString( p_pGlyphSet, &TextColour, 640.0f - TextLength,
			480.0f - ( float )p_pGlyphSet->LineHeight * 4.0f, PrintBuffer );
	}
}

bool LoadSoundBank( Uint32 *p_pAICA, char *p_pName, Sint32 *p_pSize )
{
	KTU32 *pImage = KTNULL;
	KTU32 ImageSize = 0;
	GDFS FileHandle;
	long FileBlocks;

	if( !( FileHandle = FS_OpenFile( p_pName ) ) )
	{
		return false;
	}

	gdFsGetFileSize( FileHandle, p_pSize );
	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	ImageSize = ALIGN( *p_pSize, SECTOR_SIZE );

	/* Allocate a buffer to hold the file */
	pImage = ( KTU32 * )syMalloc( ImageSize );

	if( !pImage )
	{
		return false;
	}

	gdFsReqRd32( FileHandle, FileBlocks, pImage );

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	acG2Write( p_pAICA, pImage, *p_pSize );

	syFree( pImage );

	//acSystemWaitUntilG2FifoIsEmpty( );

	return true;
}

