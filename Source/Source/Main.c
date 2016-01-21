#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <string.h>

void main( void )
{
	debug_write( SNASM_STDOUT, "TERMINAL", strlen( "TERMINAL" ) );
}

