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

#include "private.h"
#include <cmath>
#include <limits.h>

COMPIZ_PLUGIN_20090315 (locker, LockerPluginVTable);

namespace {
    static LockerLockWindow     *mWindowDefault = NULL;
    static LockerLockBackground *mBackgroundDefault = NULL;
    static Lockable             *mLockableDefault = NULL;
    static LockTrigger          *mTriggerDefault = NULL;
}

LockerLockWindow *
LockerLockWindow::Default ()
{
    return mWindowDefault;
}

void
LockerLockWindow::SetDefault (LockerLockWindow *l)
{
    if (mWindowDefault)
	delete mWindowDefault;

    mWindowDefault = l;
}

LockerLockBackground *
LockerLockBackground::Default ()
{
    return mBackgroundDefault;
}

void
LockerLockBackground::SetDefault (LockerLockBackground *b)
{
    if (mBackgroundDefault)
	delete mBackgroundDefault;

    mBackgroundDefault = b;
}

Lockable *
Lockable::Default ()
{
    return mLockableDefault;
}

void
Lockable::SetDefault (Lockable *b)
{
    if (mLockableDefault)
	delete mLockableDefault;

    mLockableDefault = b;
}

void
LockTrigger::SetDefault (LockTrigger *t)
{
    if (mTriggerDefault)
	delete mTriggerDefault;

    mTriggerDefault = t;
}

LockTrigger *
LockTrigger::Default ()
{
    return mTriggerDefault;
}


void
SimpleLockerWindow::hide ()
{
    mShowing = false;
}

void
SimpleLockerWindow::show ()
{
    mShowing = true;
}

bool
SimpleLockerWindow::visible ()
{
    return mShowing;
}

void
SimpleLockerWindow::paint (const GLMatrix &transform)
{
    /* Paint a white box in the center of the screen */

    if (!mShowing)
	return;

    glPushMatrix ();
    glLoadMatrixf (transform.getMatrix ());

    glColor4f (1.0f, 1.0f, 1.0f, 1.0f);

    glBegin (GL_QUADS);
    glVertex3f (x () + width () / 2.0f - 150, y () + height () / 2.0f + 150, 0.0f);
    glVertex3f (x () + width () / 2.0f + 150, y () + height ()  / 2.0f + 150, 0.0f);
    glVertex3f (x () + width () / 2.0f + 150, y () + height ()  / 2.0f - 150, 0.0f);
    glVertex3f (x () + width () / 2.0f - 150, y () + height ()  / 2.0f - 150, 0.0f);
    glEnd ();

    glPopMatrix ();

    mDamage += CompRect (x () + width () / 2.0f - 150, y () + height () / 2.0f - 150, 300, 300);
}

bool
SimpleLockerWindow::handleKeyPress (XKeyEvent *xk)
{
    return true;
}

bool
SimpleLockerWindow::handleKeyRelease (XKeyEvent *xk)
{
    return true;
}

bool
SimpleLockerWindow::handleButtonPress (XButtonEvent *xb)
{
    CompPoint   p;
    CompRegion  r;

    r = CompRect (x () + width () / 2.0f - 150, y () + height () / 2.0f - 150, 300, 300);
    p = CompPoint (xb->x_root, xb->y_root);

    if (r.contains (p))
	Lockable::Default ()->unlock ();

    return true;
}

bool
SimpleLockerWindow::handleButtonRelease (XButtonEvent *xb)
{
    CompPoint   p;
    CompRegion  r;

    r = CompRect (x () + width () / 2.0f - 150, y () + height () / 2.0f - 150, 300, 300);
    p = CompPoint (xb->x_root, xb->y_root);

    if (r.contains (p))
	Lockable::Default ()->unlock ();
    return true;
}

bool
SimpleLockerWindow::handleMotion (XMotionEvent *xm)
{
    return true;
}

void
SimpleLockerBackground::hide ()
{
    mShowing = false;
}

