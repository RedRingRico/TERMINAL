#ifndef __TEMRINAL_STORAGEUNIT_H__
#define __TERMINAL_STORAGEUNIT_H__

#include <sg_xpt.h>
#include <sg_bup.h>
#include <Memory.h>

#define SUI_READY		1
#define SUI_FORMATTED	2
#define SUI_CONNECTED	4

#define SU_MAX_DRIVES	8

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

Sint32 SU_GetConnectedStorageUnits( PSTORAGEUNIT_INFO p_pConnectedUnits,
	size_t *p_pConnectedUnitCount );

Sint32 SU_MountDrive( Sint32 p_Drive );
Sint32 SU_UnmountDrive( Sint32 p_Drive );
Sint32 SU_MountDrives( Sint32 *p_pDrivesMounted );
Sint32 SU_UnmountDrives( Sint32 *p_pDrivesUnmounted );

#endif /* __TERMINAL_STORAGEUNIT_H__ */


