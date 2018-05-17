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
 * This plug-in will produce sound effects.
 */

#include <compiz.h>
#include <sound.h>

static CompMetadata soundeffectsMetadata;
static int displayPrivateIndex;
static int soundDisplayPrivateIndex;

static void soundeffectsWindowAddNotify( CompWindow *w )
{
    SOUND_SCREEN( w->screen );
    SOUND_DISPLAY( w->screen->display );

    (*sd->soundPlayFile) ( w->screen->display, "new.ogg" );

    UNWRAP( ss, w->screen, windowAddNotify );
    (*w->screen->windowAddNotify) ( w );
    WRAP( ss, w->screen, windowAddNotify, soundeffectsWindowAddNotify );
}

static void soundeffectsWindowResizeNotify( CompWindow *w, int dx, int dy,
                                            int dwidth, int dheight )
{
    SOUND_WINDOW( w );
    SOUND_SCREEN( w->screen );
    SOUND_DISPLAY( w->screen->display );
    
    if ( sw->allowMask & AllowResize )
    {
        (*sd->soundPlayFile) ( w->screen->display, "resize.ogg" );
        sw->allowMask &= ~AllowResize;
    }

    UNWRAP( ss, w->screen, windowResizeNotify );
    (*w->screen->windowResizeNotify) ( w, dx, dy, dwidth, dheight );
    WRAP( ss, w->screen, windowResizeNotify, soundeffectsWindowResizeNotify );
}

static void soundeffectsWindowMoveNotify( CompWindow *w, int dx, int dy,
                                          Bool immediate )
{
    SOUND_WINDOW( w );
    SOUND_SCREEN( w->screen );
    SOUND_DISPLAY( w->screen->display );

    if ( sw->allowMask & AllowMove )
    {
        if ( !immediate && abs( dx ) >= abs( dy ) )
        {
            if ( dx > 0 )
                (*sd->soundPlayFile) ( w->screen->display, "moveright.ogg" );
            else
                (*sd->soundPlayFile) ( w->screen->display, "moveleft.ogg" );
        }
        else
        {
            if ( dy > 0 )
                (*sd->soundPlayFile) ( w->screen->display, "movedown.ogg" );
            else
                (*sd->soundPlayFile) ( w->screen->display, "moveup.ogg" );
        }
        sw->allowMask &= ~AllowMove;
    }

    UNWRAP( ss, w->screen, windowMoveNotify );
    (*w->screen->windowMoveNotify) ( w, dx, dy, immediate );
    WRAP( ss, w->screen, windowMoveNotify, soundeffectsWindowMoveNotify );
}

static void soundeffectsWindowGrabNotify( CompWindow *w, int x, int y,
                                          unsigned int state,
                                          unsigned int mask )
{
    SOUND_WINDOW( w );
    SOUND_SCREEN( w->screen );

    if ( mask & CompWindowGrabMoveMask )
        sw->allowMask |= AllowMove;
    if ( mask & CompWindowGrabResizeMask )
        sw->allowMask |= AllowResize;

    UNWRAP( ss, w->screen, windowGrabNotify );
    (*w->screen->windowGrabNotify) ( w, x, y, state, mask );
    WRAP( ss, w->screen, windowGrabNotify, soundeffectsWindowGrabNotify );
}

static void soundeffectsWindowUngrabNotify( CompWindow *w )
{
    SOUND_SCREEN( w->screen );
    SOUND_DISPLAY( w->screen->display );

    (*sd->soundPlayFile) ( w->screen->display, "ungrab.ogg" );

    UNWRAP( ss, w->screen, windowUngrabNotify );
    (*w->screen->windowUngrabNotify) ( w );
    WRAP( ss, w->screen, windowUngrabNotify, soundeffectsWindowUngrabNotify );
}

static Bool soundeffectsInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    CompPlugin *sound = findActivePlugin( "sound" );
    CompOption *option;
    int nOption;

    if ( !sound || !sound->vTable->getDisplayOptions )
        return FALSE;

    option = (*sound->vTable->getDisplayOptions) (sound, d, &nOption);

    if ( getIntOptionNamed( option, nOption, "abi", 0 ) != SOUND_ABIVERSION )
    {
        compLogMessage( d, "soundeffects", CompLogLevelError,
                        "Sound ABI version mismatch." );
        return FALSE;
    }

    soundDisplayPrivateIndex = getIntOptionNamed( option, nOption, "index", 
                                                  -1 );
    if ( soundDisplayPrivateIndex < 0 )
        return FALSE;

    return TRUE;
}

static void soundeffectsFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{
}

static Bool soundeffectsInitScreen( CompPlugin *plugin, CompScreen *s )
{
    SoundScreen *ss;

    SOUND_DISPLAY( s->display );
    
    ss = malloc( sizeof( SoundScreen ) );
    if (!ss)
        return FALSE;

    ss->windowPrivateIndex = allocateWindowPrivateIndex( s );
    if ( ss->windowPrivateIndex < 0 )
    {
        free (ss);
        return FALSE;
    }

    WRAP( ss, s, windowAddNotify, soundeffectsWindowAddNotify );
    WRAP( ss, s, windowResizeNotify, soundeffectsWindowResizeNotify );
    WRAP( ss, s, windowMoveNotify, soundeffectsWindowMoveNotify );
    WRAP( ss, s, windowGrabNotify, soundeffectsWindowGrabNotify );
    WRAP( ss, s, windowUngrabNotify, soundeffectsWindowUngrabNotify );

    s->privates[sd->screenPrivateIndex].ptr = ss;

    return TRUE;
}

static void soundeffectsFiniScreen( CompPlugin *plugin, CompScreen *s )
{
    SOUND_SCREEN( s );

    UNWRAP( ss, s, windowResizeNotify );

    free( ss );
}

static Bool soundeffectsInitWindow( CompPlugin *plugin, CompWindow *w )
{
    SoundWindow *sw;

    SOUND_SCREEN( w->screen );

    sw = malloc( sizeof( SoundWindow ) );
    if (!sw)
        return FALSE;

    sw->allowMask = 0;

    w->privates[ss->windowPrivateIndex].ptr = sw;

    return TRUE;
}

static void soundeffectsFiniWindow( CompPlugin *plugin, CompWindow *w )
{
    SOUND_WINDOW( w );
    free( sw );
}

static Bool soundeffectsInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &soundeffectsMetadata,
                                         plugin->vTable->name, 0, 0, 0, 0 ) )
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( displayPrivateIndex < 0 )
    {
        compFiniMetadata( &soundeffectsMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &soundeffectsMetadata, plugin->vTable->name );

    return TRUE;
}

static void soundeffectsFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( displayPrivateIndex );
    compFiniMetadata( &soundeffectsMetadata );
}

static int soundeffectsGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *soundeffectsGetMetadata( CompPlugin *plugin )
{
    return &soundeffectsMetadata;
}

CompPluginVTable soundeffectsVTable = 
{
    "soundeffects",
    soundeffectsGetVersion,
    soundeffectsGetMetadata,
    soundeffectsInit,
    soundeffectsFini,
    soundeffectsInitDisplay,
    soundeffectsFiniDisplay,
    soundeffectsInitScreen,
    soundeffectsFiniScreen,
    soundeffectsInitWindow,
    soundeffectsFiniWindow,
    0, /* GetDisplayOptions */
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0  /* SetScreenOption */
};

CompPluginVTable *getCompPluginInfo( void )
{
    return &soundeffectsVTable;
}