void
SimpleLockerBackground::show ()
{
    mShowing = true;
}

bool
SimpleLockerBackground::visible ()
{
    return mShowing;
}

CompRegion &
SimpleLockerBackground::damage ()
{
    return mDamage;
}

void
SimpleLockerBackground::paint (const GLMatrix &transform)
{
    /* Paint a multicolored box in the center of the screen */

    if (!mShowing)
	return;

    glPushMatrix ();
    glLoadMatrixf (transform.getMatrix ());

    glBegin (GL_QUADS);
    glColor4f (1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f (x (), y () + height (), 0.0f);
    glColor4f (0.0f, 1.0f, 0.0f, 1.0f);
    glVertex3f (x () + width (), y () + height (), 0.0f);
    glColor4f (0.0f, 0.0f, 1.0f, 1.0f);
    glVertex3f (x () + width (), y (), 0.0f);
    glColor4f (0.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f (x (), y (), 0.0f);
    glEnd ();

    glPopMatrix ();

    mDamage += CompRect (x (), y (), width (), height ());
}


CompRegion &
SimpleLockerWindow::damage ()
{
    return mDamage;
}

void
LockerScreen::handleEvent (XEvent *event)
{
    LockerLockWindow *lockWindow = LockerLockWindow::Default ();
    LockTrigger      *lockTrigger = LockTrigger::Default ();
    bool		 processed = false;

    /* FIXME */

    XScreenSaverTrigger *xt = dynamic_cast <XScreenSaverTrigger *> (lockTrigger);
    if (xt)
	xt->processEvent (event);

    TimerTrigger *tt = dynamic_cast <TimerTrigger *> (lockTrigger);
    if (tt)
	tt->processEvent (event);

    switch (event->type)
    {
    case ButtonPress:
	if (lockWindow && lockWindow->needsGrab () && mAnimationState == AnimationStateLocked)
	    processed = lockWindow->handleButtonPress ((XButtonEvent *) event);
	break;
    case ButtonRelease:
	if (lockWindow && lockWindow->needsGrab () && mAnimationState == AnimationStateLocked)
	    processed = lockWindow->handleButtonRelease ((XButtonEvent *) event);
	break;
    case KeyPress:
	if (lockWindow && lockWindow->needsGrab () && mAnimationState == AnimationStateLocked)
	    processed = lockWindow->handleKeyPress ((XKeyEvent *) event);
	break;
    case KeyRelease:
	if (lockWindow && lockWindow->needsGrab () && mAnimationState == AnimationStateLocked)
	    processed = lockWindow->handleKeyRelease ((XKeyEvent *) event);
	break;
    case MotionNotify:
	if (lockWindow && lockWindow->needsGrab () && mAnimationState == AnimationStateLocked)
	    processed = lockWindow->handleMotion ((XMotionEvent *) event);
	break;
    }

    if (!processed)
	screen->handleEvent (event);
}

void
LockerScreen::preparePaint (int ms)
{
    if (mAnimationState == AnimationStateLocking)
    {
	mAnimationProgress += (ms / (float) optionGetLockAnimationDuration ());

	if (mAnimationProgress >= 1.0f)
	    mAnimationProgress = 1.0f;

	foreach (CompWindow *w, screen->windows ())
	    LockerWindow::get (w)->setAnimationProgress (mAnimationProgress);
    }

    if (mAnimationState == AnimationStateUnlocking)
    {
	mAnimationProgress -= (ms / (float) optionGetUnlockAnimationDuration ());

	if (mAnimationProgress <= 0.0f)
	    mAnimationProgress = 0.0f;

	foreach (CompWindow *w, screen->windows ())
	    LockerWindow::get (w)->setAnimationProgress (mAnimationProgress);
    }

    cScreen->preparePaint (ms);
}

bool
LockerScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			     const GLMatrix	       &transform,
			     const CompRegion	       &region,
			     CompOutput                *output,
			     unsigned int	       mask)
{
    LockerLockWindow *lockWindow = LockerLockWindow::Default ();
    LockerLockBackground *lockScreen = LockerLockBackground::Default ();
    bool		status;

    if (mAnimationState != AnimationStateIdle &&
	mAnimationState != AnimationStateLocked)
    {
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	mask |= PAINT_SCREEN_TRANSFORMED_MASK;
	mask |= PAINT_SCREEN_CLEAR_MASK;
    }

    /* Nothing shall pass - clear the entire backbuffer and short circuit
     * the paint chain and then paint our locker windows, we absolutely do
     * not want anything else to go on top of us */
    if (mAnimationState == AnimationStateLocked)
    {
	GLint box[4];
	GLboolean scissoringEnabled = glIsEnabled (GL_SCISSOR_TEST);
	GLMatrix lTransform (transform);
	glGetIntegerv (GL_SCISSOR_BOX, box);
	if (!scissoringEnabled)
	    glEnable (GL_SCISSOR_TEST);

	lTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glScissor (output->x (), output->y (), output->width (), output->height ());
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);
	/* We could probably be a little smarter here, like binding
	 * an fbo and then never rendering it, it's not possible to break out
	 * of something like that */
	glScissor (0, 0, 0, 0);
	gScreen->glPaintOutputSetCurrentIndex (INT_MAX);
	status = gScreen->glPaintOutput (attrib, transform, region, output, mask);
	glScissor (box[0], box[1], box[2], box[3]);
	if (!scissoringEnabled)
	    glDisable (GL_SCISSOR_TEST);

	/* Now paint the lockers */
	if (lockScreen)
	{
	    CompRegion damage;
	    lockScreen->setGeometry (output->x (), output->y (),
				     output->width (), output->height ());
	    lockScreen->paint (lTransform);
	}

	if (lockWindow)
	{
	    CompRegion damage;
	    lockWindow->setGeometry (output->x (), output->y (),
				     output->width (), output->height ());
	    lockWindow->paint (lTransform);
	}

    }
    else
	status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    return status;
}

