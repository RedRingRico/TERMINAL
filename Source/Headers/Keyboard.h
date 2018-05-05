#ifndef __TERMINAL_KEYBOARD_H__
#define __TERMINAL_KEYBOARD_H__

#include <stddef.h>
#include <sg_pdkbd.h>
#include <sg_xpt.h>

#define KBD_OK			0
#define KBD_ERROR		1
#define KBD_FATAL_ERROR	-1

#define KBD_BUFFER_SIZE		16
#define KBD_REPEAT_COUNT	2
#define KBD_DELAY_COUNT		10

#define KBD_TABLE_NORMAL	0
#define KBD_TABLE_SHIFTED	1

#define KBD_INUSE	1
#define KBD_READY	2
#define KBD_REPEAT	4

typedef struct _tagKEYBOARD
{
	/* In use/Ready/Repeat */
	Uint8			Flags;
	/* Typematic rate */
	Uint8			Rate;
	Uint8			RateCount;
	/* Typematic delay */
	Uint8			Delay;
	Uint8			DelayCount;
	Uint8			LastKey;
	Uint8			OldKeys[ 8 ];
	Uint32			Port;
	size_t			WritePointer;
	size_t			ReadPointer;
	Uint16			*pKeyBuffer;
	size_t			KeyBufferSize;
	PDS_KEYBOARD	*pRawData;
	/* Key conversion table */
	Uint8			**ppKeyTable;
}KEYBOARD, *PKEYBOARD;

Sint32 KBD_Initialise( void );
void KBD_Terminate( );

void KBD_SetKeyTable( PKEYBOARD p_pKeyboard, const Uint8 **p_ppKeyTable );
void KBD_AutoSetKeyTable( PKEYBOARD p_pKeyboard, Uint8 p_Language );
void KBD_SetState( PKEYBOARD p_pKeyboard, Uint8 p_Rate, Uint8 p_Delay );
void KBD_GetState( PKEYBOARD p_pKeyboard, Uint8 *p_pRate, Uint8 *p_pDelay );

bool KBD_KeyPressed( PKEYBOARD p_pKeyboard );
bool KBD_Ready( PKEYBOARD p_pKeyboard );

PKEYBOARD KBD_Create( Uint32 p_Port );
void KBD_Destroy( PKEYBOARD p_pKeyboard );

Uint16 KBD_GetChar( PKEYBOARD p_pKeyboard );
const PDS_KEYBOARD *KBD_GetRawData( PKEYBOARD p_pKeyboard );

void KBD_Flush( PKEYBOARD p_pKeyboard );

void KBD_Server( void );

#endif /* __TERMINAL_KEYBOARD_H__ */
