#include <Hardware.h>
#include <shinobi.h>
#include <kamui2.h>
#include <Memory.h>
#include <Peripheral.h>
#include <FileSystem.h>

int HW_Initialise( void )
{
	set_imask( 15 );

	syHwInit( );
	MEM_Initialise( );
    syStartGlobalConstructor( );
	kmInitDevice( KM_DREAMCAST );

    kmSetDisplayMode( KM_DSPMODE_VGA, KM_DSPBPP_RGB888, TRUE, FALSE);
    kmSetWaitVsyncCount (0);
	syHwInit2( );

	PER_Initialise( );
	syRtcInit( );

	set_imask( 0 );

	if( FS_Initialise( ) != 0 )
	{
		return 1;
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

