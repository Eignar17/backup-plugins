#include <compiz-core.h>
#include <compiz-plugin.h>

#include <math.h>

#include "drunken_options.h"

static int displayPrivateIndex = 0;

typedef struct _DrunkenDisplay
{
    int screenPrivateIndex;

typedef struct _Stereo3DScreen
{
    int windowPrivateIndex;
    CompScreen *s;

	
	bool		mEnabled;
	
	void		toggleFunctions (bool);

    PaintWindowProc paintWindow;

	Bool toggle ();

};

#define GET_DRUNK_SCREEN(s, ds)                         \
    ((DrunkenScreen *) (s)->base.privates[(ds)->screenPrivateIndex].ptr)

typedef struct _DrunkenWindow DrunkenWindow;
	  
	  CompWindow *window;
	  
	  Bool shouldAnimate ();

	  Bool
	  glPaint (const ScreenPaintAttrib &,
		   const CompMatrix	     &,
		   const Region	             &,
		   unsigned int		      );

	  float	mDrunkFactor;
};

#define GET_DRUNK_WINDOW(w, dw)                         \
    ((DrunkenWindow *) (w)->base.privates[(dw)->windowPrivateIndex].ptr)