void
LockerScreen::donePaint ()
{
    LockerLockWindow *lockWindow = LockerLockWindow::Default ();
    LockerLockBackground *lockBackground = LockerLockBackground::Default ();

    if (mAnimationState == AnimationStateLocking ||
	mAnimationState == AnimationStateUnlocking)
    {
	if (mAnimationState == AnimationStateLocking &&
	    mAnimationProgress == 1.0f)
	{
	    mAnimationState = AnimationStateLocked;
	}

	if (mAnimationState == AnimationStateUnlocking &&
	    mAnimationProgress == 0.0f)
	{
	    mAnimationState = AnimationStateIdle;
	}

	cScreen->damageScreen ();
    }
    else if (mAnimationState == AnimationStateLocked)
    {
	if (lockWindow)
	{
	    cScreen->damageRegion (lockWindow->damage ());
	    lockWindow->damage () = CompRegion ();
	}

	if (lockBackground)
	{
	    cScreen->damageRegion (lockBackground->damage ());
	    lockBackground->damage () = CompRegion ();
	}
    }

    cScreen->donePaint ();
}

bool
LockerScreen::wake ()
{
    LockerLockWindow     *lockWindow = LockerLockWindow::Default ();
    LockerLockBackground *lockBackground = LockerLockBackground::Default ();

    if (mAnimationState == AnimationStateLocked)
    {
	/* No default locking implementation provided, just unlock
	 * the screen */
	if (!lockWindow)
	    unlock ();
	else
	{
	    if (lockWindow)
		lockWindow->show ();

	    if (lockBackground)
		lockBackground->show ();
	}
    }
    else
    {
	mWaitTimer.setTimes (optionGetTimeout () * 1000, optionGetTimeout () * 1000);
    }

    return false;
}

