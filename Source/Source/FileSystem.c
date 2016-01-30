#include <FileSystem.h>
#include <Log.h>

#define GDROM_MAX_OPEN_FILES	10
#define GDROM_BUFFER_COUNT		512

static Uint8 g_GDFSWork[ GDFS_WORK_SIZE( GDROM_MAX_OPEN_FILES ) + 32 ];
static Uint8 g_GDFSHandleTable[ GDFS_DIRREC_SIZE( GDROM_BUFFER_COUNT ) + 32 ];

void GDFSErrorCallback( void *p_pObject, long p_Error );

int FS_Initialise( void )
{
	Uint8 *pWork, *pDirectory;
	Sint32 Error, Itr;

	pWork =
		( Uint8 * )( ( ( Uint32 )g_GDFSWork & 0xFFFFFFE0 ) + 0x20 );
	pDirectory =
		( Uint8 * )( ( ( Uint32 )g_GDFSHandleTable & 0xFFFFFFE0 ) +
			0x20 );
	
	for( Itr = 8; Itr > 0; --Itr )
	{
		Error = gdFsInit( GDROM_MAX_OPEN_FILES, pWork,
			GDROM_BUFFER_COUNT, pDirectory );

		if( ( Error == GDD_ERR_TRAYOPEND ) ||
			( Error == GDD_ERR_UNITATTENT ) )
		{
			return 1;
		}
		else if( Error == GDD_ERR_OK )
		{
			break;
		}
	}

	if( Itr == 0 )
	{
		return 1;
	}

	gdFsEntryErrFuncAll( GDFSErrorCallback, NULL );

#if defined ( DEBUG )
	{
		LOG_Debug( "Initialised GD-ROM | Maximum open files: %d | Open buffer "
			"count: %d", GDROM_MAX_OPEN_FILES, GDROM_BUFFER_COUNT );
		LOG_Debug( "%s", GDD_VERSION_STR );
	}
#endif /* DEBUG */

	return 0;
}

int FS_Terminate( void )
{
	gdFsFinish( );

	return 0;
}

void GDFSErrorCallback( void *p_pObject, long p_Error )
{
	if( ( p_Error == GDD_ERR_TRAYOPEND ) ||
		( p_Error == GDD_ERR_UNITATTENT ) )
	{
		/* Hard reset, look into making this more forgiving and useful */
		HW_Terminate( );
		HW_Reboot( );
	}
}

