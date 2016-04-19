#include <NetworkMessage.h>
#include <Log.h>

Uint8 *MSG_GetBufferPosition( PNETWORK_MESSAGE p_pMessage,
	const size_t p_Length );
void MSG_CopyToInternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pSource, const size_t p_Size );
void MSG_CopyToExternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pDestination, const size_t p_Size );

int MSG_CreateNetworkMessage( PNETWORK_MESSAGE p_pMessage,
	Uint8 *p_pBuffer, size_t p_Length, PMEMORY_BLOCK p_pMemoryBlock )
{
	if( p_pBuffer )
	{
		p_pMessage->pData = p_pBuffer;
		p_pMessage->Flags = 0;
	}
	else
	{
		p_pMessage->pData = MEM_AllocateFromBlock( p_pMemoryBlock, p_Length,
			"Network message" );

		if( p_pMessage->pData == NULL )
		{
			return 1;
		}

		p_pMessage->Flags = NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER;
	}

	p_pMessage->MaxSize = p_Length;
	p_pMessage->Size = 0;
	p_pMessage->Head = 0;
	p_pMessage->pMemoryBlock = p_pMemoryBlock;

	return 0;
}

int MSG_CopyNetworkMessage( PNETWORK_MESSAGE p_pCopy,
	PNETWORK_MESSAGE p_pOriginal, size_t p_SizeToCopy )
{
	p_pCopy->pData = MEM_AllocateFromBlock( p_pOriginal->pMemoryBlock,
		p_SizeToCopy, "Network message copy" );

	if( p_pCopy->pData == NULL )
	{
		LOG_Debug( "MSG_CopyNetworkMessage <ERORR> Failed to allocate memory "
			"for the copied message\n" );

		return 1;
	}

	memcpy( p_pCopy->pData, p_pOriginal->pData, p_SizeToCopy );

	p_pCopy->Flags =
		p_pOriginal->Flags |= NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER;
	p_pCopy->MaxSize = p_SizeToCopy;
	p_pCopy->Size = 0;
	p_pCopy->Head = 0;
	p_pCopy->pMemoryBlock = p_pOriginal->pMemoryBlock;

	return 0;
}

void MSG_DestroyNetworkMessage( PNETWORK_MESSAGE p_pMessage )
{
	if( p_pMessage->Flags & NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER )
	{
		MEM_FreeFromBlock( p_pMessage->pMemoryBlock, p_pMessage->pData );
		p_pMessage->pData = NULL;
		p_pMessage->Flags &= ~NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER;
	}
}

void MSG_Write( PNETWORK_MESSAGE p_pMessage, const void *p_pBuffer,
	const size_t p_Length )
{
	memcpy( MSG_GetBufferPosition( p_pMessage, p_Length ), p_pBuffer,
		p_Length );
}

void MSG_Clear( PNETWORK_MESSAGE p_pMessage )
{
	p_pMessage->Size = 0;
	p_pMessage->Head = 0;
	p_pMessage->Flags &= ~NETWORK_MESSAGE_FLAG_OVERFLOW;
}


void MSG_WriteByte( PNETWORK_MESSAGE p_pMessage, const Uint8 p_Byte )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_Byte, SIZEOF_BYTE );
}

void MSG_WriteInt16( PNETWORK_MESSAGE p_pMessage, const Sint16 p_Int16 )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_Int16, SIZEOF_SINT16 );
}

void MSG_WriteUInt16( PNETWORK_MESSAGE p_pMessage, const Uint16 p_UInt16 )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_UInt16, SIZEOF_UINT16 );
}

void MSG_WriteInt32( PNETWORK_MESSAGE p_pMessage, const Sint32 p_Int32 )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_Int32, SIZEOF_SINT32 );
}

void MSG_WriteUInt32( PNETWORK_MESSAGE p_pMessage, const Uint32 p_UInt32 )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_UInt32, SIZEOF_UINT32 );
}

void MSG_WriteFloat( PNETWORK_MESSAGE p_pMessage, const float p_Float )
{
	MSG_CopyToInternalBuffer( p_pMessage, &p_Float, SIZEOF_FLOAT );
}

void MSG_WriteString( PNETWORK_MESSAGE p_pMessage, const char *p_pString,
	const Uint8 p_Length )
{
	MSG_Write( p_pMessage, p_pString, p_Length );
}

void MSG_Read( PNETWORK_MESSAGE p_pMessage, void *p_pBuffer,
	const size_t p_Length )
{
	MSG_CopyToExternalBuffer( p_pMessage, p_pBuffer, p_Length );
}

Uint8 MSG_ReadByte( PNETWORK_MESSAGE p_pMessage )
{
	Uint8 Byte;

	MSG_CopyToExternalBuffer( p_pMessage, &Byte, SIZEOF_BYTE );

	return Byte;
}

Sint16 MSG_ReadInt16( PNETWORK_MESSAGE p_pMessage )
{
	Sint16 Int16;

	MSG_CopyToExternalBuffer( p_pMessage, &Int16, SIZEOF_SINT16 );

	return Int16;
}

Uint16 MSG_ReadUInt16( PNETWORK_MESSAGE p_pMessage )
{
	Uint16 UInt16;

	MSG_CopyToExternalBuffer( p_pMessage, &UInt16, SIZEOF_UINT16 );

	return UInt16;
}

Sint32 MSG_ReadInt32( PNETWORK_MESSAGE p_pMessage )
{
	Sint32 Int32;

	MSG_CopyToExternalBuffer( p_pMessage, &Int32, SIZEOF_SINT32 );

	return Int32;
}

Uint32 MSG_ReadUInt32( PNETWORK_MESSAGE p_pMessage )
{
	Uint32 UInt32;

	MSG_CopyToExternalBuffer( p_pMessage, &UInt32, SIZEOF_UINT32 );

	return UInt32;
}

float MSG_ReadFloat( PNETWORK_MESSAGE p_pMessage )
{
	float Float;

	MSG_CopyToExternalBuffer( p_pMessage, &Float, SIZEOF_FLOAT );

	return Float;
}

void MSG_ReadString( PNETWORK_MESSAGE p_pMessage, char *p_pString,
	const Uint8 p_Length )
{
	MSG_Read( p_pMessage, p_pString, p_Length );
}

Uint8 *MSG_GetBufferPosition( PNETWORK_MESSAGE p_pMessage,
	const size_t p_Length )
{
	Uint8 *pBuffer;

	if( ( p_Length + p_pMessage->Size ) > p_pMessage->MaxSize )
	{
		if( p_Length > p_pMessage->MaxSize )
		{
			LOG_Debug( "Message size is greater than the supported maximum" );
			// Assert here!
		}

		MSG_Clear( p_pMessage );
		p_pMessage->Flags |= NETWORK_MESSAGE_FLAG_OVERFLOW;
	}

	pBuffer = p_pMessage->pData + p_pMessage->Size;
	p_pMessage->Size += p_Length;

	return pBuffer;
}

void MSG_CopyToInternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pSource, const size_t p_Size )
{
	memcpy( MSG_GetBufferPosition( p_pMessage, p_Size ), p_pSource, p_Size );
}

void MSG_CopyToExternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pDestination, const size_t p_Size )
{
	if( ( p_Size + p_pMessage->Size ) > p_pMessage->MaxSize )
	{
		return;
	}

	memcpy( p_pDestination, &p_pMessage->pData[ p_pMessage->Head ], p_Size );
	p_pMessage->Head += p_Size;
}

