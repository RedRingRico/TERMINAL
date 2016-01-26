#ifndef __TERMINAL_MEMORY_H__
#define __TERMINAL_MEMORY_H__

#include <sg_xpt.h>
#include <sg_maloc.h>

#define MEM_SH4_P2NonCachedMemory( p_pAddress ) \
	( ( ( ( long ) p_pAddress ) & 0x0FFFFFFF ) | 0xA0000000 )

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
}MEMORY_BLOCK;

int MEM_Initialise( void );
void MEM_Terminate( void );

int MEM_InitialiseMemoryBlock( MEMORY_BLOCK *p_pBlock, void *p_pMemoryPointer,
	size_t p_Size, unsigned char p_Alignment, const char *p_pName );
bool MEM_CreateMemoryBlock( MEMORY_BLOCK *p_pMemoryBlock, bool p_Free,
	MEMORY_BLOCK_HEADER *p_pBlockHeader, size_t p_TotalSize,
	size_t p_DataSize );
Uint32 MEM_CalculateDataOffset( MEMORY_BLOCK_HEADER *p_pBlockHeader,
	unsigned char p_Alignment );

size_t MEM_GetFreeBlockMemory( MEMORY_BLOCK *p_pBlock );
size_t MEM_GetUsedBlockMemory( MEMORY_BLOCK *p_pBlock );

void MEM_ListMemoryBlocks( MEMORY_BLOCK *p_pBlock );

#endif /* __TERMINAL_MEMORY_H__ */

