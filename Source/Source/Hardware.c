#include <Hardware.h>
#include <Memory.h>
#include <Peripheral.h>
#include <Keyboard.h>
#include <FileSystem.h>
#include <Log.h>

KMVOID PALExtCallback( PKMVOID p_pArgs );

int HW_Initialise( KMBPPMODE p_BPP, SYE_CBL *p_pCableType,
	PMEMORY_FREESTAT p_pMemoryFree )
{
	KMDISPLAYMODE DisplayMode;

	switch( syCblCheck( ) )
	{
		case SYE_CBL_NTSC:
		{
			DisplayMode = KM_DSPMODE_NTSCNI640x480;
			break;
		}
		case SYE_CBL_PAL:
		{
			if( p_pCableType )
			{
				( *p_pCableType ) = SYE_CBL_PAL;
			}
			DisplayMode = KM_DSPMODE_PALNI640x480EXT;

			break;
		}
		case SYE_CBL_VGA:
		{
			DisplayMode = KM_DSPMODE_VGA;
			break;
		}
		default:
		{
			HW_Reboot( );
		}
	}

	set_imask( 15 );

	syHwInit( );
	LOG_Debug( "Setting operand cache to RAM mode" );
	syCacheInit( SYD_CACHE_FORM_IC_ENABLE |
		SYD_CACHE_FORM_OC_ENABLE |
		SYD_CACHE_FORM_P1_CB |
		SYD_CACHE_FORM_OC_RAM |
		SYD_CACHE_FORM_OC_INDEX );

	MEM_Initialise( p_pMemoryFree );
    syStartGlobalConstructor( );
	kmInitDevice( KM_DREAMCAST );

    kmSetDisplayMode( DisplayMode, p_BPP, TRUE, FALSE);

    kmSetWaitVsyncCount( 1 );
	syHwInit2( );

	PER_Initialise( );
	KBD_Initialise( );
	syRtcInit( );

	set_imask( 0 );

	if( FS_Initialise( ) != 0 )
	{
		LOG_Debug( "Failed to initialise the file system" );
		return 1;
	}

	if( syCblCheck( ) == SYE_CBL_PAL )
	{
		kmSetPALEXTCallback( PALExtCallback, NULL );
		kmSetDisplayMode( DisplayMode, p_BPP, TRUE, FALSE );
	}

	return 0;
}

void HW_Terminate( void )
{
	FS_Terminate( );
	syRtcFinish( );
	KBD_Terminate( );
	PER_Terminate( );
	kmUnloadDevice( );
    syStartGlobalDestructor();
	MEM_Terminate( );

	syHwFinish( );

	set_imask( 15 );
}

void HW_Reboot( void )
{
	syBtExit( );
}

const char *HW_PortToName( Uint32 p_Port )
{
	switch( p_Port )
	{
		/* A-0:5 */
		case PDD_PORT_A0:
		{
			return "A-0";
		}
		case PDD_PORT_A1:
		{
			return "A-1";
		}
		case PDD_PORT_A2:
		{
			return "A-2";
		}
		case PDD_PORT_A3:
		{
			return "A-3";
		}
		case PDD_PORT_A4:
		{
			return "A-4";
		}
		case PDD_PORT_A5:
		{
			return "A-5";
		}

		/* B-0:5 */
		case PDD_PORT_B0:
		{
			return "B-0";
		}
		case PDD_PORT_B1:
		{
			return "B-1";
		}
		case PDD_PORT_B2:
		{
			return "B-2";
		}
		case PDD_PORT_B3:
		{
			return "B-3";
		}
		case PDD_PORT_B4:
		{
			return "B-4";
		}
		case PDD_PORT_B5:
		{
			return "B-5";
		}

		/* C-0:5 */
		case PDD_PORT_C0:
		{
			return "C-0";
		}
		case PDD_PORT_C1:
		{
			return "C-1";
		}
		case PDD_PORT_C2:
		{
			return "C-2";
		}
		case PDD_PORT_C3:
		{
			return "C-3";
		}
		case PDD_PORT_C4:
		{
			return "C-4";
		}
		case PDD_PORT_C5:
		{
			return "C-5";
		}

		/* D-0:5 */
		case PDD_PORT_D0:
		{
			return "D-0";
		}
		case PDD_PORT_D1:
		{
			return "D-1";
		}
		case PDD_PORT_D2:
		{
			return "D-2";
		}
		case PDD_PORT_D3:
		{
			return "D-3";
		}
		case PDD_PORT_D4:
		{
			return "D-4";
		}
		case PDD_PORT_D5:
		{
			return "D-5";
		}

		/* This should never happen */
		default:
		{
			return "UNKNOWN";
		}
	}
}

KMVOID PALExtCallback( PKMVOID p_pArgs )
{
	PKMPALEXTINFO pPALInfo;

	pPALInfo = ( PKMPALEXTINFO )p_pArgs;
	pPALInfo->nPALExtMode = KM_PALEXT_HEIGHT_RATIO_1_166;
}


