#include <StorageUnit.h>
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
#include <DebugAdapter.h>
#include <Audio.h>
#include <NetworkCore.h>
#include <ngadns.h>
#include <ngnetdb.h>
#include <ngsocket.h>
#include <string.h>
#include <Stack.h>
#include <GameStateManager.h>
#include <RefreshRateSelectState.h>
#include <AspectRatioSelectState.h>
#include <MainMenuState.h>
#include <MultiPlayerState.h>
#include <TestMenuState.h>

#include <Array.h>

#include <FileSystem.h>

#include <ac.h>
#include <am.h>

//#include <adns.h>

#define MAX_TEXTURES ( 4096 )
#define MAX_SMALLVQ ( 0 )

#pragma aligndata32( g_pTextureWorkArea )
KMDWORD g_pTextureWorkArea[ MAX_TEXTURES * 24 / 4 + MAX_SMALLVQ * 76 / 4 ];

#define MILESTONE_STRING "Milestone 1 - Networked Multiplayer"

unsigned char RecvBuf[ 1024 * 8 ];
unsigned char SendBuf[ 1024 * 8 ];

static char g_VersionString[ 256 ];
static Sint8 g_ConsoleID[ SYD_CFG_IID_SIZE + 1 ];
static char g_ConsoleIDPrint[ ( SYD_CFG_IID_SIZE * 2 ) + 1 ];
static bool g_ConnectedToDA = false;

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

typedef struct _tagVSYNC_CALLBACK
{
	PGAMESTATE_MANAGER	pGameStateManager;
}VSYNC_CALLBACK,*PVSYNC_CALLBACK;

void DrawOverlayText( GLYPHSET *p_pGlyphSet );
void DrawDebugOverlay_Int( GLYPHSET *p_pGlyphSet, PPERF_INFO p_pPerfInfo );

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
#define DrawDebugOverlay DrawDebugOverlay_Int
#else
#define DrawDebugOverlay sizeof
#endif /* DEBUG */
bool LoadSoundBank( Uint32 *p_pAICA, char *p_pName, Sint32 *p_pSize );

SOUND g_Select, g_Accept;
int DNSStatus;
int PollStatus;
static ngADnsAnswer DNSAnswer;
ngADnsTicket DNSTicket;
Uint32 DNSResolveTime;

static void VSyncCallback( PKMVOID p_pArgs );

