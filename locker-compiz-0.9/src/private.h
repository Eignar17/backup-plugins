/*
 * Compiz locker plugin
 *
 * locker.cpp
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
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <locker/locker.h>

#include "locker_options.h"

#include <X11/extensions/scrnsaver.h>

class SimpleLockerWindow :
    public LockerLockWindow
{
public:
    void hide ();
    void show ();
    bool visible ();
    void paint (const GLMatrix &transform);
    bool needsGrab () { return true; }

    bool handleKeyPress (XKeyEvent *xk);
    bool handleKeyRelease (XKeyEvent *xk);
    bool handleButtonPress (XButtonEvent *xb);
    bool handleButtonRelease (XButtonEvent *xb);
    bool handleMotion (XMotionEvent *xm);

    CompRegion & damage ();

private:
    bool mShowing;
    CompRegion mDamage;
};

class SimpleLockerBackground :
    public LockerLockBackground
{
public:
    void hide ();
    void show ();
    bool visible ();
    void paint (const GLMatrix &transform);

    CompRegion & damage ();

private:
    bool mShowing;
    CompRegion mDamage;
};

class XScreenSaverTrigger :
    public LockTrigger
{
public:

    XScreenSaverTrigger (Display *, Window root, unsigned int timeout);
    ~XScreenSaverTrigger ();

    bool usable ();
    void processEvent (XEvent *);
    void unlocked ();

private:

    Display      *dpy;
    Window       root;

    int timeout;
    int interval;
    int preferBlanking;
    int allowExposures;

    int          eventBase;
    int          eventBit;
};

/* Not exactly reliable */
class TimerTrigger :
    public LockTrigger
{
public:

    TimerTrigger (unsigned int timeout);

    void processEvent (XEvent *);
    bool check ();
    void unlocked ();

private:

    CompTimer    mTimer;
    unsigned int mTimeout;
    bool         mLocked;
};

class LockerScreen :
    public PluginClassHandler <LockerScreen, CompScreen, COMPIZ_LOCKER_ABI>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public Lockable,
    public LockerOptions
{
public:

    LockerScreen (CompScreen *);
    ~LockerScreen ();

    CompositeScreen *cScreen;
    GLScreen	*gScreen;

    typedef enum
    {
	AnimationStateIdle = 0,
	AnimationStateLocking = 1,
	AnimationStateUnlocking = 2,
	AnimationStateLocked = 3
    } AnimationState;

public:

    void preparePaint (int);
    bool glPaintOutput (const GLScreenPaintAttrib &attrib,
			const GLMatrix	      &transform,
			const CompRegion	      &region,
			CompOutput                *output,
			unsigned int	      mask);
    void donePaint ();

    bool lock ();
    bool unlock ();
    bool wake ();

    void handleEvent (XEvent *);

private:

    /* Normalized */
    float  	       mAnimationProgress;
    CompWindowList mAnimatedWindows;
    AnimationState mAnimationState;

    CompTimer      mWaitTimer;

    CompScreen::GrabHandle mGrabIndex;
};

class LockerWindow :
    public PluginClassHandler <LockerWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
public:

    LockerWindow (CompWindow *);

    CompWindow	*window;
    CompositeWindow *cWindow;
    GLWindow	*gWindow;

public:

    bool glPaint (const GLWindowPaintAttrib	&attrib,
		  const GLMatrix		&transform,
		  const CompRegion		&region,
		  unsigned int		mask);

    bool isLockerWindow ();
    bool isAnimatedWindow ();

    void setAnimationProgress (float animProgress) { mAnimationProgress = animProgress; };

private:

    /* Normalized */
    float mAnimationProgress;
};

class LockerPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <LockerScreen, LockerWindow>
{
public:

    bool init ();
    void fini ();
};
