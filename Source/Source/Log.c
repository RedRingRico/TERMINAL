#if defined ( DEBUG )
#include <Log.h>
#include <shinobi.h>
#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <SHC/errno.h>
#include <stdarg.h>

static int g_FileHandle;

static void DebugOut( const char *p_pString )
{
	if( p_pString != NULL )
	{
		debug_write( SNASM_STDOUT, p_pString, strlen( p_pString ) );
	}
}

int LOG_Initialise_Int( const char *p_pLogFileName )
{
	char *pLogFile = NULL;

	DebugOut( "Initialising log file" );

	if( p_pLogFileName )
	{
		pLogFile = malloc( ( sizeof( char ) * strlen( p_pLogFileName ) ) + 1 );
		strncpy( pLogFile, p_pLogFileName, strlen( p_pLogFileName ) );
		pLogFile[ strlen( p_pLogFileName ) ] = '\0';
	}
	else
	{
		SYS_RTC_DATE Date;
		char DateFile[ 15 ];

		memset( DateFile, '\0', 15 );
		syRtcGetDate( &Date );

		/* Format: YYYY-MM-DD_HH-MM-SS */
		sprintf( DateFile, "%04d-%02d-%02d_%02d-%02d-%02d.log", Date.year,
			Date.month, Date.day, Date.hour, Date.minute, Date.second );

		pLogFile = malloc( sizeof( char ) * strlen( DateFile ) + 1 );
		strncpy( pLogFile, DateFile, strlen( DateFile ) );
		pLogFile[ strlen( DateFile ) ] = '\0';
	}

	if( ( g_FileHandle = debug_open( pLogFile,
		SNASM_O_WRONLY | SNASM_O_CREAT | SNASM_O_TRUNC | SNASM_O_TEXT,
		SNASM_S_IREAD | SNASM_S_IWRITE ) ) == -1 )
	{
		DebugOut( "[ERROR] Failed to open file for writing" );

		switch( errno )
		{
			case SNASM_EACCESS:
			{
				DebugOut( "ERROR: Access" );
				break;
			}
			default:
			{
				DebugOut( "ERROR: Unknown" );
				break;
			}
		}

		free( pLogFile );

		return 1;
	}

	free( pLogFile );

	LOG_Debug( "Log initialised" );

	return 0;
}

void LOG_Terminate_Int( void )
{
	if( g_FileHandle != -1 )
	{
		debug_close( g_FileHandle );
		g_FileHandle = -1;
	}
}

void LOG_Debug_Int( const char *p_pMessage, ... )
{
	if( strlen( p_pMessage ) && p_pMessage )
	{
		static char Buffer[ 256 ];
		va_list Args;

		sprintf( Buffer, "[DEBUG] " );

		va_start( Args, p_pMessage );
		vsprintf( Buffer + strlen( "[DEBUG] " ), p_pMessage, Args );
		va_end( Args );

		debug_write( g_FileHandle, Buffer, strlen( Buffer ) );
		debug_write( SNASM_STDOUT, Buffer, strlen( Buffer ) );
	}
}
#else
#include <Log.h>
#endif /* DEBUG */

