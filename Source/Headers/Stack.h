#ifndef __TERMINAL_STACK_H__
#define __TERMINAL_STACK_H__

#include <Memory.h>

typedef struct _tagSTACK
{
	/* Top and capacity are specified in terms of the item size
	 * i.e. if an item is 8 bytes, a two-item capable stack has a capacity of
	 * 16 and a max top of 16 (pStack+8,pStack+16) */
	size_t			Top;
	size_t			Capacity;
	size_t			ItemSize;
	size_t			GrowableAmount;
	void			*pStack;
	PMEMORY_BLOCK	pMemoryBlock;
}STACK,*PSTACK;

int STK_Initialise( PSTACK p_pStack, PMEMORY_BLOCK p_pMemoryBlock,
	size_t p_Capacity, size_t p_ItemSize, size_t p_GrowableAmount,
	const char *p_pName );
void STK_Terminate( PSTACK p_pStack );

int STK_Push( PSTACK p_pStack, void *p_pItem );
int STK_Pop( PSTACK p_pStack, void *p_pItem );

void *STK_GetTopItem( PSTACK p_pStack );
void *STK_GetItem( PSTACK p_pStack, size_t p_Index );
size_t STK_GetCount( PSTACK p_pStack );
bool STK_IsFull( PSTACK p_pStack );
bool STK_IsEmpty( PSTACK p_pStack );

#endif /* __TERMINAL_STACK_H__ */