bool
LockerScreen::lock ()
{
    LockerLockWindow *lockWindow = LockerLockWindow::Default ();

    if (mAnimationState != AnimationStateLocked ||
	mAnimationState != AnimationStateLocking)
	mAnimationState = AnimationStateLocking;

    if (lockWindow)
    {
	if (!mGrabIndex)
	{
	    if (lockWindow->needsGrab ())
	    {
		mGrabIndex = screen->pushGrab (screen->invisibleCursor (), "locker");
	    }
	}
    }

    return false;
}

bool
LockerScreen::unlock ()
{
    LockTrigger *trigger = LockTrigger::Default ();
    LockerLockWindow *lockWindow = LockerLockWindow::Default ();
    LockerLockBackground *lockBackground = LockerLockBackground::Default ();

    if (mAnimationState != AnimationStateUnlocking ||
	mAnimationState != AnimationStateIdle)
	mAnimationState = AnimationStateUnlocking;

    if (mGrabIndex)
    {
	screen->removeGrab (mGrabIndex, NULL);
	mGrabIndex = 0;
    }

    lockWindow->hide ();
    lockBackground->hide ();

    trigger->unlocked ();

    return false;
}
    

bool
LockerWindow::glPaint (const GLWindowPaintAttrib &attrib,
		       const GLMatrix	     &transform,
		       const CompRegion	     &region,
		       unsigned int		     mask)
{
    GLWindowPaintAttrib wAttrib (attrib);
    GLMatrix            wTransform (transform);

    /* A simple fade in / out will do */
    if (isAnimatedWindow ())
    {
	wAttrib.brightness = (int) ((1.0f - mAnimationProgress) * (float) attrib.brightness);
	wTransform.translate (0.0f, 0.0f, -mAnimationProgress * 0.05);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
    }

    return gWindow->glPaint (wAttrib, wTransform, region, mask);
}

bool
LockerWindow::isAnimatedWindow ()
{
    if (window->isViewable ())
	return true;

    return false;
}

void
XScreenSaverTrigger::unlocked ()
{
    XSetScreenSaver (dpy, timeout, interval, preferBlanking, allowExposures);
}

void
XScreenSaverTrigger::processEvent (XEvent *ev)
{
    Lockable                *lockable = Lockable::Default ();
    XScreenSaverNotifyEvent *xssEvent;

    switch ((ev->type & 0x7F) - eventBase)
    {
    case ScreenSaverNotify:
	xssEvent = (XScreenSaverNotifyEvent *) ev;

	if (xssEvent->state)
	{
	    XSetScreenSaver (dpy, 0, interval, preferBlanking, allowExposures);
	    lockable->lock ();
	}
	else
	    lockable->wake ();
    }
}

bool
XScreenSaverTrigger::usable ()
{
    return eventBase != 0;
}

XScreenSaverTrigger::XScreenSaverTrigger (Display *dpy, Window root, unsigned int cTimeout) :
    dpy (dpy),
    root (root),
    timeout (0),
    interval (0),
    preferBlanking (0),
    allowExposures (0),
    eventBase (0),
    eventBit (0)
{
    int                   dummy;
    long unsigned int     mask = 0;
    XSetWindowAttributes  attr;

    /* Make sure we can actually use xss */
    if (XScreenSaverQueryExtension (dpy, &eventBase, &dummy))
    {
	eventBit = 0x7F; /* black magic */
	/* Kill the old screensaver and ensure that it doesn't come back */
	XGetScreenSaver (dpy, &timeout, &interval, &preferBlanking, &allowExposures);

	/* It's disabled. Blargh */
	if (!timeout)
	{
	    compLogMessage ("locker", CompLogLevelWarn, "XScreenSaverIntegration: no timeout has been set." \
			    " this usually means that XScreenSaver is disabled. Enable it to get integration " \
			    "with that. Using %i as the timeout", cTimeout);
	    XSetScreenSaver (dpy, cTimeout, interval, preferBlanking, allowExposures);

	    timeout = cTimeout;
	}

	XScreenSaverSetAttributes (dpy, root, -100, -100, 1, 1, 0,
				   CopyFromParent, CopyFromParent, DefaultVisual (dpy, XDefaultScreen (dpy)),
				   mask, &attr);
	XScreenSaverSelectInput (screen->dpy (), root, ScreenSaverNotifyMask);
    }
    else
    {
	compLogMessage ("locker", CompLogLevelWarn, "Unable to use the XScreenSaver extension!");
    }
}

