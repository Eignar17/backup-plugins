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
 * This plug-in controls the sound drivers.
 */

#include <compiz.h>
#include <sound.h>

#include <string.h>

static CompMetadata soundMetadata;
static int soundDisplayPrivateIndex;

#define MAKE_DUMMY_FUNCTION( func, ... ) \
static void dummySound ## func( __VA_ARGS__ ) \
{ \
    compLogMessage( NULL, "sound", CompLogLevelWarn, \
        "sound" #func "() unimplemented." ); \
}

MAKE_DUMMY_FUNCTION( Test, void )
MAKE_DUMMY_FUNCTION( Play, int *buffer, int buffer_size )
MAKE_DUMMY_FUNCTION( PlayFileOgg, CompDisplay *d, char *filename )
#undef MAKE_DUMMY_FUNCTION

static const CompMetadataOptionInfo soundDisplayOptionInfo[] = {
    { "abi", "int", 0, 0, 0 },
    { "index", "int", 0, 0, 0 },
    { "use_alsa", "bool", 0, 0, 0 },
};

static char *soundSoundFindFile( char *filename, char *theme )
{
    char *home, *fullFilename = NULL;

    home = getenv( "HOME" );
    if ( home )
    {
        fullFilename = malloc( strlen( home ) + strlen( HOME_SOUNDSDIR) +
                               strlen( theme ) + strlen( filename ) + 4 );
        if ( fullFilename )
        {
            sprintf( fullFilename, "%s/%s/%s/%s", home, HOME_SOUNDSDIR, theme, 
                     filename );
        }
    }

    return fullFilename;
}

static void soundSoundPlayFile( CompDisplay *d, char *filename )
{
    char *fullFilename;

    SOUND_DISPLAY( d );

    fullFilename = (sd->soundFindFile) ( filename, "default" );
    (sd->soundPlayFileOgg) ( d, fullFilename );
    free( fullFilename );

    /* FIXME: stub */
}

static void soundWrapDummyFunctions( SoundDisplay *sd )
{
#define DUMMY_WRAP( func ) \
    sd->sound ## func = dummySound ## func

    DUMMY_WRAP( Test );
    DUMMY_WRAP( Play );
    DUMMY_WRAP( PlayFileOgg );
#undef DUMMY_WRAP
}

static void soundWrapFunctions( SoundDisplay *sd )
{
#define SOUND_WRAP( func ) \
    sd->sound ## func = soundSound ## func

    SOUND_WRAP( PlayFile );
    SOUND_WRAP( FindFile );
#undef SOUND_WRAP
}

static Bool soundInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    SoundDisplay *sd;

    sd = malloc( sizeof( SoundDisplay ) );
    if (!sd)
        return FALSE;

    if (!compInitDisplayOptionsFromMetadata( d,
                                             &soundMetadata,
                                             soundDisplayOptionInfo,
                                             sd->opt,
                                             SOUND_DISPLAY_OPTION_NUM ))
    {
        free (sd);
        return FALSE;
    }

    sd->opt[SOUND_DISPLAY_OPTION_ABI].value.i = SOUND_ABIVERSION;
    sd->opt[SOUND_DISPLAY_OPTION_INDEX].value.i = soundDisplayPrivateIndex;

    sd->screenPrivateIndex = allocateScreenPrivateIndex( d );
    if ( sd->screenPrivateIndex < 0 )
    {
        compFiniDisplayOptions( d, sd->opt, SOUND_DISPLAY_OPTION_NUM );
        free (sd);
        return FALSE;
    }

    soundWrapDummyFunctions( sd );
    soundWrapFunctions( sd );

    d->privates[soundDisplayPrivateIndex].ptr = sd;

    return TRUE;
}

static void soundFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{   
    SOUND_DISPLAY( d );

    freeScreenPrivateIndex( d, sd->screenPrivateIndex );

    compFiniDisplayOptions( d, sd->opt, SOUND_DISPLAY_OPTION_NUM );
    
    free( sd );
}

static Bool soundInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &soundMetadata,
                                         plugin->vTable->name,
                                         soundDisplayOptionInfo,
                                         SOUND_DISPLAY_OPTION_NUM,
                                         0, 0 ) )
        return FALSE;

    soundDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( soundDisplayPrivateIndex < 0 )
    {
        compFiniMetadata( &soundMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &soundMetadata, plugin->vTable->name );

    return TRUE;
}

static void soundFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( soundDisplayPrivateIndex );
    compFiniMetadata( &soundMetadata );
}

static CompOption *soundGetDisplayOptions( CompPlugin *plugin,
                                           CompDisplay *display, int *count)
{
    SOUND_DISPLAY( display );

    *count = NUM_OPTIONS( sd );
    return sd->opt;
}

static int soundGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *soundGetMetadata( CompPlugin *plugin )
{
    return &soundMetadata;
}

CompPluginVTable soundVTable = 
{
    "sound",
    soundGetVersion,
    soundGetMetadata,
    soundInit,
    soundFini,
    soundInitDisplay,
    soundFiniDisplay,
    0, /* InitScreen */
    0, /* FiniScreen */
    0, /* InitWindow */
    0, /* FiniWindow */
    soundGetDisplayOptions,
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0  /* SetScreenOption */
};

CompPluginVTable *getCompPluginInfo( void )
{
    return &soundVTable;
}
