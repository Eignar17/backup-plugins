/*copyright*/

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "hoverfx_options.h"

class HoverScreen :
    public PluginClassHandler <HoverScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public HoverfxOptions
{
    public:
	
	HoverScreen (CompScreen *);
	
	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	CompWindowList  mHoveredWindows;
	
	void
	toggleFunctions (bool);
	
	void
	preparePaint (int);

	void
	handleEvent (XEvent *);
	
	void
	donePaint ();
	
	bool
	glPaintOutput (const GLScreenPaintAttrib &attrib,
		       const GLMatrix	      &transform,
		       const CompRegion	      &region,
		       CompOutput	      *output,
		       unsigned int	      mask);
};

#define HOVER_SCREEN(s)							      \
    HoverScreen *hs = HoverScreen::get(s)


class HoverWindow :
    public PluginClassHandler <HoverWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:
	
	HoverWindow (CompWindow *);

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow 	*gWindow;

	bool		mIsHovered;
	bool		mAnimate;

	float		mDefault;
	float		mOld;
	float		mCurrent;
	float		mTarget;
	
	void		toggleFunctions (bool);
	
	bool		shouldAnimate ();

	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int);
	
	bool
	damageRect (bool, const CompRect &);
};

#define HOVER_WINDOW(w)							      \
    HoverWindow *hw = HoverWindow::get (w);

class HoverPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <HoverScreen, HoverWindow>
{
    public:
	bool init ();
};