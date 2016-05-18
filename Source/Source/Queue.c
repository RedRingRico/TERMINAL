#include <Queue.h>
#include <Log.h>

int QUE_Initialise( PQUEUE p_pQueue, PMEMORY_BLOCK p_pMemoryBlock,
	size_t p_Capacity, size_t p_ItemSize, size_t p_GrowableAmount,
	const char *p_pName )
{
	p_pQueue->pQueue = MEM_AllocateFromBlock( p_pMemoryBlock,
		p_Capacity * p_ItemSize, p_pName );

	if( p_pQueue->pQueue == NULL )
	{
		LOG_Debug( "QUE_Initialise <ERROR> Failed to allocate a queue of %d "
			"items of size %d\n", p_Capacity, p_ItemSize );

		return 1;
	}

	p_pQueue->Front = 0;
	p_pQueue->Count = 0;
	p_pQueue->Capacity = p_Capacity * p_ItemSize;
	p_pQueue->ItemSize = p_ItemSize;
	p_pQueue->GrowableAmount = p_GrowableAmount;
	p_pQueue->pMemoryBlock = p_pMemoryBlock;

	return 0;
}

void QUE_Terminate( PQUEUE p_pQueue )
{
	if( ( p_pQueue->pMemoryBlock != NULL ) && ( p_pQueue->pQueue != NULL ) )
	{
		MEM_FreeFromBlock( p_pQueue->pMemoryBlock, p_pQueue->pQueue );
		MEM_GarbageCollectMemoryBlock( p_pQueue->pMemoryBlock );

		p_pQueue->Front = 0;
		p_pQueue->Count = 0;
		p_pQueue->Capacity = 0;
		p_pQueue->ItemSize = 0;
		p_pQueue->GrowableAmount = 0;
		p_pQueue->pQueue = NULL;
		p_pQueue->pMemoryBlock = NULL;
	}
}

int QUE_Enqueue( PQUEUE p_pQueue, void *p_pItem )
{
	size_t EnqueuePosition;

	if( p_pQueue->Capacity != p_pQueue->Count )
	{
		EnqueuePosition = ( p_pQueue->Front + p_pQueue->Count ) %
			p_pQueue->Capacity;
		EnqueuePosition += ( size_t )p_pQueue->pQueue;
		memcpy( ( void * )EnqueuePosition, p_pItem,
			p_pQueue->ItemSize );

		p_pQueue->Count += p_pQueue->ItemSize;

		return 0;
	}

	LOG_Debug( "QUE_Enqueue <ERROR> No more space left in queue\n" );

	return 1;
}

int QUE_Dequeue( PQUEUE p_pQueue, void *p_pItem )
{
	if( p_pQueue->Count > 0 )
	{
		if( p_pItem != NULL )
		{
			size_t ItemPosition = ( size_t )p_pQueue->pQueue + p_pQueue->Front;
			memcpy( p_pItem, ItemPosition, p_pQueue->ItemSize );
		}

		p_pQueue->Count -= p_pQueue->ItemSize;
		p_pQueue->Front += p_pQueue->ItemSize;

		/* Wrap-around */
		if( p_pQueue->Front == p_pQueue->Capacity )
		{
			p_pQueue->Front = 0;
		}
	}

	return 0;
}

void *QUE_GetFront( PQUEUE p_pQueue )
{
	if( p_pQueue->Count > 0 )
	{
		size_t ItemPosition = ( size_t )p_pQueue->pQueue + p_pQueue->Front;

		return ( void * )ItemPosition;
	}

	return NULL;
}

size_t QUE_GetCount( PQUEUE p_pQueue )
{
	return p_pQueue->Count / p_pQueue->ItemSize;
}

bool QUE_IsFull( PQUEUE p_pQueue )
{
	return ( p_pQueue->Capacity == p_pQueue->Count );
}

bool QUE_IsEmpty( PQUEUE p_pQueue )
{
	return ( p_pQueue->Count == 0 );
}

