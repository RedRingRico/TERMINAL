#include <FileSystem.h>
#include <Log.h>
#include <string.h>

#define GDROM_MAX_OPEN_FILES	10
#define GDROM_BUFFER_COUNT		512

static Uint8 g_GDFSWork[ GDFS_WORK_SIZE( GDROM_MAX_OPEN_FILES ) + 32 ];
static Uint8 g_GDFSHandleTable[ GDFS_DIRREC_SIZE( GDROM_BUFFER_COUNT ) + 32 ];

static GDFS_DIRREC g_RootDirectory;

void GDFSErrorCallback( void *p_pObject, long p_Error );

int FS_Initialise( void )
{
	Uint8 *pWork, *pDirectory;
	Sint32 Error, Itr;
	Uint32 RootDirectoryBuffer[ GDFS_DIRREC_SIZE( 64 ) ];

	pWork =
		( Uint8 * )( ( ( Uint32 )g_GDFSWork & 0xFFFFFFE0 ) + 0x20 );
	pDirectory =
		( Uint8 * )( ( ( Uint32 )g_GDFSHandleTable & 0xFFFFFFE0 ) +
			0x20 );
	
	for( Itr = 8; Itr > 0; --Itr )
	{
		Error = gdFsInit( GDROM_MAX_OPEN_FILES, pWork,
			GDROM_BUFFER_COUNT, pDirectory );

		if( Error == GDD_ERR_TRAYOPEND )
		{
			LOG_Debug( "[FS_Initialise] Disc tray open" );
			return 1;
		}
		else if( Error == GDD_ERR_UNITATTENT )
		{
			LOG_Debug( "[FS_Initialise] Hardware failure" );
			return 1;
		}
		else if( Error == GDD_ERR_OK )
		{
			break;
		}
	}

	if( Itr == 0 )
	{
		LOG_Debug( "[FS_Initialise] Retried initiaising the file system eight "
			"times, did not succeed" );
		return 1;
	}

	gdFsEntryErrFuncAll( GDFSErrorCallback, NULL );

	g_RootDirectory = gdFsCreateDirhn( RootDirectoryBuffer, 64 );
	gdFsLoadDir( ".", g_RootDirectory );

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

GDFS FS_OpenFile( char *p_pFilePath )
{
	char *pFileName;
	char File[ GDD_FS_FNAMESIZE ];
	char FilePath[ 256 ];
	GDFS_DIRREC Directory;
	/* Pretty bad, currently after a few calls the g_RootDirectory gets
	 * overwritten, so this is static for now */
	static Uint32 DirectoryBuffer[ GDFS_DIRREC_SIZE( 64 ) ];
	GDFS ReturnFile;

	/* Save the directory used before entering */
	Directory = gdFsCreateDirhn( DirectoryBuffer, 64 );
	gdFsLoadDir( ".", Directory );

	/* For an absolute path, begin with the root directory */
	if( p_pFilePath[ 0 ] == '/' )
	{
		gdFsSetDir( g_RootDirectory );
	}

	/* Don't modify the input file path */
	strncpy( FilePath, p_pFilePath, strlen( p_pFilePath ) );
	FilePath[ strlen( p_pFilePath ) ] = '\0';

	/* Break the path down for GDFS to consume */
	pFileName = strtok( FilePath, "/" );

	while( pFileName != NULL )
	{
		GDFS_DIRINFO FileInformation;
		strcpy( File, pFileName );

		if( gdFsGetDirInfo( pFileName, &FileInformation ) != GDD_ERR_OK )
		{
			LOG_Debug( "FILE: \"%s\" invalid", File );

			/* Restore the directory used before entering the function */
			gdFsSetDir( Directory );

			return NULL;
		}

		if( FileInformation.flag & GDD_FF_DIRECTORY )
		{
			gdFsChangeDir( File );
		}

		pFileName = strtok( NULL, "/" );
	}

	ReturnFile = gdFsOpen( File, NULL );

	/* Restore the directory used before entering the function */
	gdFsSetDir( Directory );

	return ReturnFile;
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

