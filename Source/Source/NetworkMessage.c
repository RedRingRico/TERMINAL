#include <NetworkMessage.h>
#include <Log.h>

Uint8 *MSG_GetBufferPosition( PNETWORK_MESSAGE p_pMessage,
	const size_t p_Length );
void MSG_CopyToInternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pSource, const size_t p_Size );

int MSG_CreateNetworkMessage( PNETWORK_MESSAGE p_pMessage,
	Uint8 *p_pBuffer, size_t p_Length )
{
	if( p_pBuffer )
	{
		p_pMessage->pData = p_pBuffer;
		p_pMessage->Flags = 0;
	}
	else
	{
		p_pMessage->pData = malloc( p_Length );

		if( p_pMessage->pData == NULL )
		{
			return 1;
		}

		p_pMessage->Flags = NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER;
	}

	p_pMessage->MaxSize = p_Length;
	p_pMessage->Size = 0;
	p_pMessage->Head = 0;

	return 0;
}

void MSG_DestroyNetworkMessage( PNETWORK_MESSAGE p_pMessage )
{
	if( p_pMessage->Flags & NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER )
	{
		free( p_pMessage->pData );
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


Uint8 *MSG_GetBufferPosition( PNETWORK_MESSAGE p_pMessage,
	const size_t p_Length )
{
	Uint8 *pBuffer;

	if( ( p_Length + p_pMessage->Size ) > p_pMessage->MaxSize )
	{
		if( p_Length > p_pMessage->MaxSize )
		{
			LOG_Debug( "Message size is greater than the supported maximum" );
			// Assert
		}

		MSG_Clear( p_pMessage );
		p_pMessage->Flags |= NETWORK_MESSAGE_FLAG_OVERFLOW;
	}

	pBuffer = p_pMessage->pData + p_Length;
	p_pMessage->Size += p_Length;

	return pBuffer;
}

void MSG_CopyToInternalBuffer( PNETWORK_MESSAGE p_pMessage,
	void *p_pSource, const size_t p_Size )
{
	memcpy( MSG_GetBufferPosition( p_pMessage, p_Size ), p_pSource, p_Size );
}

