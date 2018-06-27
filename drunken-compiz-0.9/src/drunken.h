#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <cmath>

#include "drunken_options.h"

class DrunkenScreen :
    public PluginClassHandler <DrunkenScreen, CompScreen>,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public DrunkenOptions
{
    public:

	DrunkenScreen (CompScreen *);

	CompositeScreen *cScreen;
	GLScreen	*gScreen;
	
	bool		mEnabled;
	
	void		toggleFunctions (bool);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix		 &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		   );

	bool toggle ();
	
	void
	preparePaint (int);

	void
	donePaint ();
};

#define DRUNK_SCREEN(s)							      \
    DrunkenScreen *ds = DrunkenScreen::get (s)

class DrunkenWindow :
    public PluginClassHandler <DrunkenWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	  DrunkenWindow (CompWindow *);
	  
	  CompWindow *window;
	  CompositeWindow *cWindow;
	  GLWindow	  *gWindow;
	  
	  bool shouldAnimate ();

	  bool
	  glPaint (const GLWindowPaintAttrib &,
		   const GLMatrix	     &,
		   const CompRegion	     &,
		   unsigned int		      );

	  float	mDrunkFactor;
};

#define DRUNK_WINDOW(w)							      \
    DrunkenWindow *dw = DrunkenWindow::get (w)

class DrunkenPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <DrunkenScreen, DrunkenWindow>
{
    public:

	bool init ();
};