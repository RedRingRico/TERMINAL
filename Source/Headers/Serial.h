#ifndef __TERMINAL_SERIAL_H__
#define __TERMINAL_SERIAL_H__

#include <sg_xpt.h>
#include <sh4scif.h>

#define SIF_OK			0
#define SIF_ERROR		1
#define SIF_FATAL_ERROR	-1

Sint32 SIF_Initialise( Sint32 p_RecvBufferSize, Sint32 p_SendBufferSize,
	Sint32 p_Speed );
void SIF_Terminate( void );

void SIF_Clear( void );
void SIF_Print( const char *p_pString );
void SIF_PrintLine( const char *p_pString );
void SIF_NewLine( void );

#endif /* __TERMINAL_SERIAL_H__ */

