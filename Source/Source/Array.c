#include <Array.h>
#include <Log.h>

int ARY_Initialise( PARRAY p_pArray, PMEMORY_BLOCK p_pMemoryBlock,
	size_t p_Capacity, size_t p_ItemSize, size_t p_GrowableAmount,
	const char *p_pName )
{
	p_pArray->pArray = MEM_AllocateFromBlock( p_pMemoryBlock,
		p_Capacity * p_ItemSize, p_pName );

	if( p_pArray->pArray == NULL )
	{
		LOG_Debug( "ARY_Initialise <ERROR> Failed to allocate an array of %d "
			"items of size %d\n", p_Capacity, p_ItemSize );

		return 1;
	}

	p_pArray->Capacity = p_Capacity * p_ItemSize;
	p_pArray->Count = 0;
	p_pArray->ItemSize = p_ItemSize;
	p_pArray->GrowableAmount = p_GrowableAmount;
	p_pArray->pMemoryBlock = p_pMemoryBlock;

	return 0;
}

void ARY_Terminate( PARRAY p_pArray )
{
	MEM_FreeFromBlock( p_pArray->pMemoryBlock, p_pArray->pArray );
	MEM_GarbageCollectMemoryBlock( p_pArray->pMemoryBlock );

	p_pArray->Capacity = 0;
	p_pArray->Count = 0;
	p_pArray->ItemSize = 0;
	p_pArray->GrowableAmount = 0;
	p_pArray->pArray = NULL;
	p_pArray->pMemoryBlock = NULL;
}

