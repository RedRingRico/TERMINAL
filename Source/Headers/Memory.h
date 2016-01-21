#ifndef __TERMINAL_MEMORY_H__
#define __TERMINAL_MEMORY_H__

#define MEM_SH4_P2NonCachedMemory( p_pAddress ) \
	( ( ( ( long ) p_pAddress ) & 0x0FFFFFFF ) | 0xA0000000 )

int MEM_Initialise( void );
void MEM_Terminate( void );

#endif /* __TERMINAL_MEMORY_H__ */

