#ifndef __TERMINAL_PERIPHERAL_H__
#define __TERMINAL_PERIPHERAL_H__

#include <shinobi.h>

#define PDD_SOFT_RESET_STANDARD_GAMEPAD \
	( PDD_DGT_TA | PDD_DGT_TB | PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_ST )

extern PDS_PERIPHERAL g_Peripherals[ 4 ];

int PER_Initialise( void );
void PER_Terminate( void );

#endif /* __TERMINAL_PERIPHERAL_H__ */
