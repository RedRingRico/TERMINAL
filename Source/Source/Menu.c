#include <Menu.h>
#include <Log.h>

int MNU_Initialise( PMENU p_pMenu, PMENU_ITEM p_pMenuItems,
	size_t p_MenuItemCount, PSELECTION_HIGHLIGHT p_pSelectionHighlight,
	PGLYPHSET p_pGlyphSet, KMPACKEDARGB p_TextColour,
	MENU_ITEM_ALIGNMENT p_MenuItemAlignment, PMEMORY_BLOCK p_pMemoryBlock )
{
	switch( p_pSelectionHighlight->Type )
	{
		case SELECTION_HIGHLIGHT_TYPE_BOX:
		{
			p_pMenu->pSelectionHighlight =
				MEM_AllocateFromBlock( p_pMemoryBlock,
					sizeof( SELECTION_HIGHLIGHT_BOX ), "Menu: SHB" );
			memcpy( p_pMenu->pSelectionHighlight, p_pSelectionHighlight,
				sizeof( SELECTION_HIGHLIGHT_BOX ) );

			break;
		}
		case SELECTION_HIGHLIGHT_TYPE_STRING:
		{
			PSELECTION_HIGHLIGHT_STRING pHighlightString;
			PSELECTION_HIGHLIGHT_STRING pHighlightStringParam;
#if defined ( DEBUG )
			char String[ 64 ] = "Menu: SHS String: ";
#else
			char String[ 1 ] = "";
#endif /* DEBUG */

			p_pMenu->pSelectionHighlight =
				MEM_AllocateFromBlock( p_pMemoryBlock,
					sizeof( SELECTION_HIGHLIGHT_STRING ), "Menu: SHS" );
			memcpy( p_pMenu->pSelectionHighlight, p_pSelectionHighlight,
				sizeof( SELECTION_HIGHLIGHT_STRING ) );
			pHighlightString =
				( PSELECTION_HIGHLIGHT_STRING )p_pMenu->pSelectionHighlight;

			pHighlightStringParam =
				( PSELECTION_HIGHLIGHT_STRING )p_pSelectionHighlight;
#if defined ( DEBUG )
			strcat( String, pHighlightStringParam->pString );
#endif /* DEBUG */
			pHighlightString->pString = MEM_AllocateFromBlock( p_pMemoryBlock,
				strlen( pHighlightStringParam->pString ) + 1,
				String );
			strncpy( pHighlightString->pString, pHighlightStringParam->pString,
				strlen( pHighlightStringParam->pString ) );
			pHighlightString->pString[
				strlen( pHighlightStringParam->pString ) ] = '\0';

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
			size_t MenuSize = sizeof( MENU_ITEM );

			p_pMenu->pMenuItems =
				MEM_AllocateFromBlock( p_pMemoryBlock,
				p_MenuItemCount * sizeof( MENU_ITEM ), "Menu: Menu items" );

			memcpy( p_pMenu->pMenuItems, p_pMenuItems,
				p_MenuItemCount * sizeof( MENU_ITEM ) );

			/* At this point, the menu item names should probably also be
			 * copied */

			/* Fix up the pointers */
			for( Index = 0; Index < p_MenuItemCount; ++Index )
			{
				size_t Offset = ( Index + 1 ) * MenuSize;
				p_pMenu->pMenuItems[ Index ].pNext =
					( PMENU_ITEM )( ( size_t )p_pMenu->pMenuItems + Offset );
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

	p_pMenu->pMemoryBlock = p_pMemoryBlock;
	p_pMenu->SelectedMenuItem = 0;
	p_pMenu->pGlyphSet = p_pGlyphSet;
	p_pMenu->TextColour = p_TextColour;
	p_pMenu->MenuItemAlignment = p_MenuItemAlignment;

	return 0;
}

void MNU_Terminate( PMENU p_pMenu )
{
	MEM_GarbageCollectMemoryBlock( p_pMenu->pMemoryBlock );

	switch( p_pMenu->pSelectionHighlight->Type )
	{
		case SELECTION_HIGHLIGHT_TYPE_STRING:
		{
			PSELECTION_HIGHLIGHT_STRING pSelectionHighlight =
				( PSELECTION_HIGHLIGHT_STRING )p_pMenu->pSelectionHighlight;

			MEM_FreeFromBlock( p_pMenu->pMemoryBlock,
				pSelectionHighlight->pString );
			break;
		}
	}

	MEM_FreeFromBlock( p_pMenu->pMemoryBlock, p_pMenu->pSelectionHighlight );

	if( p_pMenu->pMenuItems != NULL )
	{
		MEM_FreeFromBlock( p_pMenu->pMemoryBlock, p_pMenu->pMenuItems );
	}

	MEM_GarbageCollectMemoryBlock( p_pMenu->pMemoryBlock );
}

void MNU_SelectNextMenuItem( PMENU p_pMenu )
{
	if( p_pMenu->SelectedMenuItem == p_pMenu->MenuItemCount - 1 )
	{
		p_pMenu->SelectedMenuItem = 0;
	}
	else
	{
		++p_pMenu->SelectedMenuItem;
	}
}

void MNU_SelectPreviousMenuItem( PMENU p_pMenu )
{
	if( p_pMenu->SelectedMenuItem == 0 )
	{
		p_pMenu->SelectedMenuItem = p_pMenu->MenuItemCount - 1;
	}
	else
	{
		--p_pMenu->SelectedMenuItem;
	}
}

PMENU_ITEM MNU_GetSelectedMenuItem( PMENU p_pMenu )
{
	return &p_pMenu->pMenuItems[ p_pMenu->SelectedMenuItem ];
}

void MNU_Render( PMENU p_pMenu, float p_Spacing, float p_X, float p_Y )
{
	size_t Index;

	for( Index = 0; Index < p_pMenu->MenuItemCount; ++Index )
	{
		if( Index == p_pMenu->SelectedMenuItem )
		{
			switch( p_pMenu->pSelectionHighlight->Type )
			{
				case SELECTION_HIGHLIGHT_TYPE_BOX:
				{
					break;
				}
				case SELECTION_HIGHLIGHT_TYPE_STRING:
				{
					float TextLength;
					PSELECTION_HIGHLIGHT_STRING pHighlight =
						( PSELECTION_HIGHLIGHT_STRING )
							p_pMenu->pSelectionHighlight;

					TXT_MeasureString( p_pMenu->pGlyphSet, pHighlight->pString,
						&TextLength );

					switch( p_pMenu->MenuItemAlignment )
					{

						case MENU_ITEM_ALIGNMENT_LEFT:
						{
							TXT_RenderString( p_pMenu->pGlyphSet,
								&p_pMenu->pSelectionHighlight->HighlightColour,
								p_X - TextLength, p_Y +
									( float )p_pMenu->pGlyphSet->LineHeight *
									( Index * p_Spacing ),
								pHighlight->pString );

							break;
						}
						case MENU_ITEM_ALIGNMENT_RIGHT:
						{
							float ItemLength;

							TXT_MeasureString( p_pMenu->pGlyphSet,
								p_pMenu->pMenuItems[ Index ].pName,
								&ItemLength );

							TXT_RenderString( p_pMenu->pGlyphSet,
								&p_pMenu->pSelectionHighlight->HighlightColour,
								p_X - ( TextLength + ItemLength ), p_Y +
									( float )p_pMenu->pGlyphSet->LineHeight *
									( Index * p_Spacing ),
								pHighlight->pString );

							break;
						}
						case MENU_ITEM_ALIGNMENT_CENTRE:
						{
							float ItemLength;

							TXT_MeasureString( p_pMenu->pGlyphSet,
								p_pMenu->pMenuItems[ Index ].pName,
								&ItemLength );

							TXT_RenderString( p_pMenu->pGlyphSet,
								&p_pMenu->pSelectionHighlight->HighlightColour,
								p_X - ( TextLength + ItemLength * 0.5f ), p_Y +
									( float )p_pMenu->pGlyphSet->LineHeight *
									( Index * p_Spacing ),
								pHighlight->pString );

							break;
						}
					}
					break;
				}
			}

			switch( p_pMenu->MenuItemAlignment )
			{
				case MENU_ITEM_ALIGNMENT_LEFT:
				{
					TXT_RenderString( p_pMenu->pGlyphSet,
						&p_pMenu->pSelectionHighlight->HighlightColour,
						p_X, p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
				case MENU_ITEM_ALIGNMENT_RIGHT:
				{
					float TextLength;

					TXT_MeasureString( p_pMenu->pGlyphSet,
						p_pMenu->pMenuItems[ Index ].pName, &TextLength );
					TXT_RenderString( p_pMenu->pGlyphSet,
						&p_pMenu->pSelectionHighlight->HighlightColour,
						p_X - TextLength,
						p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
				case MENU_ITEM_ALIGNMENT_CENTRE:
				{
					float TextLength;

					TXT_MeasureString( p_pMenu->pGlyphSet,
						p_pMenu->pMenuItems[ Index ].pName, &TextLength );
					TXT_RenderString( p_pMenu->pGlyphSet,
						&p_pMenu->pSelectionHighlight->HighlightColour,
						p_X - ( TextLength * 0.5f ),
						p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
			}
		}
		else
		{
			switch( p_pMenu->MenuItemAlignment )
			{
				case MENU_ITEM_ALIGNMENT_LEFT:
				{
					TXT_RenderString( p_pMenu->pGlyphSet, &p_pMenu->TextColour,
						p_X, p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
				case MENU_ITEM_ALIGNMENT_RIGHT:
				{
					float TextLength;

					TXT_MeasureString( p_pMenu->pGlyphSet,
						p_pMenu->pMenuItems[ Index ].pName, &TextLength );
					TXT_RenderString( p_pMenu->pGlyphSet, &p_pMenu->TextColour,
						p_X - TextLength,
						p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
				case MENU_ITEM_ALIGNMENT_CENTRE:
				{
					float TextLength;

					TXT_MeasureString( p_pMenu->pGlyphSet,
						p_pMenu->pMenuItems[ Index ].pName, &TextLength );
					TXT_RenderString( p_pMenu->pGlyphSet, &p_pMenu->TextColour,
						p_X - ( TextLength * 0.5f ),
						p_Y + ( float )p_pMenu->pGlyphSet->LineHeight *
							( Index * p_Spacing ),
						p_pMenu->pMenuItems[ Index ].pName );

					break;
				}
			}
		}
	}
}

