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

#include "xlocker.h"
#include <poll.h>

COMPIZ_PLUGIN_20090315 (xlocker, X11LockerPluginVTable);

namespace LockerAtoms
{
    Atom lockerStatus;
    Atom lockerBgStatus;
    Atom lockerUnlock;
}

/* Create a backing window */
X11LockWindow::X11LockWindow (Window child) :
    mCBackingWindow (NULL),
    mBackingWindow (0),
    mChild (child),
    mMapped (false)
{
    unsigned char *propData;
    long          *data;
    Atom          retType;
    int           retFmt;
    unsigned long retItems, retLeft;
    XSetWindowAttributes attr;

    attr.override_redirect = true;

    /* Read the _COMPIZ_NET_LOCKER_STATUS property on this window
     * and then reparent this window into a window that we own and
     * control */

    if (XGetWindowProperty (screen->dpy (), child, LockerAtoms::lockerStatus, 0L,
			    4096L, false, XA_CARDINAL, &retType,
			    &retFmt, &retItems, &retLeft, &propData) == Success &&
			    retItems == 7 &&
			    retLeft == 0 &&
			    retFmt == 32)
    {
	data = (long *) propData;

	mBackingWindow = XCreateWindow (screen->dpy (), screen->root (),
					screen->outputDevs ().front ().centerX () - data[4] / 2,
					screen->outputDevs ().front ().centerY () - data[5] / 2,
					data[4], data[5], 0, CopyFromParent, InputOutput,
					DefaultVisual (screen->dpy (), screen->screenNum ()),
					CWOverrideRedirect, &attr);

	XAddToSaveSet (screen->dpy (), child);
	XReparentWindow (screen->dpy (), child, mBackingWindow, 0, 0);

	mGeometry = CompRect (screen->outputDevs ().front ().centerX () - data[4] / 2,
			      screen->outputDevs ().front ().centerY () - data[5] / 2,
			      data[4], data[5]);
    }
}

X11LockWindow::~X11LockWindow ()
{
    XDestroyWindow (screen->dpy (), mBackingWindow);
}

void
X11LockWindow::hide ()
{
    long data[7];
    XUnmapWindow (screen->dpy (), mBackingWindow);

    data[0] = 1;
    data[1] = mChild;
    data[2] = x ();
    data[3] = y ();
    data[4] = width ();
    data[5] = height ();
    data[6] = 0;

    XChangeProperty (screen->dpy (), mChild, LockerAtoms::lockerStatus,
		     XA_CARDINAL, 32, PropModeReplace, (const unsigned char *) data, 7);

    mMapped = false;
}

void
X11LockWindow::show ()
{
    long data[7];
    XMapRaised (screen->dpy (), mBackingWindow);
    /* Also set the visible bit */

    data[0] = 1;
    data[1] = mChild;
    data[2] = x ();
    data[3] = y ();
    data[4] = width ();
    data[5] = height ();
    data[6] = 1;

    XChangeProperty (screen->dpy (), mChild, LockerAtoms::lockerStatus,
		     XA_CARDINAL, 32, PropModeReplace, (const unsigned char *) data, 7);

    mMapped = true;
}

bool
X11LockWindow::visible ()
{
    return mMapped;
}



void
X11LockWindow::completeBackingWindow (CompWindow *w)
{
    mCBackingWindow = w;

    XSelectInput (screen->dpy (), mChild, PropertyChangeMask);
}

bool
X11LockWindow::attemptUnlock (long *data)
{
    Lockable *lockable = Lockable::Default ();

    if (data[1] == mChild)
    {
	lockable->unlock ();
	return true;
    }

    return false;
}

void
X11LockWindow::setGeometry (int x, int y, unsigned int width, unsigned int height)
{
    XWindowChanges xwc;
    unsigned int mask = 0;

    if (x != mGeometry.x ())
	mask |= CWX;

    if (y != mGeometry.y ())
	mask |= CWY;

    if (width != mGeometry.width ())
	mask |= CWWidth;

    if (height != CWHeight)
	mask |= CWHeight;

    /* Always position in the middle of the screen */
    xwc.x = screen->outputDevs ().front ().centerX () - width / 2;
    xwc.y = screen->outputDevs ().front ().centerY () - height / 2;
    xwc.width = width;
    xwc.height = height;

    if (mCBackingWindow)
	mCBackingWindow->configureXWindow (mask, &xwc);
    else
	XConfigureWindow (screen->dpy (), mBackingWindow, mask, &xwc);

    mGeometry = CompRect (x, y, width, height);
}

void
X11LockWindow::stackOffendingWindow (Window offender, XWindowChanges *xwc, unsigned int mask)
{
    CompWindow *p = mCBackingWindow->prev;

    for (; p; p = p->prev)
    {
	mask |= (CWSibling | CWStackMode);
	xwc->stack_mode = Above;
	xwc->sibling = ROOTPARENT (p);

	XConfigureWindow (screen->dpy (), offender, mask, xwc);
	return;
    }

    /* Couldn't stack this window below anything ..
     * stack the locker above instead then */

    unsigned int   vm;
    XWindowChanges xlwc;

    vm = CWStackMode | CWSibling;
    xlwc.sibling = offender;
    xlwc.stack_mode = Above;

    XConfigureWindow (screen->dpy (), mBackingWindow, mask, &xlwc);
}

