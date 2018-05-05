#include <Serial.h>

/* For now, only allow serial communicaiton for development purposes */
#if defined ( DEBUG ) || defined( DEVELOPMENT )
#include <Log.h>
#include <stddef.h>
#include <Hardware.h>
#include <stdarg.h>

static unsigned char RecvBuf[ 1024 * 8 ];
static unsigned char SendBuf[ 1024 * 8 ];
static bool g_Initialised = false;

Sint32 SIF_Initialise_Int( Sint32 p_RecvBufferSize, Sint32 p_SendBufferSize,
	Sint32 p_Speed )
{
	if( scif_init( RecvBuf, 1024 * 8, SendBuf, 1024 * 8 ) != 0 )
	{
		return SIF_FATAL_ERROR;
	}

	if( scif_open( BPS_115200 ) != 0 )
	{
		return SIF_FATAL_ERROR;
	}

	g_Initialised = true;
	LOG_Debug( "[SIF_Initialise] <INFO> Initialised" );

	return SIF_OK;
}

void SIF_Terminate_Int( void )
{
	scif_close( );
	g_Initialised = false;
	LOG_Debug( "[SIF_Initialise] <INFO> Terminated" );
}

void SIF_Clear_Int( void )
{
	if( g_Initialised == true )
	{
		/* Black background, cyan text */
		scif_putq( 0x1B );
		SIF_Print( "[40;36m" );

		scif_putq( 0x1B );
		SIF_Print( "[H" );

		scif_putq( 0x1B );
		SIF_Print( "[2J" );
	}
}

void SIF_Print_Int( const char *p_pString, ... )
{
	if( g_Initialised == true )
	{
		char *pChar;
		static char String[ 256 ];
		va_list Args;

		va_start( Args, p_pString );
		vsprintf( String, p_pString, Args );
		va_end( Args );

		pChar = String;

		while( *pChar )
		{
			if( *pChar == '\n' )
			{
				SIF_NewLine( );
			}
			else
			{
				scif_putq( *pChar );
			}
			++pChar;
		}
	}
}

void SIF_PrintLine_Int( const char *p_pString )
{
	if( g_Initialised == true )
	{
		char *pChar = p_pString;
		while( *pChar )
		{
			if( *pChar == '\n' )
			{
				SIF_NewLine( );
			}
			else
			{
				scif_putq( *pChar );
			}
			++pChar;
		}

		SIF_NewLine( );
	}
}

void SIF_NewLine_Int( void )
{
	if( g_Initialised == true )
	{
		/* Hope that nothing is larger than this, otherwise we're screwed! */
		char CursorReport[ 16 ];
		char Row[ 4 ];
		size_t Offset = 0;
		size_t RowOffset = 0;

		memset( CursorReport, '\0', sizeof( CursorReport ) );
		memset( Row, '\0', sizeof( Row ) );

		/* Advance the line */
		scif_putq( 0x1B );
		SIF_Print( "[B" );
		/* Query the cursor position */
		scif_putq( 0x1B );
		SIF_Print( "[6n" );

		/* Wait for the return */
		while( scif_isget( ) == 0 )
		{
		}

		while( scif_isget( ) )
		{
			CursorReport[ Offset++ ] = scif_get( );
		}

		/* We only care about the row, so just get that */
		if( CursorReport[ 0 ] != 0x1B || CursorReport[ 1 ] != '[' )
		{
			/* Something went horribly wrong... */
			LOG_Debug( "[SIF_NewLine] <ERROR> Unable to insert a newline!" );
			LOG_Debug( "Cursor report: %s", CursorReport );

			return;
		}

		Offset = 2;

		while( CursorReport[ Offset ] != ';' )
		{
			Row[ RowOffset++ ] = CursorReport[ Offset++ ];
		}

		sprintf( CursorReport, "[%s;1f", Row );
		scif_putq( 0x1B );
		SIF_Print( CursorReport );
	}
}
#endif /* DEBUG || DEVELOPMENT */

