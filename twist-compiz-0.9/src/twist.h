/*
* twist.h

* Description:  

* The plugin dynamically compresses bottom part of selected windows to enlarge free space on the desktop.
* "Twisted" window thus has reduced height, while still readable (more or less). 
* The feature should be especially useful on netbooks for their weak vertical screen resolution. 
* Please note that  while being quiet usable, twist was created as an experiment to find out ways of improving modern hardware-accelerated GUI.
* So don't panic if its experimental nature would show itself in some bad way 

* Using the plugin:
* To twist a window you should drag it down, and continue dragging after the screen bottom is reached. Twisted window consists of two parts - 
* normal (upper) part, which is accessible by mouse as usually, and compressed (lower) one. As for now, compressed part of the window can be accessed 
* only by keyboard, but in many cases that is not a problem, as most clickable elements (like menus or toolbars) often are residing on top.

* Contributions:

* The concept and the code created by Dmitriy Kostiuk (d.k AT list.ru) and Alexander Nikoniuk (nikoniuk AT mail.ru).

* The code is licensed under GPL v.2. 
* The plugin was created in bounds of a research project in Micro and Midi-ergonomisc Laboratory of Brest State Technical University
*/

#include <cmath>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>

#include "twist_options.h"

const int BEND_SIZE = 125;
enum Transformation 
{
        NONE = 0,
        BOTTOM = 1, 
        TOP = 2, 
        LEFT = 4, 
        RIGHT = 8, 
        TOGGLE_BOTTOM = 16, 
        TOGGLE_TOP = 32, 
        TOGGLE_LEFT = 64, 
        TOGGLE_RIGHT = 128
};

class TwistWindow :
    public PluginClassHandler <TwistWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	int transform;
        float bendingBorderTop, bendingBorderBottom, bendingBorderLeft, bendingBorderRight;
        float k_top, k_bottom, k_left, k_right; // compression ratios

	GLuint texture;

	TwistWindow (CompWindow *);
	~TwistWindow ();

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int);

	void glAddGeometry (const GLTexture::MatrixList&,
			    const CompRegion&, const CompRegion&,
			    unsigned int, unsigned int);

};

#define TWIST_WINDOW(w)							       \
	TwistWindow *tw = TwistWindow::get (w)

#define TWIST_SCREEN(w)							       \
	TwistScreen *ts = TwistScreen::get (w)

class TwistScreen :
    public PluginClassHandler <TwistScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public TwistOptions
{
    public:
	
	TwistScreen (CompScreen *);
	~TwistScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	void
	donePaint ();

	bool
	Activate (CompAction         *action,
		 CompAction::State  state,
		 CompOption::Vector options);

	void
	handleEvent (XEvent *event);

	MousePoller pollHandle;

	void
	positionUpdate (const CompPoint &);

};

class TwistPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <TwistScreen, TwistWindow>
{
    public:
	bool init ();
};
