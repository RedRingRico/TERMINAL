#include <Keyboard.h>
#include <Hardware.h>
#include <Log.h>
#include <Serial.h>

bool g_KeyboardLibraryInitialised = false;
static KEYBOARD g_KeyboardState[ 4 ];
static Uint16 g_KeyboardBuffer[ 4 ][ KBD_BUFFER_SIZE ];
static bool g_KeyboardConnected[ 4 ];

/* Keyboard layouts:
 * Japan, America, England, Germany, France, Italy, Spain, Sweden, Switzerland,
 * The Netherlands, Portugal, Latin America, Canadian French, Russia, China,
 * and Korea 
 * These are all defined in:
 * Source/KeyboardLayouts/<Language>_[Normal|Shifted].c */
extern const Uint8 KeyboardTable_Normal_Japan[ ];
extern const Uint8 KeyboardTable_Shifted_Japan[ ];

static const Uint8 *g_KeyTableJapan[ ] =
{
	KeyboardTable_Normal_Japan,
	KeyboardTable_Shifted_Japan,
};

static void KeyboardPortServer( PKEYBOARD p_pKeyboard );

Sint32 KBD_Initialise( void )
{
	LOG_Debug( "[KBD_Initialise] <INFO> Initialising" );

	pdKbdInit( );

	g_KeyboardLibraryInitialised = true;

	return KBD_OK;
}

void KBD_Terminate( )
{
	g_KeyboardLibraryInitialised = false;

	pdKbdExit( );
}

void KBD_SetKeyTable( PKEYBOARD p_pKeyboard, const Uint8 **p_ppKeyTable )
{
	if( p_ppKeyTable )
	{
		p_pKeyboard->ppKeyTable = p_ppKeyTable;
	}
	else
	{
		p_pKeyboard->ppKeyTable = g_KeyTableJapan;
	}
}

void KBD_AutoSetKeyTable( PKEYBOARD p_pKeyboard, Uint8 p_Language )
{
	if( p_pKeyboard )
	{
		switch( p_Language )
		{
			case PDD_KBDLANG_JP:
			{
				p_pKeyboard->ppKeyTable = g_KeyTableJapan;

				break;
			}
			case PDD_KBDLANG_US:
			{
				break;
			}
			case PDD_KBDLANG_UK:
			{
				p_pKeyboard->ppKeyTable = g_KeyTableJapan;

				break;
			}
			case PDD_KBDLANG_GERMANY:
			{
				break;
			}
			case PDD_KBDLANG_FRANCE:
			{
				break;
			}
			case PDD_KBDLANG_ITALY:
			{
				break;
			}
			case PDD_KBDLANG_SPAIN:
			{
				break;
			}
			case PDD_KBDLANG_SWEDEN:
			{
				break;
			}
			case PDD_KBDLANG_SWITZER:
			{
				break;
			}
			case PDD_KBDLANG_NETHER:
			{
				break;
			}
			case PDD_KBDLANG_PORTUGAL:
			{
				break;
			}
			case PDD_KBDLANG_LATIN:
			{
				break;
			}
			case PDD_KBDLANG_CANFRENCH:
			{
				break;
			}
			case PDD_KBDLANG_RUSSIA:
			{
				break;
			}
			case PDD_KBDLANG_CHINA:
			{
				break;
			}
			case PDD_KBDLANG_KOREA:
			{
				break;
			}
			default:
			{
				p_pKeyboard->ppKeyTable = g_KeyTableJapan;
				return;
			}
		}
	}
}

void KBD_SetState( PKEYBOARD p_pKeyboard, Uint8 p_Rate, Uint8 p_Delay )
{
	p_pKeyboard->Rate = p_Rate;
	p_pKeyboard->Delay = p_Delay;
}

void KBD_GetState( PKEYBOARD p_pKeyboard, Uint8 *p_pRate, Uint8 *p_pDelay )
{
	( *p_pRate ) = p_pKeyboard->Rate;
	( *p_pDelay ) = p_pKeyboard->Delay;
}

