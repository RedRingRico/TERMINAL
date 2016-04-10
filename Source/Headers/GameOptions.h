#ifndef __TERMINAL_GAMEOPTIONS_H__
#define __TERMINAL_GAMEOPTIONS_H__

#include <shinobi.h>

#define GAME_OPTION_SUBTITLES		0x00000001
#define GAME_OPTION_TUTORIAL_HINTS	0x00000002
#define GAME_OPTION_AIM_ASSIST		0x00000004

typedef struct _tagGAME_OPTIONS
{
	float	AspectRatio;
	Uint8	BGMVolume;
	Uint8	SFXVolume;
	Uint8	VideoGamma;
	Uint32	Flags;
}GAME_OPTIONS,*PGAME_OPTIONS;

#endif /* __TERMINAL_GAMEOPTIONS_H__ */

