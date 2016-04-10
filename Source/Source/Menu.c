#include <Menu.h>
#include <Log.h>

int MNU_Initialise( PMENU p_pMenu, PMENU_ITEM p_pMenuItems,
	size_t p_MenuItemCount, PSELECTION_HIGHLIGHT p_pSelectionHighlight,
	PGLYPHSET p_pGlyphSet, KMPACKEDARGB p_TextColour,
	MENU_ITEM_ALIGNMENT p_MenuItemAlignment )
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
			PSELECTION_HIGHLIGHT_STRING pHighlightString;
			PSELECTION_HIGHLIGHT_STRING pHighlightStringParam;

			p_pMenu->pSelectionHighlight =
				syMalloc( sizeof( SELECTION_HIGHLIGHT_STRING ) );
			memcpy( p_pMenu->pSelectionHighlight, p_pSelectionHighlight,
				sizeof( SELECTION_HIGHLIGHT_STRING ) );
			pHighlightString =
				( PSELECTION_HIGHLIGHT_STRING )p_pMenu->pSelectionHighlight;

			pHighlightStringParam =
				( PSELECTION_HIGHLIGHT_STRING )p_pSelectionHighlight;
			pHighlightString->pString = syMalloc( strlen(
				pHighlightStringParam->pString ) + 1 );
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
	p_pMenu->pGlyphSet = p_pGlyphSet;
	p_pMenu->TextColour = p_TextColour;
	p_pMenu->MenuItemAlignment = p_MenuItemAlignment;

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

