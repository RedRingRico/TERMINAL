#include <Renderer.h>
#include <Memory.h>

static KMSYSTEMCONFIGSTRUCT	g_Kamui2Config;
static KMSTRIPCONTEXT		g_DefaultStripContext =
{
	0x90,
	{
		KM_OPAQUE_POLYGON,
		KM_USERCLIP_DISABLE,
		KM_NORMAL_POLYGON,
		KM_FLOATINGCOLOR,
		KM_FALSE,
		KM_TRUE
	},
	{
		KM_GREATER,
		KM_CULLCCW,
		KM_FALSE,
		KM_FALSE,
		0
	},
	{
		0,
		0
	},
	{
		KM_ONE,
		KM_ZERO,
		KM_FALSE,
		KM_FALSE,
		KM_NOFOG,
		KM_FALSE,
		KM_FALSE,
		KM_FALSE,
		KM_NOFLIP,
		KM_NOCLAMP,
		KM_BILINEAR,
		KM_FALSE,
		KM_MIPMAP_D_ADJUST_1_00,
		KM_MODULATE,
		0,
		0
	}
};

int REN_Initialise( DREAMCAST_RENDERERCONFIGURATION *p_pConfiguration )
{
	KMUINT32 PassIndex;

	g_Kamui2Config.dwSize = sizeof( g_Kamui2Config );

	g_Kamui2Config.flags =
		KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER |
		KM_CONFIGFLAG_NOWAITVSYNC |
		KM_CONFIGFLAG_ENABLE_2V_LATENCY |
		KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA;
	
	g_Kamui2Config.ppSurfaceDescArray =
		p_pConfiguration->ppSurfaceDescription;
	g_Kamui2Config.fb.nNumOfFrameBuffer = p_pConfiguration->FramebufferCount;

	g_Kamui2Config.nTextureMemorySize = p_pConfiguration->TextureMemorySize;
	g_Kamui2Config.nNumOfTextureStruct = p_pConfiguration->MaximumTextureCount;
	g_Kamui2Config.nNumOfSmallVQStruct =
		p_pConfiguration->MaximumSmallVQTextureCount;
	g_Kamui2Config.pTextureWork = p_pConfiguration->pTextureWorkArea;

	g_Kamui2Config.pVertexBuffer =
		( PKMDWORD )MEM_SH4_P2NonCachedMemory(
			p_pConfiguration->pVertexBuffer );
	g_Kamui2Config.pBufferDesc = p_pConfiguration->pVertexBufferDesc;
	g_Kamui2Config.nVertexBufferSize = p_pConfiguration->VertexBufferSize;

	g_Kamui2Config.nNumOfVertexBank = 1;

	g_Kamui2Config.nPassDepth = p_pConfiguration->PassCount;

	for( PassIndex = 0; PassIndex < p_pConfiguration->PassCount; ++PassIndex )
	{
		int BufferIndex = 0;

		for( ; BufferIndex < KM_MAX_DISPLAY_LIST; ++BufferIndex )
		{
			g_Kamui2Config.Pass[ PassIndex ].fBufferSize[ BufferIndex ] =
				p_pConfiguration->PassInfo[ PassIndex ].fBufferSize[
					BufferIndex ];
			g_Kamui2Config.Pass[ PassIndex ].dwOPBSize[ BufferIndex ] =
				p_pConfiguration->PassInfo[ PassIndex ].dwOPBSize[
					BufferIndex ];
		}

		g_Kamui2Config.Pass[ PassIndex ].nDirectTransferList =
			p_pConfiguration->PassInfo[ PassIndex ].nDirectTransferList;

		g_Kamui2Config.Pass[ PassIndex ].dwRegionArrayFlag =
			p_pConfiguration->PassInfo[ PassIndex ].dwRegionArrayFlag;
	}

	if( kmSetSystemConfiguration( &g_Kamui2Config ) != KMSTATUS_SUCCESS )
	{
		return 1;
	}

	return 0;
}

void REN_Terminate( void )
{
}

void REN_SetClearColour( float p_Red, float p_Green, float p_Blue )
{
	KMSTRIPHEAD BackgroundStripHead;
	KMVERTEX_01 BackgroundClear[ 3 ];
	KMPACKEDARGB BorderColour;

	memset( &BackgroundStripHead, 0, sizeof( BackgroundStripHead ) );
	kmGenerateStripHead01( &BackgroundStripHead, &g_DefaultStripContext );

	BackgroundClear[ 0 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
	BackgroundClear[ 0 ].fX = 0.0f;
	BackgroundClear[ 0 ].fY = 479.0f;
	BackgroundClear[ 0 ].u.fZ = 0.2f;
	BackgroundClear[ 0 ].fBaseAlpha = 1.0f;
	BackgroundClear[ 0 ].fBaseRed = p_Red;
	BackgroundClear[ 0 ].fBaseGreen = p_Green;
	BackgroundClear[ 0 ].fBaseBlue = p_Blue;


	BackgroundClear[ 1 ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
	BackgroundClear[ 1 ].fX = 639.0f;
	BackgroundClear[ 1 ].fY = 0.0f;
	BackgroundClear[ 1 ].u.fZ = 0.2f;
	BackgroundClear[ 1 ].fBaseAlpha = 1.0f;
	BackgroundClear[ 1 ].fBaseRed = p_Red;
	BackgroundClear[ 1 ].fBaseGreen = p_Green;
	BackgroundClear[ 1 ].fBaseBlue = p_Blue;

	BackgroundClear[ 2 ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
	BackgroundClear[ 2 ].fX = 639.0f;
	BackgroundClear[ 2 ].fY = 479.0f;
	BackgroundClear[ 2 ].u.fZ = 0.2f;
	BackgroundClear[ 2 ].fBaseAlpha = 1.0f;
	BackgroundClear[ 2 ].fBaseRed = p_Red;
	BackgroundClear[ 2 ].fBaseGreen = p_Green;
	BackgroundClear[ 2 ].fBaseBlue = p_Blue;

	kmSetBackGround( &BackgroundStripHead, KM_VERTEXTYPE_01,
		&BackgroundClear[ 0 ], &BackgroundClear[ 1 ], &BackgroundClear[ 2 ] );

	BorderColour.dwPacked = 0;

	kmSetBorderColor( BorderColour );
}

int REN_Clear( void )
{
	KMSTATUS Status;

	Status = kmBeginScene( &g_Kamui2Config );
	Status = kmBeginPass( g_Kamui2Config.pBufferDesc );

	return 0;
}

int REN_SwapBuffers( void )
{
	kmEndPass( g_Kamui2Config.pBufferDesc );
	kmRender( KM_RENDER_FLIP );
	kmEndScene( &g_Kamui2Config );

	return 0;
}

