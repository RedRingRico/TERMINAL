#ifndef __TERMINAL_LOG_H__
#define __TERMINAL_LOG_H__

#if defined ( DEBUG )
#define LOG_Initialise LOG_Initialise_Int
#define LOG_Terminate LOG_Terminate_Int
#define LOG_Debug LOG_Debug_Int
#else
#define LOG_Initialise sizeof
#define LOG_Terminate( )
#define LOG_Debug sizeof
#endif /* DEBUG */

int LOG_Initialise_Int( const char *p_pLogFileName );
void LOG_Terminate_Int( void );
void LOG_Debug_Int( const char *p_pMessage, ... );

#endif /* __TERMINAL_LOG_H__ */

