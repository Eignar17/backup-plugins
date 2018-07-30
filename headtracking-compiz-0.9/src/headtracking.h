/*
 * Compiz Fusion Head Tracking Plugin
 * 
 * Some of this code is based on Freewins
 * Portions were inspired and tested on a modified
 * Zoom plugin, but no code from Zoom has been taken.
 * 
 * Copyright 2010 Kevin Lange <kevin.lange@phpwnage.com>
 *
 * facedetect.c is from the OpenCV sample library, modified to run
 * threaded.
 *
 * Face detection is done through OpenCV.
 * Wiimote tracking is done through the `wiimote` plugin and probably
 * doesn't work anymore.
 *
 * Video demonstrations of both webcams and wiimotes are available
 * online. Check YouTube, as well as the C-F forums.
 *
 * More information is available in README.
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

#include <opengl/opengl.h>
#include <composite/composite.h>

#include <mousepoll/mousepoll.h>

#include "headtracking_options.h"

#include <cmath>

/* blame kevin */
#define USE_WIIMOTE FALSE // Change as necessary

#define PI 3.14159265358979323846
#define ADEG2RAD(DEG) ((DEG)*((PI)/(180.0)))

// Macros/*{{{*/


#define WIN_REAL_X(w) (w->x () - w->input ().left)
#define WIN_REAL_Y(w) (w->y () - w->input ().top)

#define WIN_REAL_W(w) (w->width () + w->input ().left + w->input ().right)
#define WIN_REAL_H(w) (w->height () + w->input ().top + w->input ().bottom)

/*}}}*/
/* Kevin: folds look shit. I hate them. Use a better editor like emacs.
 * That is all - Sm */

typedef struct _WTHead {
	float x;
	float y;
	float z;
} WTHead;

class WTScreen :
    public PluginClassHandler <WTScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public HeadtrackingOptions
{
    public:

	WTScreen (CompScreen *);
	~WTScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;
	
	void
	preparePaint (int);
	
	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix		 &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		   );
		       
	void
	updatePosition (const CompPoint &p);
	
	void
	WTLeeTrackPosition (float x1, float y1, float x2, float y2);

	bool
	WTManual (CompAction         *action,
		    CompAction::State  state,
		    CompOption::Vector &options,
		    bool depth,
		    bool reset);

	bool
	WTDebug (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options,
		   bool changeX, bool changeY, bool changeZ,
		   bool x, bool y, bool z,
		   bool reset);
		   
	bool
	WTToggleMouse (CompAction          *action,
		       CompAction::State   state,
		       CompOption::Vector  options);
	
	MousePoller mMousepoller;
		       
	CompWindow *mGrabWindow;
	CompWindow *mFocusWindow;
	
	CompTimer   mMouseIntervalTimeoutHandle;
	CompPoint   mMouse;
	
	CompScreen::GrabHandle mGrabIndex;
	int		       mRotatedWindows; /// <- WTF Kevin?!?!?!
	bool		       mTrackMouse;
	
	WTHead		       mHead;
};

#define HEADTRACKING_SCREEN(s)						       \
    WTScreen *wts = WTScreen::get(s)

class WTWindow :
    public PluginClassHandler <WTWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	WTWindow (CompWindow *);
	~WTWindow ();
	
	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int		     );
		 
	bool
	damageRect (bool,
		    const CompRect &);
		    
	bool
	is3D ();

	bool
	shouldPaintStacked ();

	float mDepth;
	float mManualDepth;
	float mZDepth;
	float mOldDepth;
	float mNewDepth;
	int mTimeRemaining; // 100 to 0
	int mZIndex;
	Bool mIsAnimating;
	Bool mIsManualDepth;
	Bool mIsGrabbed;	
};

#define HEADTRACKING_WINDOW(w)						       \
    WTWindow *wtw = WTWindow::get(w)
    
class HeadtrackingPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <WTScreen, WTWindow>
{
    public:

	bool init ();
};
