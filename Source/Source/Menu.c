#include <Menu.h>
#include <Log.h>

int MNU_Initialise( PMENU p_pMenu, PMENU_ITEM p_pMenuItems,
	size_t p_MenuItemCount, PSELECTION_HIGHLIGHT p_pSelectionHighlight )
{
	switch( p_pSelectionHighlight->Type )
	{
		case SELECTION_HIGHLIGHT_TYPE_BOX:
		{
			p_pMenu->pSelectionHighlight =
				syMalloc( sizeof( SELECTION_HIGHLIGHT_BOX ) );
			memcpy( p_pMenu->pSelectionHighlight, p_pSelectionHighlight,
				sizeof( SELECTION_HIGHLIGHT_BOX ) );

			break;
		}
		case SELECTION_HIGHLIGHT_TYPE_STRING:
		{
			p_pMenu->pSelectionHighlight =
				syMalloc( sizeof( SELECTION_HIGHLIGHT_STRING ) );
			memcpy( p_pMenu->pSelectionHighlight, p_pSelectionHighlight,
				sizeof( SELECTION_HIGHLIGHT_STRING ) );

			break;
		}
		default:
		{
			LOG_Debug( "MNU_Initialise <ERROR> Unknown selection highlight "
				"type: %d\n", p_pSelectionHighlight->Type );

			return 1;
		}
	}

	if( p_pMenuItems )
	{
		if( p_MenuItemCount > 0 )
		{
			size_t Index;

			p_pMenu->pMenuItems =
				syMalloc( p_MenuItemCount * sizeof( MENU_ITEM ) );

			memcpy( p_pMenu->pMenuItems, p_pMenuItems,
				p_MenuItemCount * sizeof( MENU_ITEM ) );

			/* Fix up the pointers */
			for( Index = 0; Index < p_MenuItemCount; ++Index )
			{
				p_pMenu->pMenuItems[ Index ].pNext =
					p_pMenu->pMenuItems + ( Index * sizeof( MENU_ITEM ) );
			}

			p_pMenu->pMenuItems[ p_MenuItemCount - 1 ].pNext = NULL;
			p_pMenu->MenuItemCount = p_MenuItemCount;
		}
	}
	else
	{
		p_pMenu->pMenuItems = NULL;
		p_pMenu->MenuItemCount = 0;
	}

	p_pMenu->SelectedMenuItem = 0;

	return 0;
}

void MNU_Terminate( PMENU p_pMenu )
{
	syFree( p_pMenu->pSelectionHighlight );

	while( p_pMenu->pMenuItems != NULL )
	{
		PMENU_ITEM pNext = p_pMenu->pMenuItems->pNext;

		syFree( p_pMenu->pMenuItems );

		p_pMenu->pMenuItems = pNext;
	}
}

