#include <StorageUnit.h>
#include <Log.h>
#include <sg_syrtc.h>

static STORAGEUNIT_INFO g_StorageUnits[ SU_MAX_DRIVES ];
static PMEMORY_BLOCK g_pMemoryBlock;
static size_t g_ConnectedStorageUnits;
static size_t g_MountedStorageUnits;

static void InitialiseCallback( void );
static Sint32 CompleteCallback( Sint32 p_Drive, Sint32 p_Operation,
	Sint32 p_Status, Uint32 p_Parameter );
static Sint32 ProgressCallback( Sint32 p_Drive, Sint32 p_Operation,
	Sint32 p_Count, Sint32 p_Maximum );
static void ClearDriveInformation( Sint32 p_Drive );

Sint32 SU_Initialise( PMEMORY_BLOCK p_pMemoryBlock )
{
	if( p_pMemoryBlock == NULL )
	{
		LOG_Debug( "[SU_Initialise] Invalid memory block" );

		return -1;
	}

	g_pMemoryBlock = p_pMemoryBlock;
	g_ConnectedStorageUnits = 0;
	memset( g_StorageUnits, 0, sizeof( g_StorageUnits ) );

	switch( buInit( 0, BUD_USE_DRIVE_ALL, NULL, InitialiseCallback ) )
	{
		case BUD_ERR_OK:
		{
			LOG_Debug( "Backup library initialised" );
			break;
		}
		case BUD_ERR_INVALID_PARAM:
		{
			LOG_Debug( "Backup library received an invalid parameter" );
			return -1;
		}
	}

	return 0;
}

void SU_Terminate( void )
{
	Sint32 DrivesUnmounted = 0;

	SU_UnmountDrives( &DrivesUnmounted );
	LOG_Debug( "[SU_Terminate] <INFO> Unmounted %d drive%s", DrivesUnmounted,
		( DrivesUnmounted == 1 ) ? "" : "s");

	do
	{
	} while( buExit( ) != BUD_ERR_OK );

	LOG_Debug( "Backup library terminated" );
}

Sint32 SU_GetConnectedStorageUnits( PSTORAGEUNIT_INFO p_pConnectedUnits,
	size_t *p_pConnectedUnitCount )
{
	if( p_pConnectedUnits != NULL )
	{
		/* p_pConnectedUnits should have been initialised and allocated enough
		 * memory beforehand */
		size_t Index, Offset = 0;
		for( Index = 0; Index < SU_MAX_DRIVES; ++Index )
		{
			if( g_StorageUnits[ Index ].Flags & SUI_CONNECTED )
			{
				memcpy( &p_pConnectedUnits[ Offset++ ],
					&g_StorageUnits[ Index ], sizeof( STORAGEUNIT_INFO ) );
			}
		}
	}
	
	if( p_pConnectedUnitCount )
	{
		( *p_pConnectedUnitCount ) = g_ConnectedStorageUnits;

		return 0;
	}

	return 0;
}

Sint32 SU_GetMountedStorageUnits( PSTORAGEUNIT_INFO p_pMountedUnits,
	size_t *p_pMountedUnitCount )
{
	if( p_pMountedUnits != NULL )
	{
		if( g_MountedStorageUnits != 0 )
		{
			/* p_pMountedUnits should have been initialised and allocated
			 * enough memory beforehand */
			size_t Index, Offset = 0;
			for( Index = 0; Index < SU_MAX_DRIVES; ++Index )
			{
				if( g_StorageUnits[ Index ].Flags & SUI_READY )
				{
					memcpy( &p_pMountedUnits[ Offset++ ],
						&g_StorageUnits[ Index ], sizeof( STORAGEUNIT_INFO ) );
				}
			}
		}
	}

	if( p_pMountedUnitCount )
	{
		( *p_pMountedUnitCount ) = g_MountedStorageUnits;
	}

	return 0;
}

Sint32 SU_MountDrive( Sint32 p_Drive )
{
	PSTORAGEUNIT_INFO pInformation = &g_StorageUnits[ p_Drive ];
	Sint32 MountStatus;

	/* Already mounted */
	if( pInformation->pWorkAddress )
	{
		LOG_Debug( "[SU_MountDrive] <INFO> Drive %d already mounted",
			p_Drive );

		return 0;
	}

	pInformation->pWorkAddress = MEM_AllocateFromBlock( g_pMemoryBlock,
		pInformation->WorkSize, "Memory unit" );

	if( pInformation->pWorkAddress == NULL )
	{
		LOG_Debug( "Failed to allocate memory for memory unit %d", p_Drive );

		return -1;
	}

	MountStatus = buMountDisk( p_Drive, pInformation->pWorkAddress,
		pInformation->WorkSize );

	switch( MountStatus )
	{
		case BUD_ERR_OK:
		{
			LOG_Debug( "[SU_MountDrive] <INFO> Drive %d mounted", p_Drive );
			break;
		}
		default:
		{
			LOG_Debug( "[SU_MountDrive] <WARNING> Unhandled return value for "
				"drive %d [0x%08X]", p_Drive, MountStatus );
		}
	}

	return 0;
}

