#include <compiz-core.h>
#include <compiz-plugin.h>

#include <math.h>

#include "drunken_options.h"

static int displayPrivateIndex = 0;

typedef struct _DrunkenDisplay
{
    int screenPrivateIndex;
    int windowPrivateIndex;

typedef struct _Stereo3DScreen
{
    CompScreen *s;

	
	bool		mEnabled;
	
	void		toggleFunctions (bool);

	Bool
	glPaintOutput (const ScreenPaintAttrib &,
		       const CompMatrix		 &,
		       const Region		 &,
		       CompOutput		 *,
		       unsigned int		   );

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
