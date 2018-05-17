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
 * This header allows the sound plug-ins to communicate.
 */

#ifndef COMPIZ_SOUND_H
#define COMPIZ_SOUND_H

#define SOUND_ABIVERSION 20070614

#define HOME_SOUNDSDIR ".compiz/sounds"
#define BUFFER_SIZE (4 * 1024)

typedef void (*SoundTestProc) ( void );
typedef void (*SoundPlayProc) ( int *buffer, int buffer_size );
typedef void (*SoundPlayFileOggProc) ( CompDisplay *d, char *filename );
typedef void (*SoundPlayFileProc) ( CompDisplay *d, char *filename );
typedef char *(*SoundFindFileProc) ( char *filename, char *theme );

#define SOUND_DISPLAY_OPTION_ABI            0
#define SOUND_DISPLAY_OPTION_INDEX          1
#define SOUND_DISPLAY_OPTION_USE_ALSA       2
#define SOUND_DISPLAY_OPTION_NUM            3

#define AllowMove   (1 << 0)
#define AllowResize (1 << 1)

typedef struct _SoundDisplay {
    int screenPrivateIndex;

    CompOption opt[SOUND_DISPLAY_OPTION_NUM];

    SoundTestProc           soundTest;
    SoundPlayProc           soundPlay;
    SoundPlayFileOggProc    soundPlayFileOgg;
    SoundPlayFileProc       soundPlayFile;
    SoundFindFileProc       soundFindFile;
} SoundDisplay;

typedef struct _SoundScreen {
    int windowPrivateIndex;

    WindowAddNotifyProc     windowAddNotify;
    WindowResizeNotifyProc  windowResizeNotify;
    WindowMoveNotifyProc    windowMoveNotify;
    WindowGrabNotifyProc    windowGrabNotify;
    WindowUngrabNotifyProc  windowUngrabNotify;
} SoundScreen;

typedef struct _SoundWindow {
    unsigned int allowMask;
} SoundWindow;

#define GET_SOUND_DISPLAY(d) \
    ((SoundDisplay *) (d)->privates[soundDisplayPrivateIndex].ptr)

#define SOUND_DISPLAY(d) \
    SoundDisplay *sd = GET_SOUND_DISPLAY (d)

#define GET_SOUND_SCREEN(s, sd) \
    ((SoundScreen *) (s)->privates[(sd)->screenPrivateIndex].ptr)

#define SOUND_SCREEN(s) \
    SoundScreen *ss = GET_SOUND_SCREEN (s, \
                      GET_SOUND_DISPLAY (s->display))

#define GET_SOUND_WINDOW(w, ss) \
    ((SoundWindow *) (w)->privates[(ss)->windowPrivateIndex].ptr)

#define SOUND_WINDOW(w) \
    SoundWindow *sw = GET_SOUND_WINDOW (w, \
                      GET_SOUND_SCREEN (w->screen, \
                      GET_SOUND_DISPLAY (w->screen->display)))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

#endif /* COMPIZ_SOUND_H */
