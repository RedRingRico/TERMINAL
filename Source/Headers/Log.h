#ifndef __TERMINAL_LOG_H__
#define __TERMINAL_LOG_H__

int LOG_Initialise( const char *p_pLogFileName );
void LOG_Terminate( void );

void LOG_Debug( const char *p_pMessage, ... );

#endif /* __TERMINAL_LOG_H__ */

