#ifndef __TERMINAL_MEMORY_H__
#define __TERMINAL_MEMORY_H__

#include <sg_xpt.h>
#include <sg_maloc.h>

#define MEM_SH4_P2NonCachedMemory( p_pAddress ) \
	( ( ( ( long ) p_pAddress ) & 0x0FFFFFFF ) | 0xA0000000 )

#define MEM_KIB( p_Amount ) ( p_Amount << 10 )
#define MEM_MIB( p_Amount ) ( p_Amount << 20 )

typedef struct _tagMEMORY_BLOCK_HEADER
{
	struct _tagMEMORY_BLOCK_HEADER	*pNext;
	size_t							DataOffset;
	size_t							Size;
#if defined ( DEBUG )
	char	Name[ 64 ];
	Uint32	CRC;
#endif
	Uint16							Flags;
}MEMORY_BLOCK_HEADER;

typedef struct _tagMEMORY_BLOCK_FOOTER
{
	unsigned char	Magic;
	unsigned char	Padding;
}MEMORY_BLOCK_FOOTER;

typedef struct _tagMEMORY_BLOCK
{
	MEMORY_BLOCK_HEADER	*pFirstBlock;
	size_t				AllocatedSize;
	size_t				PaddedHeaderSize;
	void				*pAllocatedBlock;
	unsigned char		Alignment;
	unsigned char		StructAlignment;
}MEMORY_BLOCK,*PMEMORY_BLOCK;

typedef struct _tagMEMORY_FREESTAT
{
	Uint32	Free;
	Uint32	BiggestFree;
}MEMORY_FREESTAT,*PMEMORY_FREESTAT;

int MEM_Initialise( PMEMORY_FREESTAT p_pMemoryFree );
void MEM_Terminate( void );

int MEM_InitialiseMemoryBlock( MEMORY_BLOCK *p_pBlock, void *p_pMemoryPointer,
	size_t p_Size, unsigned char p_Alignment, const char *p_pName );
bool MEM_CreateMemoryBlock( MEMORY_BLOCK_HEADER *p_pBlockHeader, bool p_Free,
	size_t p_TotalSize, size_t p_DataSize );
Uint32 MEM_CalculateDataOffset( MEMORY_BLOCK_HEADER *p_pBlockHeader,
	unsigned char p_Alignment );
MEMORY_BLOCK_HEADER *MEM_GetFreeBlock( MEMORY_BLOCK *p_pBlock,
	size_t p_Size );
size_t MEM_GetBlockSize( MEMORY_BLOCK_HEADER *p_pHeader, size_t p_Size,
	unsigned char p_Alignment, unsigned char p_StructAlign );
void *MEM_GetPointerFromBlock( MEMORY_BLOCK_HEADER *p_pHeader );
MEMORY_BLOCK_HEADER *MEM_GetBlockHeader( void *p_pPointer );
void MEM_GarbageCollectMemoryBlock( MEMORY_BLOCK *p_pBlock );

void *MEM_AllocateFromBlock( MEMORY_BLOCK *p_pBlock, size_t p_Size,
	const char *p_pName );
void MEM_FreeFromBlock( MEMORY_BLOCK *p_pBlock, void *p_pPointer );

size_t MEM_GetFreeBlockSize( MEMORY_BLOCK *p_pBlock );
size_t MEM_GetUsedBlockSize( MEMORY_BLOCK *p_pBlock );

void MEM_ListMemoryBlocks( MEMORY_BLOCK *p_pBlock );

#endif /* __TERMINAL_MEMORY_H__ */

