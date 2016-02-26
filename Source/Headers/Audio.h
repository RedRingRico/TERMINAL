#ifndef __TERMINAL_AUDIO_H__
#define __TERMINAL_AUDIO_H__

#include <ac.h>
#include <shinobi.h>

#define AUDIO_PARAMETER_FLAG_TRANSFER_DMA	1
#define AUDIO_PARAMETER_FLAG_LINEAR_VOLUME	2

#define AUDIO_VOICE_FLAG_LOOP	1

/* For now, only short, simple FX are being handled */
typedef struct _tagAUDIO_VOICE
{
	Uint32	*pMemoryLocation;
	Uint32	Flags;
	Sint32	Size;
	Uint16	SampleRate;
	Uint8	Volume;
	Uint8	Channel;
}AUDIO_VOICE,*PAUDIO_VOICE;

typedef struct _tagAUDIO_PARAMETERS
{
	AC_CALLBACK_HANDLER	IntCallback;
	Uint32				Flags;
}AUDIO_PARAMETERS,*PAUDIO_PARAMETERS;

int AUD_Initialise( PAUDIO_PARAMETERS p_pAudioParameters );
void AUD_Terminate( void );

int AUD_PlayVoice( PAUDIO_VOICE p_pVoice );

#endif /* __TERMINAL_AUDIO_H__ */

