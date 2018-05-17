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
 * This plug-in the Compiz ALSA driver.
 */

#include <compiz.h>
#include <sound.h>

#include <asoundlib.h>

static CompMetadata alsaMetadata;
static int displayPrivateIndex;
static int soundDisplayPrivateIndex;

typedef struct _AlsaSoundOptionsInfo {
    snd_pcm_access_t access;
    snd_pcm_format_t format;
    unsigned int rate;
    int dir;
    unsigned int num_channels;
} AlsaSoundOptionsInfo;

typedef struct _AlsaSoundInfo {
    snd_pcm_t *handle;
    AlsaSoundOptionsInfo *options;
} AlsaSoundInfo;

static AlsaSoundInfo alsa;

#define GET_ALSA_DISPLAY(d) \
    ((SoundDisplay *) (d)->privates[soundDisplayPrivateIndex].ptr)

#define ALSA_DISPLAY(d) \
    SoundDisplay *ad = GET_ALSA_DISPLAY (d)

static void alsaSoundTest( void )
{
    int i;
    int buffer[256];  /* Random sound. */

    /* TODO: Make this use a file to play sound. */

    for ( i = 0; i < 20; i++ )
        snd_pcm_writei( alsa.handle, buffer, 256 );
}

static void alsaSoundPlay( int *buffer, int buffer_size )
{
    int ret;

    while ( buffer_size > 0 )
    {
        ret = snd_pcm_writei( alsa.handle, buffer, buffer_size );

        if ( ret == -EAGAIN )
            continue;

        if ( ret < 0 )
        {
            /* Underrun. */
            if ( ret == -EPIPE )
            {
                compLogMessage( NULL, "alsa", CompLogLevelWarn,
                    "Underrun.  error: %s",
                    snd_strerror( ret ) );
                ret = snd_pcm_prepare( alsa.handle );
                if ( ret < 0 )
                {
                    compLogMessage( NULL, "alsa", CompLogLevelError,
                        "Could not recover.  error: %s",
                        snd_strerror( ret ) );
                    return;
                }
            }
            break;
        }

        buffer_size -= ret;
        buffer += ret;
    }
}

static void alsaSetSoundOptions( void )
{
    alsa.options = malloc( sizeof( AlsaSoundOptionsInfo ) );

    /* Set defaults. */
    alsa.options->access = SND_PCM_ACCESS_RW_INTERLEAVED;
    alsa.options->format = SND_PCM_FORMAT_S16_LE;
    alsa.options->rate = 44100;
    alsa.options->dir = 0;
    alsa.options->num_channels = 2;

    /* TODO: Set user's options. */
}

static char *alsaFindSoundDevice( void )
{
    /* FIXME: stub */

    return "default";
}

static Bool alsaDoInit( void )
{
    snd_pcm_hw_params_t *params;
    char *device;
    int error;

    alsaSetSoundOptions();
    device = alsaFindSoundDevice();

    error = snd_pcm_open( &alsa.handle, device, SND_PCM_STREAM_PLAYBACK, 0 );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to open sound device: %s, error: %s",
            device, snd_strerror( error ) );
        return FALSE;
    }

    error = snd_pcm_hw_params_malloc( &params );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to malloc HW param structure.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    error = snd_pcm_hw_params_any( alsa.handle, params );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to initialize HW param structure.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    error = snd_pcm_hw_params_set_access( alsa.handle, params,
                                          alsa.options->access );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to set access type.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    error = snd_pcm_hw_params_set_format( alsa.handle, params,
                                          alsa.options->format );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to set sample format.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    error = snd_pcm_hw_params_set_rate_near( alsa.handle, params,
                                             &alsa.options->rate,
                                             &alsa.options->dir );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to set sample rate.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    error = snd_pcm_hw_params_set_channels( alsa.handle, params,
                                            alsa.options->num_channels );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to set number of channels.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }
 
    error = snd_pcm_hw_params( alsa.handle, params );
    if ( error < 0 )
    {
        compLogMessage( NULL, "alsa", CompLogLevelError,
            "Failed to set hardware params.  error: %s",
            snd_strerror( error ) );
        snd_pcm_close( alsa.handle );
        return FALSE;
    }

    snd_pcm_hw_params_free( params );

    return TRUE;
}

static void alsaDoFini( void )
{
    free( alsa.options );
    snd_pcm_close( alsa.handle );
}

static void alsaWrapFunctions( SoundDisplay *ad, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define ALSA_WRAP( func ) \
    ad->sound ## func = alsaSound ## func; \
    WRAP( sd, ad, sound ## func, alsaSound ## func )

    ALSA_WRAP( Test );
    ALSA_WRAP( Play );
#undef ALSA_WRAP
}

static void alsaUnwrapFunctions( SoundDisplay *ad, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define ALSA_UNWRAP( func ) \
    UNWRAP( sd, ad, sound ## func)

    ALSA_UNWRAP( Test );
    ALSA_UNWRAP( Play );
#undef ALSA_UNWRAP
}

static Bool alsaInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    SoundDisplay *ad;
    CompPlugin *sound = findActivePlugin( "sound" );
    CompOption *option;
    int nOption;

    if ( !sound || !sound->vTable->getDisplayOptions )
        return FALSE;

    option = (*sound->vTable->getDisplayOptions) (sound, d, &nOption);

    if ( getIntOptionNamed( option, nOption, "abi", 0 ) != SOUND_ABIVERSION )
    {
        compLogMessage( d, "alsa", CompLogLevelError,
                        "Sound ABI version mismatch." );
        return FALSE;
    }

    soundDisplayPrivateIndex = getIntOptionNamed( option, nOption, "index",
                                                  -1 );
    if ( soundDisplayPrivateIndex < 0 )
        return FALSE;

    /* No point in making an AlsaDisplay structure. */
    ad = malloc( sizeof( SoundDisplay ) );
    if (!ad)
        return FALSE;

    if ( !compInitDisplayOptionsFromMetadata( d, &alsaMetadata, 0, 0, 0 ) )
    {
        free( ad );
        return FALSE;
    }

    ad->screenPrivateIndex = allocateScreenPrivateIndex( d );
    if ( ad->screenPrivateIndex < 0 )
    {
        free( ad );
        return FALSE;
    }

    alsaWrapFunctions( ad, d );

    d->privates[displayPrivateIndex].ptr = ad;

    return TRUE;
}

static void alsaFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{
    ALSA_DISPLAY( d );

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);

    alsaUnwrapFunctions( ad, d );

    free( ad );
}

static Bool alsaInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &alsaMetadata,
                                         plugin->vTable->name, 0, 0, 0, 0 ) )
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( displayPrivateIndex < 0 )
    {
        compFiniMetadata( &alsaMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &alsaMetadata, plugin->vTable->name );

    return alsaDoInit();
}

static void alsaFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( displayPrivateIndex );
    compFiniMetadata( &alsaMetadata );

    alsaDoFini();
}

static int alsaGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *alsaGetMetadata( CompPlugin *plugin )
{
    return &alsaMetadata;
}

CompPluginVTable alsaVTable = 
{
    "alsa",
    alsaGetVersion,
    alsaGetMetadata,
    alsaInit,
    alsaFini,
    alsaInitDisplay,
    alsaFiniDisplay,
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
    return &alsaVTable;
}