Sint32 SU_UnmountDrive( Sint32 p_Drive )
{
	PSTORAGEUNIT_INFO pInformation = &g_StorageUnits[ p_Drive ];
	Sint32 Unmount;
	Uint32 Retries = 0;

	if( pInformation->pWorkAddress == NULL )
	{
		/* Memory unit isn't mounted */
		return 0;
	}

TryUnmount:
	if( Retries > 3 )
	{
		return SU_BUSY;
	}

	Unmount = buUnmount( p_Drive );
	switch( Unmount )
	{
		case BUD_ERR_OK:
		{
			MEM_FreeFromBlock( g_pMemoryBlock, pInformation->pWorkAddress );
			pInformation->pWorkAddress = NULL;
			ClearDriveInformation( p_Drive );

			return 0;
		}
		case BUD_ERR_NO_DISK:
		{
			LOG_Debug( "[SU_UnmountDrive] <INFO> No disk connected at index "
				"%d", p_Drive );

			return SU_NO_DISK;
		}
		case BUD_ERR_BUSY:
		{
			/* Wait it out... */
			LOG_Debug( "[SU_UnmountDrive] <INFO> Waiting on disk %d "
				"to finish processing", p_Drive );

			while( buStat( p_Drive ) == BUD_STAT_BUSY )
			{
			}

			LOG_Debug( "[SU_UnmountDrives] <INFO> Disk %d has "
				"finished processing", p_Drive );

			++Retries;

			goto TryUnmount;
		}
		default:
		{
			LOG_Debug( "[SU_UnmountDrive] <WARNING> Unhandled return value: "
				"0x%08X", Unmount );
		}
	}

	LOG_Debug( "[SU_UnmountDrive] Failed to unmount drive %d [0x%08X]",
		p_Drive, Unmount );
	
	return -1;
}

Sint32 SU_MountDrives( Sint32 *p_pDrivesMounted )
{
	Sint32 Drive;

	if( p_pDrivesMounted == NULL )
	{
		LOG_Debug( "[SU_MountDrives] p_pDrivesMounted parameter is NULL" );

		return 1;
	}

	/* Attempt to mount all drives, if they are connected */
	for( Drive = 0; Drive < SU_MAX_DRIVES; ++Drive )
	{
		if( g_StorageUnits[ Drive ].Flags & SUI_CONNECTED )
		{
			if( SU_MountDrive( Drive ) != 0 )
			{
				LOG_Debug( "Error mounting all drives" );

				return -1;
			}

			++( *p_pDrivesMounted );
		}
	}

	LOG_Debug( "[SU_MountDrives] Mounted %d drive%s", ( *p_pDrivesMounted ),
		( ( *p_pDrivesMounted ) == 1 ) ? "" : "s" );

	return 0;
}

Sint32 SU_UnmountDrives( Sint32 *p_pDrivesUnmounted )
{
	Sint32 Drive;

	if( p_pDrivesUnmounted == NULL )
	{
		LOG_Debug( "[SU_UnmountDrives] p_pDrivesUnmounted parameter is NULL" );

		return 1;
	}

	/* Unmount drives, this will return the drives that have been unmounted */
	for( Drive = 0; Drive < SU_MAX_DRIVES; ++Drive )
	{
		if( g_StorageUnits[ Drive ].Flags & SUI_CONNECTED )
		{
			Sint32 UnmountStatus = SU_UnmountDrive( Drive );
			switch( UnmountStatus )
			{
				case 0:
				{
					++( *p_pDrivesUnmounted );
					break;
				}
				case SU_NO_DISK:
				{
					LOG_Debug( "[SU_UnmountDrives] <ERROR> Failed to unmount "
						"drive %d, continuing to unmount other drives",
						Drive );

					continue;
				}
				case SU_BUSY:
				{
					LOG_Debug( "[SU_UnmountDrives] <INFO> Drive %d busy",
						Drive );

					continue;
				}
				default:
				{
					return -1;
				}
			}
		}
	}

	return 0;
}

Uint32 SU_DriveToFlag( Uint8 p_Drive )
{
	Uint32 Flag = 0x000000FF & p_Drive;
	Flag = Flag << 28;

	return Flag;
}

