#include <StorageUnit.h>
#include <Log.h>

static STORAGEUNIT_INFO g_StorageUnits[ SU_MAX_DRIVES ];
static PMEMORY_BLOCK g_pMemoryBlock;
static size_t g_ConnectedStorageUnits;

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
		for( Index = 0; Index < g_ConnectedStorageUnits; ++Index )
		{
			if( g_StorageUnits[ Index ].Flags & SUI_CONNECTED )
			{
				memcpy( &p_pConnectedUnits[ Offset ], &g_StorageUnits[ Index ],
					sizeof( STORAGEUNIT_INFO ) );
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

	if( pInformation->pWorkAddress == NULL )
	{
		/* Memory unit isn't mounted */
		return 0;
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
				default:
				{
					return -1;
				}
			}
		}
	}

	return 0;
}

bool SU_FindFileOnDrive( Sint32 p_Drive, char *p_pFileName )
{
	char FileName[ 13 ];
	Sint32 Status;
	PSTORAGEUNIT_INFO pInformation = &g_StorageUnits[ p_Drive ];

	if( pInformation->Flags & ( SUI_READY | SUI_FORMATTED ) )
	{
		int FileFound = 0;

		do
		{
			Status = buFindFirstFile( p_Drive, FileName );
		} while( Status == BUD_ERR_BUSY );

		if( Status < 0 )
		{
			/* Empty disk */
			if( Status == BUD_ERR_FILE_NOT_FOUND )
			{
				LOG_Debug( "[SU_FindFileOnDrive] <INFO> Emtpty disk!" );
				return false;
			}
		}

#if defined ( DEBUG )
			LOG_Debug( "[SU_FindFileOnDrive] <INFO> Status: 0x%08X", Status );
			LOG_Debug( "[SU_FindFileOnDrive] <INFO> File: %s", FileName );
#endif /* DEBUG */

		FileFound = strncmp( FileName, p_pFileName, 16 );

		if( FileFound == 0 )
		{
			goto FileFound;
		}

		do
		{
			do
			{
				Status = buFindNextFile( p_Drive, FileName );
			} while( Status == BUD_ERR_BUSY );

			if( Status < 0 )
			{
				/* End of the road */
				if( Status == BUD_ERR_FILE_NOT_FOUND )
				{
					return false;
				}
			}

#if defined ( DEBUG )
			LOG_Debug( "[SU_FindFileOnDrive] <INFO> Status: 0x%08X", Status );
			LOG_Debug( "[SU_FindFileOnDrive] <INFO> File: %s", FileName );
#endif /* DEBUG */
		} while( ( FileFound = strncmp( FileName, p_pFileName, 13 ) ) != 0 );

		if( FileFound == 0 )
		{
			goto FileFound;
		}
	}

	return false;

FileFound:
	LOG_Debug( "Found file %s", p_pFileName );
	return true;
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
			++g_ConnectedStorageUnits;
#if defined ( DEBUG )
			LOG_Debug( "Memory unit %d connected", p_Drive );
			LOG_Debug( "    Work size: %d", pInformation->WorkSize );
			LOG_Debug( "    Capacity:  %d", pInformation->Capacity );
#endif /* DEBUG */
			break;
		}
		case BUD_OP_MOUNT:
		{
			if( p_Status == BUD_ERR_OK )
			{
#if defined ( DEBUG )
				LOG_Debug( "[BUP] <INFO> Memory unit %d mounted", p_Drive );
#endif /* DEBUG */
				pInformation->Flags |= SUI_READY;
				if( buGetDiskInfo( p_Drive, &pInformation->DiskInformation ) ==
					BUD_ERR_OK )
				{
					pInformation->Flags |= SUI_FORMATTED;
				}
				pInformation->LastError = BUD_ERR_OK;

				/* Remove this! */
				SU_FindFileOnDrive( p_Drive, "SAVEDATA_002" );
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
#if defined( DEBUG )
				LOG_Debug( "[BUP] <INFO> Memory unit %d unmounted", p_Drive );
#endif /* DEBUG */
			}
			ClearDriveInformation( p_Drive );
			pInformation->Flags &= ~( SUI_CONNECTED );
			--g_ConnectedStorageUnits;
#if defined( DEBUG )
			LOG_Debug( "[BUP] <INFO> Memory unit %d disconnected", p_Drive );
#endif /* DEBUG */
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
	pInformation->Flags &= ~( SUI_READY | SUI_FORMATTED );
	memset( &pInformation->DiskInformation, 0,
		sizeof( pInformation->DiskInformation ) );
}

