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

/* Get the complete status */
int DA_Poll( Uint32 *p_pStatus );

/* Can be called at any time */
/* This should be changed to return the r0 value from polling the DA, with a
   pointer to the byte being a parameter */
Uint8 DA_GetChannelStatus( Uint32 p_Channel );

#endif /* __TERMINAL_DA_H__ */