Uint8 SU_FlagToDrive( Uint32 p_Flag )
{
	Uint8 Drive = p_Flag >> 28;

	return Drive;
}

bool SU_FindFileOnDrive( Sint32 p_Drive, char *p_pFileName )
{
	Sint32 Status;
	PSTORAGEUNIT_INFO pInformation = &g_StorageUnits[ p_Drive ];

	if( pInformation->Flags & ( SUI_READY | SUI_FORMATTED ) )
	{
		do
		{
			Status = buIsExistFile( p_Drive, p_pFileName );
		} while( Status == BUD_ERR_BUSY );

		if( Status < 0 )
		{
			if( Status == BUD_ERR_FILE_NOT_FOUND )
			{
				LOG_Debug( "[SU_FindFileOnDrive(%d)] <INFO> File \"%s\" not "
					"found", p_Drive, p_pFileName );

				return false;
			}
		}

		return true;
	}

	return false;
}

bool SU_FindFileAcrossDrives( char *p_pFileName, bool p_StopAtFirstDrive,
	Uint8 *p_pFoundOnDrives )
{
	size_t Drive;
	Uint8 FoundOnDrives = 0;

	for( Drive = 0; Drive < SU_MAX_DRIVES; ++Drive )
	{
		if( SU_FindFileOnDrive( Drive, p_pFileName ) == true )
		{
			FoundOnDrives |= ( 1 << Drive );

			if( p_StopAtFirstDrive == true )
			{
				break;
			}
		}
	}

	if( FoundOnDrives != 0 )
	{
		if( p_pFoundOnDrives )
		{
			( *p_pFoundOnDrives ) = FoundOnDrives;
		}

		return true;
	}

	return false;
}

static void InitialiseCallback( void )
{
	buSetCompleteCallback( CompleteCallback );
	buSetProgressCallback( ProgressCallback );
}

static Sint32 CompleteCallback( Sint32 p_Drive, Sint32 p_Operation,
	Sint32 p_Status, Uint32 p_Parameter )
{
	PSTORAGEUNIT_INFO pInformation;
	Sint32 Return;

	pInformation = &g_StorageUnits[ p_Drive ];

#if defined ( DEBUG )
	LOG_Debug( "[BUP] Complete callback" );
#endif /* DEBUG */

	switch( p_Operation )
	{
		case BUD_OP_CONNECT:
		{
			pInformation->WorkSize = BUM_WORK_SIZE( p_Status, 1 );
			pInformation->Capacity = p_Status;
			pInformation->Flags |= SUI_CONNECTED;
			pInformation->Flags |= SU_DriveToFlag( p_Drive );
			++g_ConnectedStorageUnits;
#if defined ( DEBUG )
			LOG_Debug( "Memory unit %d connected", p_Drive );
			LOG_Debug( "    Work size: %d", pInformation->WorkSize );
			LOG_Debug( "    Capacity:  %d", pInformation->Capacity );
			LOG_Debug( "    Drive:     %d", SU_FlagToDrive( SU_DriveToFlag( p_Drive ) ) );
#endif /* DEBUG */
			/* Attempt to auto-mount the drive */
			SU_MountDrive( p_Drive );

			break;
		}
		case BUD_OP_MOUNT:
		{
			if( p_Status == BUD_ERR_OK )
			{
				Uint8 FoundOnDrives;
#if defined ( DEBUG )
				LOG_Debug( "[BUP] <INFO> Memory unit %d mounted", p_Drive );
#endif /* DEBUG */
				pInformation->Flags |= SUI_READY;
				++g_MountedStorageUnits;
				if( buGetDiskInfo( p_Drive, &pInformation->DiskInformation ) ==
					BUD_ERR_OK )
				{
					pInformation->Flags |= SUI_FORMATTED;
				}
				pInformation->LastError = BUD_ERR_OK;
			}
			break;
		}
		case BUD_OP_UNMOUNT:
		{
			if( pInformation->pWorkAddress )
			{
				MEM_FreeFromBlock( g_pMemoryBlock,
					pInformation->pWorkAddress );
				pInformation->pWorkAddress = NULL;
				--g_MountedStorageUnits;
#if defined( DEBUG )
				LOG_Debug( "[BUP] <INFO> Memory unit %d unmounted", p_Drive );
#endif /* DEBUG */
			}
			ClearDriveInformation( p_Drive );
			if( pInformation->Flags & SUI_CONNECTED )
			{
				pInformation->Flags &= ~( SUI_CONNECTED );
				--g_ConnectedStorageUnits;
#if defined( DEBUG )
				LOG_Debug( "[BUP] <INFO> Memory unit %d disconnected",
					p_Drive );
#endif /* DEBUG */
			}
			break;
		}
		default:
		{
			pInformation->LastError = p_Status;
			/* Update the information of the memory unit */
			buGetDiskInfo( p_Drive, &pInformation->DiskInformation );
		}
	}

	pInformation->Operation = 0;

	return BUD_CBRET_OK;
}