bool KBD_KeyPressed( PKEYBOARD p_pKeyboard )
{
	return ( p_pKeyboard->ReadPointer == p_pKeyboard->WritePointer ) ?
		false : true;
}

bool KBD_Ready( PKEYBOARD p_pKeyboard )
{
	return p_pKeyboard->Flags & KBD_READY;
}

PKEYBOARD KBD_Create( Uint32 p_Port )
{
	Sint32 PortNumber;
	PKEYBOARD pKeyboard;
	const PDS_KEYBOARDINFO *pKeyboardInfo;

	/* PDD_PORT_<C><N> can be A-D for C and 0-5 for N */
	if( ( p_Port % 6 ) != 0 )
	{
		return NULL;
	}

	pKeyboardInfo = pdKbdGetInfo( p_Port );

	PortNumber = p_Port / 6;

	pKeyboard = &g_KeyboardState[ PortNumber ];

	/* Keyboard already set up */
	if( pKeyboard->Flags & KBD_INUSE )
	{
		return NULL;
	}

	pKeyboard->Flags = KBD_INUSE | KBD_REPEAT;
	pKeyboard->Rate = KBD_REPEAT_COUNT;
	pKeyboard->RateCount = 0;
	pKeyboard->Delay = KBD_DELAY_COUNT;
	pKeyboard->DelayCount = 0;
	pKeyboard->Port = p_Port;
	pKeyboard->ReadPointer = 0;
	pKeyboard->WritePointer = 0;
	pKeyboard->pKeyBuffer = g_KeyboardBuffer[ PortNumber ];
	pKeyboard->KeyBufferSize = KBD_BUFFER_SIZE;
	pKeyboard->LastKey = 0xFF;
	memset( pKeyboard->Keys, 0, sizeof( pKeyboard->Keys ) );

	/* Set the keyboard layout */
	KBD_AutoSetKeyTable( pKeyboard, pKeyboardInfo->lang );

	return pKeyboard;
}

void KBD_Destroy( PKEYBOARD p_pKeyboard )
{
	if( p_pKeyboard )
	{
		p_pKeyboard->Flags &= ~( KBD_INUSE );
		LOG_Debug( "[KBD_Destroy] <INFO> Keyboard destroyed at port %s",
			HW_PortToName( p_pKeyboard->Port ) );
	}
}

Uint16 KBD_GetChar( PKEYBOARD p_pKeyboard )
{
	if( p_pKeyboard )
	{
		Uint16 Char;

		/* Nothing new */
		if( p_pKeyboard->ReadPointer == p_pKeyboard->WritePointer )
		{
			return 0;
		}

		Char = p_pKeyboard->pKeyBuffer[ p_pKeyboard->ReadPointer++ ];
		p_pKeyboard->ReadPointer &= p_pKeyboard->KeyBufferSize - 1;

		return Char;
	}

	return 0;
}

const PDS_KEYBOARD *KBD_GetRawData( PKEYBOARD p_pKeyboard )
{
	if( p_pKeyboard )
	{
		return p_pKeyboard->pRawData;
	}

	return NULL;
}

void KBD_Flush( PKEYBOARD p_pKeyboard )
{
	if( p_pKeyboard )
	{
		p_pKeyboard->ReadPointer = 0;
		p_pKeyboard->WritePointer = 0;
	}
}

void KBD_Server( void )
{
	size_t Index;
	PKEYBOARD pKeyboard;

	pKeyboard = g_KeyboardState;

	for( Index = 0; Index < 4; ++Index, ++pKeyboard )
	{
		if( pKeyboard->Flags & KBD_INUSE )
		{
			KeyboardPortServer( pKeyboard );
		}
	}
}

