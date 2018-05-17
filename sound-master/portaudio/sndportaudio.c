/*
 * Copyright (C) 2007 Andrew Riedi <andrewriedi@gmail.com>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * This plug-in the Compiz PortAudio driver.
 */

#include <compiz.h>
#include <sound.h>

#include <portaudio.h>

static CompMetadata portaudioMetadata;
static int displayPrivateIndex;
static int soundDisplayPrivateIndex;

typedef struct _PulseAudioInfo {
} PulseAudioInfo;

static PulseAudioInfo pulseaudioData;

#define GET_PORTAUDIO_DISPLAY(d) \
    ((SoundDisplay *) (d)->privates[soundDisplayPrivateIndex].ptr)

#define PORTAUDIO_DISPLAY(d) \
    SoundDisplay *pd = GET_PORTAUDIO_DISPLAY (d)

static void portaudioSoundTest( void )
{
}

static void portaudioSoundPlay( int *buffer, int buffer_size )
{
}

static int pulseaudioCallback( void *input, void *output,
                               unsigned long framesPerBuffer,
                               PaTimestamp outTime, void *data )
{
    return 0;
}

static Bool portaudioDoInit( void )
{
    PaStream *stream;
    PaError error;

    error = Pa_Initialize();
    if ( error != paNoError )
    {
        compLogMessage( NULL, "portaudio", CompLogLevelError,
            "Failed to initialize PortAudio, error: %s",
            Pa_GetErrorText( error ) );
        return FALSE;
    }

    error = Pa_OpenDefaultStream( stream, 0, 2, paFloat32, 44100, 256, 1,
                                  pulseaudioCallback, &pulseaudioData );
    if ( error != paNoError )
    {
        compLogMessage( NULL, "portaudio", CompLogLevelError,
            "Failed to open a default stream, error: %s",
            Pa_GetErrorText( error ) );
        return FALSE;
    }

    return TRUE;
}

static void portaudioDoFini( void )
{
    PaError error;

    error = Pa_Terminate();
    if ( error != paNoError )
    {
        compLogMessage( NULL, "portaudio", CompLogLevelError,
            "Failed to terminate PortAudio, error: %s",
            Pa_GetErrorText( error ) );
        return;
    }
}

static void portaudioWrapFunctions( SoundDisplay *ad, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define PORTAUDIO_WRAP( func ) \
    ad->sound ## func = portaudioSound ## func; \
    WRAP( sd, ad, sound ## func, portaudioSound ## func )

    PORTAUDIO_WRAP( Test );
    PORTAUDIO_WRAP( Play );
#undef PORTAUDIO_WRAP
}

static void portaudioUnwrapFunctions( SoundDisplay *ad, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define PORTAUDIO_UNWRAP( func ) \
    UNWRAP( sd, ad, sound ## func)

    PORTAUDIO_UNWRAP( Test );
    PORTAUDIO_UNWRAP( Play );
#undef PORTAUDIO_UNWRAP
}

static Bool portaudioInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    SoundDisplay *pd;
    CompPlugin *sound = findActivePlugin( "sound" );
    CompOption *option;
    int nOption;

    if ( !sound || !sound->vTable->getDisplayOptions )
        return FALSE;

    option = (*sound->vTable->getDisplayOptions) (sound, d, &nOption);

    if ( getIntOptionNamed( option, nOption, "abi", 0 ) != SOUND_ABIVERSION )
    {
        compLogMessage( d, "portaudio", CompLogLevelError,
                        "Sound ABI version mismatch." );
        return FALSE;
    }

    soundDisplayPrivateIndex = getIntOptionNamed( option, nOption, "index",
                                                  -1 );
    if ( soundDisplayPrivateIndex < 0 )
        return FALSE;

    /* No point in making a PortaudioDisplay structure. */
    pd = malloc( sizeof( SoundDisplay ) );
    if (!pd)
        return FALSE;

    if ( !compInitDisplayOptionsFromMetadata( d, &portaudioMetadata, 0, 0, 0 ) )
    {
        free( pd );
        return FALSE;
    }

    pd->screenPrivateIndex = allocateScreenPrivateIndex( d );
    if ( pd->screenPrivateIndex < 0 )
    {
        free( pd );
        return FALSE;
    }

    portaudioWrapFunctions( pd, d );

    d->privates[displayPrivateIndex].ptr = pd;

    return TRUE;
}

static void portaudioFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{
    PORTAUDIO_DISPLAY( d );

    freeScreenPrivateIndex (d, pd->screenPrivateIndex);

    portaudioUnwrapFunctions( pd, d );

    free( pd );
}

static Bool portaudioInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &portaudioMetadata,
                                         plugin->vTable->name, 0, 0, 0, 0 ) )
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( displayPrivateIndex < 0 )
    {
        compFiniMetadata( &portaudioMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &portaudioMetadata, plugin->vTable->name );

    return portaudioDoInit();
}

static void portaudioFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( displayPrivateIndex );
    compFiniMetadata( &portaudioMetadata );

    portaudioDoFini();
}

static int portaudioGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *portaudioGetMetadata( CompPlugin *plugin )
{
    return &portaudioMetadata;
}

CompPluginVTable portaudioVTable = 
{
    "portaudio",
    portaudioGetVersion,
    portaudioGetMetadata,
    portaudioInit,
    portaudioFini,
    portaudioInitDisplay,
    portaudioFiniDisplay,
    0, /* InitScreen */
    0, /* FiniScreen */
    0, /* InitWindow */
    0, /* FiniWindow */
    0, /* GetDisplayOptions */
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0  /* SetScreenOption */
};

CompPluginVTable *getCompPluginInfo( void )
{
    return &portaudioVTable;
}
