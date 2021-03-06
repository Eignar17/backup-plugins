/*
 * Compiz locker plugin
 *
 * locker.h
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

#ifndef _COMPIZ_LOCKER_H
#define _COMPIZ_LOCKER_H

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#define COMPIZ_LOCKER_ABI 1

class LockerLockWindow :
    public CompRect
{
public:

    LockerLockWindow () {}
    virtual ~LockerLockWindow () {};

    virtual void hide () = 0;
    virtual void show () = 0;
    virtual bool visible () = 0;
    virtual void paint (const GLMatrix &transform) = 0;
    virtual bool needsGrab () = 0;
    virtual bool handleKeyPress (XKeyEvent *xk) { return false; };
    virtual bool handleButtonPress (XButtonEvent *xb) { return false; };
    virtual bool handleKeyRelease (XKeyEvent *xk) { return false; };
    virtual bool handleButtonRelease (XButtonEvent *xb) { return false; };
    virtual bool handleMotion (XMotionEvent *xm) { return false; };

    virtual CompRegion & damage () = 0;

    static void
    SetDefault (LockerLockWindow *);

    static LockerLockWindow *
    Default ();
};

class LockerLockBackground :
    public CompRect
{
public:

    LockerLockBackground () {}
    virtual ~LockerLockBackground () {};

    virtual void hide () = 0;
    virtual void show () = 0;
    virtual bool visible () = 0;
    virtual void paint (const GLMatrix &transform) = 0;
    virtual CompRegion & damage () = 0;

    static void
    SetDefault (LockerLockBackground *);

    static LockerLockBackground *
    Default ();
};

class LockTrigger
{
public:
    LockTrigger () {}
    virtual ~LockTrigger () {}

    virtual void unlocked () = 0;

    static LockTrigger *
    Default ();

    static void
    SetDefault (LockTrigger *);
};

class Lockable
{
public:

    Lockable () {}
    virtual ~Lockable () {}

    virtual bool lock () = 0;
    virtual bool unlock () = 0;
    virtual bool wake () = 0;

    static void
    SetDefault (Lockable *);

    static Lockable *
    Default ();

protected:

    LockerLockWindow     *mLockerWindow;
    LockerLockBackground *mLockerBackground;
    LockTrigger          *mTrigger;
};

#endif
