#ifndef __TERMINAL_DA_H__
#define __TERMINAL_DA_H__

#include <shinobi.h>

#define DA_IPINT	0x01
#define DA_IPSIE	0x02
#define DA_IPMIE	0x04
#define DA_IPRDY	0x08
#define DA_OPINT	0x10
#define DA_OPSIE	0x20
#define DA_OPMIE	0x40
#define DA_OPRDY	0x80

/* This is mainly for the game state manager's use  */
typedef struct _tagDEBUG_ADAPTER
{
	Uint8		*pData;
	Sint32		DataSize;
	Sint32		DataRead;
	bool		Connected;
}DEBUG_ADAPTER, *PDEBUG_ADAPTER;

/* Get the complete status */
Sint32 DA_Poll( Uint32 *p_pStatus );

/* Can be called at any time */
Sint32 DA_GetChannelStatus( Uint32 p_Channel, Uint8 *p_pStatus );

/* Returns the amount of data read, up to p_Size in p_pBytesRead */
Sint32 DA_GetData( void *p_pData, int p_Size, int p_Channel,
	Sint32 *p_pBytesRead );

bool DA_GetConnectionStatus( void );

#endif /* __TERMINAL_DA_H__ */