void main( void )
{
	int Run = 1;
	DREAMCAST_RENDERERCONFIGURATION RendererConfiguration;
	PKMSURFACEDESC Framebuffer[ 2 ];
	KMSURFACEDESC FrontBuffer, BackBuffer;
	PKMDWORD pVertexBuffer;
	KMVERTEXBUFFDESC VertexBufferDesc;
	void *pGSMSystemMemory = NULL, *pGSMGraphicsMemory = NULL,
		*pGSMAudioMemory = NULL;/* *pTVMMemory = NULL;*/
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
	RENDERER Renderer;
	GAMESTATE_MANAGER GameStateManager;
	GAMESTATE_MEMORY_BLOCKS GameStateMemoryBlocks;
	MEMORY_BLOCK SystemMemoryBlock, GraphicsMemoryBlock, AudioMemoryBlock;
	/*MEMORY_BLOCK TVMMemoryBlock;*/
	MEMORY_FREESTAT MemoryFree;
	VSYNC_CALLBACK VSyncCallbackArgs;

	CAMERA TestCamera;
	MATRIX4X4 Projection, Screen;
	MODEL Level, Hiro;

	if( HW_Initialise( KM_DSPBPP_RGB888, &AVCable, &MemoryFree ) != 0 )
	{
		HW_Terminate( );
		HW_Reboot( );
	}

	LOG_Initialise( NULL );
	LOG_Debug( "TERMINAL" );
	LOG_Debug( "Version: %s", GIT_VERSION );

	pVertexBuffer = ( PKMDWORD )syMalloc( 0x100000 );

	/*syCacheAllocCacheRAM( &pTVMMemory, 7 );

	if( pTVMMemory == NULL )
	{
		LOG_Debug( "Could not allocate memory for the Virtual Machine" );

		goto MainCleanup;
	}*/

	if( ( pGSMSystemMemory = syMalloc( MEM_MIB( 6 ) ) ) == NULL )
	{
		LOG_Debug( "Could not use syMalloc to allocate %ld bytes of memory "
			"for the system", MEM_MIB( 6 ) );

		goto MainCleanup;
	}

	if( ( pGSMGraphicsMemory = syMalloc( MEM_MIB( 2 ) ) ) == NULL )
	{
		LOG_Debug( "Could not use syMalloc to allocate %ld bytes of memory "
			"for the graphics", MEM_MIB( 2 ) );

		goto MainCleanup;
	}

	if( ( pGSMAudioMemory = syMalloc( MEM_MIB( 1 ) ) ) == NULL )
	{
		LOG_Debug( "Could not use syMalloc to allocate %ld bytes of memory "
			"for the audio", MEM_MIB( 2 ) );

		goto MainCleanup;
	}

	/*if( MEM_InitialiseMemoryBlock( &TVMMemoryBlock,
		pTVMMemory, MEM_KIB( 7 ), 32, "Virtual Machine" ) != 0 )
	{
		LOG_Debug( "Could not allocate %ld bytes of memory for the Virtual "
			"Machine", MEM_KIB( 7 ) );

		goto MainCleanup;
	}*/

	if( MEM_InitialiseMemoryBlock( &SystemMemoryBlock,
		pGSMSystemMemory, MEM_MIB( 6 ), 32, "GSM: System" ) != 0 )
	{
		LOG_Debug( "Could not allocate %ld bytes of memory for the system",
			MEM_MIB( 6 ) );

		goto MainCleanup;
	}

	if( MEM_InitialiseMemoryBlock( &GraphicsMemoryBlock,
		pGSMGraphicsMemory, 1024*1024*2, 32, "GSM: Graphics" ) != 0 )
	{
		LOG_Debug( "Could not allocate %ld bytes of memory", 1024*1024*2 );

		goto MainCleanup;
	}

	if( MEM_InitialiseMemoryBlock( &AudioMemoryBlock,
		pGSMAudioMemory, 1024*1024*1, 32, "GSM: Audio" ) != 0 )
	{
		LOG_Debug( "Could not allocate %ld bytes of memory", 1024*1024*1 );

		goto MainCleanup;
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
	/* Set the cursor to the next row */
	scif_putq( ']' );
	scif_putq( 0x1B );
	scif_putq( '[' );
	scif_putq( '2' );
	scif_putq( ';' );
	scif_putq( '1' );
	scif_putq( 'f' );

	SU_Initialise( &SystemMemoryBlock );

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

	RendererConfiguration.pMemoryBlock = &GraphicsMemoryBlock;

	REN_Initialise( &Renderer, &RendererConfiguration );

	kmSetWaitVsyncCallback(&VSyncCallback, &VSyncCallbackArgs);

	scif_close( );

	AudioParameters.IntCallback = NULL;
	AudioParameters.pMemoryBlock = &AudioMemoryBlock;

	/* Initialise sound */
	if( AUD_Initialise( &AudioParameters ) != 0 )
	{
		LOG_Debug( "Failed to set up the Audio64 interface" );

		goto MainCleanup;
	}

	acSetTransferMode( AC_TRANSFER_DMA );
	acSystemSetVolumeMode( USELINEAR );
	acSystemSetMasterVolume( 14 );

	g_Select.pMemoryLocation = acSystemGetFirstFreeSoundMemory( );

	if( !LoadSoundBank( g_Select.pMemoryLocation, "/AUDIO/SELECT.WAV",
		&g_Select.Size ) )
	{
		LOG_Debug( "Failed to load the audio sample: /AUDIO/SELECT.WAV" );

		goto MainCleanup;
	}

	g_Accept.pMemoryLocation = acSystemGetFirstFreeSoundMemory( );
	g_Accept.pMemoryLocation += ( g_Select.Size >> 2 );

	if( !LoadSoundBank( g_Accept.pMemoryLocation, "/AUDIO/ACCEPT.WAV",
		&g_Accept.Size ) )
	{
		LOG_Debug( "Failed to load the audio sample: /AUDIO/ACCEPT.WAV" );

		goto MainCleanup;
	}

	if( TXT_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the text system" );

		goto MainCleanup;
	}

	if( TXT_CreateGlyphSetFromFile( "/FONTS/WHITERABBIT.FNT",
		&GlyphSet, &SystemMemoryBlock ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph descriptions" );

		goto MainCleanup;
	}

	if( TXT_SetTextureForGlyphSet( "/FONTS/WHITERABBIT.PVR", &GlyphSet,
		&GraphicsMemoryBlock ) != 0 )
	{
		LOG_Debug( "Failed to load the glyph texture" );

		goto MainCleanup;
	}

	if( MDL_Initialise( &GraphicsMemoryBlock ) != 0 )
	{
		LOG_Debug( "Failed to initialise the model library" );

		goto MainCleanup;
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
	TestCamera.Position.Y = 2.0f;
	TestCamera.Position.Z = -8.0f;

	TestCamera.LookAt.X = 0.0f;
	TestCamera.LookAt.Y = 1.7f;
	TestCamera.LookAt.Z = 1.0f;

	TestCamera.WorldUp.X = 0.0f;
	TestCamera.WorldUp.Y = 1.0f;
	TestCamera.WorldUp.Z = 0.0f;

	TestCamera.GateWidth = 640.0f;
	TestCamera.GateHeight = 480.0f;

	TestCamera.FieldOfView = ( 3.141592654f / 4.0f );
	TestCamera.NearPlane = 0.001f; /* 1cm */
	TestCamera.FarPlane = 10000.0f; /* 10km (too much?) */

	GameStateMemoryBlocks.pSystemMemory = &SystemMemoryBlock;
	GameStateMemoryBlocks.pGraphicsMemory = &GraphicsMemoryBlock;
	GameStateMemoryBlocks.pAudioMemory = &AudioMemoryBlock;

	if( GSM_Initialise( &GameStateManager, &Renderer,
		&GameStateMemoryBlocks ) != 0 )
	{
		LOG_Debug( "Failed to initialise the game state manager\n" );

		goto MainCleanup;
	}

	VSyncCallbackArgs.pGameStateManager = &GameStateManager;

	GSM_RegisterGlyphSet( &GameStateManager, GSM_GLYPH_SET_DEBUG, &GlyphSet );
	GSM_RegisterGlyphSet( &GameStateManager, GSM_GLYPH_SET_GUI_1, &GlyphSet );

	RRSS_RegisterWithGameStateManager( &GameStateManager );
	ARSS_RegisterWithGameStateManager( &GameStateManager );
	MMS_RegisterWithGameStateManager( &GameStateManager );
	MP_RegisterMainWithGameStateManager( &GameStateManager );
	MP_RegisterISPConnectWithGameStateManager( &GameStateManager );
	MP_RegisterGameListServerWithGameStateManager( &GameStateManager );
	MP_RegisterMultiPlayerGameWithGameStateManager( &GameStateManager );
#if defined ( DEBUG ) || defined ( DEVELOPMENT )
	TMU_RegisterWithGameStateManager( &GameStateManager );
	MDLV_RegisterWithGameStateManager( &GameStateManager );
#endif /* DEBUG || DEVELOPMENT */

	if( AVCable == SYE_CBL_PAL )
	{
		REFRESHRATESELECT RefreshRateArgs;
		RefreshRateArgs.pGlyphSet = &GlyphSet;

		GSM_ChangeState( &GameStateManager, GAME_STATE_REFRESHRATESELECT,
			&RefreshRateArgs, NULL );
	}
	else
	{
		ASPECTRATIOSELECT AspectRatioArgs;
		AspectRatioArgs.pGlyphSet = &GlyphSet;

		GSM_ChangeState( &GameStateManager, GAME_STATE_ASPECTRATIOSELECT,
			&AspectRatioArgs, NULL );
	}

	while( GSM_IsRunning( &GameStateManager ) == true )
	{
		GSM_Run( &GameStateManager );
	}

MainCleanup:
	LOG_Debug( "Rebooting" );

	GSM_Terminate( &GameStateManager );
	AUD_Terminate( );
	REN_Terminate( &Renderer );
	SU_Terminate( );

	/*if( pTVMMemory != NULL )
	{
		MEM_GarbageCollectMemoryBlock( &TVMMemoryBlock );
		MEM_ListMemoryBlocks( &TVMMemoryBlock );
		syCacheFreeCacheRAM( pTVMMemory );
	}*/

	if( pGSMAudioMemory != NULL )
	{
		MEM_GarbageCollectMemoryBlock( &AudioMemoryBlock );
		MEM_ListMemoryBlocks( &AudioMemoryBlock );
		syFree( pGSMAudioMemory );
	}

	if( pGSMGraphicsMemory != NULL )
	{
		MEM_GarbageCollectMemoryBlock( &GraphicsMemoryBlock );
		MEM_ListMemoryBlocks( &GraphicsMemoryBlock );
		syFree( pGSMGraphicsMemory );
	}

	if( pGSMSystemMemory != NULL )
	{
		MEM_GarbageCollectMemoryBlock( &SystemMemoryBlock );
		MEM_ListMemoryBlocks( &SystemMemoryBlock );
		syFree( pGSMSystemMemory );
	}

	syFree( pVertexBuffer );

#if defined ( DEBUG )
	{
		Uint32 Free, BiggestFree;

		syMallocStat( &Free, &BiggestFree );

		LOG_Debug( "Memory freed, %ld bytes on heap\n", Free );
		if( ( MemoryFree.Free - Free ) > 0 )
		{
			LOG_Debug( "Unaccounted for memory: %ld\n",
				MemoryFree.Free - Free );
		}
	}
#endif /* DEBUG */

	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );

	/* Unused code below, get rid of it! */

	if( MDL_Initialise( &GraphicsMemoryBlock ) != 0 )
	{
		LOG_Debug( "Failed to initialise the model library" );

		AUD_Terminate( );
		REN_Terminate( &Renderer );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( MDL_LoadModel( &Hiro, "/MODELS/HIRO.TML", NULL ) != 0 )
	{
		LOG_Debug( "Failed to load the Hiro model" );

		AUD_Terminate( );
		REN_Terminate( &Renderer );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	if( MDL_LoadModel( &Level, "/MODELS/MOCK_LEVEL.TML", NULL ) != 0 )
	{
		LOG_Debug( "Failed to load the level model" );

		AUD_Terminate( );
		REN_Terminate( &Renderer );
		LOG_Terminate( );
		HW_Terminate( );
		HW_Reboot( );
	}

	CAM_CalculateProjectionMatrix( &Projection, &TestCamera );
	CAM_CalculateScreenMatrix( &Screen, &TestCamera );

	g_Peripherals[ 0 ].press = 0;
	memset( &PerfInfo, 0, sizeof( PerfInfo ) );

	StartTime = syTmrGetCount( );
	DNSStatus = -1;
	PollStatus = NG_EWOULDBLOCK;

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
		VECTOR3 CameraDefaultLookRef = { 0.0f, 1.7f, 1.0f };
		VECTOR3 CameraShoulderLookRef = { 1.0f, 1.7f, 1.0f };
		VECTOR3 CameraDefaultPosRef = { 0.0f, 2.0f, -8.0f };
		VECTOR3 CameraShoulderPosRef = { 1.5f, 2.0f, -2.0f };
		static float Rotate = 0.0f;
		static float CameraRotation = 0.0f;
		static float PlayerRotate = 0.0f;
		static float StickRotate = 0.0f;
		static char DNSString[ 80 ] = { '\0' };

		Renderer.VisiblePolygons = Renderer.CulledPolygons =
			Renderer.GeneratedPolygons = 0;

		StartTime = syTmrGetCount( );

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

			VEC3_MultiplyF( &Acceleration, &Acceleration, 0.3f );
			
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

			VEC3_MultiplyF( &Acceleration, &Acceleration, 0.3f );

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

		if( NET_GetStatus( ) == NET_STATUS_CONNECTED )
		{

			if( DNSStatus == -1 )
			{
				DNSResolveTime = syTmrGetCount( );
				ngADnsGetTicket( &DNSTicket, "redringrico.com" );
				sprintf( DNSString, "Resolving \"redringrico.com\"" );
				DNSStatus = 0;
			}
			else if( DNSStatus == 0 )
			{
				PollStatus = ngADnsPoll( &DNSAnswer );
				sprintf( DNSString, "Resolving \"redringrico.com\"" );

				if( PollStatus != NG_EWOULDBLOCK )
				{
					sprintf( DNSString, "Found IP address" );
					DNSResolveTime = syTmrCountToMicro(
						syTmrDiffCount( DNSResolveTime, syTmrGetCount( ) ) );
					DNSStatus = 1;
				}
			}
			else if( DNSStatus == 1 )
			{
				if( PollStatus == NG_EOK )
				{
					if( DNSAnswer.ticket == DNSTicket )
					{
						struct in_addr **ppAddrList;

						ppAddrList =
							( struct in_addr ** )DNSAnswer.addr->h_addr_list;

						/* Really need the micro symbol, 'u' will do */
						sprintf( DNSString, "Resolved \"redringrico.com\" [%s]"
							" in %luus", inet_ntoa( *ppAddrList[ 0 ] ),
							DNSResolveTime );
							
						ngADnsReleaseTicket( &DNSTicket );
						DNSStatus = 2;
					}
				}
				else
				{
					sprintf( DNSString, "Failed to resolve "
						"\"redringrico.com\"" );
					DNSStatus = 0;
				}
			}
			else
			{
			}
		}
		else
		{
			DNSStatus = -1;
			sprintf( DNSString, "" );
		}

		NET_Update( );

		UpdateTime = syTmrGetCount( );
		UpdateTime =
			syTmrCountToMicro( syTmrDiffCount( StartTime, UpdateTime ) );
		PerfInfo.UpdateTime = UpdateTime;

		REN_Clear( );
		RenderStartTime = syTmrGetCount( );

		MAT44_SetIdentity( &World );

		CAM_CalculateViewMatrix( &View, &TestCamera );
		
		MDL_CalculateLighting( &Hiro, &World, &LightPosition );
		MDL_CalculateLighting( &Level, &World, &LightPosition );

		MAT44_RotateAxisAngle( &World, &RotateAxis, PlayerRotate );
		MAT44_Translate( &World, &PlayerMove );

		MDL_RenderModel( &Hiro, &Renderer, &World, &View, &Projection,
			&Screen );

		MAT44_SetIdentity( &World );

		MDL_RenderModel( &Level, &Renderer, &World, &View, &Projection,
			&Screen );

#if defined ( DEBUG )

		TextColour.dwPacked = 0xFFFFFFFF;
		sprintf( PrintBuffer, "Visible polygons: %lu",
			Renderer.VisiblePolygons );
		TXT_RenderString( &GlyphSet, &TextColour, 10.0f,
			( float )GlyphSet.LineHeight * 7.0f, PrintBuffer );

		sprintf( PrintBuffer, "Generated polygons: %lu",
			Renderer.GeneratedPolygons );
		TXT_RenderString( &GlyphSet, &TextColour, 10.0f,
			( float )GlyphSet.LineHeight * 8.0f, PrintBuffer );

		sprintf( PrintBuffer, "Culled polygons: %lu",
			Renderer.CulledPolygons );
		TXT_RenderString( &GlyphSet, &TextColour, 10.0f,
			( float )GlyphSet.LineHeight * 9.0f, PrintBuffer );

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
			TestCamera.Position.X, TestCamera.Position.Y,
			TestCamera.Position.Z );
		TXT_RenderString( &GlyphSet, &TextColour,
			10.0f, ( float )GlyphSet.LineHeight * 6.0f, PrintBuffer );

#endif /* DEBUG */

		TextColour.dwPacked = 0xFFFFFFFF;
		TXT_RenderString( &GlyphSet, &TextColour, 0.0f,
			( float )GlyphSet.LineHeight * 18.0f, DNSString );

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
		TXT_MeasureString( &GlyphSet, "PRESS START TO REBOOT", &TextLength );
		TXT_RenderString( &GlyphSet, &TextColour,
			320.0f -( TextLength / 2.0f ),
			32.0f, "PRESS START TO REBOOT" );

		/*if( DA_IPRDY & DA_GetChannelStatus( 3 ) )
		{
			if( DA_GetData( PrintBuffer, 80, 3 ) )
			{
				g_ConnectedToDA = true;
			}
		}*/

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
	REN_Terminate( &Renderer );
	LOG_Terminate( );
	HW_Terminate( );
	HW_Reboot( );
}

void DrawOverlayText( GLYPHSET *p_pGlyphSet )
{
	float TextLength;
	KMPACKEDARGB TextColour;
	static char PrintBuffer[ 128 ];

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

	switch( NET_GetStatus( ) )
	{
		case NET_STATUS_NODEVICE:
		{
			TextColour.dwPacked = 0x9FFF0000;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NO NETWORK DEVICE]" );
			break;
		}
		case NET_STATUS_DISCONNECTED:
		{
			TextColour.dwPacked = 0x9FFFFF00;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: DISCONNECTED]" );
			break;
		}
		case NET_STATUS_NEGOTIATING:
		{
			TextColour.dwPacked = 0x9F0000FF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: NEGOTIATING]" );
			break;
		}
		case NET_STATUS_CONNECTED:
		{
			TextColour.dwPacked = 0x9F00FF00;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: CONNECTED]" );
			break;
		}
		case NET_STATUS_PPP_POLL:
		{
			TextColour.dwPacked = 0x9FFF00FF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: POLLING PPP]" );
			break;
		}
		case NET_STATUS_RESET:
		{
			TextColour.dwPacked = 0x9FFF00FF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: RESETTING]" );
			break;
		}
		default:
		{
			TextColour.dwPacked = 0x9FFF0000;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				0.0f, ( float )p_pGlyphSet->LineHeight,
				"[NETWORK: UNKNOWN]" );
			break;
		}
	}

	switch( NET_GetDeviceType( ) )
	{
		case NET_DEVICE_TYPE_LAN_10:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: LAN [10Mbps]", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: LAN [10Mbps]" );
			break;
		}
		case NET_DEVICE_TYPE_LAN_100:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: LAN [100Mbps]", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: LAN [100Mbps]" );
			break;
		}
		case NET_DEVICE_TYPE_LAN_UNKNOWN:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: LAN [UNKNOWN]", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: LAN [UNKNOWN]" );
			break;
		}
		case NET_DEVICE_TYPE_EXTMODEM:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: External Modem", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: External Modem" );
			break;
		}
		case NET_DEVICE_TYPE_SERIALPPP:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: Serial PPP", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: Serial PPP" );
			break;
		}
		case NET_DEVICE_TYPE_INTMODEM:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: Internal Modem", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: Internal Modem" );
			break;
		}
		case NET_DEVICE_TYPE_NONE:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: Not present", &TextLength );
			TextColour.dwPacked = 0x9FFFFFFF;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: Not present" );
			break;
		}
		default:
		{
			float TextLength;
			TXT_MeasureString( p_pGlyphSet,
				"Network Device: UNKNOWN", &TextLength );
			TextColour.dwPacked = 0x9FFF0000;
			TXT_RenderString( p_pGlyphSet, &TextColour,
				640.0f - TextLength, 0.0f,
				"Network Device: UNKNOWN" );
			break;
		}
	}

	
	TextColour.dwPacked = 0x9FFFFFFF;
	sprintf( PrintBuffer, "Dev:   %d", NET_GetDevOpen( ) );
	TXT_RenderString( p_pGlyphSet, &TextColour, 0.0f,
		( float )p_pGlyphSet->LineHeight * 16.0f, PrintBuffer );

	sprintf( PrintBuffer, "Iface: %d", NET_GetIfaceOpen( ) );
	TXT_RenderString( p_pGlyphSet, &TextColour, 0.0f,
		( float )p_pGlyphSet->LineHeight * 17.0f, PrintBuffer );
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

	return true;
}

static void VSyncCallback( PKMVOID p_pArgs )
{
	PVSYNC_CALLBACK pArgs = ( PVSYNC_CALLBACK )( p_pArgs );

	NET_Update( );

	if( GSM_RunVSync( pArgs->pGameStateManager ) != 0 )
	{
	}
}

