#include <NetworkMessage.h>

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

