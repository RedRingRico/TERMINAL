#ifndef __TERMINAL_ARRAY_H__
#define __TERMINAL_ARRAY_H__

#include <Memory.h>
#include <shinobi.h>

typedef struct _tagARRAY
{
	size_t			Capacity;
	size_t			Count;
	size_t			ItemSize;
	size_t			GrowableAmount;
	void			*pArray;
	PMEMORY_BLOCK	pMemoryBlock;
}ARRAY,*PARRAY;

int ARY_Initialise( PARRAY p_pArray, PMEMORY_BLOCK p_pMemoryBlock,
	size_t p_Capacity, size_t p_ItemSize, size_t p_GrowableAmount,
	const char *p_pName );
void ARY_Terminate( PARRAY p_pArray );

int ARY_InsertBefore( PARRAY p_pArray, size_t p_Index, void *p_pItem );
int ARY_InsertAfter( PARRAY p_pArray, size_t p_Index, void *p_pItem );
int ARY_Append( PARRAY p_pArray, void *p_pItem );
int ARY_Prepend( PARRAY p_pArrah, void *p_pItem );
int ARY_RemoveAt( PARRAY p_pArray, size_t p_Index );
int ARY_RemoveAtUnordered( PARRAY p_pArray, size_t p_Index );

void *ARY_GetItem( PARRAY p_pArray, size_t p_Index );
size_t ARY_GetCount( PARRAY p_pArray );
bool ARY_IsFull( PARRAY p_pArray );
bool ARY_IsEmpty( PARRAY p_pArray );

#endif /* __TERMINAL_ARRAY_H__ */

