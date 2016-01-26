#include <Memory.h>
#include <shinobi.h>
#include <sg_syhw.h>
#include <Log.h>

extern Uint8 *_BSG_END;

#define MEM_P1_AREA 0x80000000
#define MEM_WORK_END ( ( ( Uint32 ) _BSG_END ) & 0xE0000000 | 0x0D000000 )
#define MEM_HEAP_AREA ( ( void * )( ( ( ( Uint32 ) _BSG_END | MEM_P1_AREA ) & \
	0xFFFFFFE0 ) + 0x20 ) )
#define MEM_HEAP_SIZE ( MEM_WORK_END - ( Uint32 ) MEM_HEAP_AREA )
#define MEM_STRUCT_ALIGN ( sizeof( size_t ) )
#define MEM_BLOCK_LOCKED 0x0001
#define MEM_BLOCK_FREE 0x0002
#define MEM_MAGIC8 0xAF

#define MEM_Align( pPtr, Align ) \
	( ( ( size_t )( pPtr ) + ( Align ) - 1 ) & ( ~( ( Align ) - 1 ) ) )
#define MEM_AlignUp( pPtr, Align ) \
	( ( MEM_ALIGN( ( pPtr ), Align ) ) + ( Align ) )
#define MEM_IsAligned( pPtr, Align ) \
	( ( ( size_t )( pPtr ) & ( ( Align ) - 1 ) ) == 0 )

int MEM_Initialise( void )
{
	syMallocInit( MEM_HEAP_AREA, MEM_HEAP_SIZE );

	return 0;
}

void MEM_Terminate( void )
{
	syMallocFinish( );
}

int MEM_InitialiseMemoryBlock( MEMORY_BLOCK *p_pBlock, void *p_pMemoryPointer,
	size_t p_Size, unsigned char p_Alignment, const char *p_pName )
{
	if( p_pMemoryPointer == NULL )
	{
		return 1;
	}
	
	if( p_pBlock == NULL )
	{
		return 1;
	}

	p_pBlock->Alignment = p_Alignment;
	p_pBlock->PaddedHeaderSize = MEM_Align( sizeof( MEMORY_BLOCK_HEADER ),
		sizeof( size_t ) );
	p_pBlock->pAllocatedBlock = p_pMemoryPointer;
	p_pBlock->AllocatedSize = p_Size;

	p_pBlock->pFirstBlock = ( MEMORY_BLOCK_HEADER * )p_pBlock->pAllocatedBlock;
	p_pBlock->pFirstBlock->Size = p_pBlock->AllocatedSize;
	p_pBlock->pFirstBlock->Flags = MEM_BLOCK_FREE;
	p_pBlock->pFirstBlock->pNext = NULL;
#if defined ( DEBUG )
	memset( p_pBlock->pFirstBlock->Name, '\0',
		sizeof( p_pBlock->pFirstBlock->Name ) );
	if( strlen( p_pName ) >= sizeof( p_pBlock->pFirstBlock->Name ) )
	{
		memcpy( p_pBlock->pFirstBlock->Name, p_pName, 63 );
	}
	else
	{
		memcpy( p_pBlock->pFirstBlock->Name, p_pName, strlen( p_pName ) );
	}
#endif

	return 0;
}

bool MEM_CreateMemoryBlock( MEMORY_BLOCK *p_pMemoryBlock, bool p_Free,
	MEMORY_BLOCK_HEADER *p_pBlockHeader, size_t p_TotalSize,
	size_t p_DataSize )
{
	unsigned char *pPadding = ( unsigned char * )p_pBlockHeader;
	MEMORY_BLOCK_FOOTER *pFooter;

	p_pBlockHeader->Flags = 0;

	if( p_Free )
	{
		p_pBlockHeader->Flags |= MEM_BLOCK_FREE;
	}

	p_pBlockHeader->Size = p_TotalSize;
	p_pBlockHeader->DataOffset = p_TotalSize - p_DataSize;

	pPadding += p_pBlockHeader->DataOffset;
	pPadding -= sizeof( MEMORY_BLOCK_FOOTER );

	pFooter = ( MEMORY_BLOCK_FOOTER * )pPadding;
	pFooter->Padding -= ( unsigned char )( p_pBlockHeader->DataOffset -
		( sizeof( MEMORY_BLOCK_HEADER ) + sizeof( MEMORY_BLOCK_FOOTER ) ) );
	pFooter->Magic = MEM_MAGIC8;

#if defined ( DEBUG )
	p_pBlockHeader->CRC = 0;
#endif

	return true;
}

Uint32 MEM_CalculateDataOffset( MEMORY_BLOCK_HEADER *p_pBlockHeader,
	unsigned char p_Alignment )
{
	size_t Position, Start;

	Start = Position = ( Uint32 )p_pBlockHeader;

	Position += sizeof( MEMORY_BLOCK_HEADER );
	Position += sizeof( MEMORY_BLOCK_FOOTER );
	Position = MEM_Align( Position, p_Alignment );

	return Position - Start;
}

size_t MEM_GetFreeBlockMemory( MEMORY_BLOCK *p_pBlock )
{
	size_t FreeMemory = 0;
	MEMORY_BLOCK_HEADER *pBlock = p_pBlock->pFirstBlock;

	while( pBlock )
	{
		if( pBlock->Flags & MEM_BLOCK_FREE )
		{
			FreeMemory += pBlock->Size;
		}
		pBlock = pBlock->pNext;
	}

	return FreeMemory;
}

size_t MEM_GetUsedBlockMemory( MEMORY_BLOCK *p_pBlock )
{
	size_t UsedMemory = 0;
	MEMORY_BLOCK_HEADER *pBlock = p_pBlock->pFirstBlock;

	while( pBlock )
	{
		if( pBlock->Flags & ~MEM_BLOCK_FREE )
		{
			UsedMemory += pBlock->Size;
		}
		pBlock = pBlock->pNext;
	}

	return UsedMemory;
}

void MEM_ListMemoryBlocks( MEMORY_BLOCK *p_pBlock )
{
	MEMORY_BLOCK_HEADER *pBlock = p_pBlock->pFirstBlock;

	LOG_Debug( "Memory block dump" );
	LOG_Debug( "\tFree: %ld", MEM_GetFreeBlockMemory( p_pBlock ) );
	LOG_Debug( "\tUsed: %ld", MEM_GetUsedBlockMemory( p_pBlock ) );

	while( pBlock )
	{
		LOG_Debug( "\t%s | %s: %ld", pBlock->Name,
			( pBlock->Flags & MEM_BLOCK_FREE ) ? "FREE" : "USED",
			pBlock->Size );

		pBlock = pBlock->pNext;
	}
}

