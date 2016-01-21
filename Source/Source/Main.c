#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <string.h>
#include <Memory.h>
#include <Log.h>

void main( void )
{
	debug_write( SNASM_STDOUT, "TERMINAL", strlen( "TERMINAL" ) );

	debug_write( SNASM_STDOUT, "Initialising memory system",
		strlen( "Initialising memory system" ) );

	MEM_Initialise( );

	debug_write( SNASM_STDOUT, "Done", strlen( "Done" ) );

	LOG_Initialise( NULL );

	LOG_Terminate( );

	MEM_Terminate( );

	while( 1 )
	{
	}
}

