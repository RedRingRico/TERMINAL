#ifndef __TERMINAL_SERIAL_H__
#define __TERMINAL_SERIAL_H__

#include <sg_xpt.h>
#include <sh4scif.h>

#define SIF_OK			0
#define SIF_ERROR		1
#define SIF_FATAL_ERROR	-1

#if defined ( DEBUG ) || defined ( DEVELOPMENT )
#define SIF_Initialise SIF_Initialise_Int
#define SIF_Terminate SIF_Terminate_Int
#define SIF_Clear SIF_Clear_Int
#define SIF_Print SIF_Print_Int
#define SIF_PrintLine SIF_PrintLine_Int
#define SIF_NewLine SIF_NewLine_Int
#else
#define SIF_Initialise sizeof
#define SIF_Terminate( )
#define SIF_Clear( )
#define SIF_Print sizeof
#define SIF_PrintLine sizeof
#define SIF_NewLine( )
#endif /* DEBUG || DEVELOPMENT */

Sint32 SIF_Initialise_Int( Sint32 p_RecvBufferSize, Sint32 p_SendBufferSize,
	Sint32 p_Speed );
void SIF_Terminate_Int( void );

void SIF_Clear_Int( void );
void SIF_Print_Int( const char *p_pString );
void SIF_PrintLine_Int( const char *p_pString );
void SIF_NewLine_Int( void );

#endif /* __TERMINAL_SERIAL_H__ */

