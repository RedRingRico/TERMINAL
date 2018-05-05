#include <Peripheral.h>
#include <shinobi.h>
#include <Log.h>
#include <Keyboard.h>

static Uint8 g_MapleRecv[ 1024 * 24 * 2 + 32 ];
static Uint8 g_MapleSend[ 1024 * 24 * 2 + 32 ];
extern bool g_KeyboardLibraryInitialised;
PDS_PERIPHERAL	g_Peripherals[ 4 ];

/* Prototypes */
void MapleInterrupt( void );

Sint32 PER_Initialise( void )
{
	pdInitPeripheral( PDD_PLOGIC_ACTIVE, g_MapleRecv, g_MapleSend );
	pdSetIntFunction( MapleInterrupt );

	memset( g_Peripherals, 0, sizeof( g_Peripherals ) );

	return 0;
}

void PER_Terminate( void )
{
	pdExitPeripheral( );
}

/* This function should query all connected input devices and inform any
   handles connected to them of their status (connected, disconnected, etc.)*/
void MapleInterrupt( void )
{
    Uint32 PortIndex = PDD_PORT_A0;
    Uint32 Port = 0;

    const PDS_PERIPHERALINFO *pPeripheralInfo[ 4 ] = { NULL };

	/* Handle keyboard updates */
	KBD_Server( );

    for( PortIndex = PDD_PORT_A0; PortIndex <= PDD_PORT_D0; PortIndex +=
        PDD_PORT_B0, ++Port )
    {
        pdGetPeripheralDirect( PortIndex, &g_Peripherals[ Port ], NULL, NULL );

        pPeripheralInfo[ Port ] = pdGetPeripheralInfo( PortIndex );

        if( pPeripheralInfo[ Port ]->type )
        {
            /* Handle disconnect/connect */
        }
    }
}

