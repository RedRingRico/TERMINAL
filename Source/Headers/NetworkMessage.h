#ifndef __TERMINAL_NETWORKMESSAGE_H__
#define __TERMINAL_NETWORKMESSAGE_H__

#include <shinobi.h>

#define NETWORK_MESSAGE_FLAG_OVERFLOW			1
#define NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER	2

#define SIZEOF_BYTE sizeof( Uint8 )
#define SIZEOF_SINT16 sizeof( Sint16 )
#define SIZEOF_UINT16 sizeof( Uint16 )
#define SIZEOF_SINT32 sizeof( Sint32 )
#define SIZEOF_UINT32 sizeof( Uint32 )
#define SIZEOF_FLOAT sizeof( float )

typedef struct _tagNETWORK_MESSAGE
{
	Uint8	*pData;
	size_t	MaxSize;
	size_t	Size;
	size_t	Head;
	Uint32	Flags;
}NETWORK_MESSAGE,*PNETWORK_MESSAGE;

int MSG_CreateNetworkMessage( PNETWORK_MESSAGE p_pMessage,
	Uint8 *p_pBuffer, size_t p_Length );
void MSG_DestroyNetworkMessage( PNETWORK_MESSAGE p_pMessage );

void MSG_Clear( PNETWORK_MESSAGE p_pMessage );

void MSG_Write( PNETWORK_MESSAGE p_pMessage, const void *p_pBuffer,
	const size_t p_Length );
void MSG_WriteByte( PNETWORK_MESSAGE p_pMessage, const Uint8 p_Byte );
void MSG_WriteInt16( PNETWORK_MESSAGE p_pMessage, const Sint16 p_Int16 );
void MSG_WriteUInt16( PNETWORK_MESSAGE p_pMessage, const Uint16 p_UInt16 );
void MSG_WriteInt32( PNETWORK_MESSAGE p_pMessage, const Sint32 p_Int32 );
void MSG_WriteUInt32( PNETWORK_MESSAGE p_pMessage, const Uint32 p_UInt32 );
void MSG_WriteFloat( PNETWORK_MESSAGE p_pMessage, const float p_Float );
void MSG_WriteString( PNETWORK_MESSAGE p_pMessage, const char *p_pString,
	const Uint8 p_Length );

void MSG_Read( PNETWORK_MESSAGE p_pMessage, void *p_pBuffer,
	const size_t p_Length );
Uint8 MSG_ReadByte( PNETWORK_MESSAGE p_pMessage );
Sint16 MSG_ReadInt16( PNETWORK_MESSAGE p_pMessage );
Uint16 MSG_ReadUInt16( PNETWORK_MESSAGE p_pMessage );
Sint32 MSG_ReadInt32( PNETWORK_MESSAGE p_pMessage );
Uint32 MSG_ReadUInt32( PNETWORK_MESSAGE p_pMessage );
float MSG_ReadFloat( PNETWORK_MESSAGE p_pMessage );
void MSG_ReadString( PNETWORK_MESSAGE p_pMessage, char *p_pString,
	const Uint8 p_Length );

#endif /* __TERMINAL_NETWORKMESSAGE_H__ */

