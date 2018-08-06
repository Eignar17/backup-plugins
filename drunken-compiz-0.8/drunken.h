#include <compiz-core.h>
#include <compiz-plugin.h>

#include <math.h>

#include "drunken_options.h"

extern int displayPrivateIndex;

typedef struct _DrunkenDisplay
{
    int screenPrivateIndex;

typedef struct _DrunkenScreen
{
    int windowPrivateIndex;

    PaintOutputProc              paintOutput;
	
	bool		mEnabled;
	
	void		toggleFunctions (bool);

    PaintWindowProc paintWindow;

	Bool toggle ();

};

#define GET_DRUNK_SCREEN(s, ds)                         \
    ((DrunkenScreen *) (s)->base.privates[(ds)->screenPrivateIndex].ptr)

typedef struct _DrunkenWindow DrunkenWindow;
	  
	  
	  Bool shouldAnimate ();

	  Bool
	  glPaint (const ScreenPaintAttrib &,
		   const CompMatrix	     &,
		   const Region	             &,
		   unsigned int		      );

	  float	drunkenGetFactor;
};

#define GET_DRUNK_WINDOW(w, dw)                         \
    ((DrunkenWindow *) (w)->base.privates[(dw)->windowPrivateIndex].ptr)
