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

int MEM_Initialise( PMEMORY_FREESTAT p_pMemoryFree )
{
	syMallocInit( MEM_HEAP_AREA, MEM_HEAP_SIZE );

#if defined ( DEBUG )
	{
		syMallocStat( &p_pMemoryFree->Free, &p_pMemoryFree->BiggestFree );

		LOG_Debug( "Memory initialised with a heap of %ld bytes",
			p_pMemoryFree->Free );
	}
#endif /* DEBUG */

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
	p_pBlock->StructAlignment = sizeof( size_t );
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
#endif /* DEBUG */

	return 0;
}

bool MEM_CreateMemoryBlock( MEMORY_BLOCK_HEADER *p_pBlockHeader, bool p_Free,
	size_t p_TotalSize, size_t p_DataSize )
{
	unsigned char *pPadding = ( unsigned char * )p_pBlockHeader;
	MEMORY_BLOCK_FOOTER *pFooter;

	p_pBlockHeader->Flags = 0;

	if( p_Free == true )
	{
		p_pBlockHeader->Flags |= MEM_BLOCK_FREE;
	}

	p_pBlockHeader->Size = p_TotalSize;
	p_pBlockHeader->DataOffset = p_TotalSize - p_DataSize;

	pPadding += p_pBlockHeader->DataOffset;
	pPadding -= sizeof( MEMORY_BLOCK_FOOTER );

	pFooter = ( MEMORY_BLOCK_FOOTER * )pPadding;
	pFooter->Padding = ( unsigned char )( p_pBlockHeader->DataOffset -
		( sizeof( MEMORY_BLOCK_HEADER ) + sizeof( MEMORY_BLOCK_FOOTER ) ) );
	pFooter->Magic = MEM_MAGIC8;

#if defined ( DEBUG )
	p_pBlockHeader->CRC = 0;
#endif /* DEBUG */

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

MEMORY_BLOCK_HEADER *MEM_GetFreeBlock( MEMORY_BLOCK *p_pBlock,
	size_t p_Size )
{
	MEMORY_BLOCK_HEADER *pHeader = p_pBlock->pFirstBlock;
	MEMORY_BLOCK_HEADER *pNewBlock = NULL;
	size_t TotalSize = 0;

	while( pHeader )
	{
		TotalSize = MEM_GetBlockSize( pHeader, p_Size, p_pBlock->Alignment,
			p_pBlock->StructAlignment );

		if( ( pHeader->Flags & MEM_BLOCK_FREE ) && TotalSize >= p_Size )
		{
			size_t FreeSize, FreeTotalSize, FreeOffset;

			pNewBlock = ( MEMORY_BLOCK_HEADER * )(
				( ( unsigned char * )pHeader ) + TotalSize );

			FreeTotalSize = pHeader->Size - TotalSize;
			FreeOffset = MEM_CalculateDataOffset( pNewBlock,
				p_pBlock->Alignment );

			FreeSize = FreeTotalSize - FreeOffset;

			/* Found enough space to split the block into two halves */
			if( FreeTotalSize > FreeOffset )
			{
				MEM_CreateMemoryBlock( pNewBlock, true, FreeTotalSize,
					FreeSize );

				pNewBlock->pNext = NULL;

#if defined ( DEBUG )
				if( ( pNewBlock->Name == NULL ) ||
					( strlen( pNewBlock->Name ) == 0 ) )
				{
					memcpy( pNewBlock->Name, pHeader->Name,
						sizeof( pHeader->Name ) );
				}
#endif /* DEBUG */

				MEM_CreateMemoryBlock( pHeader, false, TotalSize, p_Size );
				pHeader->pNext = pNewBlock;
			}
			else
			{
				/* No room to split the block */
				pHeader->Flags &= ~( MEM_BLOCK_FREE );
			}

			return pHeader;
		}

		pHeader = pHeader->pNext;
	}

	return NULL;
}

size_t MEM_GetBlockSize( MEMORY_BLOCK_HEADER *p_pHeader, size_t p_Size,
	unsigned char p_Alignment, unsigned char p_StructAlign )
{
	size_t Start, End;
	Start = End = ( size_t )p_pHeader;

	End += MEM_CalculateDataOffset( p_pHeader, p_Alignment );
	End += p_Size;

	End = MEM_Align( End, p_StructAlign );

	return End - Start;
}

void *MEM_GetPointerFromBlock( MEMORY_BLOCK_HEADER *p_pHeader )
{
	unsigned char *pData = ( unsigned char * )p_pHeader;

	pData += p_pHeader->DataOffset;

	return ( void * )pData;
}


MEMORY_BLOCK_HEADER *MEM_GetBlockHeader( void *p_pPointer )
{
	MEMORY_BLOCK_FOOTER *pFooter;
	unsigned char *pData = ( unsigned char * )p_pPointer;

	pFooter =
		( MEMORY_BLOCK_FOOTER * )( pData - sizeof( MEMORY_BLOCK_FOOTER ) );
	
	pData -= sizeof( MEMORY_BLOCK_HEADER ) + sizeof( MEMORY_BLOCK_FOOTER );
	pData -= pFooter->Padding;

	return ( MEMORY_BLOCK_HEADER * )pData;
}

void MEM_GarbageCollectMemoryBlock( MEMORY_BLOCK *p_pBlock )
{
	MEMORY_BLOCK_HEADER *pHeader, *pNext;

	pHeader = p_pBlock->pFirstBlock;

	while( pHeader )
	{
		pNext = pHeader->pNext;

		if( ( pHeader->Flags & MEM_BLOCK_FREE ) &&
			( pNext->Flags & MEM_BLOCK_FREE ) )
		{
#if defined ( DEBUG )
			/* When freeing, copy the next block's name into this one */
			memcpy( pHeader->Name, pNext->Name, sizeof( pHeader->Name ) );
#endif /* DEBUG */
			pHeader->Size += pNext->Size;
			pHeader->pNext = pNext->pNext;
		}
		else
		{
			pHeader = pHeader->pNext;
		}
	}
}

void *MEM_AllocateFromBlock( MEMORY_BLOCK *p_pBlock, size_t p_Size,
	const char *p_pName )
{
	MEMORY_BLOCK_HEADER *pNewBlock = NULL;

	if( !( pNewBlock = MEM_GetFreeBlock( p_pBlock, p_Size ) ) )
	{
		MEM_GarbageCollectMemoryBlock( p_pBlock );
		pNewBlock = MEM_GetFreeBlock( p_pBlock, p_Size );
	}

	if( pNewBlock )
	{
#if defined ( DEBUG )
		/* Copy the previous block's name, if necessary */
		if( pNewBlock->pNext->Flags & MEM_BLOCK_FREE )
		{
			memcpy( pNewBlock->pNext->Name, pNewBlock->Name,
				sizeof( pNewBlock->Name ) );
		}

		/* Set the new block's name */
		memset( pNewBlock->Name, '\0', sizeof( pNewBlock->Name ) );
		if( strlen( p_pName ) >= sizeof( pNewBlock->Name ) )
		{
			memcpy( pNewBlock->Name, p_pName, 63 );
		}
		else
		{
			memcpy( pNewBlock->Name, p_pName, strlen( p_pName ) );
		}
#endif /* DEBUG */

		return MEM_GetPointerFromBlock( pNewBlock );
	}

	return NULL;
}

void MEM_FreeFromBlock( MEMORY_BLOCK *p_pBlock, void *p_pPointer )
{
	MEMORY_BLOCK_HEADER *pFree = MEM_GetBlockHeader( p_pPointer );

	pFree->Flags |= MEM_BLOCK_FREE;
}

void *MEM_ReallocateFromBlock( MEMORY_BLOCK *p_pBlock, size_t p_NewSize,
	void *p_pOriginalPointer )
{
	MEMORY_BLOCK_HEADER *pHeader;
	size_t DesiredSize;

	pHeader = MEM_GetBlockHeader( p_pOriginalPointer );

	if( p_NewSize < pHeader->Size )
	{
		/* Resize downward, join with the next chunk if possible */
		if( pHeader->pNext->Flags & MEM_BLOCK_FREE )
		{
			MEMORY_BLOCK_HEADER *pNewBlock;
			size_t NewSize = 0;
			size_t FreeTotalSize = 0;
			size_t FreeOffset = 0;

			NewSize = MEM_GetBlockSize( pHeader, p_NewSize,
				p_pBlock->Alignment, p_pBlock->StructAlignment );

			pNewBlock = ( MEMORY_BLOCK_HEADER * )(
				( ( Uint8 * )pHeader ) + NewSize );

			FreeTotalSize = ( pHeader->Size + pHeader->pNext->Size ) - NewSize;
			FreeOffset = MEM_CalculateDataOffset( pNewBlock,
				p_pBlock->Alignment );

			MEM_CreateMemoryBlock( pNewBlock, true, FreeTotalSize,
				FreeTotalSize - FreeOffset );
			pNewBlock->pNext = pHeader->pNext->pNext;

#if defined ( DEBUG )
			memcpy( pNewBlock->Name, pHeader->pNext->Name,
				sizeof( pHeader->pNext->Name ) );
#endif /* DEBUG */

			MEM_CreateMemoryBlock( pHeader, false, NewSize, p_NewSize );
			pHeader->pNext = pNewBlock;
		}
		else
		{
		}

		return MEM_GetPointerFromBlock( pHeader );
	}
	else
	{
		/* Is there enough memory in the next block (plus the next if possible)
		 * to just resize the current chunk of memory? */
		bool MemoryContiguous = false;
		MEMORY_BLOCK_HEADER *pNextBlock = pHeader->pNext;

		DesiredSize = p_NewSize - pHeader->Size;
		
		while( pNextBlock != NULL )
		{
			/* Join two blocks if they are both free */
			if( ( pNextBlock->Flags & MEM_BLOCK_FREE ) &&
				( pNextBlock->pNext != NULL ) )
			{
				if( pNextBlock->pNext->Flags & MEM_BLOCK_FREE )
				{
#if defined ( DEBUG )
					memcpy( pNextBlock->Name, pNextBlock->pNext->Name,
						sizeof( pHeader->Name ) );
#endif /* DEBUG */
					pNextBlock->Size += pNextBlock->pNext->Size;
					pNextBlock->pNext = pNextBlock->pNext->pNext;
				}
			}

			if( pNextBlock->Flags & MEM_BLOCK_FREE )
			{
				if( pNextBlock->Size >= DesiredSize )
				{
					MemoryContiguous = true;

					break;
				}

				pNextBlock = pNextBlock->pNext;
			}
			else
			{
				break;
			}
		}

		if( ( pNextBlock == NULL ) || ( MemoryContiguous == false ) )
		{
			MEMORY_BLOCK_HEADER *pNewBlock = NULL;
			void *pNewMemory = NULL;

			MEM_GarbageCollectMemoryBlock( p_pBlock );

			pNewBlock = MEM_GetFreeBlock( p_pBlock, p_NewSize );

			if( pNewBlock == NULL )
			{
				return NULL;
			}

			pNewMemory = MEM_GetPointerFromBlock( pNewBlock );

			/* Copy the memory contents to the new block */
			memcpy( pNewMemory, p_pOriginalPointer, pHeader->Size );
#if defined ( DEBUG )
			memcpy( pNewBlock->Name, pHeader->Name, sizeof( pHeader->Name ) );
#endif /* DEBUG */

			/* Free the old memory */
			pHeader->Flags |= MEM_BLOCK_FREE;

			return pNewMemory;
		}
		else
		{
			MEMORY_BLOCK_HEADER *pNewBlock = NULL;
			size_t TotalSize = 0;
			size_t FreeTotalSize = 0, FreeOffset = 0;

			/* Join with the next chunk(s) */
			pHeader->Size += pNextBlock->Size;

			TotalSize = MEM_GetBlockSize( pHeader, p_NewSize,
				p_pBlock->Alignment, p_pBlock->StructAlignment );

			/* Split it if possible */
			pNewBlock = ( MEMORY_BLOCK_HEADER * )(
				( ( Uint8 * )pHeader ) + TotalSize );

			FreeTotalSize = pHeader->Size - TotalSize;
			FreeOffset = MEM_CalculateDataOffset( pNewBlock,
				p_pBlock->Alignment );

			if( FreeTotalSize > FreeOffset )
			{
				MEM_CreateMemoryBlock( pNewBlock, true, FreeTotalSize,
					FreeTotalSize - FreeOffset );
				pNewBlock->pNext = pNextBlock->pNext;

#if defined ( DEBUG )
				if( ( pNewBlock->Name == NULL ) ||
					( strlen( pNewBlock->Name ) == 0 ) )
				{
					if( pNextBlock != NULL )
					{
						memcpy( pNewBlock->Name, pNextBlock->Name,
							sizeof( pNextBlock->pNext->Name ) );
					}
					else
					{
						memcpy( pNewBlock->Name, pHeader->Name,
							sizeof( pHeader->Name ) );
					}
				}
#endif

				MEM_CreateMemoryBlock( pHeader, false, TotalSize, p_NewSize );
				pHeader->pNext = pNewBlock;
			}
			else
			{
				/* Cannot split it */
				pHeader->Flags &= ~( MEM_BLOCK_FREE );
			}

			return MEM_GetPointerFromBlock( pHeader );
		}
	}
	
	return NULL;
}

size_t MEM_GetFreeBlockSize( MEMORY_BLOCK *p_pBlock )
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

size_t MEM_GetUsedBlockSize( MEMORY_BLOCK *p_pBlock )
{
	size_t UsedMemory = 0;
	MEMORY_BLOCK_HEADER *pBlock = p_pBlock->pFirstBlock;

	while( pBlock )
	{
		if( ( pBlock->Flags & MEM_BLOCK_FREE ) == 0 )
		{
			UsedMemory += pBlock->Size;
		}
		pBlock = pBlock->pNext;
	}

	return UsedMemory;
}

void MEM_ListMemoryBlocks( MEMORY_BLOCK *p_pBlock )
{
#if defined ( DEBUG )
	MEMORY_BLOCK_HEADER *pBlock = p_pBlock->pFirstBlock;

	LOG_Debug( "Memory block dump" );
	LOG_Debug( "\tFree: %ld", MEM_GetFreeBlockSize( p_pBlock ) );
	LOG_Debug( "\tUsed: %ld", MEM_GetUsedBlockSize( p_pBlock ) );

	while( pBlock )
	{
		LOG_Debug( "\t%s | %s: %ld", pBlock->Name,
			( pBlock->Flags & MEM_BLOCK_FREE ) ? "FREE" : "USED",
			pBlock->Size );

		pBlock = pBlock->pNext;
	}
#endif /* DEBUG */
}

