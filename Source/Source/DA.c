#include <DA.h>

static g_DAConnected = false;

bool DA_GetConnectionStatus( void )
{
	return g_DAConnected;
}