XScreenSaverTrigger::~XScreenSaverTrigger ()
{
    XSetScreenSaver (dpy, timeout, interval, preferBlanking, allowExposures);

    XScreenSaverSelectInput (dpy, root, 0);
    XScreenSaverUnsetAttributes (dpy, root);
}

void
TimerTrigger::unlocked ()
{
    mTimer.setTimes (mTimeout * 1000, mTimeout * 1000);
    mTimer.start ();

    mLocked = false;
}

bool
TimerTrigger::check ()
{
    Lockable *lockable = Lockable::Default ();

    if (lockable)
    {
	if (!mLocked)
	    return lockable->lock ();
	else
	    return lockable->wake ();
    }

    return false;
}

void
TimerTrigger::processEvent (XEvent *event)
{
    Lockable *lockable = Lockable::Default ();

    if (!lockable)
	return;

    switch (event->type)
    {
    case ButtonPress:
    case ButtonRelease:
    case KeyPress:
    case KeyRelease:
    case MotionNotify:
	if (lockable && mLocked)
	    lockable->wake ();
	else
	    mTimer.setTimes (mTimeout * 1000, mTimeout * 1000);
	break;
    }
}

TimerTrigger::TimerTrigger (unsigned int timeout) :
    mTimeout (timeout)
{
    mTimer.setCallback (boost::bind (&TimerTrigger::check, this));
    mTimer.setTimes (timeout * 1000, timeout * 1000);
    mTimer.start ();
}

LockerScreen::LockerScreen (CompScreen *s) :
    PluginClassHandler <LockerScreen, CompScreen, COMPIZ_LOCKER_ABI> (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mAnimationProgress (0.0f),
    mAnimatedWindows (0),
    mAnimationState (AnimationStateIdle),
    mGrabIndex (0)
{
    Lockable::mLockerWindow  = new SimpleLockerWindow ();
    Lockable::mLockerBackground  = new SimpleLockerBackground ();

    /* Attempt to create an XSS Trigger first,
     * then change it later */
    XScreenSaverTrigger *xssTrigger = new XScreenSaverTrigger (screen->dpy (), screen->root (), optionGetTimeout ());
    TimerTrigger        *timerTrigger = NULL;

    if (!xssTrigger->usable ())
    {
	delete xssTrigger;

	timerTrigger = new TimerTrigger (optionGetTimeout ());
	Lockable::mTrigger = timerTrigger;
    }
    else
	Lockable::mTrigger = xssTrigger;

    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    LockerLockWindow::SetDefault (Lockable::mLockerWindow);
    LockerLockBackground::SetDefault (Lockable::mLockerBackground);
    LockTrigger::SetDefault (Lockable::mTrigger);
    Lockable::SetDefault (this);
}

LockerScreen::~LockerScreen ()
{
    LockTrigger::SetDefault (NULL);
    LockerLockWindow::SetDefault (NULL);
    LockerLockBackground::SetDefault (NULL);
    Lockable::SetDefault (NULL);
}

LockerWindow::LockerWindow (CompWindow *w) :
    PluginClassHandler <LockerWindow, CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    mAnimationProgress (0.0f)
{
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);
}

bool
LockerPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_LOCKER_ABI;
    screen->storeValue ("locker_ABI", p);

    return true;
}

void
LockerPluginVTable::fini ()
{
    screen->eraseValue ("locker_ABI");
}
    
