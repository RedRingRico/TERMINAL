#ifndef __TERMINAL_NETWORKMESSAGE_H__
#define __TERMINAL_NETWORKMESSAGE_H__

#include <shinobi.h>

#define NETWORK_MESSAGE_FLAG_OVERFLOW			1
#define NETWORK_MESSAGE_FLAG_INTERNAL_BUFFER	2

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

#endif /* __TERMINAL_NETWORKMESSAGE_H__ */