void
X11LockWindow::paint (const GLMatrix &transform)
{
    GLFragment::Attrib a (GLWindow::get (mCBackingWindow)->lastPaintAttrib ());
    a.setBrightness (BRIGHT);
    a.setOpacity (OPAQUE);
    a.setSaturation (COLOR);
    if (!mCBackingWindow)
	return;

    glPushMatrix ();
    glLoadMatrixf (transform.getMatrix ());

    GLWindow::get (mCBackingWindow)->glDraw (transform, a,
					     infiniteRegion,
					     GLWindow::get (mCBackingWindow)->lastMask ());
}

void
X11LockerScreen::handleEvent (XEvent *event)
{
    switch (event->type)
    {
    case PropertyNotify:
	if (event->xproperty.atom == LockerAtoms::lockerStatus)
	{
	    X11LockWindow *xlw = dynamic_cast <X11LockWindow *> (LockerLockWindow::Default ());

	    if (!xlw)
	    {
		xlw = new X11LockWindow (event->xproperty.window);
		LockerLockWindow::SetDefault (xlw);
	    }
	    else
	    {
		/* Either a new locker window was created or the old one needs to be resized */

		if (!xlw->isChildWindow (event->xproperty.window))
		    LockerLockWindow::SetDefault (new X11LockWindow (event->xproperty.window));
		else
		{
		    unsigned char *propData;
		    long          *data;
		    Atom          retType;
		    int           retFmt;
		    unsigned long retItems, retLeft;

		    if (XGetWindowProperty (screen->dpy (), event->xproperty.window,
					    LockerAtoms::lockerStatus, 0L,
					    4096L, false, XA_CARDINAL, &retType,
					    &retFmt, &retItems, &retLeft, &propData) == Success &&
					    retItems == 7 &&
					    retLeft == 0 &&
					    retFmt == 32)
		    {
			//xlw->setGeometry (data[2], data[3], data[4], data[5]);
		    }
		}
	    }
	}
	break;
    case ClientMessage:
	if (event->xclient.message_type == LockerAtoms::lockerUnlock)
	{
	    X11LockWindow *xlw = dynamic_cast <X11LockWindow *> (LockerLockWindow::Default ());

	    if (xlw)
		xlw->attemptUnlock (event->xclient.data.l);
	}
    case ConfigureRequest:
	if (event->xconfigurerequest.value_mask & (CWSibling | CWStackMode))
	{
	    X11LockWindow *xlw = dynamic_cast <X11LockWindow *> (LockerLockWindow::Default ());

	    if (xlw)
	    {
		if (xlw->isBackingWindow (event->xconfigurerequest.above))
		{
		    XWindowChanges xwc;

		    xwc.x = event->xconfigurerequest.x;
		    xwc.y = event->xconfigurerequest.y;
		    xwc.width = event->xconfigurerequest.width;
		    xwc.height = event->xconfigurerequest.height;
		    xwc.stack_mode = event->xconfigurerequest.detail;
		    xwc.sibling = event->xconfigurerequest.above;

		    /* Do not allow other windows to go above this one,
		     * find the next window below this one and go above
		     * that instead */
		    xlw->stackOffendingWindow (event->xconfigurerequest.window, &xwc, event->xconfigurerequest.value_mask);
		}
	    }
	}
    }

    screen->handleEvent (event);
}

X11LockerScreen::~X11LockerScreen ()
{
    LockerLockWindow::SetDefault (NULL);
}

X11LockerScreen::X11LockerScreen (CompScreen *s) :
    PluginClassHandler <X11LockerScreen, CompScreen> (s)
{
    ScreenInterface::setHandler (s);

    LockerAtoms::lockerStatus = XInternAtom (screen->dpy (),
					     X_LOCKER_PROPERTY_NAME, 0);
    LockerAtoms::lockerBgStatus = XInternAtom (screen->dpy (),
					       X_LOCKER_BG_PROPERTY_NAME, 0);
    LockerAtoms::lockerUnlock = XInternAtom (screen->dpy (),
					     X_LOCKER_UNLOCK_PROPERTY_NAME, 0);
}

X11LockerWindow::X11LockerWindow (CompWindow *w) :
    PluginClassHandler <X11LockerWindow, CompWindow> (w),
    window (w),
    gWindow (GLWindow::get (w))
{
    X11LockWindow *xlw = dynamic_cast <X11LockWindow *> (LockerLockWindow::Default ());

    if (xlw)
    {
	if (xlw->isBackingWindow (w->id ()))
	    xlw->completeBackingWindow (w);
    }
    else
    {
	/* No locker defined, maybe check to see if this is the one */
	unsigned char *propData;
	long          *data;
	Atom          retType;
	int           retFmt;
	unsigned long retItems, retLeft;

	/* Read the _COMPIZ_NET_LOCKER_STATUS property on this window
	 * and then reparent this window into a window that we own and
	 * control */

	int status = XGetWindowProperty (screen->dpy (), window->id (), XInternAtom (screen->dpy (), "_COMPIZ_NET_LOCKER_STATUS", 0), 0L, 4096L,
				false, XA_CARDINAL, &retType,
				&retFmt, &retItems, &retLeft, &propData);

	if (status == Success)
	{

	    if (retItems == 7 && retFmt == 32 && retLeft == 0)
	    {
		xlw = new X11LockWindow (window->id ());
		LockerLockWindow::SetDefault (xlw);
	    }
	}
    }
}

bool
X11LockerPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("locker", COMPIZ_LOCKER_ABI))
	return false;

    return true;
}


