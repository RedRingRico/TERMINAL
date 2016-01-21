#include <Hardware.h>
#include <shinobi.h>
#include <kamui2.h>
#include <Memory.h>
#include <Peripheral.h>

int HW_Initialise( void )
{
	set_imask( 15 );

	syHwInit( );
	MEM_Initialise( );
	kmInitDevice( KM_DREAMCAST );

    kmSetDisplayMode (KM_DSPMODE_VGA, KM_DSPBPP_RGB888, TRUE, FALSE);
    kmSetWaitVsyncCount (0);
	syHwInit2( );

	PER_Initialise( );
	syRtcInit( );

	set_imask( 0 );

	return 0;
}

void HW_Terminate( void )
{
	syRtcFinish( );
	PER_Terminate( );
	kmUnloadDevice( );
	MEM_Terminate( );

	syHwFinish( );

	set_imask( 15 );
}

void HW_Reboot( void )
{
	syBtExit( );
}