static void KeyboardPortServer( PKEYBOARD p_pKeyboard )
{
	const PDS_KEYBOARD *pKeyboard;
	const Uint8 *pKeyTable;
	Uint8 Data;
	size_t Index;
	bool InputFlag;
	bool BufferFull = false, KeyPressed = false;

	InputFlag = false;
	pKeyboard = pdKbdGetData( p_pKeyboard->Port );

	if( pKeyboard == NULL )
	{
		p_pKeyboard->Flags &= ~( KBD_READY );
		p_pKeyboard->pRawData = NULL;

		return;
	}

	if( ( p_pKeyboard->Flags & KBD_READY ) == 0 )
	{
		/* Newly connected/reconnected keyboard */
		if( pKeyboard->info->lang )
		{
			KBD_AutoSetKeyTable( p_pKeyboard, pKeyboard->info->lang );
			p_pKeyboard->Flags |= KBD_READY;
		}
		else
		{
			/* Wait until we can set the key table before attempting to allow
			 * for the key to character map */
			return;
		}
	}

	/* Reset the keyboard state */
	memset( p_pKeyboard->Keys, 0, sizeof( p_pKeyboard->Keys ) );
	p_pKeyboard->pRawData = pKeyboard;

	/* Save the currently pressed keys */
	memcpy( p_pKeyboard->OldKeys, p_pKeyboard->pRawData->key,
		sizeof( p_pKeyboard->pRawData->key ) );

	/* Set the keyboard state */
	for( Index = 0; Index < 6; ++Index )
	{
		Data = pKeyboard->key[ 5 - Index ];

		switch( Data )
		{
			/* No key pressed at this location, or key rollover triggered */
			case 0x00:
			case 0x01:
			{
				break;
			}
			default:
			{
				/* Buffer full */
				if( ( ( p_pKeyboard->WritePointer + 1 ) &
					( p_pKeyboard->KeyBufferSize - 1 ) ) ==
						p_pKeyboard->ReadPointer )
				{
					BufferFull = true;
				}

				/* Set the key state */
				p_pKeyboard->Keys[ Data ] = 1;
				/*SIF_Print( "Key %d held", Key );*/
				KeyPressed = true;
			}
		}
	}

	if( BufferFull == true )
	{
		/* Can't read the key correctly for converting to a character */
		return;
	}

	if( KeyPressed == true )
	{
		goto ReadKey;
	}

	p_pKeyboard->DelayCount = 0;
	p_pKeyboard->Flags &= ~( KBD_REPEAT );
	p_pKeyboard->LastKey = 0;

	return;

ReadKey:
	if( Data == p_pKeyboard->LastKey )
	{
		++p_pKeyboard->DelayCount;

		if( p_pKeyboard->DelayCount > p_pKeyboard->Delay )
		{
			p_pKeyboard->Flags |= KBD_REPEAT;
			/*SIF_Print( "DelayCount: %d", p_pKeyboard->DelayCount );*/
		}
	}
	else
	{
		p_pKeyboard->DelayCount = 0;
		p_pKeyboard->Flags &= ~( KBD_REPEAT );
		InputFlag = true;
	}

	if( p_pKeyboard->Flags & KBD_REPEAT )
	{
		++p_pKeyboard->RateCount;

		if( p_pKeyboard->RateCount >= p_pKeyboard->Rate )
		{
			InputFlag = true;
			p_pKeyboard->RateCount = 0;
		}
	}
	else
	{
		p_pKeyboard->RateCount = 0;
	}

	if( InputFlag )
	{
		Uint16 Key;
		bool Shift;

		/* Switch the key table, depending on the state of the shift key */
		Key = pKeyboard->ctrl & 0xF8;
		Key |= ( pKeyboard->ctrl & 0x07 ) << 4;

		Shift = ( Key & 0x20 ) ? true : false;

		if( Shift == true )
		{
			pKeyTable = p_pKeyboard->ppKeyTable[ KBD_TABLE_SHIFTED ];
		}
		else
		{
			pKeyTable = p_pKeyboard->ppKeyTable[ KBD_TABLE_NORMAL ];
		}

		p_pKeyboard->pKeyBuffer[ p_pKeyboard->WritePointer++ ] =
			pKeyTable[ Data ] | ( Key << 8 );

		p_pKeyboard->WritePointer &= ( p_pKeyboard->KeyBufferSize - 1 );
		p_pKeyboard->LastKey = Data;
	}
}

