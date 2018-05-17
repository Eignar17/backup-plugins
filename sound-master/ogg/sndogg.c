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
 * This plug-in is for loading OGG/Vorbis sound files.
 */

#include <compiz.h>
#include <sound.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

static CompMetadata oggMetadata;
static int displayPrivateIndex;
static int soundDisplayPrivateIndex;

#define GET_OGG_DISPLAY(d) \
    ((SoundDisplay *) (d)->privates[soundDisplayPrivateIndex].ptr)

#define OGG_DISPLAY(d) \
    SoundDisplay *od = GET_OGG_DISPLAY (d)

static void oggSoundPlayFileOgg( CompDisplay *d, char *filename )
{
    OggVorbis_File vf;
    FILE *fp;
    long ret;
    char buffer[BUFFER_SIZE];
    int session;
    Bool eof;

    SOUND_DISPLAY( d );

    fp = fopen( filename, "rb" );
    if ( fp == NULL )
    {
        compLogMessage( NULL, "ogg", CompLogLevelWarn,
            "Failed to fopen file: %s.", filename );
        return;
    }

    ret = ov_open( fp, &vf, NULL, 0 );
    if ( ret < 0 )
    {
        compLogMessage( NULL, "ogg", CompLogLevelWarn,
            "Failed to open Ogg file: %s.", filename );
        fclose( fp );
        return;
    }

    eof = FALSE;
    while ( !eof )
    {
        ret = ov_read( &vf, buffer, sizeof( buffer ), 0, 2, 1, &session );
        if ( ret == 0 )
            eof = TRUE;
        else if ( ret > 0 ) /* Sound is OK to play. */
            (sd->soundPlay) ( (int *) buffer, ret );
    }

    ov_clear( &vf );
}

static void oggWrapFunctions( SoundDisplay *od, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define OGG_WRAP( func ) \
    od->sound ## func = oggSound ## func; \
    WRAP( sd, od, sound ## func, oggSound ## func )

    OGG_WRAP( PlayFileOgg );
#undef OGG_WRAP
}

static void oggUnwrapFunctions( SoundDisplay *od, CompDisplay *d )
{
    SOUND_DISPLAY( d );
#define OGG_UNWRAP( func ) \
    UNWRAP( sd, od, sound ## func)

    OGG_UNWRAP( PlayFileOgg );
#undef OGG_UNWRAP
}

static Bool oggInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    SoundDisplay *od;
    CompPlugin *sound = findActivePlugin( "sound" );
    CompOption *option;
    int nOption;

    if ( !sound || !sound->vTable->getDisplayOptions )
        return FALSE;

    option = (*sound->vTable->getDisplayOptions) (sound, d, &nOption);

    if ( getIntOptionNamed( option, nOption, "abi", 0 ) != SOUND_ABIVERSION )
    {
        compLogMessage( d, "ogg", CompLogLevelError,
                        "Sound ABI version mismatch." );
        return FALSE;
    }

    soundDisplayPrivateIndex = getIntOptionNamed( option, nOption, "index",
                                                  -1 );
    if ( soundDisplayPrivateIndex < 0 )
        return FALSE;

    /* No point in making an OggDisplay structure. */
    od = malloc( sizeof( SoundDisplay ) );
    if (!od)
        return FALSE;

    if ( !compInitDisplayOptionsFromMetadata( d, &oggMetadata, 0, 0, 0 ) )
    {
        free( od );
        return FALSE;
    }

    od->screenPrivateIndex = allocateScreenPrivateIndex( d );
    if ( od->screenPrivateIndex < 0 )
    {
        free( od );
        return FALSE;
    }

    oggWrapFunctions( od, d );

    d->privates[displayPrivateIndex].ptr = od;

    return TRUE;
}

static void oggFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{
    OGG_DISPLAY( d );

    freeScreenPrivateIndex (d, od->screenPrivateIndex);

    oggUnwrapFunctions( od, d );

    free( od );
}

static Bool oggInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &oggMetadata,
                                         plugin->vTable->name, 0, 0, 0, 0 ) )
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( displayPrivateIndex < 0 )
    {
        compFiniMetadata( &oggMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &oggMetadata, plugin->vTable->name );

    return TRUE;
}

static void oggFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( displayPrivateIndex );
    compFiniMetadata( &oggMetadata );
}

static int oggGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *oggGetMetadata( CompPlugin *plugin )
{
    return &oggMetadata;
}

CompPluginVTable oggVTable = 
{
    "sndogg",
    oggGetVersion,
    oggGetMetadata,
    oggInit,
    oggFini,
    oggInitDisplay,
    oggFiniDisplay,
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
    return &oggVTable;
}
