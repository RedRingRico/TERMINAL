#ifndef __TERMINAL_MENU_H__
#define __TERMINAL_MENU_H__

#include <shinobi.h>
#include <kamui2.h>

typedef int ( *MenuFunction )( void * );

typedef enum
{
	SELECTION_HIGHLIGHT_TYPE_BOX,
	SELECTION_HIGHLIGHT_TYPE_STRING
}SELECTION_HIGHLIGHT_TYPE;

typedef struct _tagMENU_ITEM
{
	char					*pName;
	MenuFunction			Function;
	struct _tagMENU_ITEM	*pNext;
}MENU_ITEM,*PMENU_ITEM;

typedef struct _tagSELECTION_HIGHLIGHT
{
	SELECTION_HIGHLIGHT_TYPE	Type;
	Uint32						Flags;
	KMPACKEDARGB				TextColour;
	KMPACKEDARGB				TextHighlightColour;
	float						TextPulseRate;
	float						TextHighlightPulseRate;
}SELECTION_HIGHLIGHT,*PSELECTION_HIGHLIGHT;

typedef struct _tagSELECTION_HIGHLIGHT_BOX
{
	SELECTION_HIGHLIGHT	Base;
	KMPACKEDARGB		Colour;
}SELECTION_HIGHLIGHT_BOX,*PSELECTION_HIGHLIGHT_BOX;

typedef struct _tagSELECTION_HIGHLIGHT_STRING
{
	SELECTION_HIGHLIGHT	Base;
	char				*pString;
}SELECTION_HIGHLIGHT_STRING,*PSELECTION_HIGHLIGHT_STRING;

typedef struct _tagMENU
{
	PMENU_ITEM				pMenuItems;
	size_t					MenuItemCount;
	size_t					SelectedMenuItem;
	PSELECTION_HIGHLIGHT	*pSelectionHighlight;
}MENU,*PMENU;

int MNU_Initialise( PMENU p_pMenu, PMENU_ITEM p_pMenuItems,
	size_t p_MenuItemCount, PSELECTION_HIGHLIGHT p_pSelectionHighlight );
void MNU_Terminate( PMENU p_pMenu );

#endif /* __TERMINAL_MENU_H__ */

