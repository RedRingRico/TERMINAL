#ifndef __TERMINAL_QUEUE_H__
#define __TERMINAL_QUEUE_H__

#include <Memory.h>
#include <shinobi.h>

/* The queue uses an array of memory for quick iteration */
typedef struct _tagQUEUE
{
	/* The front will move around as elements are dequeued */
	size_t				Front;
	size_t				Count;
	size_t				Capacity;
	size_t				ItemSize;
	size_t				GrowableAmount;
	void				*pQueue;
	PMEMORY_BLOCK		pMemoryBlock;
}QUEUE,*PQUEUE;

int QUE_Initialise( PQUEUE p_pQueue, PMEMORY_BLOCK p_pMemoryBlock,
	size_t p_Capacity, size_t p_ItemSize, size_t p_GrowableAmount,
	const char *p_pName );
void QUE_Terminate( PQUEUE p_pQueue );

int QUE_Enqueue( PQUEUE p_pQueue, void *p_pItem );
/* Optionally, copy the item */
int QUE_Dequeue( PQUEUE p_pQueue, void *p_pItem );

void *QUE_GetFront( PQUEUE p_pQueue );
size_t 	QUE_GetCount( PQUEUE p_pQueue );
bool QUE_IsFull( PQUEUE p_pQueue );
bool QUE_IsEmpty( PQUEUE p_pQueue );

#endif /* __TERMINAL_QUEUE_H__ */

