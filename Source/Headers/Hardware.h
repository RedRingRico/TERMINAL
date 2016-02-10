#ifndef __TERMINAL_HARDWARE_H__
#define __TERMINAL_HARDWARE_H__

#include <kamui2.h>
#include <shinobi.h>

int HW_Initialise( KMBPPMODE p_BPP, SYE_CBL *p_pCableType );
void HW_Terminate( void );

void HW_Reboot( void );

#endif /* __TERMINAL_HARDWARE_H__ */