static Sint32 ProgressCallback( Sint32 p_Drive, Sint32 p_Operation,
	Sint32 p_Count, Sint32 p_Maximum )
{
	PSTORAGEUNIT_INFO pInformation = &g_StorageUnits[ p_Drive ];

	pInformation->ProgressCount = p_Count;
	pInformation->ProgressMaximum = p_Maximum;
	pInformation->Operation = p_Operation;

	return BUD_CBRET_OK;
}


static void ClearDriveInformation( Sint32 p_Drive )
{
	PSTORAGEUNIT_INFO pInformation;

	pInformation = &g_StorageUnits[ p_Drive ];

	pInformation->pWorkAddress = NULL;
	pInformation->ProgressCount = 0;
	pInformation->ProgressMaximum = 0;
	pInformation->Operation = 0;
	LOG_Debug( "Drive %d flags before: 0x%08X", p_Drive, pInformation->Flags );
	pInformation->Flags &= ~( SUI_READY | SUI_FORMATTED );
	LOG_Debug( "Drive %d flags after:  0x%08X", p_Drive, pInformation->Flags );
	memset( &pInformation->DiskInformation, 0,
		sizeof( pInformation->DiskInformation ) );
}


Sint32 SU_SaveFile( Sint32 p_Drive, const char *p_pFileName, const *p_pData,
	const Sint32 p_BlockCount, const char *p_pVMUComment,
	const char *p_pBootROMComment )
{
	BUS_BACKUPFILEHEADER FileHeader;
	Uint8 TestData[ 128 ];
	Uint8 IconData[ 512 ];
	Uint16 IconPalette[ 16 ];
	Sint32 BlockCount;
	void *pBuffer;
	BUS_TIME Time =
	{
		1998, 12, 31,
		23, 59, 59,
		4
	};
	Sint32 SaveReturn;
	Sint32 ReturnValue = SU_OK;

	memset( IconData, 0, sizeof( IconData ) );
	memset( IconPalette, 0, sizeof( IconPalette ) );

	strncpy( FileHeader.vms_comment, p_pVMUComment, 16 );
	strncpy( FileHeader.btr_comment, p_pBootROMComment, 32 );
	strncpy( FileHeader.game_name, SU_GAME_NAME, 16 );
	FileHeader.icon_palette = IconPalette;
	FileHeader.icon_data = IconData;
	FileHeader.icon_num = 1;
	FileHeader.icon_speed = 1;
	FileHeader.visual_data = NULL;
	FileHeader.visual_type = BUD_VISUALTYPE_NONE;
	FileHeader.reserved = 0;
	FileHeader.save_data = TestData;
	FileHeader.save_size = 128;

	BlockCount = buCalcBackupFileSize( FileHeader.icon_num,
		FileHeader.visual_type, FileHeader.save_size );

	if( BlockCount < 0 )
	{
		LOG_Debug( "[SU_SaveFile] <ERROR> Failed to calculate the backup file "
			"size.  Invalid parameter?" );

		return SU_FATAL_ERROR;
	}

	pBuffer = MEM_AllocateFromBlock( g_pMemoryBlock, BlockCount * 512,
		"Backup File Header" );
	BlockCount = buMakeBackupFileImage( pBuffer, &FileHeader );

	SaveReturn = buSaveFile( p_Drive, p_pFileName, pBuffer, BlockCount, &Time,
		BUD_FLAG_VERIFY | BUD_FLAG_COPY( 0X00 ) );

	if( SaveReturn != BUD_ERR_OK )
	{
		switch( SaveReturn )
		{
			case BUD_ERR_CANNOT_CREATE:
			{
				LOG_Debug( "[SU_SaveFile] <ERROR> Could not create save file: "
					"\"%s\"", p_pFileName );

				ReturnValue = SU_CREATE_ERROR;

				break;
			}
			case BUD_ERR_VERIFY:
			{
				LOG_Debug( "[SU_SaveFile] <ERROR> Could not verify the save "
					"file: \"%s\"", p_pFileName );

				ReturnValue = SU_VERIFY_ERROR;

				break;
			}
			default:
			{
				LOG_Debug( "[SU_SaveFile] <ERROR> Unknown error occurred "
					"saving file: \"\"", p_pFileName );

				ReturnValue = SU_UNKNOWN_ERROR;
			}
		}
	}

	MEM_FreeFromBlock( g_pMemoryBlock, pBuffer );

	return ReturnValue;
}

