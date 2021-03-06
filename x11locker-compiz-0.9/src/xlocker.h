/*
 * Compiz x11 locker plugin
 *
 * xlocker.cpp
 *
 * Copyright (c) 2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <core/core.h>
#include <locker/locker.h>

#include <X11/Xatom.h>

#include "xlocker_options.h"

/*
 * Definition of the _COMPIZ_NET_LOCKER_STATUS property:
 *
 * data[0] = API Version
 * data[1] = Window ID of the locker window
 * data[2] = X position of the locker window
 * data[3] = Y position of the locker window
 * data[4] = Width of the locker window
 * data[5] = Height of the locker window
 * data[6] = Visibility of the locker window
 */

#define X_LOCKER_PROPERTY_NAME "_COMPIZ_NET_LOCKER_STATUS"

/*
 * Definition of the _COMPIZ_NET_LOCKER_BACKGROUND property:
 *
 * data[0] = API Version
 * data[1] = Window ID of the background window
 * data[2] = X position of the background window
 * data[3] = Y position of the background window
 * data[4] = Width of the background window
 * data[5] = Height of the background window
 */

#define X_LOCKER_BG_PROPERTY_NAME "_COMPIZ_NET_LOCKER_BACKGROUND_STATUS"

/*
 * Definition of the _COMPIZ_NET_LOCKER_UNLOCK property:
 *
 * data[0] = API Version
 * data[1] = Window ID
 * data[2] = Unlock cookie (hash value generated by
 *                          a random number seed *
 *                          timestamp of sent unlocker
 *                          window creation)
 */

#define X_LOCKER_UNLOCK_PROPERTY_NAME "_COMPIZ_NET_LOCKER_UNLOCK"

class X11LockWindow :
    public LockerLockWindow
{
public:
    X11LockWindow (Window child);
    ~X11LockWindow ();

    void hide ();
    void show ();
    bool visible ();
    void paint (const GLMatrix &transform);

    bool needsGrab () { return false; }
    bool matched (Window, int &);
    bool isBackingWindow (Window w) { return mBackingWindow == w; }
    void completeBackingWindow (CompWindow *w);

    bool isChildWindow (Window w) { return mChild == w; }
    bool attemptUnlock (long *data);

    void setGeometry (int x, int y, unsigned int width, unsigned int height);
    void stackOffendingWindow (Window offender, XWindowChanges *xwc, unsigned int mask);

    static bool processEvent (short int, Display *dpy);

    CompRegion & damage () { return mDamageRegion; }

private:

    CompWindow *mCBackingWindow;
    Window      mBackingWindow;
    Window      mChild;
    bool        mMapped;
    CompRect    mGeometry;
    CompRegion  mDamageRegion;
};

class X11LockerScreen :
    public PluginClassHandler <X11LockerScreen, CompScreen>,
    public ScreenInterface,
    public XlockerOptions
{
public:
    X11LockerScreen (CompScreen *);
    ~X11LockerScreen ();

    /* We need to communicate with the thing doing
     * the locking */
    void handleEvent (XEvent *);
};

class X11LockerWindow :
    public PluginClassHandler <X11LockerWindow, CompWindow>,
    public WindowInterface,
    public GLWindowInterface
{
public:
    X11LockerWindow (CompWindow *w);

    CompWindow *window;
    GLWindow   *gWindow;

private:

    bool mExternalPaint;
};

class X11LockerPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <X11LockerScreen, X11LockerWindow>
{
public:

    bool init ();
};
