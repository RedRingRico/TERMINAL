#ifndef __TERMINAL_PERIPHERAL_H__
#define __TERMINAL_PERIPHERAL_H__

#include <shinobi.h>

extern PDS_PERIPHERAL g_Peripherals[ 4 ];

int PER_Initialise( void );
void PER_Terminate( void );

#endif /* __TERMINAL_PERIPHERAL_H__ */
