#ifndef __TEMRINAL_STORAGEUNIT_H__
#define __TERMINAL_STORAGEUNIT_H__

#include <sg_xpt.h>
#include <sg_bup.h>
#include <Memory.h>

#define SU_GAME_NAME "[TERMINAL]"

#define SU_FILETYPE_NORMAL		BUD_FILETYPE_NORMAL
#define SU_FILETYPE_EXECUTABLE	BUD_FILETYPE_EXECUTABLE

#define SUI_READY		1
#define SUI_FORMATTED	2
#define SUI_CONNECTED	4

#define SU_MAX_DRIVES	8

#define SU_OK 0
#define SU_ERROR 1
#define SU_FATAL_ERROR -1
#define SU_UNKNOWN_ERROR -2
#define SU_CREATE_ERROR -10
#define SU_VERIFY_ERROR -11
#define SU_FILEINFO_ERROR -20

#define SU_BUSY BUD_ERR_BUSY
#define SU_NO_DISK BUD_ERR_NO_DISK

typedef struct _tagSTORAGEUNIT_INFO
{
	void			*pWorkAddress;
	Uint32			WorkSize;
	Uint32			Capacity;
	Uint32			LastError;
	Uint32			ProgressCount;
	Uint32			ProgressMaximum;
	Uint32			Operation;
	Uint32			Flags;
	BUS_DISKINFO	DiskInformation;
}STORAGEUNIT_INFO, *PSTORAGEUNIT_INFO;

Sint32 SU_Initialise( PMEMORY_BLOCK p_pMemoryBlock );
void SU_Terminate( void );

/* VMU enumeration */
Sint32 SU_GetConnectedStorageUnits( PSTORAGEUNIT_INFO p_pConnectedUnits,
	size_t *p_pConnectedUnitCount );
Sint32 SU_GetMountedStorageUnits( PSTORAGEUNIT_INFO p_pMountedUnits,
	size_t *p_pMountedUnitCount );

/* Mount/Unmount */
Sint32 SU_MountDrive( Sint32 p_Drive );
Sint32 SU_UnmountDrive( Sint32 p_Drive );
Sint32 SU_MountDrives( Sint32 *p_pDrivesMounted );
Sint32 SU_UnmountDrives( Sint32 *p_pDrivesUnmounted );

/* Drive/Flag conversion */
Uint32 SU_DriveToFlag( Uint8 p_Drive );
Uint8 SU_FlagToDrive( Uint32 p_Flag );

/* File discovery */
bool SU_FindFileOnDrive( Sint32 p_Drive, char *p_pFileName );
bool SU_FindFileAcrossDrives( char *p_pFileName, bool p_StopAtFirstDrive,
	Uint8 *p_pFoundOnDrives );
Sint32 SU_GetFileSize( Sint32 p_Drive, const char *p_pFileName,
	Uint32 *p_pDataOffset, const Uint8 p_FileType );

/* Space management */
Sint32 SU_GetDrivesWithFreeBlocks( Uint16 p_Blocks, bool p_StopAtFirstDrive,
	Uint8 *p_pDrives );
Sint32 SU_GetTotalBlockSize( Sint32 p_Drive, Uint16 *p_pBlockCount );
Sint32 SU_GetUsedBlockSize( Sint32 p_Drive, Uint16 *p_pBlockCount );
Sint32 SU_GetFreeBlockSize( Sint32 p_Drive, Uint16 *p_pBlockCount );
Sint32 SU_DefragmentDisk( Sint32 p_Drive );

/* Save/Load */
Sint32 SU_SaveFile( Sint32 p_Drive, const char *p_pFileName,
	const void *p_pData, const Uint32 p_DataSize, const char *p_pVMUComment,
	const char *p_pBootROMComment );
Sint32 SU_LoadFile( Sint32 p_Drive, const char *p_pFileName, void *p_pData,
	const Uint32 p_DataSize, const Uint32 p_DataOffset );

#endif /* __TERMINAL_STORAGEUNIT_H__ */


