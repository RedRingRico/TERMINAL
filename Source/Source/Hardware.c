#include <Hardware.h>
#include <Memory.h>
#include <Peripheral.h>
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
	MEM_Initialise( p_pMemoryFree );
    syStartGlobalConstructor( );
	kmInitDevice( KM_DREAMCAST );

    kmSetDisplayMode( DisplayMode, p_BPP, TRUE, FALSE);

    kmSetWaitVsyncCount( 1 );
	syHwInit2( );

	PER_Initialise( );
	syRtcInit( );

	set_imask( 0 );

	if( FS_Initialise( ) != 0 )
	{
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

KMVOID PALExtCallback( PKMVOID p_pArgs )
{
	PKMPALEXTINFO pPALInfo;

	pPALInfo = ( PKMPALEXTINFO )p_pArgs;
	pPALInfo->nPALExtMode = KM_PALEXT_HEIGHT_RATIO_1_166;
}

