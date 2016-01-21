#include <Memory.h>
#include <shinobi.h>
#include <sg_syhw.h>

extern Uint8 *_BSG_END;

#define MEM_P1_AREA 0x80000000
#define MEM_WORK_END ( ( ( Uint32 ) _BSG_END ) & 0xE0000000 | 0x0D000000 )
#define MEM_HEAP_AREA ( ( void * )( ( ( ( Uint32 ) _BSG_END | MEM_P1_AREA ) & \
	0xFFFFFFE0 ) + 0x20 ) )
#define MEM_HEAP_SIZE ( MEM_WORK_END - ( Uint32 ) MEM_HEAP_AREA )


int MEM_Initialise( void )
{
	syMallocInit( MEM_HEAP_AREA, MEM_HEAP_SIZE );

	return 0;
}

void MEM_Terminate( void )
{
	syMallocFinish( );
}

