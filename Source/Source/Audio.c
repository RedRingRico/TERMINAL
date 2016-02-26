#include <Audio.h>
#include <FileSystem.h>
#include <Log.h>

static AC_ERROR *g_pACError = NULL;

int AUD_Initialise( PAUDIO_PARAMETERS p_pAudioParameters )
{
	/* Driver image must be 32-byte aligned */
	Uint32 *pDriverImage = NULL;
	Sint32 DriverSize;
	Uint32 DriverImageSize;
	GDFS FileHandle;
	long FileBlocks;

	g_pACError = acErrorGetLast( );

	acIntInstallCallbackHandler( p_pAudioParameters->IntCallback );

	/* Initialise the interrupt system */
	if( acIntInit( ) == 0 )
	{
		LOG_Debug( "Failed to start the audio interrupt system" );

		return 1;
	}

	if( !( FileHandle = FS_OpenFile( "/AUDIO/AUDIO64.DRV" ) ) )
	{
		LOG_Debug( "Failed to open the audio driver file" );

		return 1;
	}

	gdFsGetFileSize( FileHandle, &DriverSize );
	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	DriverImageSize = SECTOR_ALIGN( DriverSize );

	/* Temporarily load the driver */
	/* REMINDER:
	 * This should be using the memory manager */
	pDriverImage = ( Uint32 * )syMalloc( DriverImageSize );

	if( pDriverImage == NULL )
	{
		LOG_Debug( "Failed to allocate memory for the driver image" );

		return 1;
	}

	gdFsReqRd32( FileHandle, FileBlocks, pDriverImage );

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	if( acSystemInit( AC_DRIVER_DA, pDriverImage, DriverSize, false ) == 0 )
	{
		LOG_Debug( "Failed to initialise the Audio64 system" );

		return 1;
	}
#if defined ( DEBUG )
	{
		Uint8 Major, Minor;
		char Local;

		acSystemGetDriverRevision( ( Uint8 * )pDriverImage, &Major, &Minor,
			&Local );

		LOG_Debug( "Initialised Audio64" );
		LOG_Debug( "%s: %d.%d.%c", AUDIO_64_REVISION, AC_MAJOR_REVISION,
			AC_MINOR_REVISION, AC_LOCAL_REVISION );

		LOG_Debug( "Using sound driver \"%s\" version: %d.%d.%d",
			"/AUDIO/AUDIO64.DRV", Major, Minor, Local );
	}
#endif /* DEBUG */

	syFree( pDriverImage );

	return 0;
}

void AUD_Terminate( void )
{
	acSystemShutdown( );
}

