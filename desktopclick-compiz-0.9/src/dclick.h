/*
 * Compiz desktopclick plugin
 *
 * desktopclick.h
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * Based on code by vpswitch, thanks to:
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <string.h>
#include "dclick_options.h"

class DClickScreen :
    public PluginClassHandler <DClickScreen, CompScreen>,
    public ScreenInterface,
    public DclickOptions
{
    public:

	DClickScreen (CompScreen *);

	int  activeMods;

	void
	handleEvent (XEvent *);

	static int 
	dclickModMaskFromEnum (int num);

	bool
	dclickHandleDesktopButtonPress (int buttonMask,
					unsigned int win,
					unsigned int root);

	bool
	dclickHandleDesktopButtonRelease (int buttonMask,
				          unsigned int win,
				   	  unsigned int root);
};

class DClickPluginVTable :
    public CompPlugin::VTableForScreen <DClickScreen>
{
    public:

	bool
	init ();
};

#define DCLICK_SCREEN (s)						       \
    DClickScreen *ds = DClickScreen::get (s);

#define MAX_INFO 32
