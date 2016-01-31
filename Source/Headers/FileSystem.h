#ifndef __TERMINAL_FILESYSTEM_H__
#define __TERMINAL_FILESYSTEM_H__

#include <shinobi.h>

int FS_Initialise( void );
int FS_Terminate( void );

GDFS FS_OpenFile( char *p_pFilePath );

#endif /* __TERMINAL_FILESYSTEM_H__ */

