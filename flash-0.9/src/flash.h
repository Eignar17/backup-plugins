/**
*
* Compiz plugin Flash
*
* Copyright : (C) 2007 by Jean-Fran�is Souville & Charles Jeremy
* E-mail    : souville@ecole.ensicaen.fr , charles@ecole.ensicaen.fr
*
* Ported to compiz++ by:
* Sam Spilsbury <smspillaz@gmail.com>
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

#include <math.h>

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <opengl/opengl.h>
#include <composite/composite.h>

#include "flash_options.h"

#define POS_TOP 0
#define POS_BOTTOM 1
#define POS_LEFT 2
#define POS_RIGHT 3

class FlashScreen :
    public PluginClassHandler <FlashScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public FlashOptions
{
    public:

	FlashScreen (CompScreen *);

	CompositeScreen *cScreen;
	GLScreen *gScreen;

	int pointX, pointY;
	int winX,winY,winW,winH;

	double** CoordinateArray;

	int tailleTab;

	int *remainTime;

	int *interval;

	int** alea;

	int location;

	unsigned int WindowMask;

	bool active;
	bool *lightning;

	bool existWindow;

	bool isAnimated;
	bool anotherPluginIsUsing;

	void
	preparePaint (int ms);

	bool
	glPaintOutput (const GLScreenPaintAttrib &sAttrib,
		       const GLMatrix            &transform,
		       const CompRegion          &region,
		       CompOutput                *output,
		       unsigned int              mask);

	void
	donePaint ();

	static bool
	flashInitiate (CompAction *action,
		       CompAction::State state,
		       CompOption::Vector options);
};

#define FLASH_SCREEN(s)							       \
    FlashScreen *fs = FlashScreen::get (s)

class FlashPluginVTable :
    public CompPlugin::VTableForScreen <FlashScreen>
{
    public:

	bool
	init ();
};
