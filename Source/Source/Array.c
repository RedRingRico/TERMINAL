#include <Array.h>
#include <Log.h>

static bool ARY_Grow( PARRAY p_pArray );

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
	p_pArray->GrowableAmount = p_GrowableAmount * p_ItemSize;
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

int ARY_Append( PARRAY p_pArray, void *p_pItem )
{
	size_t ArrayAppend = ( size_t )p_pArray->pArray + p_pArray->Count;

	if( ARY_Grow( p_pArray ) == false )
	{
		LOG_Debug( "ARY_Append <ERROR> Grow failed\n" );

		return 1;
	}

	memcpy( ( void * )ArrayAppend, p_pItem, p_pArray->ItemSize );
	p_pArray->Count += p_pArray->ItemSize;

	return 0;
}

int ARY_Prepend( PARRAY p_pArray, void *p_pItem )
{
	size_t ArrayEnd = ( size_t )p_pArray->pArray;
	
	if( p_pArray->Count > 0 )
	{
		ArrayEnd += ( p_pArray->Count );
	}

	if( ARY_Grow( p_pArray ) == false )
	{
		LOG_Debug( "ARY_Prepend <ERROR> Grow failed\n" );

		return 1;
	}

	/* Shift the memory upward */
	while( ArrayEnd > ( size_t )p_pArray->pArray )
	{
		memcpy( ( void * )( ArrayEnd ),// + p_pArray->ItemSize ),
			( void * )( ArrayEnd - p_pArray->ItemSize ), p_pArray->ItemSize );
		ArrayEnd -= p_pArray->ItemSize;
	}

	memcpy( p_pArray->pArray, p_pItem, p_pArray->ItemSize );
	p_pArray->Count += p_pArray->ItemSize;

	return 0;
}

void *ARY_GetItem( PARRAY p_pArray, size_t p_Index )
{
	size_t Item = ( size_t )p_pArray->pArray + p_Index * p_pArray->ItemSize;

	return ( void * )Item;
}

size_t ARY_GetCount( PARRAY p_pArray )
{
	return p_pArray->Count / p_pArray->ItemSize;
}

bool ARY_IsFull( PARRAY p_pArray )
{
	return ( p_pArray->Count == p_pArray->Capacity ) ? true : false;
}

bool ARY_IsEmpty( PARRAY p_pArray )
{
	return ( p_pArray->Count == 0 ) ? true : false;
}

static bool ARY_Grow( PARRAY p_pArray )
{
	if( p_pArray->Count == p_pArray->Capacity )
	{
		if( p_pArray->GrowableAmount > 0 )
		{
			void *pTempArray;

			pTempArray = MEM_ReallocateFromBlock( p_pArray->pMemoryBlock,
				p_pArray->Capacity + p_pArray->GrowableAmount,
				p_pArray->pArray );

			if( pTempArray == NULL )
			{
				LOG_Debug( "ARY_Grow <WARN> Unable to allocate more memory "
					"for array\n" );

				return false;
			}

			p_pArray->pArray = pTempArray;
			p_pArray->Capacity += p_pArray->GrowableAmount;
		}
		else
		{
			LOG_Debug( "ARY_Grow <ERROR> Cannot adjust array size\n" );
			return false;
		}
	}

	return true;
}

